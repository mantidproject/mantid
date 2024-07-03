# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import IqtFitSequential, Load


class IqtFitSequentialTest(systemtesting.MantidSystemTest):

    def runTest(self):
        Load(Filename="irs26176_graphite002_iqt.nxs", OutputWorkspace="irs26176_graphite002_iqt")

        func = (
            "name=LinearBackground,A0=0.213439,A1=0,ties=(A1=0);name=StretchExp,"
            "Height=0.786561,Lifetime=0.0247894,"
            "Stretching=1;ties=(f1.Height=1-f0.A0)"
        )
        IqtFitSequential(
            InputWorkspace="irs26176_graphite002_iqt",
            Function=func,
            StartX=0.0,
            EndX=0.12,
            SpecMin=0,
            SpecMax=9,
            OutputWorkspace="Result",
            OutputParameterWorkspace="Parameters",
            OutputWorkspaceGroup="ResultGroup",
        )

    def validate(self):
        # Fitting can give slightly different results across different operating systems
        self.tolerance = 1e-2
        return "Result__Result", "irs26176_graphite002_iqt_sequential_fit_parameters.nxs"
