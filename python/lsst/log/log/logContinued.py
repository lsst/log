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

__all__ = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "CRITICAL", "WARNING",
           "Log", "configure", "configure_prop", "configure_pylog_MDC", "getDefaultLogger",
           "getLogger", "MDC", "MDCDict", "MDCRemove", "MDCRegisterInit", "setLevel",
           "getLevel", "isEnabledFor", "log", "trace", "debug", "info", "warn", "warning",
           "error", "fatal", "critical", "logf", "tracef", "debugf", "infof", "warnf", "errorf", "fatalf",
           "lwpID", "usePythonLogging", "doNotUsePythonLogging", "UsePythonLogging",
           "LevelTranslator", "LogHandler", "getEffectiveLevel", "getLevelName"]

import logging
import inspect
import os

from typing import Optional
from deprecated.sphinx import deprecated

from lsst.utils import continueClass

from .log import Log

TRACE = 5000
DEBUG = 10000
INFO = 20000
WARN = 30000
ERROR = 40000
FATAL = 50000

# For compatibility with python logging
CRITICAL = FATAL
WARNING = WARN


@continueClass  # noqa: F811 (FIXME: remove for py 3.8+)
class Log:  # noqa: F811
    UsePythonLogging = False
    """Forward Python `lsst.log` messages to Python `logging` package."""

    CRITICAL = CRITICAL
    WARNING = WARNING

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

    @property
    def name(self):
        return self.getName()

    @property
    def level(self):
        return self.getLevel()

    @property
    def parent(self):
        """Returns the parent logger, or None if this is the root logger."""
        if not self.name:
            return None
        parent_name = self.name.rpartition(".")[0]
        if not parent_name:
            return self.getDefaultLogger()
        return self.getLogger(parent_name)

    def trace(self, fmt, *args):
        self._log(Log.TRACE, False, fmt, *args)

    def debug(self, fmt, *args):
        self._log(Log.DEBUG, False, fmt, *args)

    def info(self, fmt, *args):
        self._log(Log.INFO, False, fmt, *args)

    def warn(self, fmt, *args):
        self._log(Log.WARN, False, fmt, *args)

    def warning(self, fmt, *args):
        # Do not call warn() because that will result in an incorrect
        # line number in the log.
        self._log(Log.WARN, False, fmt, *args)

    def error(self, fmt, *args):
        self._log(Log.ERROR, False, fmt, *args)

    def fatal(self, fmt, *args):
        self._log(Log.FATAL, False, fmt, *args)

    def critical(self, fmt, *args):
        # Do not call fatal() because that will result in an incorrect
        # line number in the log.
        self._log(Log.FATAL, False, fmt, *args)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
    def tracef(self, fmt, *args, **kwargs):
        self._log(Log.TRACE, True, fmt, *args, **kwargs)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
    def debugf(self, fmt, *args, **kwargs):
        self._log(Log.DEBUG, True, fmt, *args, **kwargs)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
    def infof(self, fmt, *args, **kwargs):
        self._log(Log.INFO, True, fmt, *args, **kwargs)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
    def warnf(self, fmt, *args, **kwargs):
        self._log(Log.WARN, True, fmt, *args, **kwargs)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
    def errorf(self, fmt, *args, **kwargs):
        self._log(Log.ERROR, True, fmt, *args, **kwargs)

    @deprecated(reason="f-string log messages are now deprecated to match python logging convention."
                " Will be removed after v25",
                version="v23.0", category=FutureWarning)
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
                levelno = LevelTranslator.lsstLog2logging(level)
                levelName = logging.getLevelName(levelno)

                pylog = logging.getLogger(self.getName())
                record = logging.makeLogRecord(dict(name=self.getName(),
                                                    levelno=levelno,
                                                    levelname=levelName,
                                                    msg=msg,
                                                    funcName=funcname,
                                                    filename=filename,
                                                    pathname=frame.f_code.co_filename,
                                                    lineno=frame.f_lineno))
                pylog.handle(record)
            else:
                self.logMsg(level, filename, funcname, frame.f_lineno, msg)

    def __reduce__(self):
        """Implement pickle support.
        """
        args = (self.getName(), )
        # method has to be module-level, not class method
        return (getLogger, args)

    def __repr__(self):
        # Match python logging style.
        cls = type(self)
        class_name = f"{cls.__module__}.{cls.__qualname__}"
        prefix = "lsst.log.log.log"
        if class_name.startswith(prefix):
            class_name = class_name.replace(prefix, "lsst.log")
        return f"<{class_name} '{self.name}' ({getLevelName(self.getEffectiveLevel())})>"


class MDCDict(dict):
    """Dictionary for MDC data.

    This is internal class used for better formatting of MDC in Python logging
    output. It behaves like `defaultdict(str)` but overrides ``__str__`` and
    ``__repr__`` method to produce output better suited for logging records.
    """
    def __getitem__(self, name: str):
        """Returns value for a given key or empty string for missing key.
        """
        return self.get(name, "")

    def __str__(self):
        """Return string representation, strings are interpolated without
        quotes.
        """
        items = (f"{k}={self[k]}" for k in sorted(self))
        return "{" + ", ".join(items) + "}"

    def __repr__(self):
        return str(self)


# Export static functions from Log class to module namespace


def configure(*args):
    Log.configure(*args)


def configure_prop(properties):
    Log.configure_prop(properties)


def configure_pylog_MDC(level: str, MDC_class: Optional[type] = MDCDict):
    """Configure log4cxx to send messages to Python logging, with MDC support.

    Parameters
    ----------
    level : `str`
        Name of the logging level for root log4cxx logger.
    MDC_class : `type`, optional
        Type of dictionary which is added to `logging.LogRecord` as an ``MDC``
        attribute. Any dictionary or ``defaultdict``-like class can be used as
        a type. If `None` the `logging.LogRecord` will not be augmented.

    Notes
    -----
    This method does two things:

    - Configures log4cxx with a given logging level and a ``PyLogAppender``
      appender class which forwards all messages to Python `logging`.
    - Installs a record factory for Python `logging` that adds ``MDC``
      attribute to every `logging.LogRecord` object (instance of
      ``MDC_class``). This will happen by default but can be disabled
      by setting the ``MDC_class`` parameter to `None`.
    """
    if MDC_class is not None:
        old_factory = logging.getLogRecordFactory()

        def record_factory(*args, **kwargs):
            record = old_factory(*args, **kwargs)
            record.MDC = MDC_class()
            return record

        logging.setLogRecordFactory(record_factory)

    properties = """\
log4j.rootLogger = {}, PyLog
log4j.appender.PyLog = PyLogAppender
""".format(level)
    configure_prop(properties)


def getDefaultLogger():
    return Log.getDefaultLogger()


def getLogger(loggername):
    return Log.getLogger(loggername)


def MDC(key, value):
    return Log.MDC(key, str(value))


def MDCRemove(key):
    Log.MDCRemove(key)


def MDCRegisterInit(func):
    Log.MDCRegisterInit(func)


def setLevel(loggername, level):
    Log.getLogger(loggername).setLevel(level)


def getLevel(loggername):
    return Log.getLogger(loggername).getLevel()


def getEffectiveLevel(loggername):
    return Log.getLogger(loggername).getEffectiveLevel()


def isEnabledFor(loggername, level):
    return Log.getLogger(loggername).isEnabledFor(level)


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


def critical(fmt, *args):
    fatal(fmt, *args)


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


def getLevelName(level):
    """Return the name associated with this logging level.

    Returns "Level %d" if no name can be found.
    """
    names = ("DEBUG", "TRACE", "WARNING", "FATAL", "INFO", "ERROR")
    for name in names:
        test_level = getattr(Log, name)
        if test_level == level:
            return name
    return f"Level {level}"


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
