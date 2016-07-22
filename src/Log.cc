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
 * We try to avoid doing configuration at global initialization time (or at DSO loading
 * time). What we want to do:
 * - Let user re-configure later with LOG_CONFIG(filename) - this means that we do not
 *   do anything fancy by default like initializing from some pre-determined file
 * - if LSST_LOG_CONFIG is set to existing file name then we want to configure from
 *   that file but only if user does not call LOG_CONFIG(filename)
 *   - this means that we should leave it to log4cxx to default-configure using that file
 *     name (meaning that we want to set LOG4CXX_CONFIGURATION)
 * - otherwise do basic configuration only but at a later time (before the first use of
 *   any logging method)
 *
 * As a result initialization is done in two steps:
 * - at globals init time we determine whether LSST_LOG_CONFIG envvar is set, if yes
 *   then we set LOG4CXX_CONFIGURATION to the same value and let LOG4CXX to
 *   configure itself later
 * - otherwise we set `doBasicConfig` global flag used by the second init step
 */
bool globalInit() {
    if (const char* env = getenv(::configEnv)) {
        // check that file actually exists
        if (env[0] and access(env, R_OK) == 0) {
            // prepare for default initialization in log4cxx if
            // user does not do LOG_CONFIG(...)
            ::setenv("LOG4CXX_CONFIGURATION", env, 1);
            return false;
        }
    }

    return true;
}

bool doBasicConfig = globalInit();

/*
 * This is the method for the second init step triggered by first call
 * to `_defaultLogger()` method below via static variable initialization.
 * If `doBasicConfig` flag is set then we call `BasicConfigurator` to
 * initialize LOG4CXX. Note that `doBasicConfig` can be reset by one
 * of the user-called configuration methods below.
 */
log4cxx::LoggerPtr log4cxxInit() {
    if (doBasicConfig) {
        // do basic configuration only once (if at all)
        doBasicConfig = false;
        log4cxx::BasicConfigurator::configure();
    }
    // returns root logger to be used as default logger
    return log4cxx::Logger::getRootLogger();
}

// Utility method to guess file extension
std::string getFileExtension(std::string const& filename) {
    size_t dotpos = filename.find_last_of(".");
    if (dotpos == std::string::npos) {
      return std::string();
    }
    return filename.substr(dotpos);
}


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

/**
 *  Returns default LOG4CXX logger.
 */
log4cxx::LoggerPtr& Log::_defaultLogger() {
    static log4cxx::LoggerPtr _default(::log4cxxInit());
    return _default;
}

/** Explicitly configures log4cxx and initializes logging system.
  *
  * Uses either default configuration or log4cxx basic
  * configuration. Default configuration can be specified via
  * environment variable LSST_LOG_CONFIG, if it is set and specifies
  * existing file name then this file name is used for configuration.
  * Otherwise log4cxx BasicConfigurator class is used to configure,
  * which is hardwired to add to the root logger a ConsoleAppender. In this
  * case, the output will be formatted using a PatternLayout set to the
  * pattern "%-4r [%t] %-5p %c %x - %m%n".
  */
void Log::configure() {

    // in case log4cxxInit() was not called yet tell it to ignore basic config
    ::doBasicConfig = false;

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

    // reset default logger to the root logger
    _defaultLogger() = rootLogger;
}

/** Configures log4cxx from specified file.
  *
  * If file name ends with ".xml", it is passed to
  * log4cxx::xml::DOMConfigurator::configure(). Otherwise, it assumed to be
  * a log4j Java properties file and is passed to
  * log4cxx::PropertyConfigurator::configure(). See
  * http://logging.apache.org/log4cxx/usage.html for additional details.
  *
  * @param filename  Path to configuration file.
  */
void Log::configure(std::string const& filename) {
    // in case log4cxxInit() was not called yet tell it to ignore basic config
    ::doBasicConfig = false;
    // TODO: does resetConfiguration() remove existing appenders?
    log4cxx::BasicConfigurator::resetConfiguration();
    if (getFileExtension(filename).compare(".xml") == 0) {
        log4cxx::xml::DOMConfigurator::configure(filename);
    } else {
        log4cxx::PropertyConfigurator::configure(filename);
    }
    // reset default logger to the root logger
    _defaultLogger() = log4cxx::Logger::getRootLogger();
}

/** Configures log4cxx using a string containing the list of properties,
  * equivalent to configuring from a file containing the same content
  * but without creating temporary files.
  *
  * @param properties  Configuration properties.
  */
void Log::configure_prop(std::string const& properties) {
    // in case log4cxxInit() was not called yet tell it to ignore basic config
    ::doBasicConfig = false;

    std::vector<unsigned char> data(properties.begin(), properties.end());
    log4cxx::helpers::InputStreamPtr inStream(new log4cxx::helpers::ByteArrayInputStream(data));
    log4cxx::helpers::Properties prop;
    prop.load(inStream);
    log4cxx::PropertyConfigurator::configure(prop);

    // reset default logger to the root logger
    _defaultLogger() = log4cxx::Logger::getRootLogger();
}

/** Get the current default logger name.
  * @return String containing the default logger name.
  */
std::string Log::getDefaultLoggerName() {
    return getDefaultLogger().getName();
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

/** Returns logger object for a given name.
  *
  * If name is empty then current logger is returned and not
  * a root logger.
  *
  * @param loggername  Name of logger to return.
  * @return Log instance corresponding to logger name.
  */
Log Log::getLogger(std::string const& loggername) {
    if (loggername.empty()){
        return getDefaultLogger();
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
    std::string newName = _defaultLogger()->getName();
    if (newName == "root") {
        newName = name;
    } else {
        newName += ".";
        newName += name;
    }
    // Update defaultLogger
    _defaultLogger() = log4cxx::Logger::getLogger(newName);
}

/** Pops the last pushed name off the global hierarchical default logger
  * name.
  */
void Log::popContext() {
    // switch to parent logger, this assumes that loggers are not
    // re-parented between calls to push and pop
    log4cxx::LoggerPtr parent = _defaultLogger()->getParent();
    // root logger does not have parent, stay at root instead
    if (parent) {
        _defaultLogger() = parent;
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
int Log::getLevel() const {
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
bool Log::isEnabledFor(int level) const {
    if (_logger->isEnabledFor(log4cxx::Level::toLevel(level))) {
        return true;
    } else {
        return false;
    }
}

/** Method used by LOG_INFO and similar macros to process a log message
  * with variable arguments along with associated metadata.
  */
void Log::log(log4cxx::LevelPtr level,     ///< message level
              log4cxx::spi::LocationInfo const& location,  ///< message origin location
              char const* fmt,             ///< message format string
              ...                          ///< message arguments
             ) {
    va_list args;
    va_start(args, fmt);
    char msg[MAX_LOG_MSG_LEN];
    vsnprintf(msg, MAX_LOG_MSG_LEN, fmt, args);
    logMsg(level, location, msg);
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
    logger.logMsg(level, location, msg);
}

/** Method used by LOGS_INFO and similar macros to process a log message..
  */
void Log::logMsg(log4cxx::LevelPtr level,     ///< message level
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
    _logger->forcedLog(level, msg, location);
}

/** Method used by LOGS_INFO and similar macros to process a log message..
  */
void Log::logMsg(Log logger, ///< the logger
                 log4cxx::LevelPtr level,     ///< message level
                 log4cxx::spi::LocationInfo const& location,  ///< message origin location
                 std::string const& msg       ///< message string
                 ) {
    logger.logMsg(level, location, msg);
}

unsigned lwpID() {
    return detail::lwpID();
}

}} // namespace lsst::log
