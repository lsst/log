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
#include <mutex>
#include <pthread.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Third-party headers
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/helpers/bytearrayinputstream.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>

// Local headers
#include "lsst/log/Log.h"
#include "lwpID.h"


// Max message length for varargs/printf style logging
#define MAX_LOG_MSG_LEN 1024

namespace {

// name of the env. variable pointing to logging config file
const char configEnv[] = "LSST_LOG_CONFIG";

/*
 * Logging is configured at global initialization time (though everybody knows this is evil
 * thing to do). What we want to do:
 * - Let user re-configure later with LOG_CONFIG(filename) - this means that we do not
 *   do anything fancy here like initializing from some other file
 * - if LSST_LOG_CONFIG is set to existing file name then we want to configure from
 *   that file but only if user does not call LOG_CONFIG(filename)
 *   - this means that we should leave it to log4cxx to default-configure using that file
 *     name (meaning that we want to set LOG4CXX_CONFIGURATION)
 * - otherwise do basic configuration only
 */
bool init() {
    if (const char* env = getenv(::configEnv)) {
        // check that file actually exists
        if (env[0] and access(env, R_OK) == 0) {
            // prepare for default initialization in log4cxx if
            // user does not do LOG_CONFIG(...)
            ::setenv("LOG4CXX_CONFIGURATION", env, 1);
            return true;
        }
    }
    // do basic configuration only
    log4cxx::BasicConfigurator::configure();
    lsst::log::Log::initLog();

    return true;
}

bool initialized = init();

// List of the MDC initialization functions
std::vector<std::function<void()>> mdcInitFunctions;
std::mutex mdcInitMutex;

// more efficient alternative to pthread_once
struct PthreadKey {
    PthreadKey() {
        // we don't need destructor for a key
        pthread_key_create(&key, nullptr);
    }
    pthread_key_t key;
} pthreadKey;

}


namespace lsst {
namespace log {


// Log class

/** Reference to the defaultLogger used by LOG* macros.
  */
Log Log::defaultLogger(log4cxx::Logger::getRootLogger());

/** Initializes logging module (e.g. default logger and logging context).
  */
void Log::initLog() {
    // Default logger initially set to root logger
    defaultLogger = log4cxx::Logger::getRootLogger();
}

/** Configures log4cxx and initializes logging system by invoking
  * initLog(). Uses either default configuration or log4cxx basic
  * configuration. Default configuration can be specified via
  * environment variable LSST_LOG_CONFIG, if it is set and specifies
  * existing file name then this file name is used for configuration.
  * Otherwise log4cxx BasicConfigurator class is used to configure,
  * which is hardwired to add to the root logger a ConsoleAppender. In this
  * case, the output will be formatted using a PatternLayout set to the
  * pattern "%-4r [%t] %-5p %c %x - %m%n".
  */
void Log::configure() {
    // TODO: does resetConfiguration() remove existing appenders?
    log4cxx::BasicConfigurator::resetConfiguration();

    // if LSST_LOG_CONFIG is set then use that file
    if (const char* env = getenv(::configEnv)) {
        if (env[0] and access(env, R_OK) == 0) {
            configure(env);
            return;
        }
    }

    // Do basic configuration (only if not configured already?)
    log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
    if (rootLogger->getAllAppenders().size() == 0) {
        log4cxx::BasicConfigurator::configure();
    }
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
    // TODO: does resetConfiguration() remove existing appenders?
    log4cxx::BasicConfigurator::resetConfiguration();
    if (getFileExtension(filename).compare(".xml") == 0) {
        log4cxx::xml::DOMConfigurator::configure(filename);
    } else {
        log4cxx::PropertyConfigurator::configure(filename);
    }
    initLog();
}

/** Configures log4cxx using a string containing the list of properties,
  * equivalent to configuring from a file containing the same content
  * but without creating temporary files.
  *
  * @param properties  COnfiguration properties.
  */
void Log::configure_prop(std::string const& properties) {
    std::vector<unsigned char> data(properties.begin(), properties.end());
    log4cxx::helpers::InputStreamPtr inStream(new log4cxx::helpers::ByteArrayInputStream(data));
    log4cxx::helpers::Properties prop;
    prop.load(inStream);
    log4cxx::PropertyConfigurator::configure(prop);
    initLog();
}

/** Get the current default logger name.
  * @return String containing the default logger name.
  */
std::string Log::getDefaultLoggerName() {
    return defaultLogger.getName();
}

/** Get the logger name associated with the Log object.
  * @return String containing the logger name.
  */
std::string Log::getName() const {
    std::string name = _logger->getName();
    if (name == "root") {
        name.clear();
    }
    return name;
}

/** Returns a pointer to the log4cxx logger object associated with
  * LOGGERNAME.
  * @return log4cxx::LoggerPtr corresponding to LOGGERNAME.
  *
  * @param loggername  Name of logger to return.
  */
Log Log::getLogger(std::string const& loggername) {
    if (loggername.empty()){
        return defaultLogger;
    } else {
        return Log(log4cxx::Logger::getLogger(loggername));
    }
}

/** Pushes NAME onto the global hierarchical default logger name.
  *
  * @param name  String to push onto logging context.
  */
void Log::pushContext(std::string const& name) {
    // can't handle empty names
    if (name.empty()) {
        throw std::invalid_argument("lsst::log::Log::pushContext(): "
                "empty context name is not allowed");
    }
    // we do not allow multi-level context (logger1.logger2)
    if (name.find('.') != std::string::npos) {
        throw std::invalid_argument("lsst::log::Log::pushContext(): "
                "multi-level contexts are not allowed: " + name);
    }

    // Construct new default logger name
    std::string newName = defaultLogger._logger->getName();
    if (newName == "root") {
        newName = name;
    } else {
        newName += ".";
        newName += name;
    }
    // Update defaultLogger
    defaultLogger = Log(log4cxx::Logger::getLogger(newName));
}

/** Pops the last pushed name off the global hierarchical default logger
  * name.
  */
void Log::popContext() {
    // switch to parent logger, this assumes that loggers are not
    // re-parented between calls to push and pop
    log4cxx::LoggerPtr parent = defaultLogger._logger->getParent();
    // root logger does not have parent, stay at root instead
    if (parent) {
        defaultLogger = Log(parent);
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

int Log::MDCRegisterInit(std::function<void()> function) {

    std::lock_guard<std::mutex> lock(mdcInitMutex);

    // logMsg may have been called already in this thread, to make sure that
    // this function is executed in this thread call it explicitly
    function();

    // store function for later use
    ::mdcInitFunctions.push_back(std::move(function));

    // return arbitrary number
    return 1;
}

/** Set the logging threshold to LEVEL.
  *
  * @param level   New logging threshold.
  */
void Log::setLevel(int level) {
    _logger->setLevel(log4cxx::Level::toLevel(level));
}

/** Retrieve the logging threshold.
  * @return int Indicating the logging threshold.
  */
int Log::getLevel() {
    log4cxx::LevelPtr level = _logger->getLevel();
    int levelno = -1;
    if (level != NULL) {
        levelno = level->toInt();
    }
    return levelno;
}

/** Return whether the logging threshold of the logger is less than or equal
  * to LEVEL.
  * @return Bool indicating whether or not logger is enabled.
  *
  * @param level   Logging threshold to check.
  */
bool Log::isEnabledFor(int level) {
    if (_logger->isEnabledFor(log4cxx::Level::toLevel(level))) {
        return true;
    } else {
        return false;
    }
}

/** Method used by LOG_INFO and similar macros to process a log message
  * with variable arguments along with associated metadata.
  */
void Log::log(Log logger,   ///< the logger
              log4cxx::LevelPtr level,     ///< message level
              log4cxx::spi::LocationInfo const& location,  ///< message origin location
              char const* fmt,             ///< message format string
              ...                          ///< message arguments
             ) {
    va_list args;
    va_start(args, fmt);
    char msg[MAX_LOG_MSG_LEN];
    vsnprintf(msg, MAX_LOG_MSG_LEN, fmt, args);
    logMsg(logger, level, location, msg);
}

/** Method used by LOGS_INFO and similar macros to process a log message..
  */
void Log::logMsg(Log logger, ///< the logger
                 log4cxx::LevelPtr level,     ///< message level
                 log4cxx::spi::LocationInfo const& location,  ///< message origin location
                 std::string const& msg       ///< message string
                 ) {

    // do one-time per-thread initialization, this was implemented
    // with thread_local initially but clang on OS X did not support it
    void *ptr = pthread_getspecific(::pthreadKey.key);
    if (ptr == nullptr) {

        // use pointer value as a flag, don't care where it points to
        ptr = static_cast<void*>(&::pthreadKey);
        pthread_setspecific(::pthreadKey.key, ptr);

        std::lock_guard<std::mutex> lock(mdcInitMutex);
		// call all functions in the MDC init list
        for (auto& fun: mdcInitFunctions) {
            fun();
        }
    }

    // forward everything to logger
    logger._logger->forcedLog(level, msg, location);
}

unsigned lwpID() {
    return detail::lwpID();
}

}} // namespace lsst::log
