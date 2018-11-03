# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init

import stresstesting
from mantid.simpleapi import Load, EQSANSTOFStructure

class EQSANSTOFSTructureTest(stresstesting.MantidStressTest):

    alg_out = None

    def requiredFiles(self):
        return ['EQSANS_92353_event_eqsanstofstr_in.nxs',
                'EQSANS_92353_event_eqsanstofstr_out.nxs']

    def runTest(self):
        in_file = 'EQSANS_92353_event_eqsanstofstr_in.nxs'
        Load(Filename='EQSANS_92353_event_eqsanstofstr_in.nxs',
             OutputWorkspace='change_tof_structure')
        self.alg_out = EQSANSTOFStructure(InputWorkspace='change_tof_structure',
                                          LowTOFCut=500,
                                          HighTOFCut=2000)

    def validate(self):
        self.tolerance = 0.001
        self.assertEqual(self.alg_out.FrameSkipping, True)
        self.assertDelta(self.alg_out.TofOffset, 11389.65, 0.01,
                         'Incorrect TofOffset')
        self.assertDelta(self.alg_out.WavelengthMin, 2.6109,0.0001,
                         'Incorrect WavelengthMin')
        self.assertDelta(self.alg_out.WavelengthMax, 5.7032, 0.0001,
                         'Incorrect WavelengthMax')
        self.assertDelta(self.alg_out.WavelengthMinFrame2, 9.5472, 0.0001,
                         'Incorrect WavelengthMinFrame2')
        self.assertDelta(self.alg_out.WavelengthMaxFrame2, 12.9809, 0.0001,
                         'Incorrect TofOffset')
        return 'change_tof_structure',\
               'EQSANS_92353_event_eqsanstofstr_out.nxs'
