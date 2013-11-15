#
# Permission is hereby granted, free of charge, to any person

# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# This permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHOR(S) BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import os
import subprocess

from core import checkDir, testBinDir, Test, TestResult
from exectest import ExecTest

glean_executable = os.path.join(testBinDir, "glean")

# GleanTest: Execute a sub-test of Glean
class GleanTest(ExecTest):
    globalParams = []

    def __init__(self, name):
        ExecTest.__init__(self, [glean_executable,
                                 "-o", "-v", "-v", "-v", "-t",
                                 "+"+name] + GleanTest.globalParams)
        self.name = name

    def interpretResult(self, out, returncode, results, dmesg):
        if "{'result': 'skip'}" in out:
            results['result'] = 'skip'
        elif out.find('FAIL') >= 0:
            results['result'] = 'dmesg-fail' if dmesg != '' else 'fail'
        else:
            results['result'] = 'dmesg-warn' if dmesg != '' else 'pass'
        return out
