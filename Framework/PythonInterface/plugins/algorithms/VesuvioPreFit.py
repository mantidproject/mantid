# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, StringListValidator
import numpy as np
from vesuvio.base import VesuvioBase


class VesuvioPreFit(VesuvioBase):
    def summary(self):
        return "Apply preprocessing steps to loaded vesuvio data"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    # ------------------------------------------------------------------------------------------------

    def PyInit(self):
        # Inputs
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Input TOF workspace from LoadVesuvio")

        smooth_opts = ["Neighbour", "None"]
        self.declareProperty("Smoothing", smooth_opts[0], StringListValidator(smooth_opts), doc="Defines the smoothing method.")
        self.declareProperty("SmoothingOptions", "NPoints=3", doc="Override the default smoothing options")

        self.declareProperty(
            "BadDataError", 1.0e6, doc="Mask any data point with an error greater than this value. Set to 0 to turn it off"
        )

        # Outputs
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="The name of the output workspace")

    def validateInputs(self):
        errors = dict()
        smoothing = self.getProperty("Smoothing").value
        if smoothing == "Neighbour":
            options = self.getProperty("SmoothingOptions").value
            if not options.startswith("NPoints="):
                errors["SmoothingOptions"] = "Invalid value for smoothing option. It must begin the format NPoints=3"

        return errors

    def PyExec(self):
        data = self._apply_smoothing(self.getProperty("InputWorkspace").value)
        data = self._mask_bad_data(data)
        self.setProperty("OutputWorkspace", data)

    def _apply_smoothing(self, data):
        smoothing = self.getProperty("Smoothing").value
        options = self.getProperty("SmoothingOptions").value
        if smoothing == "None":
            # This will guarantee we don't touch the original workspace later
            return self._execute_child_alg("CloneWorkspace", InputWorkspace=data)
        elif smoothing == "Neighbour":
            npts = int(options[-1])
            return self._execute_child_alg("SmoothData", InputWorkspace=data, NPoints=npts)

    def _mask_bad_data(self, data):
        error_threshold = self.getProperty("BadDataError").value
        if error_threshold <= 0.0:
            return data
        # The data shouldn't be too big, clone it to numpy and use its search capabilities
        errors = data.extractE()
        if len(errors.shape) != 2:
            raise RuntimeError("Expected 2D array of errors, found %dD" % len(errors.shape))

        indices = np.where(errors > error_threshold)  # Indices in 2D matrix where errors above threshold
        # The output is a tuple of 2 arrays where the indices from each array are paired to
        # give the correct index in the original 2D array.
        for ws_index, pt_index in zip(indices[0], indices[1]):
            ws_index = int(ws_index)
            pt_index = int(pt_index)
            data.dataY(ws_index)[pt_index] = 0.0
            data.dataE(ws_index)[pt_index] = 0.0

        return data


# -----------------------------------------------------------------------------------------
AlgorithmFactory.subscribe(VesuvioPreFit)
