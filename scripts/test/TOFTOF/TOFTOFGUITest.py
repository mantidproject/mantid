# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import QApplication
import unittest

from mantid.py3compat import mock
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, OptionalFloat
from reduction_gui.widgets.toftof.toftof_setup import TOFTOFSetupWidget


try:
    unicode('test for unicode type')
except NameError:
    unicode = str


class TOFTOFScriptElementTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication([])

    def setUp(self):
        self.settingsMock = mock.Mock()
        self.settingsMock.facility_name = "name of facility"
        self.settingsMock.instrument_name = "name of instrument"
        self.setupWidget = TOFTOFSetupWidget(self.settingsMock)

    def tearDown(self):
        self.settingsMock = None
        self.setupWidget = None

    def test_that_preserves_data(self):
        scriptElement = TOFTOFScriptElement()
        scriptElement.reset()
        scriptElement.facility_name   = 'nonsense'
        scriptElement.instrument_name = 'The 5th nonSense'

        # prefix of (some) workspace names
        scriptElement.prefix   = 'ws'

        # data files are here
        scriptElement.dataDir  = ''

        # vanadium runs & comment
        scriptElement.vanRuns  = 'TOFTOFTestdata.nxs'
        scriptElement.vanCmnt  = 'vanadium comment'
        scriptElement.vanTemp  = OptionalFloat(None)

        # empty can runs, comment, and factor
        scriptElement.ecRuns   = 'TOFTOFTestdata.nxs'
        scriptElement.ecTemp   = OptionalFloat(21.0)
        scriptElement.ecFactor = 0.9

        # data runs: [(runs,comment, temperature), ...]
        scriptElement.dataRuns = [
            [unicode('TOFTOFTestdata.nxs'), unicode('H2O 21C'), OptionalFloat(None)],
            [unicode('TOFTOFTestdata.nxs'), unicode('H2O 34C'), OptionalFloat(34.0)]
        ]

        # additional parameters
        scriptElement.binEon        = True
        scriptElement.binEstart     = -1.0
        scriptElement.binEstep      = 0.4
        scriptElement.binEend       = 1.8

        scriptElement.binQon        = True
        scriptElement.binQstart     = 0.4
        scriptElement.binQstep      = 0.2
        scriptElement.binQend       = 1.0

        scriptElement.maskDetectors = '1,2'

        # options
        scriptElement.subtractECVan = True
        scriptElement.normalise     = TOFTOFScriptElement.NORM_MONITOR
        scriptElement.correctTof    = TOFTOFScriptElement.CORR_TOF_VAN
        scriptElement.replaceNaNs   = True
        scriptElement.createDiff    = True
        scriptElement.keepSteps     = True

        # save data
        scriptElement.saveDir        = ''
        scriptElement.saveSofTWNxspe = False
        scriptElement.saveSofTWNexus = True
        scriptElement.saveSofTWAscii = True
        scriptElement.saveSofQWNexus = False
        scriptElement.saveSofQWAscii = True

        self.setupWidget.set_state(scriptElement)

        with mock.patch('reduction_gui.reduction.toftof.toftof_reduction.TOFTOFScriptElement.reset'):
            scriptElement2 = self.setupWidget.get_state()

        scriptElement.facility_name   = self.settingsMock.facility_name
        scriptElement.instrument_name = self.settingsMock.instrument_name
        for name in dir(scriptElement):
            attr1 = getattr(scriptElement, name)
            try:
                attr2 = getattr(scriptElement2, name)
            except AttributeError:
                self.fail("TOFTOFSetupWidget.get_state() doesn't set the attribute '{}'".format(name))

            if not name.startswith('__') and not hasattr(attr1, '__call__'):
                self.assertEqual(attr1, attr2, "TOFTOFSetupWidget doesn't preserve state of attribute '{}'".format(name))

    def test_that_preserves_empty_data(self):
        scriptElement = TOFTOFScriptElement()
        scriptElement.reset()

        self.setupWidget.set_state(scriptElement)

        with mock.patch('reduction_gui.reduction.toftof.toftof_reduction.TOFTOFScriptElement.reset'):
            scriptElement2 = self.setupWidget.get_state()

        scriptElement.facility_name   = self.settingsMock.facility_name
        scriptElement.instrument_name = self.settingsMock.instrument_name
        for name in dir(scriptElement):
            attr1 = getattr(scriptElement, name)
            try:
                attr2 = getattr(scriptElement2, name)
            except AttributeError:
                self.fail("TOFTOFSetupWidget.get_state() doesn't set the attribute '{}'".format(name))

            if not name.startswith('__') and not hasattr(attr1, '__call__'):
                self.assertEqual(attr1, attr2, "TOFTOFSetupWidget doesn't preserve state of attribute '{}'".format(name))

if __name__ == '__main__':
    unittest.main()
