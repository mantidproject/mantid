# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.contexts.plotting_context import PlottingContext


class MuonPlottingContextTest(unittest.TestCase):
    def setUp(self):

        self.context = PlottingContext()

    def tearDown(self):
        return

    def test_min_y_range(self):
        self.assertEqual(1.0, self.context.min_y_range)

    def test_y_axis_margin(self):
        self.assertEqual(0.20, self.context.y_axis_margin)

    def test_default_xlims(self):
        self.assertEqual([-0.1,0.1], self.context.default_xlims)

    def test_default_ylims(self):
        self.assertEqual([-10.,10.], self.context.default_ylims)

    def test_set_defaults(self):
        self.context.set_defaults([1., 15.], [-0.3, 0.3])
        self.assertEqual([1.,15.], self.context.default_xlims)
        self.assertEqual([-0.3,0.3], self.context.default_ylims)

    def test_add_subplot(self):
        self.assertEqual({}, self.context._subplots)
        self.context.add_subplot("test", 2)
        self.assertEqual(["test"], list(self.context._subplots.keys()))
        self.assertEqual(2, self.context._subplots["test"].axis)

    def test_clear_subplot(self):
        self.context.add_subplot("test", 2)
        self.assertEqual(["test"], list(self.context._subplots.keys()))
        self.context.clear_subplots()
        self.assertEqual({}, self.context._subplots)

    """ tests for the all methods """
    def test_get_xlim_all(self):
        self.assertEqual([-0.1,0.1], self.context.get_xlim_all)

    def test_update_xlim_all(self):
        self.context.update_xlim_all([1.,10.])
        self.assertEqual([1.0,10.], self.context.get_xlim_all)

    def test_get_ylim_all(self):
        self.assertEqual([-10.,10.], self.context.get_ylim_all)

    def test_update_ylim_all(self):
        self.context.update_ylim_all([1.,10.])
        self.assertEqual([1.0,10.], self.context.get_ylim_all)

    def test_get_autoscale_all(self):
        self.assertEqual(False, self.context.get_autoscale_all)

    def test_set_autoscale_all(self):
        self.context.set_autoscale_all(True)
        self.assertEqual(True, self.context.get_autoscale_all)

    def test_get_error_all(self):
        self.assertEqual(False, self.context.get_error_all)

    def test_set_error_all(self):
        self.context.set_error_all(True)
        self.assertEqual(True, self.context.get_error_all)

    """tests for specific subplots """
    def add_subplot(self):
        self.context.add_subplot("a", 1)
        self.context.add_subplot("b", 2)
        self.context.add_subplot("c", 3)

    def test_update_xlim(self):
        self.add_subplot()
        values = {"a": [1.,10.], "b":[2.,11.], "c":[3.,12.]}
        for name in values.keys():
            self.context.update_xlim(name, values[name])
        
        for name in values.keys():
            self.assertEqual(values[name], self.context.get_xlim(name))

    def test_update_ylim(self):
        self.add_subplot()
        values = {"a": [1.,10.], "b":[2.,11.], "c":[3.,12.]}
        for name in values.keys():
            self.context.update_ylim(name, values[name])
        
        for name in values.keys():
            self.assertEqual(values[name], self.context.get_ylim(name))

    def test_update_xlim_all(self):
        self.add_subplot()
        values = {"a": [1.,10.], "b":[2.,11.], "c":[3.,12.]}
        for name in values.keys():
            self.context.update_xlim(name, values[name])
        self.context.update_xlim("All", [5.,50.])

        self.assertEqual([5., 50.], self.context.get_xlim("All"))
        for name in values.keys():
            self.assertEqual([5., 50.], self.context.get_xlim(name))

    def test_update_ylim_all(self):
        self.add_subplot()
        values = {"a": [1.,10.], "b":[2.,11.], "c":[3.,12.]}
        for name in values.keys():
            self.context.update_ylim(name, values[name])
        self.context.update_ylim("All", [5., 50.])

        self.assertEqual([5., 50.], self.context.get_ylim("All"))
        for name in values.keys():
            self.assertEqual([5., 50.], self.context.get_ylim(name))

    def test_autoscale_state(self):
        self.add_subplot()
        values = {"a": True, "b": False, "c": True}
        for name in values.keys():
            self.context.update_autoscale_state(name, values[name])

        for name in values.keys():
            self.assertEqual(values[name], self.context.get_autoscale_state(name))

    def test_autoscale_state_all(self):
        self.add_subplot()
        values = {"a": True, "b": False, "c": True}
        for name in values.keys():
            self.context.update_autoscale_state(name, values[name])
        self.context.update_autoscale_state("All", False)

        self.assertEqual(False, self.context.get_autoscale_state("All"))
        for name in values.keys():
            self.assertEqual(False, self.context.get_autoscale_state(name))

    def test_error_state(self):
        self.add_subplot()
        values = {"a": True, "b": False, "c": True}
        for name in values.keys():
            self.context.update_error_state(name, values[name])

        for name in values.keys():
            self.assertEqual(values[name], self.context.get_error_state(name))

    def test_error_state_all(self):
        self.add_subplot()
        values = {"a": True, "b": False, "c": True}
        for name in values.keys():
            self.context.update_error_state(name, values[name])
        self.context.update_error_state("All", False)

        self.assertEqual(False, self.context.get_error_state("All"))
        for name in values.keys():
            self.assertEqual(False, self.context.get_error_state(name))

    def test_get_axis(self):
        self.add_subplot()
        values = {"a":1, "b":2, "c":3}
        for name in values.keys():
            self.assertEqual(values[name], self.context.get_axis(name))

    def test_update_axis(self):
        self.add_subplot()
        values = {"a":5, "b":6, "c":8}
        for name in values.keys():
            self.context.update_axis(name, values[name])

        for name in values.keys():
            self.assertEqual(values[name], self.context.get_axis(name))

    def test_add_axis(self):
        self.add_subplot()
        self.assertEqual(["a","b","c"], list(self.context._subplots.keys()))

        self.context.update_axis("new", 6)
        self.assertEqual(["a","b","c", "new"], list(self.context._subplots.keys()))
        self.assertEqual(6, self.context.get_axis("new"))

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
