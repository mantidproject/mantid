# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Utility functions for running python test scripts
inside Workbench.

Public methods:
    runTests(): to run Workbench unit tests

"""
from __future__ import (absolute_import, division, print_function)
import sys
from qtpy.QtCore import QTime
from workbench.plotting.qappthreadcall import QAppThreadCall
from mantidqt.utils.qt.test.gui_test_runner import open_in_window


class MultiTestRunner(object):

    dots_per_line = 70

    def __init__(self, methods):
        self.methods = methods

    def __call__(self, w):
        time = QTime()
        time.start()
        count = 0
        for method in self.methods:
            yield method
            count += 1
            end = '\n' if count % self.dots_per_line == 0 else ''
            print('.', end=end, file=sys.stderr)
        print('', file=sys.stderr)
        print('-'*self.dots_per_line, file=sys.stderr)
        print('Ran {num} tests in {time}s'.format(num=len(self.methods), time=time.elapsed()*0.001), file=sys.stderr)


class RunTests(object):

    def __init__(self, test_method=None, close_on_finish=True, pause=0):
        self.test_method = test_method
        self.close_on_finish = close_on_finish
        self.pause = pause

    def __call__(self, classname):
        def dummy(self):
            pass

        def test():
            classname.runTest = dummy
            test_case = classname()
            if self.test_method is None or self.test_method == 'all':
                test_methods = [t[0] for t in test_case.get_all_tests()]
            else:
                test_methods = self.test_method if isinstance(self.test_method, list) else [self.test_method]
            runner = MultiTestRunner([getattr(test_case, name) for name in test_methods])
            try:
                return open_in_window(test_case.create_widget, runner, pause=self.pause,
                                      close_on_finish=self.close_on_finish, attach_debugger=False)
            except:
                return 1
        res = QAppThreadCall(test)()
        sys.exit(res)


runTests = RunTests()
