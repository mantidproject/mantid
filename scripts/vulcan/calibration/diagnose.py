# Script to diagnose the calibration result
from mantid.simpleapi import mtd
from mantid.simpleapi import Load, CreateGroupingWorkspace, FitPeaks
import numpy as np
from typing import Tuple, Any, Union
from matplotlib import pyplot as plt


class DiagnosticData(object):
    """
    Class to hold various diagnostic data for visualization
    """
    def __init__(self, diamond_ws, pixels,
                 expected_peak_pos_d: float,
                 peak_range_d: Tuple[float, float],
                 tag: str):
        """Init

        Parameters
        ----------
        diamond_ws:
            diamond MatrixWorkspace
        pixels: ~numpy.ndarray
            array of pixels (workspace indexed in consecutive order)
        expected_peak_pos_d: float
            expected peak position in dSpacing
        peak_range_d: ~tuple
            lower and upper boundary
        tag: str
            like prefix for output workspace
        """
        # workspace in D
        self.diamond_ws = diamond_ws
        # pixels (workspace indexes)
        self.pixels = pixels
        # expected peak position
        self.expected_peak_pos_d = expected_peak_pos_d
        # peak range
        self.peak_range_d = peak_range_d
        # peak centers
        self.peak_centers_fit = None
        self.peak_centers_max = None
        # tag
        self.tag = tag

    def find_peaks_centers(self):
        # get peak center by find out maximum value inside given range
        _,  self.peak_centers_max = observe_peak_centers(self.diamond_ws,
                                                         (self.pixels[0], self.pixels[-1]),
                                                         self.expected_peak_pos_d,
                                                         self.peak_range_d,
                                                         'max')
        # get peak center by fitting Gaussian
        print(f'{self.diamond_ws}')
        print(f'{self.pixels}')
        print(f'{self.expected_peak_pos_d}')
        print(f'{self.peak_range_d}')
        _, self.peak_centers_fit = get_peak_centers(self.diamond_ws,
                                                    (self.pixels[0], self.pixels[-1]),
                                                    self.expected_peak_pos_d,
                                                    self.peak_range_d,
                                                    self.tag)


class DiagnosticPlot(object):
    """
    Class to handle data structure and methods to plot
    """
    def __init__(self, num_rows: int, num_cols: int):
        """

        Parameters
        ----------
        num_rows: int
            number of rows in the output figure
        num_cols: int
            number of columns in the output figure
        """
        self._num_rows = num_rows
        self._num_cols = num_cols

        self._matrix = dict()  # key = cell location such as (0, 0), (0, 1) for row 0, column 1 and etc
        self._title = dict()

    def set_cell(self, row: int, column: int,
                 diagnostic_data: DiagnosticData,
                 title: str):
        self._matrix[(row, column)] = diagnostic_data
        self._title[(row, column)] = title

    def plot(self, center_find_method, png_name: Union[None, str], relative_position=True):
        """Plot

        Parameters
        ----------
        center_find_method: str
            method how peak centers are found: max or fit
        png_name: None, str
            if specified, then save the figure rather than plot

        Returns
        -------

        """
        # Clear canvas
        # plt.cla()

        # Protype multiple plots
        fig = plt.figure()

        # ax1 = fig.add_subplot(221)
        # ax1.plot([(1, 2), (3, 4)], [(4, 3), (2, 3)])
        #
        # ax2 = fig.add_subplot(224)
        # ax2.plot([(7, 2), (5, 3)], [(1, 6), (9, 5)])
        #
        # plt.show()

        for index, row_col in enumerate(self._matrix.keys()):
            # get row and columun
            row, column = row_col

            # get raw data
            vec_pixels = self._matrix[(row, column)].pixels
            vec_peak_pos = self._matrix[(row, column)].peak_centers_fit

            print(f'Number of NaN in peak positions: {len(np.where(np.isnan(vec_peak_pos))[0])}')

            # process
            if relative_position:
                vec_peak_pos = 2 * (
                        vec_peak_pos - self._matrix[(row, column)].expected_peak_pos_d) / (
                        vec_peak_pos + self._matrix[(row, column)].expected_peak_pos_d)
                vec_peak_pos = 1 - vec_peak_pos

            # set up plot
            plot_index = self._num_rows * 100 + self._num_cols * 10 + (index + 1)
            print(f'subplot index = {plot_index}')

            ax_i = fig.add_subplot(plot_index)
            ax_i.plot(vec_pixels, vec_peak_pos, linestyle='None', marker='.', color='red',
                      label=self._title[(row, column)])

            # Legend setup
            plt.legend()

            # plt.plot(vec_pixels, vec_peak_pos, linestyle='None', marker='.', color='red',
            #          label=self._title[(row, column)])

        # Save or show
        if png_name:
            plt.savefig(png_name)
        else:
            plt.show()


def get_peak_centers(diamond_ws,
                     ws_index_range: Tuple[int, int],
                     peak_center: float,
                     peak_range: Tuple[float, float],
                     tag) -> Tuple[Any, Any]:
    """Get peak centers by fitting

    Parameters
    ----------
    diamond_ws:
        workspace for Diamond
    ws_index_range: ~tuple
        start workspace index, stop workspace index (excluded)
    peak_center: float
        proposed peak center
    peak_range: ~tuple
        lower boundary for dSpacing, upper boundary for dSpacing
    tag: str
        workspace basename

    Returns
    -------
    ~tuple
        vector, vector

    """
    #
    first_ws_index, last_ws_index = ws_index_range
    min_d, max_d = peak_range

    # Define
    out_peak_pos_ws = f'{tag}_peak_pos'

    # Default to Gaussian and Linear background
    FitPeaks(InputWorkspace=diamond_ws,
             OutputWorkspace=out_peak_pos_ws,
             StartWorkspaceIndex=int(first_ws_index),
             StopWorkspaceIndex=int(last_ws_index),
             PeakCenters=f'{peak_center}',
             FitWindowBoundaryList=f'{min_d}, {max_d}',
             FittedPeaksWorkspace=f'{tag}_model',
             OutputPeakParametersWorkspace=f'{tag}_params',
             OutputParameterFitErrorsWorkspace=f'{tag}_errors')

    # Get peak position return
    vec_pid = np.arange(first_ws_index, last_ws_index)
    vec_pos = mtd[out_peak_pos_ws].extractY().flatten()

    return vec_pid, vec_pos


def observe_peak_centers(diamond_ws,
                         ws_index_range: Tuple[int, int],
                         peak_center: float,
                         peak_range: Tuple[float, float],
                         method: str) -> Tuple[Any, Any]:
    """Get peak centers by observation

    Parameters
    ----------
    diamond_ws:
        workspace for Diamond
    ws_index_range: ~tuple
        start workspace index, stop workspace index (included)
    peak_center: float
        proposed peak center
    peak_range: ~tuple
        lower boundary for dSpacing, upper boundary for dSpacing
    method: str
        'max' (max Y's position) or 'com' (center of mass)

    Returns
    -------
    ~tuple
        vector, vector

    """
    # TODO FIXME - assumption: Diamond Workspace is NOT ragged
    first_ws_index, last_ws_index = ws_index_range

    # Get X and Y arrays
    x_array = diamond_ws.extractX()[first_ws_index:last_ws_index]
    y_array = diamond_ws.extractY()[first_ws_index:last_ws_index]

    # determine y index
    min_d, max_d = peak_range
    # print(f'x range: {x_array[0][0]}, {x_array[0][x_array.shape[1] - 1]}')
    min_d_index = np.argmin(np.abs(x_array[0] - min_d))
    max_d_index = np.argmin(np.abs(x_array[0] - max_d))
    # print(f'Peak range (index): {min_d_index}, {max_d_index}')

    peak_pos_vec = np.zeros(shape=(y_array.shape[0],), dtype='float')

    for index in range(y_array.shape[0]):
        # observe
        if method == 'max':
            # maximum value
            # max_y_i = np.max(y_array[index][min_d_index:max_d_index])
            max_y_index = np.argmax(y_array[index][min_d_index:max_d_index]) + min_d_index
            pos_x = x_array[index][max_y_index]
        elif method == 'com':
            # center of mass
            i_s = min_d_index
            i_e = max_d_index
            pos_x = np.sum(y_array[index][i_s:i_e] * x_array[index][i_s:i_e]) / np.sum(y_array[index][i_s:i_e])
        else:
            raise RuntimeError(f'Method {method} is not supported')

        # set value
        peak_pos_vec[index] = pos_x

    vec_pid = np.arange(first_ws_index, last_ws_index)

    return vec_pid, peak_pos_vec


def create_groups():
    """Create group workspace
    """
    # create group workspace
    group_ws = CreateGroupingWorkspace(InstrumentName='vulcan',
                                       GroupDetectorsBy='Group',
                                       OutputWorkspace='VULCAN_3Bank_Groups')

    return group_ws


def test_main_old():
    """Test main for prototyping
    """
    # Inputs
    diamond_file = 'VULCAN_164960_matrix.nxs'
    peak_pos = 1.076
    peak_range = 1.00, 1.15
    bank_id = 3

    # Load data
    diamond_ws = Load(Filename=diamond_file, OutputWorkspace='VULCAN_164960_matrix')

    # dictionary for bank
    bank_pixel_dict = {1: (0, 3234), 2: (3234, 6468), 3: (6468, 24900)}

    # workflow
    spec_range = bank_pixel_dict[bank_id]

    x0, y0 = observe_peak_centers(diamond_ws, spec_range, peak_pos, peak_range, 'max')
    x1, y1 = observe_peak_centers(diamond_ws, spec_range, peak_pos, peak_range, 'com')
    x2, y2 = get_peak_centers(diamond_ws, spec_range, peak_pos, peak_range, 'bank1_peak1')

    plt.plot(x0, y0, linestyle='None', marker='+', color='blue', label='max Y')
    plt.plot(x1, y1, linestyle='None', marker='x', color='red', label='c.o.m')
    print(f'{x2[0]}, {x2[-1]}')
    plt.plot(x2, y2, linestyle='None', marker='.', color='black', label='Gaussian')
    plt.legend()
    plt.show()

    return


def test_main():
    """Test main for prototyping
    """
    # Inputs
    diamond_file = 'VULCAN_164960_matrix.nxs'
    peak_pos = 1.076
    peak_range = 1.00, 1.15
    bank_id = 3

    # Load data
    diamond_ws = Load(Filename=diamond_file, OutputWorkspace='VULCAN_164960_matrix')

    # dictionary for bank
    bank_pixel_dict = {1: (0, 3234), 2: (3234, 6468), 3: (6468, 24900)}

    # workflow

    # Init diagnostic instance
    bank_id = 1
    spec_range = bank_pixel_dict[bank_id]
    diagnostic_bank1_peak1 = DiagnosticData(diamond_ws, np.arange(spec_range[0], spec_range[1]),
                                            peak_pos, peak_range, f'bank{bank_id}_peak1')
    diagnostic_bank1_peak1.find_peaks_centers()

    bank_id = 3
    spec_range = bank_pixel_dict[bank_id]
    diagnostic_bank1_peak3 = DiagnosticData(diamond_ws, np.arange(spec_range[0], spec_range[1]),
                                            peak_pos, peak_range, f'bank{bank_id}_peak1')
    diagnostic_bank1_peak3.find_peaks_centers()

    # Plot
    diagnostic_plot = DiagnosticPlot(2, 1)
    diagnostic_plot.set_cell(0, 0, diagnostic_bank1_peak1, 'Bank 1, Peak 1.076')
    diagnostic_plot.set_cell(1, 0, diagnostic_bank1_peak3, 'Bank 3, Peak 1.076')
    diagnostic_plot.plot('fit', None)

    return


if __name__ == '__main__':
    test_main()
