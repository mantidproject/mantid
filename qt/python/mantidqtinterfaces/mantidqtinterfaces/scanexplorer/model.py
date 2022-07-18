# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from bisect import bisect_left
from pathlib import Path
import numpy as np

from mantid.simpleapi import SANSILLParameterScan
from mantid.api import MatrixWorkspace


class ScanExplorerModel:

    def __init__(self, presenter=None):
        self.presenter = presenter

    def process_file(self, file_name: str):
        """
        Process the given file without any special treatment, as a quick representation of the data.
        @param file_name: the name of the file to process, containing a scan with more than one point.
        """
        name = Path(file_name).stem + "_scan"
        out = SANSILLParameterScan(SampleRun=file_name, OutputWorkspace=name, NormaliseBy="None")
        self.presenter.create_slice_viewer(out)

    def process_background(self, background_file_name: str):
        """
        Process the background without treatment.
        """
        name = "_" + Path(background_file_name).stem
        SANSILLParameterScan(SampleRun=background_file_name, OutputWorkspace=name, NormaliseBy="None")
        self.presenter.set_bg_ws(name)

    @staticmethod
    def roi_integration(ws: MatrixWorkspace, rectangles: list, bg_ws=None) -> (list, list):
        """
        Sum the workspace values in each provided ROI and returns the sum, and the sum corrected by the background.
        The correction by the background is just the subtraction of the background value to the sample value for each
        2theta.

        @param ws: a workspace from which to integrate the ROIs.
        @param rectangles: the list of ROIs, as matplotlib Rectangle patches.
        @param bg_ws: the workspace containing background data, if there is one.
        @return the list of the sums of all values inside each ROIs and the list of the corrected sums
        """
        data = ws.extractY()

        xaxis = ws.getAxis(0).extractValues()
        yaxis = ws.getAxis(1).extractValues()

        bg_data = bg_ws.extractY()[0] if bg_ws else np.zeros(len(xaxis))

        integrated_values = []
        corrected_values = []

        for rect in rectangles:
            x0, x1 = rect.get_x(), rect.get_x() + rect.get_width()
            y0, y1 = rect.get_y(), rect.get_y() + rect.get_height()

            x0, x1 = (x0, x1) if x0 <= x1 else (x1, x0)
            y0, y1 = (y0, y1) if y0 <= y1 else (y1, y0)

            xmin_index = bisect_left(xaxis, x0)
            xmax_index = bisect_left(xaxis, x1)

            ymin_index = bisect_left(yaxis, y0)
            ymax_index = bisect_left(yaxis, y1)

            # TODO check for off-by-one issues everywhere in the vicinity

            ws_integration = np.sum(data[ymin_index:ymax_index, xmin_index:xmax_index])

            integrated_values.append(ws_integration)
            corrected_values.append(ws_integration - np.sum(bg_data[xmin_index:xmax_index]) * (ymax_index - ymin_index))

        return integrated_values, corrected_values
