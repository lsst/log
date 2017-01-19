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

// Allow Python2 to accept unicode strings
%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

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
    void logMsg(int level, std::string const& filename,
                std::string const& funcname, unsigned int lineno, std::string const& msg) {
        self->logMsg(log4cxx::Level::toLevel(level),
                     log4cxx::spi::LocationInfo(filename.c_str(), funcname.c_str(), lineno), msg.c_str());
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
        self._log(Log.TRACE, False, fmt, *args)

    def debug(self, fmt, *args):
        self._log(Log.DEBUG, False, fmt, *args)

    def info(self, fmt, *args):
        self._log(Log.INFO, False, fmt, *args)

    def warn(self, fmt, *args):
        self._log(Log.WARN, False, fmt, *args)

    def error(self, fmt, *args):
        self._log(Log.ERROR, False, fmt, *args)

    def fatal(self, fmt, *args):
        self._log(Log.FATAL, False, fmt, *args)

    def tracef(self, fmt, *args, **kwargs):
        self._log(Log.TRACE, True, fmt, *args, **kwargs)

    def debugf(self, fmt, *args, **kwargs):
        self._log(Log.DEBUG, True, fmt, *args, **kwargs)

    def infof(self, fmt, *args, **kwargs):
        self._log(Log.INFO, True, fmt, *args, **kwargs)

    def warnf(self, fmt, *args, **kwargs):
        self._log(Log.WARN, True, fmt, *args, **kwargs)

    def errorf(self, fmt, *args, **kwargs):
        self._log(Log.ERROR, True, fmt, *args, **kwargs)

    def fatalf(self, fmt, *args, **kwargs):
        self._log(Log.FATAL, True, fmt, *args, **kwargs)

    def _log(self, level, use_format, fmt, *args, **kwargs):
        if self.isEnabledFor(level):
            frame = inspect.currentframe().f_back    # calling method
            frame = frame.f_back    # original log location
            filename = os.path.split(frame.f_code.co_filename)[1]
            funcname = inspect.stack()[2][3]
            if use_format:
                msg = fmt.format(*args, **kwargs) if args or kwargs else fmt
            else:
                msg = fmt % args if args else fmt
            self.logMsg(level, filename, funcname, frame.f_lineno, msg)
    }
}
