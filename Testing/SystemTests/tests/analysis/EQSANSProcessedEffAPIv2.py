# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from reduction_workflow.instruments.sans.hfir_command_interface import SensitivityCorrection, SetTransmission
from mantid.api import FileFinder

import os


class EQSANSProcessedEff(stresstesting.MantidStressTest):

    def cleanup(self):
        absfile = FileFinder.getFullPath("EQSANS_1466_event_reduction.log")
        if os.path.exists(absfile):
            os.remove(absfile)
        return True

    def runTest(self):
        """
            System test for sensitivity correction
        """
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS(False)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        SetTransmission(1.0, 0.0)
        TotalChargeNormalization(normalize_to_beam=False)
        SensitivityCorrection("EQSANS_sensitivity.nxs")
        Reduce1D()
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=277.781,
              Operation='Multiply', OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "EQSANS_1466_event_Iq", 'EQSANSProcessedEff.nxs'
