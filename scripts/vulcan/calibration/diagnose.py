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

        # Start (multiple) plots
        fig = plt.figure()

        for index, row_col in enumerate(self._matrix.keys()):
            # get row and columun
            row, column = row_col

            # get raw data
            vec_pixels = self._matrix[(row, column)].pixels
            vec_peak_pos = self._matrix[(row, column)].peak_centers_fit

            print(f'Max position = {vec_peak_pos.max()},  Min position = {vec_peak_pos.min()}')

            # filter out valid peak positions (positive)
            valid_center_vec = vec_peak_pos[vec_peak_pos > 0]
            valid_spec_vec = vec_pixels[vec_peak_pos > 0]

            # process
            if relative_position:
                valid_center_vec = 2 * (
                        valid_spec_vec - self._matrix[(row, column)].expected_peak_pos_d) / (
                        valid_center_vec + self._matrix[(row, column)].expected_peak_pos_d)
                valid_center_vec = 1 - valid_center_vec

            # set up plot
            plot_index = self._num_rows * 200 + self._num_cols * 10 + (index + 1)
            print(f'subplot index = {plot_index}')
            ax_i = fig.add_subplot(plot_index)
            self.plot_center_regular(ax_i, valid_spec_vec, valid_center_vec, self._title[(row, column)])

            plot_index = self._num_rows * 200 + self._num_cols * 10 + (index + 2)
            print(f'subplot index = {plot_index}')
            ax_i = fig.add_subplot(plot_index)
            self.plot_center_sigma(ax_i, valid_spec_vec, valid_center_vec, self._title[(row, column)], n_sigma=3)

            # ax_i.plot(vec_pixels, vec_peak_pos, linestyle='None', marker='.', color='red',
            #           label=self._title[(row, column)])
            #
            # # Get the good fitting ones only
            # valid_centers_vec = peak_center_vec[peak_center_vec > 0]
            # valid_spec_vec = ws_index_vec[peak_center_vec > 0]
            #
            # pos_average = np.mean(valid_centers_vec)
            # pos_std_dev = np.std(valid_centers_vec)
            # print(f'Positon = {pos_average} +/- Standard deviation = {pos_std_dev}')
            #
            # plt.plot(valid_spec_vec, valid_centers_vec, linestyle='None', marker='.', color='blue', label='good fit')
            # plt.plot([valid_spec_vec[0], valid_spec_vec[-1]], [pos_average + pos_std_dev, pos_average + pos_std_dev],
            #          linestyle='--', color='black')  # , linestyle='.')
            # plt.plot([valid_spec_vec[0], valid_spec_vec[-1]], [pos_average - pos_std_dev, pos_average - pos_std_dev],
            #          linestyle='--', color='black')  # linesyle='-')
            # plt.plot([valid_spec_vec[0], valid_spec_vec[-1]],
            #          [pos_average + 3 * pos_std_dev, pos_average + 3 * pos_std_dev], linestyle='--',
            #          color='black')  # , linestyle='.')
            # plt.plot([valid_spec_vec[0], valid_spec_vec[-1]],
            #          [pos_average - 3 * pos_std_dev, pos_average - 3 * pos_std_dev], linestyle='--',
            #          color='black')  # linesyle='-')
            #
            # plt.ylim(top=pos_average + 4 * pos_std_dev)  # adjust the top leaving bottom unchanged
            # plt.ylim(bottom=pos_average - 4 * pos_std_dev)  # adjust the bottom leaving top unchanged
            #
            # plt.show()

            # Legend setup
            plt.legend()

            # plt.plot(vec_pixels, vec_peak_pos, linestyle='None', marker='.', color='red',
            #          label=self._title[(row, column)])

        # Save or show
        if png_name:
            plt.savefig(png_name)
        else:
            plt.show()

    @staticmethod
    def plot_center_regular(axis, vec_pixels, vec_y, title):
        axis.plot(vec_pixels, vec_y, linestyle='None', marker='.', color='red',
                  label=title)

        plt.legend()

    @staticmethod
    def plot_center_sigma(axis, vec_pixels, vec_y, title, n_sigma):
        # average and standard deviation
        pos_average = np.mean(vec_y)
        pos_std_dev = np.std(vec_y)
        title += f': {pos_average:.5f} +/- {pos_std_dev:.5f}'

        axis.plot(vec_pixels, vec_y, linestyle='None', marker='.', color='red',
                  label=title)

        if n_sigma < 1:
            return

        # plot lines for 1-sigma, n-sigma and limit on Y
        for sigma in [1, n_sigma]:
            axis.plot([vec_pixels[0], vec_pixels[-1]],
                      [pos_average - sigma * pos_std_dev, pos_average - sigma * pos_std_dev],
                      linestyle='--', color='black')
            axis.plot([vec_pixels[0], vec_pixels[-1]],
                      [pos_average + sigma * pos_std_dev, pos_average + sigma * pos_std_dev],
                      linestyle='--', color='black')
        top = pos_average + (n_sigma + 2) * pos_std_dev
        bottom = pos_average - (n_sigma + 2) * pos_std_dev
        axis.set_ylim(bottom, top)  # adjust the bottom/top leaving bottom unchanged
        axis.set_xlim(vec_pixels[0] - 1, vec_pixels[-1] + 1)  # adjust the bottom/top leaving bottom unchanged

        plt.legend()


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


def test_main():
    """Test main for prototyping
    """
    # Inputs
    diamond_file = 'VULCAN_164960_matrix.nxs'
    peak_pos = 1.076
    peak_range = 1.00, 1.15

    # Load data
    diamond_ws = Load(Filename=diamond_file, OutputWorkspace='VULCAN_164960_matrix')

    # dictionary for bank
    bank_pixel_dict = {1: (0, 3234), 2: (3234, 6468), 3: (6468, 24900)}

    # workflow

    # Init diagnostic instance
    # bank_id = 1
    # spec_range = bank_pixel_dict[bank_id]
    # diagnostic_bank1_peak1 = DiagnosticData(diamond_ws, np.arange(spec_range[0], spec_range[1]),
    #                                         peak_pos, peak_range, f'bank{bank_id}_peak1')
    # diagnostic_bank1_peak1.find_peaks_centers()

    bank_id = 3
    spec_range = bank_pixel_dict[bank_id]
    diagnostic_bank1_peak3 = DiagnosticData(diamond_ws, np.arange(spec_range[0], spec_range[1]),
                                            peak_pos, peak_range, f'bank{bank_id}_peak1')
    diagnostic_bank1_peak3.find_peaks_centers()

    # Plot
    diagnostic_plot = DiagnosticPlot(1, 1)
    # diagnostic_plot.set_cell(0, 0, diagnostic_bank1_peak1, 'Bank 1, Peak 1.076')
    diagnostic_plot.set_cell(0, 0, diagnostic_bank1_peak3, 'Bank 3, Peak 1.076')
    diagnostic_plot.plot('fit', None, relative_position=False)

    return


if __name__ == '__main__':
    test_main()
