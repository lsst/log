/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include "pybind11/pybind11.h"

#include "lsst/log/Log.h"

namespace py = pybind11;

namespace lsst {
namespace log {

// Wrapper for Python callable object to make sure that we have GIL
// when we call Python. Note that we are leaking Python callable,
// as C++ callables may be (and actually are in our particular case)
// outliving Python interpreter and attempt to delete Python object
// will result in crash.
//
// See DM-9708

class callable_wrapper {
public:
    explicit callable_wrapper(PyObject* callable) : _callable(callable) { Py_XINCREF(_callable); }
    void operator()() {
        // make sure we own GIL before doing Python call
        auto state = PyGILState_Ensure();
        PyObject_CallObject(_callable, nullptr);
        PyGILState_Release(state);
    }

private:
    PyObject* _callable;
};

PYBIND11_MODULE(log, mod) {
    py::class_<Log> cls(mod, "Log");

    /* Constructors */
    cls.def(py::init<>());

    /* Members */
    cls.attr("TRACE") = py::int_(5000);
    cls.attr("DEBUG") = py::int_(10000);
    cls.attr("INFO") = py::int_(20000);
    cls.attr("WARN") = py::int_(30000);
    cls.attr("ERROR") = py::int_(40000);
    cls.attr("FATAL") = py::int_(50000);

    cls.def("isDebugEnabled", &Log::isDebugEnabled);
    cls.def("isErrorEnabled", &Log::isErrorEnabled);
    cls.def("isFatalEnabled", &Log::isFatalEnabled);
    cls.def("isInfoEnabled", &Log::isInfoEnabled);
    cls.def("isTraceEnabled", &Log::isTraceEnabled);
    cls.def("isWarnEnabled", &Log::isWarnEnabled);
    cls.def("getName", &Log::getName);
    cls.def("setLevel", &Log::setLevel);
    cls.def("getLevel", &Log::getLevel);
    cls.def("getEffectiveLevel", &Log::getEffectiveLevel);
    cls.def("isEnabledFor", &Log::isEnabledFor);
    cls.def("getChild", &Log::getChild);
    cls.def("logMsg", [](Log &log, int level, std::string const &filename, std::string const &funcname,
                         unsigned int lineno, std::string const &msg) {
        log.logMsg(log4cxx::Level::toLevel(level),
                   log4cxx::spi::LocationInfo(filename.c_str(), log4cxx::spi::LocationInfo::calcShortFileName(filename.c_str()), funcname.c_str(), lineno),
                   msg);
    });
    cls.def("lwpID", [](Log const& log) -> unsigned { return lsst::log::lwpID(); });

    cls.def_static("getDefaultLogger", Log::getDefaultLogger);
    cls.def_static("configure", (void (*)())Log::configure);
    cls.def_static("configure", (void (*)(std::string const&))Log::configure);
    cls.def_static("configure_prop", Log::configure_prop);
    cls.def_static("getLogger", (Log(*)(Log const&))Log::getLogger);
    cls.def_static("getLogger", (Log(*)(std::string const&))Log::getLogger);
    cls.def_static("MDC", Log::MDC);
    cls.def_static("MDCRemove", Log::MDCRemove);
    cls.def_static("MDCRegisterInit", [](py::function func) {
        auto handle = func.release();  // will leak as described in callable_wrapper
        Log::MDCRegisterInit(std::function<void()>(callable_wrapper(handle.ptr())));
    });
}

}  // log
}  // lsst
