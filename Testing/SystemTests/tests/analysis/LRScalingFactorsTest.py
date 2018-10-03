# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
import os
from mantid import *
from mantid.simpleapi import *


class LRPrimaryFractionTest(stresstesting.MantidStressTest):
    """
    #y=a+bx
    #
    #lambdaRequested[Angstroms] S1H[mm] (S2/Si)H[mm] S1W[mm] (S2/Si)W[mm] a b error_a error_b
    #
    #IncidentMedium=Air LambdaRequested=5.0 S1H=0.202 S2iH=0.196767 S1W=9.002 S2iW=9.004654
    #a=7.79202718645 b=3.39894488089e-05 error_a=0.0936282133631 error_b=5.79945870854e-06
    #IncidentMedium=Air LambdaRequested=5.0 S1H=0.202 S2iH=0.196767 S1W=20.0 S2iW=20.007348
    #a=24.4210636894 b=0.00010609255313 error_a=0.390001391338 error_b=2.51447482677e-05

    """

    def runTest(self):
        self.cfg_file = os.path.join(config.getString('defaultsave.directory'),'scaling_factors.cfg')
        LRScalingFactors(DirectBeamRuns=[124168, 124169, 124170, 124171, 124172],
                         Attenuators = [0, 1, 1, 2, 2, 2],
                         TOFRange=[10008, 35000], TOFSteps=200,
                         SignalPeakPixelRange=[150, 160],
                         SignalBackgroundPixelRange=[147, 163],
                         LowResolutionPixelRange=[94, 160],
                         ScalingFactorFile=self.cfg_file)

    def validate(self):
        if not os.path.isfile(self.cfg_file):
            raise RuntimeError("Output file was not created")

        reference = [[7.82, 3.20e-05, 0.054, 3.3e-06],
                     [24.42, 0.000105, 0.225, 1.45e-05]]
        tolerances = [[0.1, 5e-6, 0.1, 3e-6], [0.1, 1e-5, 0.2, 2e-5]]
        filed = open(self.cfg_file, 'r')
        item_number = 0
        for line in filed.readlines():
            if line.startswith("#"):
                continue
            toks = line.split()
            for token in toks:
                pair = token.split('=')
                if pair[0].strip() == 'a':
                    data_a = float(pair[1])
                elif pair[0].strip() == 'b':
                    data_b = float(pair[1])
                elif pair[0].strip() == 'error_a':
                    error_a = float(pair[1])
                elif pair[0].strip() == 'error_b':
                    error_b = float(pair[1])
            if not (abs(reference[item_number][0]- data_a ) < tolerances[item_number][0]
                    and abs(reference[item_number][1] - data_b) < tolerances[item_number][1]
                    and abs(reference[item_number][2]-error_a) < tolerances[item_number][2]
                    and abs(reference[item_number][3] - error_b) < tolerances[item_number][3]):
                logger.error("Found    %5.3g %5.3g %5.3g %5.3g" % (data_a, data_b, error_a, error_b))
                logger.error("Expected %5.3g %5.3g %5.3g %5.3g" % (reference[item_number][0], reference[item_number][1],
                                                                   reference[item_number][2], reference[item_number][3]))
                return False
            item_number += 1
        filed.close()
        os.remove(self.cfg_file)
        return True
