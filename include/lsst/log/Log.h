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
#include <stack>
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
  * @def LOG_DEFAULT_NAME()
  * Get the current default logger name.
  * @return String containing the default logger name.
  */
#define LOG_DEFAULT_NAME() lsst::log::Log::getDefaultLoggerName()

/**
  * @def LOG_GET(logger)
  * Returns a pointer to the log4cxx logger object associated with LOGGER.
  * @return log4cxx::LoggerPtr corresponding to LOGGER.
  *
  * @param logger  Either a logger name or a log4cxx logger object.
  */
#define LOG_GET(logger) lsst::log::Log::getLogger(logger)

/**
  * @def LOG_NEWCTX(name)
  * Create a new logging context object.
  * @return LogContext object.
  *
  * @param name  String containing name to push onto the logging context.
  */
#define LOG_NEWCTX(name) (new lsst::log::LogContext(name))

/**
  * @def LOG_PUSHCTX(name)
  * Pushes NAME onto the global hierarchical default logger name.
  *
  * @param name  String to push onto logging context.
  */
#define LOG_PUSHCTX(name) lsst::log::Log::pushContext(name)

/**
  * @def LOG_POPCTX()
  * Pops the last pushed name off the global hierarchical default logger
  * name.
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
  * @param logger  Either name of logger or log4cxx logger with threshold
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
  * @param logger  Either name of logger or log4cxx logger being queried.
  * @param level   Logging threshold to check.
  */
#define LOG_CHECK_LVL(logger, level) \
    lsst::log::Log::isEnabledFor(logger, level)

/**
  * @def LOGF(logger, level, message)
  * Log a message using a boost::format style interface.
  *
  * @param logger   Either name of logger or log4cxx logger object.
  * @param level    Logging level associated with message.
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF(logger, level, message) \
    if (lsst::log::Log::isEnabledFor(logger, level)) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::getLogger(logger)->forcedLog( \
            log4cxx::Level::toLevel(level), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_TRACE(message)
  * Log a trace-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_TRACE(message) \
    if (lsst::log::Log::defaultLogger->isTraceEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getTrace(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_DEBUG(message)
  * Log a debug-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_DEBUG(message) \
    if (lsst::log::Log::defaultLogger->isDebugEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getDebug(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_INFO(message)
  * Log a info-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_INFO(message) \
    if (lsst::log::Log::defaultLogger->isInfoEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getInfo(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_WARN(message)
  * Log a warn-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_WARN(message) \
    if (lsst::log::Log::defaultLogger->isWarnEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getWarn(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_ERROR(message)
  * Log a error-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_ERROR(message) \
    if (lsst::log::Log::defaultLogger->isErrorEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getError(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOGF_FATAL(message)
  * Log a fatal-level message to the default logger using a boost::format
  * syle interface.
  *
  * @param message  A boost::format compatible format string followed by
  *                 zero, one, or more arguments separated by `%`.
  */
#define LOGF_FATAL(message) \
    if (lsst::log::Log::defaultLogger->isFatalEnabled()) { \
        lsst::log::LogFormatter fmter_; \
        lsst::log::Log::defaultLogger->forcedLog( \
            log4cxx::Level::getFatal(), (fmter_ % message).str().c_str(), \
            LOG4CXX_LOCATION); }

/**
  * @def LOG(logger, level, message...)
  * Log a message using a varargs/printf style interface.
  *
  * @param logger      Either name of logger or log4cxx logger object.
  * @param level       Logging level associated with message.
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG(logger, level, message...) \
    if (lsst::log::Log::isEnabledFor(logger, level)) { \
        lsst::log::Log::log(logger, log4cxx::Level::toLevel(level), \
        __BASE_FILE__, __PRETTY_FUNCTION__, __LINE__, message); }

/**
  * @def LOG_TRACE(message...)
  * Log a trace-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_TRACE(message...) \
    if (lsst::log::Log::defaultLogger->isTraceEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getTrace(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

/**
  * @def LOG_DEBUG(message...)
  * Log a debug-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_DEBUG(message...) \
    if (lsst::log::Log::defaultLogger->isDebugEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getDebug(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

/**
  * @def LOG_INFO(message...)
  * Log a info-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_INFO(message...) \
    if (lsst::log::Log::defaultLogger->isInfoEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getInfo(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

/**
  * @def LOG_WARN(message...)
  * Log a warn-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_WARN(message...) \
    if (lsst::log::Log::defaultLogger->isWarnEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getWarn(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

/**
  * @def LOG_ERROR(message...)
  * Log a error-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_ERROR(message...) \
    if (lsst::log::Log::defaultLogger->isErrorEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getError(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

/**
  * @def LOG_FATAL(message...)
  * Log a fatal-level message to the default logger using a varargs/printf
  * syle interface.
  *
  * @param message...  An sprintf-compatible format string followed by zero,
  *                    one, or more comma-separated arguments.
  */
#define LOG_FATAL(message...) \
    if (lsst::log::Log::defaultLogger->isFatalEnabled()) { \
        lsst::log::Log::log(lsst::log::Log::defaultLogger, \
            log4cxx::Level::getFatal(), __BASE_FILE__, __PRETTY_FUNCTION__, \
            __LINE__, message); }

#define LOG_LVL_TRACE log4cxx::Level::TRACE_INT
#define LOG_LVL_DEBUG log4cxx::Level::DEBUG_INT
#define LOG_LVL_INFO log4cxx::Level::INFO_INT
#define LOG_LVL_WARN log4cxx::Level::WARN_INT
#define LOG_LVL_ERROR log4cxx::Level::ERROR_INT
#define LOG_LVL_FATAL log4cxx::Level::FATAL_INT

#define LOG_LOGGER log4cxx::LoggerPtr
#define LOG_CTX lsst::log::LogContext

namespace lsst {
namespace log {

/** This class is used by the LOGF_INFO and similar macros to support the
  * boost::format-like operators in the message parameter.
  */
class LogFormatter {
public:
    LogFormatter(void);
    ~LogFormatter(void);

    /** Converts a format string into a boost::format object.
      * @return a new boost::format object
      *
      * @param fmt  format string
      */
    template <typename T> boost::format& operator %(T fmt) {
        _enabled = true;
        _fmter = new boost::format(fmt);
        return *_fmter;
    }
private:
    bool _enabled;
    boost::format* _fmter;
};

/** This class handles the default logger name of a logging context.
  */
class LogContext {
public:
    LogContext(std::string const& name);
    ~LogContext(void);
private:
    std::string _name;
};

/** This static class includes a variety of methods for interacting with the
  * the logging module. These methods are not meant for direct use. Rather,
  * they are used by the LOG* macros and the SWIG interface declared in
  * logInterface.h.
  */
class Log {
public:
    static log4cxx::LoggerPtr defaultLogger;
    static void initLog(void);
    static void configure(void);
    static void configure(std::string const& filename);
    static std::string getDefaultLoggerName(void);
    static log4cxx::LoggerPtr getLogger(log4cxx::LoggerPtr logger);
    static log4cxx::LoggerPtr getLogger(std::string const& loggername);
    static void pushContext(std::string const& name);
    static void popContext(void);
    static void MDC(std::string const& key, std::string const& value);
    static void MDCRemove(std::string const& key);
    static void setLevel(log4cxx::LoggerPtr logger, int level);
    static void setLevel(std::string const& loggername, int level);
    static int getLevel(log4cxx::LoggerPtr logger);
    static int getLevel(std::string const& loggername);
    static bool isEnabledFor(log4cxx::LoggerPtr logger, int level);
    static bool isEnabledFor(std::string const& loggername, int level);
    static void vlog(log4cxx::LoggerPtr logger, log4cxx::LevelPtr level,
                     std::string const& filename, std::string const& funcname,
                     unsigned int lineno, std::string const& fmt, va_list args);
    static void log(std::string const& loggername, log4cxx::LevelPtr level,
                    std::string const& filename, std::string const& funcname,
                    unsigned int lineno, std::string const& fmt, ...);
    static void log(log4cxx::LoggerPtr logger, log4cxx::LevelPtr level,
                    std::string const& filename, std::string const& funcname,
                    unsigned int lineno, std::string const& fmt, ...);
private:
    static std::stack<std::string> context;
    static std::string defaultLoggerName;
};

}} // namespace lsst::log

#endif // LSST_LOG_LOG_H
