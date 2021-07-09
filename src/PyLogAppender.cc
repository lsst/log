/*
 * This file is part of log.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <memory>
#include <stdexcept>

#include "./PyLogAppender.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/helpers/stringhelper.h"

// macro below dows not work without this using directive
// (and it breaks if placed inside namespaces)
using lsst::log::detail::PyLogAppender;
IMPLEMENT_LOG4CXX_OBJECT(PyLogAppender)

using namespace log4cxx::helpers;

namespace {

// GIL wrapper class
class GilGuard {
public:

    GilGuard() : gil_state(PyGILState_Ensure()) {}

    ~GilGuard() { PyGILState_Release(gil_state); }

private:
    PyGILState_STATE gil_state;
};

/**
 *  Re-raise Python exception as C++ exception.
 */
std::string reraise(std::string const& message) {
    lsst::log::detail::PyObjectPtr ptype, pvalue, ptraceback;
    PyErr_Fetch(&ptype.get(), &pvalue.get(), &ptraceback.get());

    std::string exc_msg = message;
    if (pvalue != nullptr or ptype != nullptr) {
        lsst::log::detail::PyObjectPtr exc_str(PyObject_Str(pvalue != nullptr ? pvalue : ptype));
        exc_msg += ": ";
        exc_msg += PyUnicode_AsUTF8(exc_str);
    }

    throw std::runtime_error(exc_msg);
}

// Maximum size of the logger name cache
unsigned const MAX_LRU_CACHE_SIZE = 32;

}

namespace lsst::log::detail {

PyLogAppender::PyLogAppender() {

    GilGuard gil_guard;

    PyObjectPtr logging(PyImport_ImportModule("logging"));
    if (logging == nullptr) {
        ::reraise("ImportError: Failed to import Python logging module");
    }
    _getLogger = PyObject_GetAttrString(logging, "getLogger");
    if (_getLogger == nullptr) {
        ::reraise("AttributeError: logging.getLogger method does not exist");
    }

    PyObjectPtr lsstlog_module(PyImport_ImportModule("lsst.log"));
    if (lsstlog_module == nullptr) {
        ::reraise("ImportError: Failed to import lsst.log module");
    }
    _mdc_class = PyObject_GetAttrString(lsstlog_module, "MDCDict");
    if (_mdc_class == nullptr) {
        ::reraise("AttributeError: lsst.log.MDCDict class does not exist");
    }
}

void PyLogAppender::append(const spi::LoggingEventPtr& event, log4cxx::helpers::Pool& p) {

    // logger name, UTF-8 encoded
    std::string logger_name;
    log4cxx::helpers::Transcoder::encodeUTF8(event->getLoggerName(), logger_name);
    int const level = event->getLevel()->toInt();
    int const pyLevel = level / 1000;

    // Check logger name in cache first, this does not need GIL but needs
    // synchronization
    PyObjectPtr logger;
    {
        std::lock_guard<std::mutex> lock(_cache_mutex);
        auto cache_iter = _cache.find(logger_name);
        if (cache_iter != _cache.end()) {
            // use it, age is updated later
            logger = cache_iter->second.logger;
        }
    }

    // Need Python at this point
    GilGuard gil_guard;

    if (logger == nullptr) {
        // was not found in cache get one from Python
        if (logger_name == "root") {
            logger = PyObject_CallFunction(_getLogger, nullptr);
        } else {
            logger = PyObject_CallFunction(_getLogger, "s", logger_name.c_str());
        }
    }
    if (logger == nullptr) {
        ::reraise("Failed to retrieve Python logger \"" + logger_name + "\"");
    } else {
        // remember it in cache
        std::lock_guard<std::mutex> lock(_cache_mutex);
        _cache.emplace(logger_name, LRUEntry({logger, _lru_age ++}));
        while (_cache.size() > ::MAX_LRU_CACHE_SIZE) {
            // find oldest element and remove
            auto iter = std::min_element(
                _cache.begin(), _cache.end(),
                [](const LRUCache::value_type& lhs, const LRUCache::value_type& rhs) {
                    return lhs.second.age < rhs.second.age;
                }
            );
            _cache.erase(iter);
        }
        // Age counter could potentially overflow (wrap to 0), reset age for
        // all cache entries to avoid issues.
        if (_lru_age == 0) {
            for (auto& cache_value: _cache) {
                // give them different but "random" ages
                cache_value.second.age = _lru_age ++;
            }
        }
    }

    // before doing anything check logging level
    PyObjectPtr py_is_enabled(PyObject_CallMethod(logger, "isEnabledFor", "i", pyLevel));
    if (py_is_enabled == nullptr) {
        ::reraise("Failure when calling logger.isEnabledFor() method");
    }
    if (not PyObject_IsTrue(py_is_enabled)) {
        return;
    }

    // collect all necessary info
    auto& loc = event->getLocationInformation();
    std::string file_name;
    if (loc.getFileName() != nullptr) {
        file_name = loc.getFileName();
    }
    int const lineno = loc.getLineNumber();

    // if layout is defined then use formatted message
    std::string message;
    if (this->layout) {
        LogString msg;
        this->layout->format(msg, event, p);
        // get rid of trailing new-line, just in case someone uses %n in format
        if (not msg.empty() and msg.back() == '\n') {
            msg.pop_back();
        }
        log4cxx::helpers::Transcoder::encodeUTF8(msg, message);
    } else {
        log4cxx::helpers::Transcoder::encodeUTF8(event->getMessage(), message);
    }

    // record = logger.makeRecord(name, level, fn, lno, msg, args, exc_info,
    //                            func=None, extra=None, sinfo=None)
    // I would like to pass MDC as an `extra` argument but that does not
    // work reliably in case we want to override record factory.
    PyObjectPtr record(PyObject_CallMethod(logger, "makeRecord", "sisisOO",
                                            logger_name.c_str(),
                                            pyLevel,
                                            file_name.c_str(),
                                            lineno,
                                            message.c_str(),
                                            Py_None,
                                            Py_None));
    if (record == nullptr) {
        ::reraise("Failed to create LogRecord instance");
    }

    // Record should already have an `MDC` attribute added by a record factory,
    // and it may be pre-filled with some info by the same factory. Here we
    // assume that if it already exists then it is dict-like, if it does not
    // we add this attribute as lsst.log.MDCDict instance (which is a dict with
    // some extra niceties).

    // mdc = getattr(record, "MDC")
    PyObjectPtr mdc(PyObject_GetAttrString(record, "MDC"));
    if (mdc == nullptr) {
        PyErr_Clear();
        // mdc = lsst.log.MDCDict()
        mdc = PyObject_CallObject(_mdc_class, nullptr);
        if (mdc == nullptr) {
            ::reraise("Failed to make MDCDict instance");
        }
        // record.MDC = mdc
        if (PyObject_SetAttrString(record, "MDC", mdc) == -1) {
            ::reraise("Failed to set LogRecord MDC attribute");
        }
    }

    // Copy MDC to dict, for key in getMDCKeySet(): mdc[key] = event.getMDC(key)
    for (auto& mdc_key: event->getMDCKeySet()) {
        std::string key, value;
        log4cxx::helpers::Transcoder::encodeUTF8(mdc_key, key);
        log4cxx::LogString mdc_value;
        event->getMDC(mdc_key, mdc_value);
        log4cxx::helpers::Transcoder::encodeUTF8(mdc_value, value);
        PyObjectPtr py_value(PyUnicode_FromStringAndSize(value.data(), value.size()));
        PyObjectPtr py_key(PyUnicode_FromStringAndSize(key.data(), key.size()));
        if (PyObject_SetItem(mdc, py_key, py_value) == -1) {
            // it is probably not a dictionary
            ::reraise("Failed to update MDC dictionary");
        }
    }

    // logger.handle(record)
    PyObjectPtr res(PyObject_CallMethod(logger, "handle", "O", record.get()));
    if (res == nullptr) {
        ::reraise("Logger failed to handle LogRecord");
    }
}

void PyLogAppender::close() {
}

bool PyLogAppender::requiresLayout() const {
    return false;
}

void PyLogAppender::setOption(const LogString &option, const LogString &value) {

    if (StringHelper::equalsIgnoreCase(option, LOG4CXX_STR("MESSAGEPATTERN"),
                                       LOG4CXX_STR("messagepattern"))) {
        setLayout(LayoutPtr(new PatternLayout(value)));
    } else {
        AppenderSkeleton::setOption(option, value);
    }
}

} // namespace lsst::log::detail
