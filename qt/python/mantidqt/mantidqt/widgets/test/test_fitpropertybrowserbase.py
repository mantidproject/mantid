# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid import FrameworkManager
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowserBase


@start_qapplication
class TestFitPropertyBrowser(unittest.TestCase):
    def create_widget(self):
        return FitPropertyBrowserBase()

    def test_multiple_function_string_loaded_correctly(self):
        property_browser = self.create_widget()
        func = (
            "name=Gaussian,Height=100,PeakCentre=1.45,Sigma=0.2,ties=(PeakCentre=1.45);name=Gaussian,Height=100,"
            "PeakCentre=7.5,Sigma=0.2,constraints=(0.18<Sigma<0.22),ties=(PeakCentre=7.5);"
            "ties=(f0.Sigma=f1.Sigma,f1.Height=f0.Height)"
        )

        property_browser.loadFunction(func)

        # tests composite func set correctly in browser (string incl. ties and constraints)
        self.assertEqual(func, property_browser.getFunctionString())
        for prefix in property_browser.getPeakPrefixes():
            h = property_browser.getPeakHandler(prefix)
            # check that the ties (as opposed to fixes) have been set on the child function property handlers
            # note that the non-fix tie string lives on the composite function but the properties whereas
            # the tie properties (m_ties in the C++ class) are on the child's handler
            self.assertTrue(h.hasTies())
            # check the peak centre is fixed
            self.assertTrue(h.ifun().isFixed(1))
        # check constraints on last function have correct length
        self.assertEqual(15, len(h.ifun().getConstraints()))

    def test_single_function_string_loaded_correctly(self):
        property_browser = self.create_widget()
        func = "name=Gaussian,Height=487,PeakCentre=5,Sigma=5;ties=(f0.Sigma=f0.PeakCentre)"

        property_browser.loadFunction(func)

        # test composite func set correctly in browser (string incl. ties and constraints)
        # note property_browser.getFunctionString() returns the child function (not composite) if only one function
        self.assertEqual(func, str(property_browser.currentHandler().ifun()))
        for prefix in property_browser.getPeakPrefixes():
            h = property_browser.getPeakHandler(prefix)
            self.assertTrue(h.hasTies())


if __name__ == "__main__":
    unittest.main()
    FrameworkManager.clear()
