/*
 * LSST Data Management System
 * Copyright 2013-2014 LSST Corporation.
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

/**
 * @file Log.h
 * @brief LSST DM logging module built on log4cxx.
 *
 * @author Bill Chickering
 * Contact: chickering@cs.stanford.edu
 *
 */

#ifndef LSST_LOG_LOG_H
#define LSST_LOG_LOG_H


// System headers
#include <functional>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <vector>

// Third-party headers
#include <log4cxx/logger.h>
#include <boost/format.hpp>

/**
  * @def LOG_CONFIG(filename)
  * Configures log4cxx and initializes logging module.
  *
  * @param filename  Path to configuration file.
  */
#define LOG_CONFIG(filename) lsst::log::Log::configure(filename)

/**
  * @def LOG_CONFIG_PROP(string)
  * Configures log4cxx from a string containing list of properties.
  * This is equivalent to configuring with a file name containing the same
  * data as a string.
  *
  * @note Use of this macro will likely produce hard-coded configuration
  * which is not advised for general-use code. It may be useful where
  * pre-defined hard-coded configuration is necessary, e.g. in unit tests.
  *
  * @param string  List of properties (lines separated by new line character)
  */
#define LOG_CONFIG_PROP(string) lsst::log::Log::configure_prop(string)

/**
  * @def LOG_DEFAULT_NAME()
  * Get the current default logger name. Returns empty string for root logger.
  * @return String containing the default logger name.
  */
#define LOG_DEFAULT_NAME() lsst::log::Log::getDefaultLoggerName()

/**
  * @def LOG_GET(logger)
  * Returns a Log object associated with logger.
  * @return Log object corresponding to logger.
  *
  * @param logger  Either a logger name or a Log object.
  */
#define LOG_GET(logger) lsst::log::Log::getLogger(logger)

/**
  * @def LOG_PUSHCTX(name)
  * Pushes name onto the global hierarchical default logger name.
  * Note that we only allow simple non-dotted names to be used for
  * context names, multi-level context name (e.g. "componen1.component2")
  * will result in exception. Empty context names are disallowed as
  * well, exception will be raised for empty name.
  *
  * @note Call to this macro is not thread-safe, moreover context is
  * global and applies to all threads (which means you want to avoid
  * using this in multi-threaded applications).
  *
  * @param name  String to push onto logging context.
  * @throw std::invalid_argument raised for empty name or when name contains dot.
  */
#define LOG_PUSHCTX(name) lsst::log::Log::pushContext(name)

/**
  * @def LOG_POPCTX()
  * Pops the last pushed name off the global hierarchical default logger
  * name.
  *
  * @note Call to this macro is not thread-safe.
  */
#define LOG_POPCTX() lsst::log::Log::popContext()

/**
  * @def LOG_MDC(key, value)
  * Places a KEY/VALUE pair in the Mapped Diagnostic Context (MDC) for the
  * current thread. The VALUE may then be included in log messages by using
  * the following the `X` conversion character within a pattern layout as
  * `%X{KEY}`.
  *
  * @param key    Unique key.
  * @param value  String value.
  */
#define LOG_MDC(key, value) lsst::log::Log::MDC(key, value)

/**
  * @def LOG_MDC_REMOVE(key)
  * Remove the value associated with KEY within the MDC.
  *
  * @param key  Key identifying value to remove.
  */
#define LOG_MDC_REMOVE(key) lsst::log::Log::MDCRemove(key)

/**
  * @def LOG_MDC_INIT(function)
  * Register function for initialization of MDC. This function will be called
  * for current thread and every new thread (but not for other existing
  * threads) before any message is logged using one of the macros below. Its
  * main purpose is to initialize MDC (using LOG_MDC macro). In some cases the
  * function may be called more than once per thread.
  *
  * This macro is thread safe, but typically it will be called from main
  * thread before any other LOG macro.
  *
  * Macro returns an integer number, the value is not specified, but this
  * allows it to be used in one-time initialization constructs like:
  *
  *     @code
  *     static int dummyMdcInit = LOG_MDC_INIT(some_init_func);
  *     @endcode
  *
  * @param func Any function object which takes no arguments and returns void.
  */
#define LOG_MDC_INIT(func) lsst::log::Log::MDCRegisterInit(std::function<void()>(func))

/**
  * @def LOG_SET_LVL(logger, level)
  * Set the logging threshold for LOGGER to LEVEL.
  *
  * @param logger  Logger with threshold to adjust.
  * @param level   New logging threshold.
  */
#define LOG_SET_LVL(logger, level) \
    lsst::log::Log::setLevel(logger, level)

/**
  * @def LOG_GET_LVL(logger)
  * Retrieve the logging threshold for LOGGER.
  * @return int Indicating the logging threshold.
  *
  * @param logger  Either a logger name or a Log object with threshold
  *                to return.
  */
#define LOG_GET_LVL(logger) \
    lsst::log::Log::getLevel(logger)

/**
  * @def LOG_CHECK_LVL(logger, level)
  * Return whether the logging threshold of LOGGER is less than or equal
  * to LEVEL.
  * @return Bool indicating whether or not logger is enabled.
  *
  * @param logger  Either a logger name or a Log object being queried.
  * @param level   Logging threshold to check.
  */
#define LOG_CHECK_LVL(logger, level) \
    lsst::log::Log::isEnabledFor(logger, level)

/**
  * @def LOG_CHECK_TRACE()
  * Return whether the logging threshold of the default logger is less
  * than or equal to TRACE.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_TRACE() \
    LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isTraceEnabled())

/**
  * @def LOG_CHECK_DEBUG()
  * Return whether the logging threshold of the default logger is less
  * than or equal to DEBUG.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_DEBUG() \
    LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isDebugEnabled())

/**
  * @def LOG_CHECK_INFO()
  * Return whether the logging threshold of the default logger is less
  * than or equal to INFO.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_INFO() \
        lsst::log::Log::defaultLogger.isInfoEnabled()

/**
  * @def LOG_CHECK_WARN()
  * Return whether the logging threshold of the default logger is less
  * than or equal to WARN.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_WARN() \
        lsst::log::Log::defaultLogger.isWarnEnabled()

/**
  * @def LOG_CHECK_ERROR()
  * Return whether the logging threshold of the default logger is less
  * than or equal to ERROR.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_ERROR() \
        lsst::log::Log::defaultLogger.isErrorEnabled()

/**
  * @def LOG_CHECK_FATAL()
  * Return whether the logging threshold of the default logger is less
  * than or equal to FATAL.
  * @return Bool indicating whether or not logger is enabled.
  */
#define LOG_CHECK_FATAL() \
        lsst::log::Log::defaultLogger.isFatalEnabled()

/**
  * @def LOG(logger, level, message...)
  * Log a message using a varargs/printf style interface.
  *
  * @param logger      Either a logger name or a Log object.
  * @param level       Logging level associated with message.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG(logger, level, message...) \
    do { if (lsst::log::Log::isEnabledFor(logger, level)) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), \
        log4cxx::Level::toLevel(level), \
        LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_TRACE(message...)
  * Log a trace-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_TRACE(message...) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isTraceEnabled())) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getTrace(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_DEBUG(message...)
  * Log a debug-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_DEBUG(message...) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isDebugEnabled())) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getDebug(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_INFO(message...)
  * Log a info-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_INFO(message...) \
    do { if (lsst::log::Log::defaultLogger.isInfoEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getInfo(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_WARN(message...)
  * Log a warn-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_WARN(message...) \
    do { if (lsst::log::Log::defaultLogger.isWarnEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getWarn(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_ERROR(message...)
  * Log a error-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_ERROR(message...) \
    do { if (lsst::log::Log::defaultLogger.isErrorEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getError(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOG_FATAL(message...)
  * Log a fatal-level message to the default logger using a varargs/printf
  * style interface.
  *
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_FATAL(message...) \
    do { if (lsst::log::Log::defaultLogger.isFatalEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getFatal(), LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGS(logger, level, message)
  * Log a message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS("logger", LOG_LVL_DEBUG, "coordinates: x=" << x << " y=" << y);`.
  * Usual caveat regarding commas inside macro arguments applies to
  * message argument.
  *
  * @param logger   Either a logger name or a Log object.
  * @param level    Logging level associated with message.
  * @param message  Message to be logged.
  */
#define LOGS(logger, level, message) \
    do { if (lsst::log::Log::isEnabledFor(logger, level)) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::toLevel(level), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_TRACE(message)
  * Log a trace-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_TRACE("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_TRACE(message) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isTraceEnabled())) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getTrace(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_DEBUG(message)
  * Log a debug-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_DEBUG("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_DEBUG(message) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::defaultLogger.isDebugEnabled())) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getDebug(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_INFO(message)
  * Log a info-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_INFO("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_INFO(message) \
    do { if (lsst::log::Log::defaultLogger.isInfoEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getInfo(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_WARN(message)
  * Log a warning-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_WARN("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_WARN(message) \
    do { if (lsst::log::Log::defaultLogger.isWarnEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getWarn(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_ERROR(message)
  * Log a error-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_ERROR("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_ERROR(message) \
    do { if (lsst::log::Log::defaultLogger.isErrorEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getError(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGS_FATAL(message)
  * Log a fatal-level message to the default logger using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGS_FATAL("coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param message Message to be logged.
  */
#define LOGS_FATAL(message) \
    do { if (lsst::log::Log::defaultLogger.isFatalEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getFatal(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGL_TRACE(logger, message...)
  * Log a trace-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_TRACE(logger, message...) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::getLogger(logger).isTraceEnabled())) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getTrace(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGL_DEBUG(logger, message...)
  * Log a debug-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_DEBUG(logger, message...) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::getLogger(logger).isDebugEnabled())) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getDebug(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGL_INFO(logger, message...)
  * Log a info-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_INFO(logger, message...) \
    do { if (lsst::log::Log::getLogger(logger).isInfoEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getInfo(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGL_WARN(logger, message...)
  * Log a warn-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_WARN(logger, message...) \
    do { if (lsst::log::Log::getLogger(logger).isWarnEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getWarn(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGL_ERROR(logger, message...)
  * Log a error-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_ERROR(logger, message...) \
    do { if (lsst::log::Log::getLogger(logger).isErrorEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getError(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGL_FATAL(logger, message...)
  * Log a fatal-level message using a varargs/printf style interface.
  *
  * @param logger   Either a logger name or a Log object.
  * @param message  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOGL_FATAL(logger, message...) \
    do { if (lsst::log::Log::getLogger(logger).isFatalEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::getLogger(logger), log4cxx::Level::getFatal(), \
            LOG4CXX_LOCATION, message); } \
    } while (false)

/**
  * @def LOGLS_TRACE(logger, message)
  * Log a trace-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_TRACE(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_TRACE(logger, message) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::getLogger(logger).isTraceEnabled())) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getTrace(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGLS_DEBUG(logger, message)
  * Log a debug-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_DEBUG(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_DEBUG(logger, message) \
    do { if (LOG4CXX_UNLIKELY(lsst::log::Log::getLogger(logger).isDebugEnabled())) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getDebug(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGLS_INFO(logger, message)
  * Log a info-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_INFO(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_INFO(logger, message) \
    do { if (lsst::log::Log::getLogger(logger).isInfoEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getInfo(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGLS_WARN(logger, message)
  * Log a warn-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_WARN(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_WARN(logger, message) \
    do { if (lsst::log::Log::getLogger(logger).isWarnEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getWarn(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGLS_ERROR(logger, message)
  * Log a error-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_ERROR(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_ERROR(logger, message) \
    do { if (lsst::log::Log::getLogger(logger).isErrorEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getError(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

/**
  * @def LOGLS_FATAL(logger, message)
  * Log a fatal-level message using an iostream-based interface.
  *
  * Message is any expression which can appear on the right side of the
  * stream insertion operator, e.g.
  * `LOGLS_FATAL(logger, "coordinates: x=" << x << " y=" << y);`. Usual caveat regarding
  * commas inside macro arguments applies to message argument.
  *
  * @param logger  Either a logger name or a Log object.
  * @param message Message to be logged.
  */
#define LOGLS_FATAL(logger, message) \
    do { if (lsst::log::Log::getLogger(logger).isFatalEnabled()) { \
        std::ostringstream stream_; \
        stream_ << message; \
        lsst::log::Log::logMsg(lsst::log::Log::getLogger(logger), \
            log4cxx::Level::getFatal(), LOG4CXX_LOCATION, \
            stream_.str()); } \
    } while (false)

#define LOG_LVL_TRACE static_cast<int>(log4cxx::Level::TRACE_INT)
#define LOG_LVL_DEBUG static_cast<int>(log4cxx::Level::DEBUG_INT)
#define LOG_LVL_INFO static_cast<int>(log4cxx::Level::INFO_INT)
#define LOG_LVL_WARN static_cast<int>(log4cxx::Level::WARN_INT)
#define LOG_LVL_ERROR static_cast<int>(log4cxx::Level::ERROR_INT)
#define LOG_LVL_FATAL static_cast<int>(log4cxx::Level::FATAL_INT)

#define LOG_LOGGER lsst::log::Log
#define LOG_CTX lsst::log::LogContext

namespace lsst {
namespace log {

/** This static class includes a variety of methods for interacting with the
  * the logging module. These methods are not meant for direct use. Rather,
  * they are used by the LOG* macros and the SWIG interface declared in
  * logInterface.h.
  */
class Log {
public:
    Log() : _logger(defaultLogger._logger) { }
    Log(std::string const& contextName) {
        pushContext(contextName);
        _logger = defaultLogger._logger;
        popContext();
    }

    /**
     *  Check whether the logger is enabled for the DEBUG Level
     */
    bool isDebugEnabled(void) const { return _logger->isDebugEnabled(); }
    /**
     *  Check whether the logger is enabled for the ERROR Level
     */
    bool isErrorEnabled(void) const { return _logger->isErrorEnabled(); }
    /**
     *  Check whether the logger is enabled for the FATAL Level
     */
    bool isFatalEnabled(void) const { return _logger->isFatalEnabled(); }
    /**
     *  Check whether the logger is enabled for the INFO Level
     */
    bool isInfoEnabled(void) const { return _logger->isInfoEnabled(); }
    /**
     *  Check whether the logger is enabled for the TRACE Level
     */
    bool isTraceEnabled(void) const { return _logger->isTraceEnabled(); }
    /**
     *  Check whether the logger is enabled for the WARN Level
     */
    bool isWarnEnabled(void) const { return _logger->isWarnEnabled(); }

    static Log defaultLogger;
    static void initLog(void);
    static void configure(void);
    static void configure(std::string const& filename);
    static void configure_prop(std::string const& properties);
    static std::string getDefaultLoggerName(void);
    static Log getLogger(Log logger) { return logger; }
    static Log getLogger(std::string const& loggername);
    static void pushContext(std::string const& name);
    static void popContext(void);
    static void MDC(std::string const& key, std::string const& value);
    static void MDCRemove(std::string const& key);
    static int MDCRegisterInit(std::function<void()> function);
    static void setLevel(Log logger, int level);
    static void setLevel(std::string const& loggername, int level);
    static int getLevel(Log logger);
    static int getLevel(std::string const& loggername);
    static bool isEnabledFor(Log logger, int level);
    static bool isEnabledFor(std::string const& loggername, int level);
    static void log(Log logger, log4cxx::LevelPtr level,
                    log4cxx::spi::LocationInfo const& location,
                    char const* fmt, ...);
    static void logMsg(Log logger, log4cxx::LevelPtr level,
                       log4cxx::spi::LocationInfo const& location,
                       std::string const& msg);

private:
    Log(log4cxx::LoggerPtr const& logger) : _logger(logger) { }

    log4cxx::LoggerPtr _logger;
};

/** This class handles the default logger name of a logging context.
  */
class LogContext {
public:
    /** Create a logging context associated with a default logger name
      * constructed by pushing \p name onto the pre-existing hierarchical default
      * logger name. See comment to \c LOG_PUSHCTX about allowed names.
      *
      * @param name  String to push onto logging context.
      */
    explicit LogContext(std::string const& name) {
        Log::pushContext(name);
    }
    ~LogContext() {
        Log::popContext();
    }

private:
    // cannot copy instances
    LogContext(const LogContext&);
    LogContext& operator=(const LogContext&);
};


/**
 * Function which returns LWP ID on platforms which support it.
 *
 * On all other platforms a small incremental integer number (counting number
 * of threads) is returned. This function can be used to produce more
 * human-friendly thread ID for logging instead of regular %t format.
 */
unsigned lwpID();

}} // namespace lsst::log

#endif // LSST_LOG_LOG_H
