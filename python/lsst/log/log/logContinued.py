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

__all__ = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
           "Log", "configure", "configure_prop", "getDefaultLogger", "getLogger",
           "MDC", "MDCRemove", "MDCRegisterInit", "setLevel", "getLevel", "isEnabledFor",
           "log", "trace", "debug", "info", "warn", "warning", "error", "fatal", "logf",
           "tracef", "debugf", "infof", "warnf", "errorf", "fatalf", "lwpID",
           "usePythonLogging", "doNotUsePythonLogging", "UsePythonLogging",
           "LevelTranslator", "LogHandler"]

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


@continueClass  # noqa F811 redefinition
class Log:
    UsePythonLogging = False
    """Forward Python `lsst.log` messages to Python `logging` package."""

    @classmethod
    def usePythonLogging(cls):
        """Forward log messages to Python `logging`

        Notes
        -----
        This is useful for unit testing when you want to ensure
        that log messages are captured by the testing environment
        as distinct from standard output.

        This state only affects messages sent to the `lsst.log`
        package from Python.
        """
        cls.UsePythonLogging = True

    @classmethod
    def doNotUsePythonLogging(cls):
        """Forward log messages to LSST logging system.

        Notes
        -----
        This is the default state.
        """
        cls.UsePythonLogging = False

    def trace(self, fmt, *args):
        self._log(Log.TRACE, False, fmt, *args)

    def debug(self, fmt, *args):
        self._log(Log.DEBUG, False, fmt, *args)

    def info(self, fmt, *args):
        self._log(Log.INFO, False, fmt, *args)

    def warn(self, fmt, *args):
        self._log(Log.WARN, False, fmt, *args)

    def warning(self, fmt, *args):
        self.warn(fmt, *args)

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
            funcname = frame.f_code.co_name
            if use_format:
                msg = fmt.format(*args, **kwargs) if args or kwargs else fmt
            else:
                msg = fmt % args if args else fmt
            if self.UsePythonLogging:
                pylog = logging.getLogger(self.getName())
                record = logging.LogRecord(self.getName(), LevelTranslator.lsstLog2logging(level),
                                           filename, frame.f_lineno, msg, None, False, func=funcname)
                pylog.handle(record)
            else:
                self.logMsg(level, filename, funcname, frame.f_lineno, msg)

    def __reduce__(self):
        """Implement pickle support.
        """
        args = (self.getName(), )
        # method has to be module-level, not class method
        return (getLogger, args)

# Export static functions from Log class to module namespace


def configure(*args):
    Log.configure(*args)


def configure_prop(properties):
    Log.configure_prop(properties)


def getDefaultLogger():
    return Log.getDefaultLogger()


def getLogger(loggername):
    return Log.getLogger(loggername)


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


# This will cause a warning in Sphinx documentation due to confusion between
# Log and log. https://github.com/astropy/sphinx-automodapi/issues/73 (but
# note that this does not seem to be Mac-only).
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


def warning(fmt, *args):
    warn(fmt, *args)


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


# This will cause a warning in Sphinx documentation due to confusion between
# UsePythonLogging and usePythonLogging.
# https://github.com/astropy/sphinx-automodapi/issues/73 (but note that this
# does not seem to be Mac-only).
def usePythonLogging():
    Log.usePythonLogging()


def doNotUsePythonLogging():
    Log.doNotUsePythonLogging()


class UsePythonLogging:
    """Context manager to enable Python log forwarding temporarily.
    """

    def __init__(self):
        self.current = Log.UsePythonLogging

    def __enter__(self):
        Log.usePythonLogging()

    def __exit__(self, exc_type, exc_value, traceback):
        Log.UsePythonLogging = self.current


class LevelTranslator:
    """Helper class to translate levels between ``lsst.log`` and Python
    `logging`.
    """
    @staticmethod
    def lsstLog2logging(level):
        """Translates from lsst.log/log4cxx levels to `logging` module levels.

        Parameters
        ----------
        level : `int`
            Logging level number used by `lsst.log`, typically one of the
            constants defined in this module (`DEBUG`, `INFO`, etc.)

        Returns
        -------
        level : `int`
            Correspoding logging level number for Python `logging` module.
        """
        # Python logging levels are same as lsst.log divided by 1000,
        # logging does not have TRACE level by default but it is OK to use
        # that numeric level and we may even add TRACE later.
        return level//1000

    @staticmethod
    def logging2lsstLog(level):
        """Translates from standard python `logging` module levels to
        lsst.log/log4cxx levels.

        Parameters
        ----------
        level : `int`
            Logging level number used by Python `logging`, typically one of
            the constants defined by `logging` module (`logging.DEBUG`,
            `logging.INFO`, etc.)

        Returns
        -------
        level : `int`
            Correspoding logging level number for `lsst.log` module.
        """
        return level*1000


class LogHandler(logging.Handler):
    """Handler for Python logging module that emits to LSST logging.

    Parameters
    ----------
    level : `int`
        Level at which to set the this handler.

    Notes
    -----
    If this handler is enabled and `lsst.log` has been configured to use
    Python `logging`, the handler will do nothing itself if any other
    handler has been registered with the Python logger.  If it does not
    think that anything else is handling the message it will attempt to
    send the message via a default `~logging.StreamHandler`.  The safest
    approach is to configure the logger with an additional handler
    (possibly the ROOT logger) if `lsst.log` is to be configured to use
    Python logging.
    """

    def __init__(self, level=logging.NOTSET):
        logging.Handler.__init__(self, level=level)
        # Format as a simple message because lsst.log will format the
        # message a second time.
        self.formatter = logging.Formatter(fmt="%(message)s")

    def handle(self, record):
        logger = Log.getLogger(record.name)
        if logger.isEnabledFor(LevelTranslator.logging2lsstLog(record.levelno)):
            logging.Handler.handle(self, record)

    def emit(self, record):
        if Log.UsePythonLogging:
            # Do not forward this message to lsst.log since this may cause
            # a logging loop.

            # Work out whether any other handler is going to be invoked
            # for this logger.
            pylgr = logging.getLogger(record.name)

            # If another handler is registered that is not LogHandler
            # we ignore this request
            if any(not isinstance(h, self.__class__) for h in pylgr.handlers):
                return

            # If the parent has handlers and propagation is enabled
            # we punt as well (and if a LogHandler is involved then we will
            # ask the same question when we get to it).
            if pylgr.parent and pylgr.parent.hasHandlers() and pylgr.propagate:
                return

            # Force this message to appear somewhere.
            # If something else should happen then the caller should add a
            # second Handler.
            stream = logging.StreamHandler()
            stream.setFormatter(logging.Formatter(fmt="%(name)s %(levelname)s (fallback): %(message)s"))
            stream.handle(record)
            return

        logger = Log.getLogger(record.name)
        # Use standard formatting class to format message part of the record
        message = self.formatter.format(record)

        logger.logMsg(LevelTranslator.logging2lsstLog(record.levelno),
                      record.filename, record.funcName,
                      record.lineno, message)
