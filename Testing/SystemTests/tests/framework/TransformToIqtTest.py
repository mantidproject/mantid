# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import Load, TransformToIqt


class TransformToIqtNormalizedTest(systemtesting.MantidSystemTest):

    def runTest(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="irs26176_graphite002_red")
        Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace="irs26173_graphite002_res")

        TransformToIqt(
            SampleWorkspace="irs26176_graphite002_red",
            ResolutionWorkspace="irs26173_graphite002_res",
            EnergyMin=-0.5,
            EnergyMax=0.5,
            BinReductionFactor=10,
            DryRun=False,
            NumberOfIterations=50,
            EnforceNormalization=True,
            OutputWorkspace="Result",
        )

    def validate(self):
        # The calculation of Monte Carlo errors means the Iqt errors are randomized within a set amount
        self.tolerance = 5e-1
        return "Result", "irs26176_graphite002_iqt.nxs"


class TransformToIqtUnnormalizedTest(systemtesting.MantidSystemTest):

    def runTest(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="irs26176_graphite002_red")
        Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace="irs26173_graphite002_res")

        TransformToIqt(
            SampleWorkspace="irs26176_graphite002_red",
            ResolutionWorkspace="irs26173_graphite002_res",
            EnergyMin=-0.5,
            EnergyMax=0.5,
            BinReductionFactor=10,
            DryRun=False,
            NumberOfIterations=50,
            EnforceNormalization=False,
            OutputWorkspace="Result",
        )

    def validate(self):
        # The calculation of Monte Carlo errors means the Iqt errors are randomized within a set amount
        self.tolerance = 5e-1
        return "Result", "irs26176_graphite002_unnormalized_iqt.nxs"
