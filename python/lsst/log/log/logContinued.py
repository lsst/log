#!/usr/bin/env python

#
# LSST Data Management System
# Copyright 2013 LSST Corporation.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#

import logging
import inspect
import os

from lsst.utils import continueClass

from .log import Log

TRACE = 5000
DEBUG = 10000
INFO = 20000
WARN = 30000
ERROR = 40000
FATAL = 50000

@continueClass
class Log:
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


# Export static functions from Log class to module namespace


def configure(*args):
    Log.configure(*args)


def configure_prop(properties):
    Log.configure_prop(properties)


def getDefaultLoggerName():
    return Log.getDefaultLoggerName()


def pushContext(name):
    Log.pushContext(name)


def popContext():
    Log.popContext()


def MDC(key, value):
    Log.MDC(key, str(value))


def MDCRemove(key):
    Log.MDCRemove(key)


def MDCRegisterInit(func):
    Log.MDCRegisterInit(func)


def setLevel(loggername, level):
    Log.getLogger(loggername).setLevel(level)


def getLevel(loggername):
    Log.getLogger(loggername).getLevel()


def isEnabledFor(logger, level):
    Log.getLogger(logger).isEnabledFor(level)


def log(loggername, level, fmt, *args, **kwargs):
    Log.getLogger(loggername)._log(level, False, fmt, *args)


def trace(fmt, *args):
    Log.getDefaultLogger()._log(TRACE, False, fmt, *args)


def debug(fmt, *args):
    Log.getDefaultLogger()._log(DEBUG, False, fmt, *args)


def info(fmt, *args):
    Log.getDefaultLogger()._log(INFO, False, fmt, *args)


def warn(fmt, *args):
    Log.getDefaultLogger()._log(WARN, False, fmt, *args)


def error(fmt, *args):
    Log.getDefaultLogger()._log(ERROR, False, fmt, *args)


def fatal(fmt, *args):
    Log.getDefaultLogger()._log(FATAL, False, fmt, *args)


def logf(loggername, level, fmt, *args, **kwargs):
    Log.getLogger(loggername)._log(level, True, fmt, *args, **kwargs)


def tracef(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(TRACE, True, fmt, *args, **kwargs)


def debugf(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(DEBUG, True, fmt, *args, **kwargs)


def infof(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(INFO, True, fmt, *args, **kwargs)


def warnf(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(WARN, True, fmt, *args, **kwargs)


def errorf(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(ERROR, True, fmt, *args, **kwargs)


def fatalf(fmt, *args, **kwargs):
    Log.getDefaultLogger()._log(FATAL, True, fmt, *args, **kwargs)


def lwpID():
    return Log.lwpID


class LogContext(object):
    """Context manager for logging."""

    def __init__(self, name=None, level=None):
        self.name = name
        self.level = level

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def __del__(self):
        self.close()

    def open(self):
        if self.name is not None:
            Log.pushContext(self.name)
        if self.level is not None:
            Log.getDefaultLogger().setLevel(self.level)

    def close(self):
        if self.name is not None:
            Log.popContext()
            self.name = None

    def setLevel(self, level):
        Log.getDefaultLogger().setLevel(level)

    def getLevel(self):
        return Log.getDefaultLogger().getLevel()

    def isEnabledFor(self, level):
        return Log.getDefaultLogger().isEnabledFor(level)


class LogHandler(logging.Handler):
    """Handler for Python logging module that emits to LSST logging."""

    def __init__(self, name=None, level=None):
        self.context = LogContext(name=name, level=level)
        self.context.open()
        logging.Handler.__init__(self)

    def __del__(self):
        self.close()

    def close(self):
        if self.context is not None:
            self.context.close()
            self.context = None
        logging.Handler.close(self)

    def handle(self, record):
        if self.context.isEnabledFor(self.translateLevel(record.levelno)):
            logging.Handler.handle(self, record)

    def emit(self, record):
        Log.getLogger(record.name).logMsg(self.translateLevel(record.levelno),
                                          record.filename, record.funcName,
                                          record.lineno,
                                          record.msg % record.args)

    def translateLevel(self, levelno):
        """
        Translates from standard python logging module levels
        to standard log4cxx levels.
        """
        return levelno*1000

