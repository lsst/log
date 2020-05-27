#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2016  AURA/LSST.
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
# see <https://www.lsstcorp.org/LegalNotices/>.
#
from contextlib import contextmanager

from lsst.log import Log


def traceSetAt(name, number):
    """!Adjust logging level to display messages with trace number <= NUMBER

    Set the levels of loggers "TRACEn.name" with n > NUMBER to INFO,
    and those with n <= NUMBER to DEBUG, so that only tracing messages
    with n <= NUMBER are shown.

    @param[in] name  The logger name
    @param[in] number  The trace number threshold for display
    """
    for i in range(6):
        level = Log.INFO if i > number else Log.DEBUG
        Log.getLogger('TRACE%d.%s' % (i, name)).setLevel(level)


@contextmanager
def temporaryLogLevel(name, level):
    """A context manager that temporarily sets the level of a `Log`.

    Parameters
    ----------
    name : `str`
        Name of the log to modify.
    level : `int`
        Integer enumeration constant indicating the temporary log level.
    """
    log = Log.getLogger(name)
    old = log.getLevel()
    log.setLevel(level)
    try:
        yield
    finally:
        log.setLevel(old)
