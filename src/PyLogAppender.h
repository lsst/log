/*
 * This file is part of log.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSST_LOG_PYLOGAPPENDER_H
#define LSST_LOG_PYLOGAPPENDER_H

// Python header has to be first to avoid compilation warnings
#include "Python.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

// Base class header
#include "log4cxx/appenderskeleton.h"

#include "log4cxx/helpers/object.h"
#include "PyObjectPtr.h"

namespace lsst::log::detail {

// This needs to be here for all LOG4CXX macros to work
using namespace log4cxx;

/**
 *  This class defines special log4cxx appender which "appends" log messages
 *  to Python logging. To use this logger one has to explicitly add it to
 *  \c log4cxx configuration using \c PyLogAppender as appender class name,
 *  for example:
 *  \code
 *  log4j.rootLogger = INFO, PyLog
 *  log4j.appender.PyLog = org.apache.log4j.PyLogAppender
 *  log4j.appender.PyLog.layout = org.apache.log4j.PatternLayout
 *  log4j.appender.PyLog.layout.ConversionPattern = %m (%X{LABEL})
 *  \endcode
 */
class PyLogAppender : public AppenderSkeleton {
public:

    DECLARE_LOG4CXX_OBJECT(PyLogAppender)
    BEGIN_LOG4CXX_CAST_MAP()
            LOG4CXX_CAST_ENTRY(PyLogAppender)
            LOG4CXX_CAST_ENTRY_CHAIN(AppenderSkeleton)
    END_LOG4CXX_CAST_MAP()

    // Make an instance
    PyLogAppender();

    /**
     * Forward the event to Python logging.
     */
    virtual void append(const spi::LoggingEventPtr& event, log4cxx::helpers::Pool& p);

    /**
     * Close this appender instance, this is no-op.
     */
    virtual void close();

    /**
     * Returns true if appender "requires" layout to be defined for it.
     *
     * This appender returns true, but layout is optional in configuration.
     */
    virtual bool requiresLayout() const;

private:

    // we do not support copying
    PyLogAppender(const PyLogAppender&);
    PyLogAppender& operator=(const PyLogAppender&);

    // cache entry type
    struct LRUEntry {
        PyObjectPtr logger;
        uint32_t age = 0;
    };

    using LRUCache = std::map<std::string, LRUEntry>;

    PyObjectPtr _getLogger;  // logging.getLogger() method
    std::mutex _cache_mutex;
    uint32_t _lru_age = 0;
    LRUCache _cache;  // LRU cache for loggers
};

} // namespace lsst::log::detail

#endif // LSST_LOG_PYLOGAPPENDER_H
