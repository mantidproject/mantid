# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name, too-many-instance-attributes

import math
import numpy as np
from mantid.api import AlgorithmFactory, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction, IntArrayProperty


class PeakFinderDerivation(object):
    """
    Determine various types of peak for reflectivity.
    Those include specular peaks and low-resolution direction signal range.
    """

    xdata_firstderi = []
    ydata_firstderi = []
    five_highest_ydata = []
    five_highest_xdata = []
    sum_peak_counts = -1
    sum_peak_counts_time_pixel = -1
    peak_pixel = -1
    deri_min = 1
    deri_max = -1
    deri_min_pixel_value = -1
    deri_max_pixel_value = -1
    mean_counts_firstderi = -1
    std_deviation_counts_firstderi = -1
    peak_max_final_value = -1
    peak_min_final_value = -1

    def __init__(self, workspace, back_offset=4):
        self.back_offset = back_offset
        self.ydata = workspace.dataY(0)
        self.xdata = np.arange(len(self.ydata))

        self.compute()

    def compute(self):
        """
        Perform the computation
        """
        self.initArrays()
        self.calculate_five_highest_points()
        self.calculate_peak_pixel()
        self.calculate_first_derivative()
        self.calculate_min_max_derivative_pixels()
        self.calculate_avg_and_derivative()
        self.calculate_min_max_signal_pixels()
        self.low_resolution_range()

    def initArrays(self):
        """
        Initialize internal data members
        """
        self.xdata_firstderi = []
        self.ydata_firstderi = []
        self.peak = [-1, -1]
        self.low_res = [-1, -1]
        self.five_highest_ydata = []
        self.five_highest_xdata = []
        self.sum_five_highest_ydata = -1
        self.peak_pixel = -1
        self.deri_min = 1
        self.deri_max = -1
        self.deri_min_pixel_value = -1
        self.deri_max_pixel_value = -1
        self.mean_counts_firstderi = -1
        self.std_deviation_counts_firstderi = -1
        self.peak_max_final_value = -1
        self.peak_min_final_value = -1

    def calculate_five_highest_points(self):
        _xdata = self.xdata
        _ydata = self.ydata

        _sort_ydata = np.sort(_ydata)
        _decreasing_sort_ydata = _sort_ydata[::-1]
        self.five_highest_ydata = _decreasing_sort_ydata[0:5]

        _sort_index = np.argsort(_ydata)
        _decreasing_sort_index = _sort_index[::-1]
        _5decreasing_sort_index = _decreasing_sort_index[0:5]
        self.five_highest_xdata = _xdata[_5decreasing_sort_index]

    def calculate_peak_pixel(self):
        self.sum_peak_counts = sum(self.five_highest_ydata)
        _sum_peak_counts_time_pixel = -1
        for index, yvalue in enumerate(self.five_highest_ydata):
            _sum_peak_counts_time_pixel += yvalue * self.five_highest_xdata[index]
        self.sum_peak_counts_time_pixel = _sum_peak_counts_time_pixel
        self.peak_pixel = round(self.sum_peak_counts_time_pixel / self.sum_peak_counts)

    def calculate_first_derivative(self):
        xdata = self.xdata
        ydata = self.ydata

        _xdata_firstderi = []
        _ydata_firstderi = []
        for i in range(len(xdata) - 1):
            _left_x = xdata[i]
            _right_x = xdata[i + 1]
            _xdata_firstderi.append(np.mean([_left_x, _right_x]))

            _left_y = ydata[i]
            _right_y = ydata[i + 1]
            _ydata_firstderi.append((_right_y - _left_y) / (_right_x - _left_x))

        self.xdata_firstderi = _xdata_firstderi
        self.ydata_firstderi = _ydata_firstderi

    def calculate_min_max_derivative_pixels(self):
        _pixel = self.xdata_firstderi
        _counts_firstderi = self.ydata_firstderi

        _sort_counts_firstderi = np.sort(_counts_firstderi)
        self.deri_min = _sort_counts_firstderi[0]
        self.deri_max = _sort_counts_firstderi[-1]

        _sort_index = np.argsort(_counts_firstderi)
        self.deri_min_pixel_value = int(min([_pixel[_sort_index[0]], _pixel[_sort_index[-1]]]))
        self.deri_max_pixel_value = int(max([_pixel[_sort_index[0]], _pixel[_sort_index[-1]]]))

    def calculate_avg_and_derivative(self):
        _counts_firstderi = np.array(self.ydata_firstderi)
        self.mean_counts_firstderi = np.mean(_counts_firstderi)
        _mean_counts_firstderi = np.mean(_counts_firstderi * _counts_firstderi)
        self.std_deviation_counts_firstderi = math.sqrt(_mean_counts_firstderi)

    def calculate_min_max_signal_pixels(self):
        """
        Determine specular peak region
        """
        _counts = self.ydata_firstderi
        _pixel = self.xdata_firstderi

        _deri_min_pixel_value = self.deri_min_pixel_value
        _deri_max_pixel_value = self.deri_max_pixel_value

        _std_deviation_counts_firstderi = self.std_deviation_counts_firstderi

        px_offset = 0
        while (
            int(_deri_min_pixel_value - px_offset) < len(_counts)
            and int(_deri_min_pixel_value - px_offset) > 0
            and abs(_counts[int(_deri_min_pixel_value - px_offset)]) > _std_deviation_counts_firstderi
        ):
            px_offset += 1
        _peak_min_final_value = _pixel[int(_deri_min_pixel_value - px_offset)]

        px_offset = 0
        while (
            int(round(_deri_max_pixel_value + px_offset)) < len(_counts) - 1
            and int(round(_deri_max_pixel_value + px_offset)) >= 0
            and abs(_counts[int(round(_deri_max_pixel_value + px_offset))]) > _std_deviation_counts_firstderi
        ):
            px_offset += 1
        _peak_max_final_value = _pixel[int(round(_deri_max_pixel_value + px_offset))]

        self.peak = [int(_peak_min_final_value), int(np.ceil(_peak_max_final_value))]

    def low_resolution_range(self):
        """
        Determine the x range of the signal
        """
        y_integrated = []
        total = 0.0
        for y_value in self.ydata:
            total += y_value
            y_integrated.append(total)

        for i in range(len(y_integrated)):
            y_integrated[i] /= total

        # Derivative of the flipped integrated distribution
        deriv = []
        offset = 1
        for i in range(offset, len(y_integrated)):
            value = (self.xdata[i] - self.xdata[i - offset]) / (y_integrated[i] - y_integrated[i - offset])
            deriv.append(value)

        # Find lower edge of the main peak
        center = int(len(deriv) / 2.0)
        middle_value = deriv[center]
        i_min = 0
        for i in range(center, 0, -1):
            if deriv[i] / middle_value > 3:
                i_min = i
                break
        # Find upper edge of the main peak
        i_max = len(deriv)
        for i in range(center, i_max):
            if deriv[i] / middle_value > 3:
                i_max = i
                break

        self.low_res = [int(self.xdata[i_min]) - self.back_offset, int(self.xdata[i_max]) + self.back_offset]
        return self.low_res


class LRPeakSelection(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRPeakSelection"

    def version(self):
        return 1

    def summary(self):
        return "Find reflectivity peak and return its pixel range."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", Direction.Input), "Workspace to select peak from")
        self.declareProperty(IntArrayProperty("PeakRange", [0, 0], direction=Direction.Output))
        self.declareProperty(IntArrayProperty("LowResRange", [0, 0], direction=Direction.Output))
        self.declareProperty(IntArrayProperty("PrimaryRange", [0, 0], direction=Direction.Output))
        self.declareProperty("ComputePrimaryRange", False, doc="If True, the primary fraction range will be determined")

    def PyExec(self):
        workspace = self.getProperty("InputWorkspace").value

        # Main peak finding algorithm
        pf = PeakFinderDerivation(workspace)
        self.setProperty("PeakRange", pf.peak)
        logger.information("Peak: [%s, %s]" % (pf.peak[0], pf.peak[1]))
        self.setProperty("LowResRange", pf.low_res)
        logger.information("Low Res: [%s, %s]" % (pf.low_res[0], pf.low_res[1]))

        # Primary fraction range
        compute_primary = self.getProperty("ComputePrimaryRange").value
        if compute_primary:
            primary_range = self.clocking_range(workspace)
            self.setProperty("PrimaryRange", primary_range)
            logger.information("Primary: [%s, %s]" % (primary_range[0], primary_range[1]))

    def clocking_range(self, workspace):
        """
        Determine the primary fraction range
        @param workspace: workspace to determine the range from
        """
        # Get the full range
        pf = PeakFinderDerivation(workspace, back_offset=0)
        [left_max, right_min] = pf.low_res
        # Process left-end data
        pf.ydata = workspace.dataY(0)[0:left_max]
        pf.xdata = np.arange(len(pf.ydata))
        pf.compute()
        left_clocking = pf.low_resolution_range()[0]

        pf.ydata = workspace.dataY(0)[right_min:-1]
        pf.xdata = np.arange(len(pf.ydata))
        pf.compute()
        right_clocking = pf.low_resolution_range()[1] + right_min

        return [left_clocking, right_clocking]


AlgorithmFactory.subscribe(LRPeakSelection)
