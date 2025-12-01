# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, ADSValidator, WorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty, FloatArrayProperty, FloatBoundedValidator
from mantid.simpleapi import LinearBackground, AnalysisDataService, Fit

import numpy as np


class StitchByBackground(DataProcessorAlgorithm):
    def name(self):
        return "StitchByBackground"

    def category(self):
        return "Utility"

    def summary(self):
        return "Stitch banks together at a given Q value, without rebinning the data to preserve resolution."

    def seeAlso(self):
        return []

    def checkGroups(self):
        return True

    def validateInputs(self):
        pass

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspaces", "", direction=Direction.Input, validator=ADSValidator()),
            doc="List of workspaces to be stitched together.",
        )
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The stitched workspace.")
        self.declareProperty(
            FloatArrayProperty("StitchPoints", [], direction=Direction.Input),
            doc="Values in Q space where the stitch should take place. i.e. Overlaps in the data.",
        )
        self.declareProperty(
            "OverlapWidth",
            0.05,
            validator=FloatBoundedValidator(lower=0),
            doc="The extent to which the fit limits extend from the points given in StitchPoints.",
        )

    def PyExec(self):
        ws_name_list = self.getProperty("InputWorkspaces").value
        ws_list = []
        for ws_name in ws_name_list:
            ws_list.append(AnalysisDataService.retrieve(ws_name))
        stitch_locations = self.getProperty("StitchPoints").value
        overlap_width = self.getProperty("OverlapWidth").value

        self.create_offsests(ws_list, stitch_locations, overlap_width)

    def create_offsets(self, workspaces, stitch_points, overlap_width):
        offsets = np.zeros(len(workspaces))
        background_func = LinearBackground()

        for stitch_index, stitch_point in enumerate(stitch_points):
            backgrounds = []
            # Get backgrounds for each side of the stitch.
            for side in range(2):
                ws = workspaces[stitch_index + side]
                i_bin_of_stitch = ws.yIndexOfX(stitch_point)
                background_func["A0"] = ws.readY(0)[i_bin_of_stitch]
                background_func["A1"] = 0
                fit_result = Fit(background_func, InputWorkspace=ws, StartX=stitch_point - overlap_width, EndX=stitch_point + overlap_width)
                backgrounds.append(fit_result.Function(stitch_point))
            offsets[stitch_index] = backgrounds[1] - backgrounds[0]


# Register the algorithm to appear in the API.
AlgorithmFactory.subscribe(StitchByBackground)
