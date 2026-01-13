# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import tempfile

from sys import platform
from systemtesting import MantidSystemTest
from mantid.kernel import config
from mantid.simpleapi import BayesQuasi, CropWorkspace, Load


class BayesQuasiTest(MantidSystemTest):
    _sample_name = "irs26176_graphite002_red"
    _resolution_name = "irs26173_graphite002_res"
    _reference_result_file_name = "irs26176_graphite002_QLr_Result.nxs"

    def skipTests(self):
        return platform == "darwin"

    def runTest(self):
        workdir = tempfile.mkdtemp(prefix="bayes_")
        config["defaultsave.directory"] = workdir
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
        if platform == "linux":
            return self._validate_linux()

        self.tolerance = 1e-10

        return (
            "irs26176_graphite002_QLr_Result",
            self._reference_result_file_name,
            "irs26176_graphite002_QLr_Prob",
            "irs26176_graphite002_QLr_Prob.nxs",
        )

    def validateMethod(self):
        if platform == "linux":
            return "ValidateWorkspaceToWorkspace"

        return "WorkspaceToNeXus"

    def _validate_linux(self):
        # With the crop below applied, this is the smallest relative tolerance that can be consistently applied
        # on all three of CentOS7, Rocky8, Alma9.
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True

        # The first 11 spectra in the result workspace correspond to fit parameters used
        # when fitting with one or two peaks. Anything beyond that comes from the three peak
        # fit, which is overparameterised, and therefore unstable between different Linux OS.
        max_spectrum_index = 10

        # Beyond q = 1.6, the statistics become low enough so that the fit becomes too unstable
        # for a meaningful system test.
        max_q = 1.6

        Load(Filename="irs26176_graphite002_QLr_Result.nxs", OutputWorkspace="bayesquasi_reference")
        CropWorkspace(
            InputWorkspace="bayesquasi_reference",
            OutputWorkspace="bayesquasi_reference_cropped",
            XMax=max_q,
            EndWorkspaceIndex=max_spectrum_index,
        )
        CropWorkspace(
            InputWorkspace="irs26176_graphite002_QLr_Result",
            OutputWorkspace="bayesquasi_result_cropped",
            XMax=max_q,
            EndWorkspaceIndex=max_spectrum_index,
        )

        return "bayesquasi_result_cropped", "bayesquasi_reference_cropped"
