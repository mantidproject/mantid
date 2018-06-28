#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__  import absolute_import

# std imports
from unittest import TestCase, main

# thirdparty imports
import matplotlib
matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt

# local imports
from workbench.plotting.functions import current_figure_or_none, figure_title


# Avoid importing the whole of mantid for a single mock of the workspace class
class FakeWorkspace(object):

    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


class FunctionsTest(TestCase):

    def test_current_figure_or_none_returns_none_if_no_figures_exist(self):
        plt.close('all')
        self.assertTrue(current_figure_or_none() is None)

    def test_figure_title_with_single_string(self):
        self.assertEqual("test-1", figure_title("test", 1))

    def test_figure_title_with_list_of_strings(self):
        self.assertEqual("first-10", figure_title(["first", "second"], 10))

    def test_figure_title_with_single_workspace(self):
        self.assertEqual("fake-5", figure_title(FakeWorkspace("fake"), 5))

    def test_figure_title_with_workspace_list(self):
        self.assertEqual("fake-10", figure_title((FakeWorkspace("fake"),
                                                  FakeWorkspace("nextfake")), 10))

    def test_figure_title_with_empty_list_raises_assertion(self):
        with self.assertRaises(AssertionError):
            figure_title([], 5)


if __name__ == '__main__':
    main()
