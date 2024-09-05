# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import *
from PolarizationCorrectionsBase import PolarizationCorrectionsBase


class DepolarizedAnalyzerTransmissionTest(PolarizationCorrectionsBase):
    def __init__(self):
        PolarizationCorrectionsBase.__init__(self)
        self.tolerance = 1e-5

    def _run_test(self):
        Load("ZOOM00038238.nxs", OutputWorkspace="mt_run")
        Load("ZOOM00038335.nxs", OutputWorkspace="dep_run")

        mt_group = self._prepare_workspace("mt_run")
        dep_group = self._prepare_workspace("dep_run")
        mt = self._average_workspaces_in_group(list(mt_group))
        dep = self._average_workspaces_in_group(list(dep_group))

        DepolarizedAnalyserTransmission(dep, mt, OutputWorkspace="params" "", OutputFitCurves="curves")

    def _validate(self):
        result_curves = "curves"
        reference_curves = "DepolCurvesReference.nxs"
        result_params = "params"
        reference_params = "DepolParamsReference.nxs"

        is_curves_match = self._validate_workspace(result_curves, reference_curves)
        is_params_match = self._validate_workspace(result_params, reference_params)
        return is_curves_match and is_params_match
