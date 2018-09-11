
# LSST Data Management System
# Copyright 2018 LSST Corporation.
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

"""
This tests logging redirection to Python streams.
"""


import io
import os
import shutil
import tempfile
import unittest

import lsst.log as log
import lsst.log.utils as logUtils


class TestRedir(unittest.TestCase):

    class StdoutCapture(object):
        """
        Context manager to redirect stdout to a file.
        """

        def __init__(self, filename):
            self.stdout = None
            self.outputFilename = filename

        def __enter__(self):
            self.stdout = os.dup(1)
            os.close(1)
            os.open(self.outputFilename, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)

        def __exit__(self, type, value, traceback):
            if self.stdout is not None:
                os.close(1)
                os.dup(self.stdout)
                os.close(self.stdout)
                self.stdout = None

    def setUp(self):
        """Make a temporary directory and a log file in it."""
        self.tempDir = tempfile.mkdtemp()
        self.outputFilename = os.path.join(self.tempDir, "log.out")
        self.stdout = None

    def tearDown(self):
        """Remove the temporary directory."""
        shutil.rmtree(self.tempDir)

    def check(self, reference):
        """Compare the log file with the provided reference text."""
        with open(self.outputFilename, 'r') as f:
            # strip everything up to first ] to remove timestamp and thread ID
            lines = [l.split(']')[-1].rstrip("\n") for l in f.readlines()]
            reflines = [rl for rl in reference.split("\n") if rl != ""]
            self.maxDiff = None
            self.assertListEqual(lines, reflines)

###############################################################################

    def testRedir(self):
        """
        Test redirection to stream.
        """
        with TestRedir.StdoutCapture(self.outputFilename):
            log.configure()
            dest = io.StringIO()
            lr = logUtils.LogRedirect(1, dest)
            log.log(log.getDefaultLoggerName(), log.INFO, "This is INFO")
            log.info(u"This is unicode INFO")
            log.trace("This is TRACE")
            log.debug("This is DEBUG")
            log.warn("This is WARN")
            log.error("This is ERROR")
            log.fatal("This is FATAL")
            lr.finish()
            log.warn("Format %d %g %s", 3, 2.71828, "foo")
        self.assertEqual(dest.getvalue(), """root INFO: This is INFO
root INFO: This is unicode INFO
root WARN: This is WARN
root ERROR: This is ERROR
root FATAL: This is FATAL
""")
        self.check("""
root WARN: Format 3 2.71828 foo
""")


if __name__ == "__main__":
    unittest.main()
