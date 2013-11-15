# Copyright (c) 2013 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

""" Status ordering from best to worst:

pass
dmesg-warn
warn
dmesg-fail
fail
crash
timeout
skip


The following are regressions:

pass|warn|dmesg-warn|fail|dmesg-fail|crash|timeout -> skip
pass|warn|dmesg-warn|fail|dmesg-fail|crash -> timeout|skip
pass|warn|dmesg-warn|fail|dmesg-fail -> crash|timeout|skip
pass|warn|dmesg-warn|fail -> dmesg-fail|crash|timeout|skip
pass|warn|dmesg-warn -> fail|dmesg-fail|crash|timeout|skip
pass|warn -> dmesg-warn|fail|dmesg-fail|crash|timeout|skip
pass -> warn|dmesg-warn|fail|dmesg-fail|crash|timeout|skip


The following are fixes:

skip -> pass|warn|dmesg-warn|fail|dmesg-fail|crash|timeout
timeout|skip -> pass|warn|dmesg-warn|fail|dmesg-fail|crash
crash|timeout|skip - >pass|warn|dmesg-warn|fail|dmesg-fail
dmesg-fail|crash|timeout|skip -> pass|warn|dmesg-warn|fail
fail|dmesg-fail|crash|timeout|skip -> pass|warn|dmesg-warn
dmesg-warn|fail|dmesg-fail|crash|timeout|skip -> pass|warn
warn|dmesg-warn|fail|dmesg-fail|crash|timeout|skip -> pass


NotRun -> * and * -> NotRun is a change, but not a fix or a regression. This is
because NotRun is not a status, but a representation of an unknown status.

"""


def status_lookup(status):
    """ Provided a string return a status object instance
    
    When provided a string that corresponds to a key in it's status_dict
    variable, this function returns a status object instance. If the string
    does not correspond to a key it will raise an exception
    
    """
    status_dict = {'skip': Skip,
                   'pass': Pass,
                   'warn': Warn,
                   'fail': Fail,
                   'crash': Crash,
                   'dmesg-warn': DmesgWarn,
                   'dmesg-fail': DmesgFail,
                   'timeout' : Timeout,
                   'notrun': NotRun}

    try:
        return status_dict[status]()
    except KeyError:
        # Raise a StatusException rather than a key error
        raise StatusException


class StatusException(LookupError):
    """ Raise this exception when a string is passed to status_lookup that
    doesn't exists
    
    The primary reason to have a special exception is that otherwise
    status_lookup returns a KeyError, but there are many cases where it is
    desireable to except a KeyError and have an exception path. Using a custom
    Error class here allows for more fine-grained control.
    
    """
    pass


class Status(object):
    """
    A simple class for representing the output values of tests.

    This is a base class, and should not be directly called. Instead a child
    class should be created and called. This module provides 8 of them: Skip,
    Pass, Warn, Fail, Crash, NotRun, DmesgWarn, and DmesgFail.
    """

    # Using __slots__ allows us to implement the flyweight pattern, limiting
    # the memory consumed for creating tens of thousands of these objects.
    __slots__ = ['name', 'value', 'fraction']

    name = None
    value = None
    fraction = (0, 1)

    def __init__(self):
        raise NotImplementedError

    def split(self, spliton):
        return (self.name.split(spliton))

    def __repr__(self):
        return self.name

    def __str__(self):
        return str(self.name)

    def __unicode__(self):
        return unicode(self.name)

    def __lt__(self, other):
        return int(self) < int(other)

    def __le__(self, other):
        return int(self) <= int(other)

    def __eq__(self, other):
        return int(self) == int(other)

    def __ne__(self, other):
        return int(self) != int(other)

    def __ge__(self, other):
        return int(self) >= int(other)

    def __gt__(self, other):
        return int(self) > int(other)

    def __int__(self):
        return self.value


class NotRun(Status):
    name = 'Not Run'
    value = 0
    fraction = (0, 0)

    def __init__(self):
        pass


class Pass(Status):
    name = 'pass'
    value = 10
    fraction = (1, 1)

    def __init__(self):
        pass


class DmesgWarn(Status):
    name = 'dmesg-warn'
    value = 20

    def __init__(self):
        pass


class Warn(Status):
    name = 'warn'
    value = 25

    def __init__(self):
        pass


class DmesgFail(Status):
    name = 'dmesg-fail'
    value = 30

    def __init__(self):
        pass


class Fail(Status):
    name = 'fail'
    value = 35

    def __init__(self):
        pass


class Crash(Status):
    name = 'crash'
    value = 40

    def __init__(self):
        pass


class Timeout(Status):
    name = 'timeout'
    value = 50

    def __init__(self):
        pass


class Skip(Status):
    name = 'skip'
    value = 60
    fraction = (0, 0)

    def __init__(self):
        pass
