# Zoo of methods that are develooped for analyze the calibration
from mantid.simpleapi import (AlignDetectors, FitPeaks, FindPeakBackground, DiffractionFocussing, Rebin,
                              ConvertToMatrixWorkspace, EditInstrumentGeometry, SaveNexusProcessed)
from mantid.simpleapi import mtd
import numpy as np


class FindDiamondPeaks(object):

    def __init(self):

        self._diamond_ws_name = None

    def find_peak(self, peak_name, exp_pos, d_min, d_max, start_ws_index, end_ws_index):

        # fit peaks
        peak_pos_ws_name = self.fit_peaks(exp_pos, d_min, d_max)

        # separate good fit/bad fit/no fit
        self.analyze_fitted_peaks(peak_pos_ws_name)

        # do statistics on fit result (average, standard deviation)

        # ...

    def fit_peaks(self, expected_peak_pos, min_d, max_d, start_ws_index, end_ws_index, suffix):

        # base workspace name (output)
        tag = f'{self._diamond_ws_name}_{suffix}'

        # output workspace names
        peak_pos_ws_name = f'{tag}_peak_positions'

        # Default to Gaussian and Linear background
        FitPeaks(InputWorkspace=self._diamond_ws_name,
                 OutputWorkspace=out_peak_pos_ws,
                 StartWorkspaceIndex=int(start_ws_index),
                 StopWorkspaceIndex=int(end_ws_index),
                 PeakCenters=f'{expected_peak_pos}',
                 FitWindowBoundaryList=f'{min_d}, {max_d}',
                 FittedPeaksWorkspace=f'{tag}_model',
                 OutputPeakParametersWorkspace=f'{tag}_params',
                 OutputParameterFitErrorsWorkspace=f'{tag}_errors')

        return peak_pos_ws_name

    def analyze_fitted_peaks(self, peak_pos_ws_name: str,
                             start_ws_index: int,
                             end_ws_index: int):
        # . -1 for data is zero;
        # -2 for maximum value is smaller than specified minimum value.and
        # -3 for non-converged fitting.

        peak_pos_ws = mtd[peak_pos_ws_name]

        ws_index_vec = np.arange(start_ws_index, end_ws_index)
        # ws_index_vec = np.arange(6468, 24900)

        # Sanity check
        print(f'Number of histograms in {peak_pos_ws_name} = {peak_pos_ws.getNumberHistograms()}, '
              f'Spectrum vector shape = {ws_index_vec.shape}')
        # peak positions
        peak_center_vec = peak_pos_ws.extractY().flatten()
        assert ws_index_vec.shape == peak_center_vec.shape

        # Type 1: zero counts
        zero_counts_pixels = np.where(np.abs(peak_center_vec - (-1)) < 1E-5)[0]
        print(f'Zero counts = {len(zero_counts_pixels)}')

        # Type 2: low counts
        low_counts_pixels = np.where(np.abs(peak_center_vec - (-2)) < 1E-5)[0]
        print(f'Low counts = {len(low_counts_pixels)}')

        bad_fit_pixels = np.where(np.abs(peak_center_vec - (-3)) < 1E-5)[0]
        print(f'Type 1 bad counts = {len(bad_fit_pixels)}')
        # print(f'  they are ... {bad_fit_pixels + 6468}')

        bad_fit_pixels = np.where(np.abs(peak_center_vec - (-4)) < 1E-5)[0]
        print(f'Type 2 bad counts = {len(bad_fit_pixels)}')
        # print(f'  they are ... {bad_fit_pixels + 6468}')

        # Check again with counts
        diamond_ws = mtd[self._diamond_ws_name]
        counts_vec = diamond_ws.extractY().sum(axis=1)[start_ws_index:end_ws_index]

    def get_peak_height_max_y(self):
        # Get peak height as maximum Y value by removing background

        FindPeakBackground(InputWorkspace='VULCAN_164960_matrix', WorkspaceIndex=10000, FitWindow='1,1.2',
                           OutputWorkspace='allmultiple')

        ws_index = cell(0, 0)
        peak_min_index = cell(0, 1)
        peak_max_index = cell(0, 2)
        bgkd_0 = cell(0, 3)
        bkgd_1 = cell(0, 4)
        bkgd_2 = cell(0, 5)


def report_masked_pixels(data_workspace, mask_ws, wi_start, wi_stop):
    """Generate a report for all masked pixels

    Parameters
    ----------
    data_workspace: str, MatrixWorkspace
        A diamond workspace.  It can be either the raw EventWorkspace or a Workspace2D with 1 bin (count)
    mask_ws: str, MaskWorkspace
        Mask workspace
    wi_start: int, None
        starting workspace index
    wi_stop: int, None
        stopping workspace index (excluded)

    Returns
    -------

    """
    # Get input
    if isinstance(data_workspace, str):
        data_workspace = mtd[data_workspace]
    if isinstance(mask_ws, str):
        mask_ws = mtd[mask_ws]
    if wi_start is None:
        wi_start = 0
    if wi_stop is None:
        wi_stop = mask_ws.getNumberHistograms()

    # Check input
    # get the number of events per spectrum
    events_number_vec = data_workspace.extractY().sum(axis=1).flatten()
    assert mask_ws.getNumberHistograms() == events_number_vec.shape[0], f'Number of histograms does not match.  counts vector shape = {events_number_vec.shape}'

    # Set initial value for statistics
    num_masked = 0
    zero_masked = 0
    event_spectrum_list = list()
    for ws_index in range(wi_start, wi_stop+1):
        # skip non-masked pixels
        if mask_ws.readY(ws_index)[0] < 0.1:
            continue
        else:
            num_masked += 1

        # analyze masking information
        if events_number_vec[ws_index] == 0:
            zero_masked += 1
        else:
            event_spectrum_list.append((events_number_vec[ws_index], ws_index))

    # Make report
    report = '[REPORT]'
    report += '\nFrom {} to {}: Number of masked pixels = {}, including '.format(wi_start, wi_stop-1, num_masked)
    report += f'\n  (1) {zero_masked} pixels with zero counts'
    report += f'\n  (2) {num_masked - zero_masked} pixels with non-zero counts and they are ... '
    event_spectrum_list.sort(reverse=True)
    for i in range(min(100, len(event_spectrum_list))):
        num_events_i, ws_index = event_spectrum_list[i]
        print('      ws-index = {}, num of events = {}'.format(ws_index, num_events_i))

    return report


def align_focus_event_ws(event_ws_name, calib_ws_name, group_ws_name):
    """
    overwrite the input
    """

    # Align detector
    print(f'Event workspace: {event_ws_name}.  X unit = {mtd[event_ws_name].getAxis(0).getUnit().unitID()}')

    if calib_ws_name:
        AlignDetectors(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                       CalibrationWorkspace=calib_ws_name)
   
        matrix_ws_name = f'{event_ws_name}_matrix'
        Rebin(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name, Params='0.3,-0.0003,3')
        ConvertToMatrixWorkspace(InputWorkspace=event_ws_name, OutputWorkspace=matrix_ws_name)
        SaveNexusProcessed(InputWorkspace=matrix_ws_name, Filename=f'{event_ws_name}_aligned.nxs')
        print(f'[CHECK] saved aligned workspace size: {mtd[matrix_ws_name].extractY().shape}')

    # Get units
    event_ws = mtd[event_ws_name]
    assert event_ws.getAxis(0).getUnit().unitID() == 'dSpacing', f'Expecting {event_ws_name} to be dSpacing but ' \
                                                                 f'it is {event_ws.getAxis(0).getUnit().unitID()}'

    DiffractionFocussing(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                         GroupingWorkspace=group_ws_name)

    ConvertToMatrixWorkspace(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name)

    EditInstrumentGeometry(Workspace=event_ws_name, PrimaryFlightPath=42, SpectrumIDs='1-3', L2='2,2,2',
                           Polar='89.9284,90.0716,150.059', Azimuthal='0,0,0', DetectorIDs='1-3',
                           InstrumentName='vulcan_3bank')

    SaveNexusProcessed(InputWorkspace=event_ws_name, Filename=f'{event_ws_name}_3banks.nxs')

    return event_ws_name


def get_masked_ws_indexes(mask_ws):
    """
    get the workspace indexes that are masked
    :param mask_ws:
    :return:
    """
    if isinstance(mask_ws, str):
        mask_ws = mtd[mask_ws]

    masked_list = list()
    for iws in range(mask_ws.getNumberHistograms()):
        if mask_ws.readY(iws)[0] > 0.5:
            masked_list.append(iws)

    return masked_list
