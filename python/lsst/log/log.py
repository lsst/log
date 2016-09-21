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

from ._log import Log

TRACE = Log.TRACE
DEBUG = Log.DEBUG
INFO = Log.INFO
WARN = Log.WARN
ERROR = Log.ERROR
FATAL = Log.FATAL

def Log_trace(self, fmt, *args):
    self._log(Log.TRACE, fmt, *args)
Log.trace = Log_trace
del Log_trace

def Log_debug(self, fmt, *args):
    self._log(Log.DEBUG, fmt, *args)
Log.debug = Log_debug
del Log_debug

def Log_info(self, fmt, *args):
    self._log(Log.INFO, fmt, *args)
Log.info = Log_info
del Log_info

def Log_warn(self, fmt, *args):
    self._log(Log.WARN, fmt, *args)
Log.warn = Log_warn
del Log_warn

def Log_error(self, fmt, *args):
    self._log(Log.ERROR, fmt, *args)
Log.error = Log_error
del Log_error

def Log_fatal(self, fmt, *args):
    self._log(Log.FATAL, fmt, *args)
Log.fatal = Log_fatal
del Log_fatal

def Log__log(self, level, fmt, *args):
    if self.isEnabledFor(level):
        frame = inspect.currentframe().f_back    # calling method
        frame = frame.f_back    # original log location
        filename=os.path.split(frame.f_code.co_filename)[1]
        funcname=inspect.stack()[2][3]
        msg=fmt % args if args else fmt
        self.logMsg(level, filename, funcname, frame.f_lineno, msg)
Log._log = Log__log
del Log__log

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
    Log.getLogger(loggername)._log(level, fmt, *args)


def trace(fmt, *args):
    Log.getDefaultLogger()._log(TRACE, fmt, *args)


def debug(fmt, *args):
    Log.getDefaultLogger()._log(DEBUG, fmt, *args)


def info(fmt, *args):
    Log.getDefaultLogger()._log(INFO, fmt, *args)


def warn(fmt, *args):
    Log.getDefaultLogger()._log(WARN, fmt, *args)


def error(fmt, *args):
    Log.getDefaultLogger()._log(ERROR, fmt, *args)


def fatal(fmt, *args):
    Log.getDefaultLogger()._log(FATAL, fmt, *args)


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
        Log.getLogger(record.name).logMsg(self.translateLevel(record.levelno), record.filename,
                                          record.funcName, record.lineno, record.msg % record.args)

    def translateLevel(self, levelno):
        """
        Translates from standard python logging module levels
        to standard log4cxx levels.
        """
        return levelno*1000
