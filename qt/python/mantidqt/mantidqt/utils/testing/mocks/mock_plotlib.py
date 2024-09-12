# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""
The aim of this module is to provide a swap-in object for
the matplotlib.pyplot module, which has the same interface.
Every function is instantiated as a mock, which allows to record
calls to the plotting functions, without executing any real
matplotlib code that requires a running GUI application.

The mocking follows the real matplotlib structure so that it can be easily
swapped in when necessary. There are two ways to do that - either
by injecting the plotting dependency in the constructor or
by using mock.patch to replace the 'matplotlib.pyplot'.

Note that not all matplotlib.pyplot functions have been added,
only the ones that have been necessary so far. If another function
needs to be mocked it can be freely added in the relevant class below
and it should not break any existing tests.
"""

from unittest.mock import Mock


class MockAx:
    def __init__(self):
        self.plot = Mock()
        self.scatter = Mock()
        self.errorbar = Mock()
        self.legend = Mock()
        self.set_xlabel = Mock()
        self.set_ylabel = Mock()


class MockCanvas:
    def __init__(self):
        self.set_window_title = Mock()


class MockFig:
    def __init__(self):
        self.show = Mock()
        self.mock_canvas = MockCanvas()
        self.canvas = Mock(return_value=self.mock_canvas)


class MockPlotLib:
    def __init__(self):
        self.mock_ax = MockAx()
        self.mock_fig = MockFig()
        self.subplots = Mock(return_value=[self.mock_fig, self.mock_ax])
