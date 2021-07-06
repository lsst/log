// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2014 AURA/LSST.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
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
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#include <algorithm>
#include <stdexcept>

#include "./PyLogAppender.h"

// macro below dows not work without this using directive
// (and it breaks if placed inside namespaces)
using lsst::log::detail::PyLogAppender;
IMPLEMENT_LOG4CXX_OBJECT(PyLogAppender)

namespace {

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

unsigned const g_max_lru_cache_size = 32;

}

namespace lsst {
namespace log {
namespace detail {

PyLogAppender::PyLogAppender() {

    GilGuard gil_guard;

    PyObjectPtr logging(PyImport_ImportModule("logging"));
    if (logging == nullptr) {
        ::reraise("ImportError: Failed to import Python logging module");
    }
    m_getLogger = PyObject_GetAttrString(logging, "getLogger");
    if (m_getLogger == nullptr) {
        ::reraise("AttributeError: logging.getLogger method does not exist");
    }
    m_getLogRecordFactory = PyObject_GetAttrString(logging, "getLogRecordFactory");
    if (m_getLogRecordFactory == nullptr) {
        ::reraise("AttributeError: logging.getLogRecordFactory method does not exist");
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
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        auto cache_iter = m_cache.find(logger_name);
        if (cache_iter != m_cache.end()) {
            // use it an update its age
            logger = cache_iter->second.logger;
            cache_iter->second.age = m_lru_age ++;
        }
    }

    // Need Python at this point
    GilGuard gil_guard;

    if (logger == nullptr) {
        // was not found in cache get one from Python
        if (logger_name == "root") {
            logger = PyObject_CallFunction(m_getLogger, nullptr);
        } else {
            logger = PyObject_CallFunction(m_getLogger, "s", logger_name.c_str());
        }
    }
    if (logger == nullptr) {
        ::reraise("Failed to retrieve Python logger \"" + logger_name + "\"");
    } else {
        // remember it in cache
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        m_cache.emplace(logger_name, LRUEntry({logger, m_lru_age ++}));
        while (m_cache.size() > ::g_max_lru_cache_size) {
            // find oldest element and remove
            auto iter = std::min_element(
                m_cache.begin(), m_cache.end(),
                [](const LRUCache::value_type& lhs, const LRUCache::value_type& rhs) {
                    return lhs.second.age < rhs.second.age;
                }
            );
            m_cache.erase(iter);
        }
    }

    // before doing anything check logging level
    PyObjectPtr py_is_enabled(PyObject_CallMethod(logger, "isEnabledFor", "i", pyLevel));
    if (py_is_enabled == nullptr) {
        ::reraise("Failure when calling logger.isEnabledFor() method");
    }
    int is_enabled = PyObject_IsTrue(py_is_enabled);
    if (is_enabled == 1) {

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
            if (not msg.empty() and msg[msg.size()-1] == '\n') {
                msg[msg.size()-1] = '\0';
            }
            log4cxx::helpers::Transcoder::encodeUTF8(msg, message);
        } else {
            log4cxx::helpers::Transcoder::encodeUTF8(event->getMessage(), message);
        }

        // factory = logging.getLogRecordFactory()
        PyObjectPtr factory(PyObject_CallFunction(m_getLogRecordFactory, nullptr));
        if (factory == nullptr) {
            ::reraise("Failed to retrieve LogRecord factory");
        }

        // record = factory(logger_name, pyLevel, file_name, lineno, message, Py_None, Py_None)
        PyObjectPtr record(PyObject_CallFunction(factory, "sisisOO",
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

        // add MDC as record attribute, setattr(record, "MDC", dict(event.MDC()))
        auto mdc_keys = event->getMDCKeySet();
        if (not mdc_keys.empty()) {
            PyObjectPtr mdc_dict(PyDict_New());
            for (auto& mdc_key: mdc_keys) {
                std::string key, value;
                log4cxx::LogString mdc_value;
                event->getMDC(mdc_key, mdc_value);
                log4cxx::helpers::Transcoder::encodeUTF8(mdc_key, key);
                log4cxx::helpers::Transcoder::encodeUTF8(mdc_value, value);
                PyObjectPtr py_value(PyUnicode_FromStringAndSize(value.data(), value.size()));
                PyDict_SetItemString(mdc_dict, key.c_str(), py_value);
            }
            if (PyObject_SetAttrString(record, "MDC", mdc_dict) == -1) {
                ::reraise("Failed to set LogRecord.MDC attribute");
            }
        }

        // logger.handle(record)
        PyObjectPtr res(PyObject_CallMethod(logger.get(), "handle", "O", record.get()));
        if (res == nullptr) {
            ::reraise("Logger failed to handle LogRecord");
        }
    }
}

void PyLogAppender::close() {
}

bool PyLogAppender::requiresLayout() const {
    return true;
}

}}} // namespace lsst::log::detail
