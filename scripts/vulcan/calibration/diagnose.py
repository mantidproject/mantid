# Script to diagnose the calibration result
from mantid.simpleapi import mtd
from mantid.simpleapi import Load, CreateGroupingWorkspace, FitPeaks
import numpy as np
from typing import Tuple, Any
from matplotlib import pyplot as plt


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
             StartWorkspaceIndex=first_ws_index,
             StopWorkspaceIndex=last_ws_index - 1,
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
            # print(f'Max Y = {max_y_i} @ {max_y_index} ... x = {x_array[index][max_y_index]}')
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


def test_main():
    """Test main for prototyping
    """
    # Inputs
    diamond_file = 'VULCAN_164960_matrix.nxs'
    peak_pos = 1.076
    peak_range = 1.00, 1.15
    bank_id = 1
    ref_spec_index = 100

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

    # workspace

    return


if __name__ == '__main__':
    test_main()
