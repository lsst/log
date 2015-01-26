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
from os import path

from .logLib import Log

TRACE = Log.TRACE
DEBUG = Log.DEBUG
INFO = Log.INFO
WARN = Log.WARN
ERROR = Log.ERROR
FATAL = Log.FATAL

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

def isEnabledFor(logger, level):
    Log.isEnabledFor(Log.getLogger(logger), level)

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

class LogContext(object):
    """Context manager for logging."""

    def __init__(self, name=None, level=None):
        self.name = name
        self.level = level

    def __enter__(self):
        self.open()
        return self

    def __exit__(self ,type, value, traceback):
        self.close()

    def __del__(self):
        self.close()

    def open(self):
        if self.name is not None:
            Log.pushContext(self.name)
        if self.level is not None:
            Log.setLevel("", self.level)

    def close(self):
        if self.name is not None:
            Log.popContext()
            self.name = None

    def setLevel(self, level):
        Log.setLevel("", level)

    def getLevel(self):
        return Log.getLevel("")

    def isEnabledFor(self, level):
        return Log.isEnabledFor("", level)

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
        Log.log(Log.getDefaultLogger(),
                self.translateLevel(record.levelno),
                record.filename, record.funcName, record.lineno,
                record.msg % record.args)

    def translateLevel(self, levelno):
        """
        Translates from standard python logging module levels
        to standard log4cxx levels.
        """
        return levelno*1000
