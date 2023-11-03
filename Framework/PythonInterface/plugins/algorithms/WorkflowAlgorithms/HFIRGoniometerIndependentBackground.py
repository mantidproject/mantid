# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, IMDHistoWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, IntBoundedValidator, StringListValidator, Property
from mantid.simpleapi import CloneWorkspace

import numpy as np
import scipy.ndimage


class HFIRGoniometerIndependentBackground(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Reduction;Diffraction\\Utility"

    def summary(self):
        return "Generates a background from a 3 dimensional MDHistoWorkspace."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            doc="Input workspace, must be a 3 dimensional MDHistoWorkspace",
        )
        self.declareProperty(
            name="BackgroundLevel",
            defaultValue=50.0,
            direction=Direction.Input,
            doc="Backgound level defines percentile range, (default 50, median filter)",
            validator=FloatBoundedValidator(-100.0, 100.0),
        )
        self.declareProperty(
            name="BackgroundWindowSize",
            defaultValue=Property.EMPTY_INT,
            direction=Direction.Input,
            doc="Background Window Size, only applies to the rotation axis, assumes the detectors are already \
              grouped. Integer value or -1 for All values",
            validator=IntBoundedValidator(lower=1),
        )
        self.declareProperty(
            name="FilterMode",
            defaultValue="nearest",
            validator=StringListValidator(["nearest", "wrap"]),
            doc="Mode should be 'nearest' if the rotation is incomplete or 'wrap' if complete within a reasonable tolerance",
        )
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value

        if inWS.getNumDims() != 3:
            issues["InputWorkspace"] = "InputWorkspace has wrong number of dimensions, need 3"

        return issues

    def PyExec(self):
        data_ws = self.getProperty("InputWorkspace").value
        signal = data_ws.getSignalArray().copy()

        bkg_level = self.getProperty("BackgroundLevel").value
        bkg_size = self.getProperty("BackgroundWindowSize").value
        filter_mode = self.getProperty("FilterMode").value

        if bkg_size == Property.EMPTY_INT:
            percent = np.percentile(signal, bkg_level, axis=2)
            bkg = np.repeat(percent[:, :, np.newaxis], signal.shape[2], axis=2)
        else:
            bkg = scipy.ndimage.percentile_filter(signal, bkg_level, size=(1, 1, bkg_size), mode=filter_mode)

        outputWS = self.getPropertyValue("OutputWorkspace")
        outputWS = CloneWorkspace(InputWorkspace=data_ws, OutputWorkspace=outputWS)
        outputWS.setSignalArray(bkg)

        self.setProperty("OutputWorkspace", outputWS)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(HFIRGoniometerIndependentBackground)
