#!/usr/bin/env python
# coding: utf-8

import os, signal, sys, unittest

sys.path.insert(0, '.')
from pinktrace import bitness, event, syscall, string, trace

class TestStringLinux_02(unittest.TestCase):

    def test_01_decode(self):
        pid = os.fork()
        if not pid: # child
            trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            open('/dev/null', 'r')
            os._exit(0)
        else: # parent
            os.waitpid(pid, 0)
            trace.setup(pid, trace.OPTION_SYSGOOD)

            # Loop until we get to the open() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            ev = -1
            while ev != event.EVENT_EXIT_GENUINE:
                trace.syscall(pid)
                pid, status = os.waitpid(pid, 0)

                ev = event.decide(status)
                if ev == event.EVENT_SYSCALL:
                    scno = syscall.get_no(pid)
                    name = syscall.name(scno)
                    if name == 'open':
                        path = string.decode(pid, 0)
                        self.assertEqual(path, '/dev/null')
                        break

            try: trace.kill(pid)
            except OSError: pass

    def test_02_decode_max(self):
        pid = os.fork()
        if not pid: # child
            trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            open('/dev/null', 'r')
            os._exit(0)
        else: # parent
            os.waitpid(pid, 0)
            trace.setup(pid, trace.OPTION_SYSGOOD)

            # Loop until we get to the open() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            ev = -1
            while ev != event.EVENT_EXIT_GENUINE:
                trace.syscall(pid)
                pid, status = os.waitpid(pid, 0)

                ev = event.decide(status)
                if ev == event.EVENT_SYSCALL:
                    scno = syscall.get_no(pid)
                    name = syscall.name(scno)
                    if name == 'open':
                        path = string.decode(pid, 0, 9)
                        self.assertEqual(path, '/dev/null')
                        break

            try: trace.kill(pid)
            except OSError: pass

    def test_03_encode(self):
        pid = os.fork()
        if not pid: # child
            trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            try:
                open('/dev/null', 'r')
            except IOError:
                os._exit(0)
            else:
                os._exit(1)
        else: # parent
            os.waitpid(pid, 0)
            trace.setup(pid, trace.OPTION_SYSGOOD)

            # Loop until we get to the open() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            ev = -1
            while ev != event.EVENT_EXIT_GENUINE:
                trace.syscall(pid)
                pid, status = os.waitpid(pid, 0)

                ev = event.decide(status)
                if ev == event.EVENT_SYSCALL:
                    scno = syscall.get_no(pid)
                    name = syscall.name(scno)
                    if name == 'open':
                        string.encode(pid, 0, '/dev/NULL')

            self.assert_(os.WIFEXITED(status))
            self.assertEqual(os.WEXITSTATUS(status), 0)

if __name__ == '__main__':
    unittest.main()
