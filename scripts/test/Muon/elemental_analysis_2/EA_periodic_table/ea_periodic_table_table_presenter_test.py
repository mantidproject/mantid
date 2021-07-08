# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter import PeriodicTableTabPresenter


@start_qapplication
class PeriodicTableTabPresenterTest(unittest.TestCase):

    def setUp(self):
        self.periodic_table = PeriodicTableTabPresenter(mock.Mock(), mock.Mock())

    def test_will_fail(self):
        self.assertTrue(False)
