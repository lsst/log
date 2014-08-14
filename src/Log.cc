// -*- LSST-C++ -*-
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
 * @file Log.cc
 * @brief LSST DM logging module built on log4cxx.
 *
 * @author Bill Chickering
 * Contact: chickering@cs.stanford.edu
 *
 */

// System headers
#include <stdio.h>

// Third-party headers
#include <log4cxx/consoleappender.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

// Local headers
#include "lsst/log/Log.h"


// Max message length for varargs/printf style logging
#define MAX_LOG_MSG_LEN 1024

namespace lsst {
namespace log {

// LogFormatter class

LogFormatter::LogFormatter() : _enabled(false) {}
    
LogFormatter::~LogFormatter() {
    if (_enabled) {
        delete _fmter;
    }
}

// LogContext class

/** Create a logging context associated with a default logger name
  * constructed by pushing NAME onto the pre-existing hierarchical default
  * logger name.
  *
  * @param name  String to push onto logging context.
  */
LogContext::LogContext(std::string const& name) {
    _name = name;
    Log::pushContext(name);
}

LogContext::~LogContext() {
    if (!_name.empty()) {
        Log::popContext();
    }
}

// Log class

/** Reference to the defaultLogger used by LOG* macros.
  */
log4cxx::LoggerPtr Log::defaultLogger = log4cxx::Logger::getRootLogger();

std::stack<std::string> Log::context;
std::string Log::defaultLoggerName;

/** Initializes logging module (e.g. default logger and logging context).
  */
void Log::initLog() {
    // Clear context
    while (!context.empty()) context.pop();
    // Default logger initially set to root logger
    defaultLogger = log4cxx::Logger::getRootLogger();
}

/** Configures log4cxx and initializes logging system by invoking
  * initLog(). Uses either default configuration or log4cxx basic
  * configuration according to the following algorithm (see
  * http://logging.apache.org/log4cxx/usage.html for additional details):
  *
  * Set the configurationOptionStr string variable to the value of the
  * LOG4CXX_CONFIGURATION environment variable if set, otherwise the value
  * of the log4j.configuration or LOG4CXX_CONFIGURATION environment
  * variable if set, otherwise the first of the following file names which
  * exist in the current working directory, "log4cxx.xml",
  * "log4cxx.properties", "log4j.xml" and "log4j.properties". If
  * configurationOptionStr has not been set, then use BasicConfigurator,
  * which is hardwired to add to the root logger a ConsoleAppender. In this
  * case, the output will be formatted using a PatternLayout set to the
  * pattern "%-4r [%t] %-5p %c %x - %m%n".
  */
void Log::configure() {
    log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
    if (rootLogger->getAllAppenders().size() == 0) {
        log4cxx::BasicConfigurator::configure();
    }
    LOG4CXX_INFO(rootLogger, "Initializing Logging System");
    initLog();
}

std::string getFileExtension(std::string const& filename) {
    size_t dotpos = filename.find_last_of(".");
    if (dotpos == std::string::npos) {
      return "";
    }
    return filename.substr(dotpos, filename.size() - dotpos);
}

/** Configures log4cxx using FILENAME and initializes logging module by
  * invoking initLog(). If FILENAME ends with ".xml", it is passed to
  * log4cxx::xml::DOMConfigurator::configure(). Otherwise, it assumed to be
  * a log4j Java properties file and is passed to
  * log4cxx::PropertyConfigurator::configure(). See
  * http://logging.apache.org/log4cxx/usage.html for additional details.
  *
  * @param filename  Path to configuration file.
  */
void Log::configure(std::string const& filename) {
    log4cxx::BasicConfigurator::resetConfiguration();
    if (getFileExtension(filename).compare(".xml") == 0) {
        log4cxx::xml::DOMConfigurator::configure(filename);
    } else {
        log4cxx::PropertyConfigurator::configure(filename);
    }
    initLog();
}

/** Get the current default logger name.
  * @return String containing the default logger name.
  */
std::string Log::getDefaultLoggerName() {
    return defaultLoggerName;
}

/** This method exists solely to simplify the LOGF macro. It merely returns
  * the argument LOGGER.
  * @return log4cxx::LoggerPtr passed in.
  *
  * @param logger  log4cxx::LoggerPtr to return.
  */
log4cxx::LoggerPtr Log::getLogger(log4cxx::LoggerPtr logger) {
    return logger;
}

/** Returns a pointer to the log4cxx logger object associated with
  * LOGGERNAME.
  * @return log4cxx::LoggerPtr corresponding to LOGGERNAME.
  *
  * @param loggername  Name of logger to return.
  */
log4cxx::LoggerPtr Log::getLogger(std::string const& loggername) {
    if (loggername.empty()){
        return defaultLogger;
    } else {
        return log4cxx::Logger::getLogger(loggername);
    }
}

/** Pushes NAME onto the global hierarchical default logger name.
  *
  * @param name  String to push onto logging context.
  */
void Log::pushContext(std::string const& name) {
    context.push(name);
    // Construct new default logger name
    std::stringstream ss;
    if (defaultLoggerName.empty()) {
        ss << name;
    } else {
        ss << defaultLoggerName << "." << name;
    }
    defaultLoggerName = ss.str();
    // Update defaultLogger
    defaultLogger = log4cxx::Logger::getLogger(defaultLoggerName);
}

/** Pops the last pushed name off the global hierarchical default logger
  * name.
  */
void Log::popContext() {
    context.pop();
    // construct new default logger name
    std::string::size_type pos = defaultLoggerName.find_last_of('.');

    // Update defaultLogger
    if (pos >= std::string::npos) {
        defaultLoggerName = "";
        defaultLogger = log4cxx::Logger::getRootLogger();
    } else {
        defaultLoggerName = defaultLoggerName.substr(0, pos);
        defaultLogger = log4cxx::Logger::getLogger(defaultLoggerName);
    }
}

/** Places a KEY/VALUE pair in the Mapped Diagnostic Context (MDC) for the
  * current thread. The VALUE may then be included in log messages by using
  * the following the `X` conversion character within a pattern layout as
  * `%X{KEY}`.
  *
  * @param key    Unique key.
  * @param value  String value.
  */
void Log::MDC(std::string const& key, std::string const& value) {
    log4cxx::MDC::put(key, value);
}

/** Remove the value associated with KEY within the MDC.
  *
  * @param key  Key identifying value to remove.
  */
void Log::MDCRemove(std::string const& key) {
    log4cxx::MDC::remove(key);
}

/** Set the logging threshold for LOGGER to LEVEL.
  *
  * @param logger  Logger with threshold to adjust.
  * @param level   New logging threshold.
  */
void Log::setLevel(log4cxx::LoggerPtr logger, int level) {
    logger->setLevel(log4cxx::Level::toLevel(level));
}

/** Set the logging threshold for the logger named LOGGERNAME to LEVEL.
  *
  * @param loggername  Name of logger with threshold to adjust.
  * @param level       New logging threshold.
  */
void Log::setLevel(std::string const& loggername, int level) { 
    setLevel(getLogger(loggername), level); 
}

/** Retrieve the logging threshold for LOGGER.
  * @return int Indicating the logging threshold.
  *
  * @param logger  Logger with threshold to return.
  */
int Log::getLevel(log4cxx::LoggerPtr logger) {
    log4cxx::LevelPtr level = logger->getLevel();
    int levelno = -1;
    if (level != NULL) {
        levelno = level->toInt();
    }
    return levelno;
}

/** Retrieve the logging threshold for the logger name LOGGERNAME.
  * @return Int indicating the logging threshold.
  *
  * @param loggername  Name of logger with threshold to return.
  */
int Log::getLevel(std::string const& loggername) {
    return getLevel(getLogger(loggername));
}

/** Return whether the logging threshold of LOGGER is less than or equal
  * to LEVEL.
  * @return Bool indicating whether or not logger is enabled.
  *
  * @param logger  Logger being queried.
  * @param level   Logging threshold to check.
  */
bool Log::isEnabledFor(log4cxx::LoggerPtr logger, int level) {
    if (logger->isEnabledFor(log4cxx::Level::toLevel(level))) {
        return true;
    } else {
        return false;
    }
}

/** Return whether the logging threshold of the logger named LOGGERNAME
  * is less than or equal to LEVEL.
  * @return Bool indicating whether or not logger is enabled.
  *
  * @param loggername  Name of logger being queried.
  * @param level       Logging threshold to check.
  */
bool Log::isEnabledFor(std::string const& loggername, int level) {
    return isEnabledFor(getLogger(loggername), level);
}

void Log::vlog(log4cxx::LoggerPtr logger,   ///< the logger
               log4cxx::LevelPtr level,     ///< message level
               std::string const& filename, ///< source filename
               std::string const& funcname, ///< source function name
               unsigned int lineno,         ///< source line number
               std::string const& fmt,      ///< message format string
               va_list args                 ///< message arguments
              ) {
    char msg[MAX_LOG_MSG_LEN];
    vsnprintf(msg, MAX_LOG_MSG_LEN, fmt.c_str(), args);
    logger->forcedLog(level, msg, log4cxx::spi::LocationInfo(filename.c_str(),
                                                             funcname.c_str(),
                                                             lineno));
}

void Log::log(std::string const& loggername, ///< name of logger
              log4cxx::LevelPtr level,       ///< message level
              std::string const& filename,   ///< source filename
              std::string const& funcname,   ///< source function name
              unsigned int lineno,           ///< source line number
              std::string const& fmt,        ///< message format string
              ...                            ///< message arguments
             ) {
    va_list args;
    va_start(args, fmt);
    vlog(getLogger(loggername), level, filename, funcname, lineno, fmt, args);
}

/** Method used by LOG_INFO and similar macros to process a log message
  * with variable arguments along with associated metadata.
  */
void Log::log(log4cxx::LoggerPtr logger,   ///< the logger
              log4cxx::LevelPtr level,     ///< message level
              std::string const& filename, ///< source filename
              std::string const& funcname, ///< source function name
              unsigned int lineno,         ///< source line number
              std::string const& fmt,      ///< message format string
              ...                          ///< message arguments
             ) {
    va_list args;
    va_start(args, fmt);
    vlog(logger, level, filename, funcname, lineno, fmt, args);
}

}} // namespace lsst::log
