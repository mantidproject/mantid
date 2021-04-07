# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os.path

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid import config, FileFinder
from sans.command_interface.ISISCommandInterface import (SANS2D, Set1D, Detector,
                                                         MaskFile, Gravity, BatchReduce, UseCompatibilityMode)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DBatchTest_TOML(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('Mask_SANS2D_Batch_Toml.toml')
        Gravity(True)

        csv_file = FileFinder.getFullPath('SANS2D_periodTests.csv')

        BatchReduce(csv_file, 'nxs', plotresults=False, saveAlgs={'SaveCanSAS1D': 'xml', 'SaveNexus': 'nxs'})
        xml_path = os.path.join(config['defaultsave.directory'], '5512p7_SANS2DBatch_p7rear_1D_2.0_14.0Phi-45.0_45.0.xml')
        if os.path.exists(xml_path):
            os.remove(xml_path)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return '5512p7_SANS2DBatch_p7rear_1D_2.0_14.0Phi-45.0_45.0', 'SANS2DBatch.nxs'
