# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import DepolarizedAnalyserTransmission
from SANSPolarizationCorrectionsBase import SANSPolarizationCorrectionsBase


class DepolarizedAnalyzerTransmissionTest(SANSPolarizationCorrectionsBase):
    def __init__(self):
        SANSPolarizationCorrectionsBase.__init__(self)

    def _run_test(self):
        mt_group = self._prepare_workspace("ZOOM00038238.nxs")
        dep_group = self._prepare_workspace("ZOOM00038335.nxs")
        mt = self._average_workspaces_in_group(list(mt_group))
        dep = self._average_workspaces_in_group(list(dep_group))

        DepolarizedAnalyserTransmission(dep, mt, OutputWorkspace="params" "", OutputFitCurves="curves")

    def _validate(self):
        result_curves = "curves"
        reference_curves = "DepolCurvesReference.nxs"
        result_params = "params"
        reference_params = "DepolParamsReference.nxs"

        return result_curves, reference_curves, result_params, reference_params
