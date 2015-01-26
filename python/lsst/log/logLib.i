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

%pythoncode {
import inspect
import os
}

%include "lsst/log/Log.h"

%extend lsst::log::Log {
    static lsst::log::Log getDefaultLogger() { return lsst::log::Log::defaultLogger; };
    static void log(std::string const& name, int level,
                    std::string const& filename, std::string const& funcname,
                    unsigned int lineno, std::string const& msg) {
        lsst::log::Log::log(name, log4cxx::Level::toLevel(level), filename,
                            funcname, lineno, msg.c_str());
    };
    static void log(Log logger, int level, std::string const& filename,
                    std::string const& funcname, unsigned int lineno,
                    std::string const& msg) {
        lsst::log::Log::log(logger, log4cxx::Level::toLevel(level), filename,
                            funcname, lineno, msg.c_str());
    };

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
        if Log.isEnabledFor(self, level):
            frame = inspect.currentframe().f_back    # calling method
            frame = frame.f_back    # original log location
            Log.log(self, level, os.path.split(frame.f_code.co_filename)[1],
                    inspect.stack()[2][3], frame.f_lineno, fmt % args)
    }
}

