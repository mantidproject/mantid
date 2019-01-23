# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
import systemtesting
from mantid import *
from mantid.simpleapi import *


class RefRoiTest(systemtesting.MantidSystemTest):
    def runTest(self):
        workspace = Load(Filename="REF_L_119814")
        workspace = Integration(InputWorkspace=workspace)
        roi = RefRoi(InputWorkspace=workspace,
                     NXPixel=256, NYPixel=304,
                     IntegrateY=False, ConvertToQ=False)
        roi = Transpose(InputWorkspace=roi)

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "roi", 'REFL_119814_roi_peak.nxs'
