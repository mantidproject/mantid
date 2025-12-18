# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, ADSValidator, WorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty, FloatArrayProperty, FloatBoundedValidator
from mantid.simpleapi import (
    AnalysisDataService,
    Fit,
    CropWorkspaceRagged,
    CreateSingleValuedWorkspace,
    CreateWorkspace,
    Plus,
)

import numpy as np


class StitchByBackground(DataProcessorAlgorithm):
    def name(self):
        return "StitchByBackground"

    def category(self):
        return "Utility"

    def summary(self):
        return "Stitch banks together at given x-axis values, without rebinning the data to preserve resolution."

    def seeAlso(self):
        return []

    def checkGroups(self):
        return False

    def validateInputs(self):
        errors = {}

        upper_bound = self.getProperty("CropUpperBound").value
        lower_bound = self.getProperty("CropLowerBound").value
        if not upper_bound > lower_bound:
            err_msg = f"Upper bound ({upper_bound}) must be greater than lower bound ({lower_bound})."
            errors["CropUpperBound"] = err_msg
            errors["CropLowerBound"] = err_msg

        ws_name_list = self.getProperty("InputWorkspaces").value
        ws_list = [AnalysisDataService.retrieve(ws_name) for ws_name in ws_name_list]

        data_upper = ws_list[-1].dataX(0)[-1]
        data_lower = ws_list[0].dataX(0)[0]

        if upper_bound > data_upper:
            errors["CropUpperBound"] = f"{upper_bound} is outside the upper limit of the data ({data_upper})."
        if lower_bound < data_lower:
            errors["CropLowerBound"] = f"{lower_bound} is outside the lower limit of the data ({data_lower})."

        stitch_points_list = self.getProperty("StitchPoints").value

        if not all(stitch_points_list[i] <= stitch_points_list[i + 1] for i in range(len(stitch_points_list) - 1)):
            errors["StitchPoints"] = "All stitch points must be in order - from smallest to largest."

        if len(ws_name_list) != len(stitch_points_list) + 1:
            err_msg = f"There must be one less stitch point ({len(stitch_points_list)}) than input workspaces ({len(ws_name_list)})."
            errors["InputWorkspaces"] = err_msg
            errors["StitchPoints"] = err_msg
            return errors  # The next check relies on this one being valid.

        invalid_points = []
        for i, stitch_point in enumerate(stitch_points_list):
            left_upper = ws_list[i].dataX(0)[-1]
            right_lower = ws_list[i + 1].dataX(0)[0]
            if not (right_lower <= stitch_point <= left_upper):
                invalid_points.append(f"{stitch_point} is not between {right_lower} and {left_upper}")
        if invalid_points:
            errors["StitchPoints"] = (
                f"All stitch points must be in the overlap regions between spectra. Invalid points: {', '.join(invalid_points)}."
            )

        return errors

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspaces", "", direction=Direction.Input, validator=ADSValidator()),
            doc="List of workspaces to be stitched together.",
        )
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The stitched workspace.")
        self.declareProperty(
            FloatArrayProperty("StitchPoints", [], direction=Direction.Input),
            doc="Values on the x-axis (between adjacent spectra) where the stitch should take place. i.e. Overlaps in the data.",
        )
        self.declareProperty(
            "OverlapWidth",
            0.05,
            validator=FloatBoundedValidator(lower=0),
            doc="A linear background is fitted to data in the region StitchPoint +/- OverlapWidth "
            "(i.e. the data is fitted to span range 2*OverlapWidth).",
        )
        self.declareProperty("CropLowerBound", 0.0, doc="The XMin to use when cropping the output workspace.")
        self.declareProperty("CropUpperBound", 0.0, doc="The XMax to use when cropping the output workspace.")

    def PyExec(self):
        ws_name_list = self.getProperty("InputWorkspaces").value
        ws_list = [AnalysisDataService.retrieve(ws_name) for ws_name in ws_name_list]

        stitch_locations = self.getProperty("StitchPoints").value

        offsets = self.create_offsets(ws_list, stitch_locations)

        x, y, e = self.subtract_background(ws_list, offsets, stitch_locations)

        parent_ws = ws_list[-1]

        out_ws_name = self.getProperty("OutputWorkspace").value
        stitched_ws = CreateWorkspace(
            x,
            y,
            e,
            OutputWorkspace=out_ws_name,
            UnitX=parent_ws.getAxis(0).getUnit().unitID(),
            Distribution=parent_ws.isDistribution(),
            ParentWorkspace=parent_ws,
            EnableLogging=False,
            StoreInADS=False,
        )

        stitched_ws = self.crop_output(stitched_ws)

        self.setProperty("OutputWorkspace", stitched_ws)

    def create_offsets(self, workspaces, stitch_points):
        overlap_width = self.getProperty("OverlapWidth").value
        offsets = np.zeros(len(workspaces))
        for stitch_index, stitch_point in enumerate(stitch_points):
            backgrounds = []
            # Get backgrounds for each side of the stitch.
            for side in range(2):
                ws = workspaces[stitch_index + side]
                i_bin_of_stitch = ws.yIndexOfX(stitch_point)
                background_func = f"name=LinearBackground, A0={ws.readY(0)[i_bin_of_stitch]}, A1={0};"
                fit_result = Fit(
                    Function=background_func,
                    InputWorkspace=ws,
                    StartX=stitch_point - overlap_width,
                    EndX=stitch_point + overlap_width,
                    StoreInADS=False,
                )
                backgrounds.append(fit_result.Function(stitch_point))
            offsets[stitch_index] = backgrounds[1] - backgrounds[0]
        return offsets

    def subtract_background(self, ws_list, offsets, stitch_locations):
        x = []
        y = []
        e = []
        subtracted_ws = None
        x_min = self.getProperty("CropLowerBound").value
        x_max = self.getProperty("CropUpperBound").value
        for ws_index, ws in enumerate(ws_list):
            background_ws = CreateSingleValuedWorkspace(offsets[ws_index:].sum(), ErrorValue=0, StoreInADS=False)
            subtracted_ws = Plus(ws, background_ws, OutputWorkspace=f"bank{ws_index + 1}_offset", StoreInADS=False)
            crop_kwargs = {"XMin": x_min, "XMax": x_max}
            if ws_index > 0:
                crop_kwargs["XMin"] = stitch_locations[ws_index - 1]
            if ws_index < len(ws_list) - 1:
                crop_kwargs["XMax"] = stitch_locations[ws_index]
            subtracted_ws = CropWorkspaceRagged(
                InputWorkspace=subtracted_ws, OutputWorkspace=subtracted_ws.name(), **crop_kwargs, StoreInADS=False
            )
            x.extend(subtracted_ws.readX(0)[:-1])
            y.extend(subtracted_ws.readY(0))
            e.extend(subtracted_ws.readE(0))
        # Add the final bin edge.
        x.append(subtracted_ws.readX(0)[-1])
        return x, y, e

    def crop_output(self, stitched_ws):
        # Crop the output.
        x_min = self.getProperty("CropLowerBound").value
        x_max = self.getProperty("CropUpperBound").value
        stitched_ws = CropWorkspaceRagged(
            InputWorkspace=stitched_ws, OutputWorkspace=stitched_ws.name(), XMin=x_min, XMax=x_max, StoreInADS=False
        )
        return stitched_ws


# Register the algorithm to appear in the API.
AlgorithmFactory.subscribe(StitchByBackground)
