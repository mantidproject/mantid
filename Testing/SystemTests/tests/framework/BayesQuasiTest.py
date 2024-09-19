# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import BayesQuasi, Load


class BayesQuasiTest(MantidSystemTest):
    _sample_name = "irs26176_graphite002_red"
    _resolution_name = "irs26173_graphite002_res"

    def runTest(self):
        Load(Filename=f"{self._sample_name}.nxs", OutputWorkspace=self._sample_name, LoadHistory=False)
        Load(Filename=f"{self._resolution_name}.nxs", OutputWorkspace=self._resolution_name, LoadHistory=False)
        BayesQuasi(
            SampleWorkspace=self._sample_name,
            ResolutionWorkspace=self._resolution_name,
            MinRange=-0.54760699999999995,
            MaxRange=0.54411200000000004,
            Elastic=False,
            Background="Sloping",
            FixedWidth=False,
            OutputWorkspaceFit=f"{self._sample_name}_QLr_quasielasticbayes_Fit",
            OutputWorkspaceResult=f"{self._sample_name}_QLr_Result",
            OutputWorkspaceProb=f"{self._sample_name}_QLr_Prob",
        )

    def validate(self):
        self.tolerance = 1e-10
        return (
            "irs26176_graphite002_QLr_Result",
            "irs26176_graphite002_QLr_Result.nxs",
            "irs26176_graphite002_QLr_Prob",
            "irs26176_graphite002_QLr_Prob.nxs",
        )
