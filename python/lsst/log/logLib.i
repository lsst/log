// -*- lsst-c++ -*-
%define log_DOCSTRING
"
Access to the classes from the log library
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.log", docstring=log_DOCSTRING) logLib

%naturalvar;
%include "std_string.i"

%{
#include "lsst/log/Log.h"
%}

%{
// Wrapper for Python callable object to make sure that we have GIL
// when we call Python. Note that we are leaking Python callable,
// as C++ callables may be (and actually are in our particular case) 
// outliving Python interpreter and attempt to delete Python object
// will result in crash.
class callable_wrapper {
public:
    callable_wrapper(PyObject* callable) : _callable(callable) {
        Py_XINCREF(_callable);
    }
    void operator()() {
        // make sure we own GIL before doing Python call
        auto state = PyGILState_Ensure();
        PyObject_CallObject(_callable, nullptr);
        PyGILState_Release(state);
    }
private:
    PyObject* _callable;
};
%}

// this should be for std::function<void()> but I can't convince SWIG
// to understand that syntax. Because we only have one type of function
// we can use more general std::function
%typemap(in) std::function {
    if (!PyCallable_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "argument is not a callable");
        SWIG_fail;
    }
    $1 = std::function<void()>(callable_wrapper($input));
}

%pythoncode {
import inspect
import os
}

%include "lsst/log/Log.h"

%extend lsst::log::Log {
    static lsst::log::Log getDefaultLogger() { return lsst::log::Log::defaultLogger; };
    void log(int level, std::string const& filename,
             std::string const& funcname, unsigned int lineno,
             std::string const& msg) {
        self->log(log4cxx::Level::toLevel(level),
                  log4cxx::spi::LocationInfo(filename.c_str(), funcname.c_str(), lineno),
                  msg.c_str());
    };
    unsigned lwpID() { return lsst::log::lwpID(); };

    %pythoncode {
    TRACE = 5000
    DEBUG = 10000
    INFO = 20000
    WARN = 30000
    ERROR = 40000
    FATAL = 50000

    def trace(self, fmt, *args):
        self._log(Log.TRACE, fmt, *args)

    def debug(self, fmt, *args):
        self._log(Log.DEBUG, fmt, *args)

    def info(self, fmt, *args):
        self._log(Log.INFO, fmt, *args)

    def warn(self, fmt, *args):
        self._log(Log.WARN, fmt, *args)

    def error(self, fmt, *args):
        self._log(Log.ERROR, fmt, *args)

    def fatal(self, fmt, *args):
        self._log(Log.FATAL, fmt, *args)

    def _log(self, level, fmt, *args):
        if self.isEnabledFor(level):
            frame = inspect.currentframe().f_back    # calling method
            frame = frame.f_back    # original log location
            self.log(level, os.path.split(frame.f_code.co_filename)[1],
                     inspect.stack()[2][3], frame.f_lineno, fmt % args)
    }
}

