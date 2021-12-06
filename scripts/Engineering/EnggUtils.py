# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum
from mantid.api import *
from mantid.kernel import IntArrayProperty, UnitConversion, DeltaEModeType
import mantid.simpleapi as mantid
from mantid.simpleapi import AnalysisDataService as ADS
import numpy as np

ENGINX_BANKS = ['', 'North', 'South', 'Both: North, South', '1', '2']
ENGINX_MASK_BIN_MINS = [0, 19930, 39960, 59850, 79930]
ENGINX_MASK_BIN_MAXS = [5300, 20400, 40450, 62000, 82670]


class GROUP(Enum):
    """Group Enum with attributes: banks (list of banks required for calibration) and grouping ws name"""

    def __new__(self, value, banks):
        obj = object.__new__(self)
        obj._value_ = value  # overwrite value to be first arg
        obj.banks = banks  # set attribute bank
        return obj

    # value of enum is the file suffix of .prm (for easy creation)
    #       value,  banks
    BOTH = "banks", [1, 2]
    NORTH = "1", [1]
    SOUTH = "2", [2]
    CROPPED = "Cropped", []  # pdcal results will be saved with grouping file with same suffix
    CUSTOM = "Custom", []  # pdcal results will be saved with grouping file with same suffix
    TEXTURE = "Texture", [1, 2]


def plot_tof_vs_d_from_calibration(diag_ws, ws_foc, dspacing, calibration):
    """
    Plot fitted TOF vs expected d-spacing from diagnostic workspaces output from PDCalibration
    :param diag_ws: workspace object of group of diagnostic workspaces
    :param ws_foc: workspace object of focused data (post ApplyDiffCal with calibrated diff_consts)
    :param calibration: CalibrationInfo object used to determine subplot axes titles
    :return:
    """
    from matplotlib.pyplot import subplots

    fitparam = mtd[diag_ws.name() + "_fitparam"].toDict()
    fiterror = mtd[diag_ws.name() + "_fiterror"].toDict()
    d_table = mtd[diag_ws.name() + "_dspacing"].toDict()
    dspacing = np.array(sorted(dspacing))  # PDCal sorts the dspacing list passed
    x0 = np.array(fitparam['X0'])
    x0_er = np.array(fiterror['X0'])
    ws_index = np.array(fitparam['wsindex'])
    nspec = len(set(ws_index))
    si = ws_foc.spectrumInfo()

    ncols_per_fig = 4  # max number of spectra per figure window
    figs = []
    for ispec in range(nspec):
        # extract data from tables
        detid = ws_foc.getSpectrum(ispec).getDetectorIDs()[0]
        irow = d_table['detid'].index(detid)
        valid = [np.isfinite(d_table[key][irow]) for key in d_table if '@' in key]  # nan if fit unsuccessful
        ipks = ws_index == ispec  # peak centres for this spectrum
        x, y, e = dspacing[valid], x0[ipks][valid], x0_er[ipks][valid]
        # get poly fit
        diff_consts = si.diffractometerConstants(ispec)  # output is a UnitParametersMap
        yfit = np.array([UnitConversion.run("dSpacing", "TOF", xpt, 0, DeltaEModeType.Elastic, diff_consts)
                         for xpt in x])
        # plot polynomial fit to TOF vs dSpacing
        if ispec + 1 > len(figs) * ncols_per_fig:
            # create new figure
            ncols = ncols_per_fig if not nspec - ispec < ncols_per_fig else nspec % ncols_per_fig
            fig, ax = subplots(2, ncols, sharex=True, sharey='row', subplot_kw={'projection': 'mantid'})
            ax = np.reshape(ax, (-1, 1)) if ax.ndim == 1 else ax  # to ensure is 2D matrix even if ncols==1
            ax[0, 0].set_ylabel('Fitted TOF (\u03BCs)')
            ax[1, 0].set_ylabel('Residuals (\u03BCs)')
            figs.append(fig)
            icol = 0
        # plot TOF vs d
        ax[0, icol].set_title(calibration.get_subplot_title(ispec))
        ax[0, icol].errorbar(x, y, yerr=e, marker='o', markersize=3, capsize=2, ls='', color='b', label='Peak centres')
        ax[0, icol].plot(x, yfit, '-r', label='quadratic fit')
        # plot residuals
        ax[1, icol].errorbar(x, y - yfit, yerr=e, marker='o', markersize=3, capsize=2, ls='', color='b', label='resids')
        ax[1, icol].axhline(color='r', ls='-')
        ax[1, icol].set_xlabel('d-spacing (Ang)')
        icol += 1
    # Format figs and show
    for fig in figs:
        fig.tight_layout()
        fig.show()


def create_spectrum_list_from_string(str_list):
    array = IntArrayProperty('var', str_list).value
    int_list = [int(i) for i in array]  # cast int32 to int
    return int_list


def default_ceria_expected_peaks(final=False):
    """
    Get the list of expected Ceria peaks, which can be a good default for the expected peaks
    properties of algorithms like PDCalibration
    @param :: final - if true, returns a list better suited to a secondary fitting of focused banks
    @Returns :: a list of peaks in d-spacing as a float list
    """
    _CERIA_EXPECTED_PEAKS = [1.104598643, 1.352851554, 1.631600313, 1.913220892, 2.705702376]
    _CERIA_EXPECTED_PEAKS_FINAL = [0.781069, 0.855618487, 0.901900955, 0.914694494, 0.956610446, 1.04142562,
                                   1.104598643, 1.210027059, 1.241461538, 1.352851554, 1.562138267, 1.631600313,
                                   1.913220892, 2.705702376]

    return _CERIA_EXPECTED_PEAKS_FINAL if final else _CERIA_EXPECTED_PEAKS


def default_ceria_expected_peak_windows(final=False):
    """
    Get the list of windows over which to fit ceria peaks in calls to PDCalibration
    @param :: final - if true, returns a list better suited to a secondary fitting of focused banks
    @Returns :: a list of peak windows in d-spacing as a float list
    """
    _CERIA_EXPECTED_WINDOW = [1.06515, 1.15210,  1.30425, 1.41292, 1.59224, 1.68462, 1.84763, 1.98891, 2.64097, 2.77186]
    _CERIA_EXPECTED_WINDOW_FINAL = [0.77, 0.805, 0.83702, 0.88041, 0.88041, 0.90893, 0.90893, 0.93474, 0.93474, 0.98908,
                                    1.01625, 1.06515, 1.06515, 1.15210, 1.16297, 1.22817, 1.22817, 1.29338, 1.30425,
                                    1.41292, 1.53242, 1.59224, 1.59224, 1.68462, 1.84763, 1.98891, 2.64097, 2.77186]

    return _CERIA_EXPECTED_WINDOW_FINAL if final else _CERIA_EXPECTED_WINDOW

# DEPRECATED FUNCTIONS BELOW


def read_in_expected_peaks(filename, expected_peaks):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggCalibrateFull, EnggCalibrate and EnggFitPeaks)

    Reads in expected peaks from the .csv file if requested. Otherwise fall back to the list of
    peaks given (and check that it is not empty).

    @param :: filename name of the csv file to read from. If empty, we take the peaks given in an option.
    This is passed to Engg algorithms in the (optional) input property 'ExpectedPeaksFromFile'

    @param :: expected_peaks list of expected peaks given as an input property to an Engg algorithm
    (ExpectedPeaks)

    @returns the expected peaks either from a file or input list, sorted in ascending order
    """

    if filename:
        ex_peak_array = []
        read_in_array = []
        try:
            with open(filename) as f:
                for line in f:
                    read_in_array.append([float(x) for x in line.split(',')])
            for a in read_in_array:
                for b in a:
                    ex_peak_array.append(b)
        except RuntimeError as exc:
            raise RuntimeError("Error while reading file of expected peaks '%s': %s" % (filename, exc))

        if not ex_peak_array:
            # "File could not be read. Defaults in alternative option used."
            if not expected_peaks:
                raise ValueError("Could not read any peaks from the file given in 'ExpectedPeaksFromFile: '"
                                 + filename + "', and no expected peaks were given in the property "
                                 "'ExpectedPeaks' either. Cannot continue without a list of expected peaks.")
            expected_peaks_d = sorted(expected_peaks)

        else:
            expected_peaks_d = sorted(ex_peak_array)

    else:
        if 0 == len(expected_peaks):
            raise ValueError("No expected peaks were given in the property 'ExpectedPeaks', "
                             "could not get default expected peaks, and 'ExpectedPeaksFromFile' "
                             "was not given either. Cannot continout without a list of expected peaks.")
        expected_peaks_d = sorted(expected_peaks)

    return expected_peaks_d


def get_ws_indices_from_input_properties(workspace, bank, detector_indices):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggCalibrateFull, EnggCalibrate and EnggFitPeaks)

    Get the detector indices that the user requests, either through the input property 'Bank' or
    'DetectorIndices'

    @param workspace :: input workspace (with instrument)
    @param bank :: value passed in the input property 'Bank' to an Engg algorithm
    @param detector_indices :: value passed in the 'Det

    @returns list of workspace indices that can be used in mantid algorithms such as CropWorkspace.
    """

    if bank and detector_indices:
        raise ValueError("It is not possible to use at the same time the input properties 'Bank' and "
                         "'DetectorIndices', as they overlap. Please use either of them. Got Bank: '%s', "
                         "and DetectorIndices: '%s'" % (bank, detector_indices))
    elif bank:
        bank_aliases = {'North': '1', 'South': '2', 'Both: North, South': '-1'}
        bank = bank_aliases.get(bank, bank)
        indices = get_ws_indices_for_bank(workspace, bank)
        if not indices:
            raise RuntimeError("Unable to find a meaningful list of workspace indices for the "
                               "bank passed: %s. Please check the inputs." % bank)
        return indices
    elif detector_indices:
        indices = parse_spectrum_indices(workspace, detector_indices)
        if not indices:
            raise RuntimeError("Unable to find a meaningful list of workspace indices for the "
                               "range(s) of detectors passed: %s. Please check the inputs." % detector_indices)
        return indices
    else:
        raise ValueError("You have not given any value for the properties 'Bank' and 'DetectorIndices' "
                         "One of them is required")


def parse_spectrum_indices(workspace, spectrum_numbers):
    """
    DEPRECATED: not used in UI, only in get_ws_indices_from_input_properties which is used in
    deprecated functions (EnggCalibrateFull, EnggCalibrate and EnggFitPeaks)

    Get a usable list of workspace indices from a user provided list of spectra that can look like:
    '8-10, 20-40, 100-110'. For that example this method will produce: [7,8,9,19, 20,... , 109]

    @param workspace :: input workspace (with instrument)
    @param spectrum_numbers :: range of spectrum numbers (or list of ranges) as given to an algorithm

    @return list of workspace indices, ready to be used in mantid algorithms such as CropWorkspace
    """
    segments = [s.split("-") for s in spectrum_numbers.split(",")]
    indices = [idx for s in segments for idx in range(int(s[0]), int(s[-1]) + 1)]
    # remove duplicates and sort
    indices = sorted(set(indices))
    max_index = workspace.getNumberHistograms()
    if indices[-1] >= max_index:
        raise ValueError("A workspace index equal or bigger than the number of histograms available in the "
                         "workspace '" + workspace.name() + "' (" + str(workspace.getNumberHistograms())
                         + ") has been given. Please check the list of indices.")
    # and finally translate from 'spectrum numbers' to 'workspace indices'
    return [workspace.getIndexFromSpectrumNumber(sn) for sn in indices]


def get_ws_indices_for_bank(workspace, bank):
    """
    DEPRECATED: not used in UI, only in deprecated function EnggVanadiumCorrections

    get_ws_indices_from_input_properties
    Finds the workspace indices of all the pixels/detectors/spectra corresponding to a bank.

    ws :: workspace with instrument definition
    bank :: bank number as it appears in the instrument definition.  A <0 value implies all banks.

    @returns :: list of workspace indices for the bank
    """
    detector_ids = get_detector_ids_for_bank(bank)

    def index_in_bank(index):
        try:
            det = workspace.getDetector(index)
            return det.getID() in detector_ids
        except RuntimeError:
            return False

    return [i for i in range(workspace.getNumberHistograms()) if index_in_bank(i)]


def get_detector_ids_for_bank(bank):
    """
    DEPRECATED: not used in UI, only in get_ws_indices_for_bank which is used in
    deprecated functions EnggVanadiumCorrections

    Find the detector IDs for an instrument bank. Note this is at this point specific to
    the ENGINX instrument.

    @param bank :: name/number as a string.

    @returns list of detector IDs corresponding to the specified Engg bank number
    """
    import os
    grouping_file_path = os.path.join(mantid.config.getInstrumentDirectory(),
                                      'Grouping', 'ENGINX_Grouping.xml')

    alg = AlgorithmManager.create('LoadDetectorsGroupingFile')
    alg.initialize()
    alg.setLogging(False)
    alg.setProperty('InputFile', grouping_file_path)
    group_name = '__EnginXGrouping'
    alg.setProperty('OutputWorkspace', group_name)
    alg.execute()

    # LoadDetectorsGroupingFile produces a 'Grouping' workspace.
    # PropertyWithValue<GroupingWorkspace> not working (GitHub issue 13437)
    # => cannot run as child and get outputworkspace property properly
    if not ADS.doesExist(group_name):
        raise RuntimeError('LoadDetectorsGroupingFile did not run correctly. Could not '
                           'find its output workspace: ' + group_name)
    grouping = mtd[group_name]

    detector_ids = set()

    # less then zero indicates both banks, from line 98
    bank_int = int(bank)
    if bank_int < 0:
        # both banks, north and south
        bank_int = [1, 2]
    else:
        # make into list so that the `if in` check works
        bank_int = [bank_int]

    for i in range(grouping.getNumberHistograms()):
        if grouping.readY(i)[0] in bank_int:
            detector_ids.add(grouping.getDetector(i).getID())

    mantid.DeleteWorkspace(grouping)

    if len(detector_ids) == 0:
        raise ValueError('Could not find any detector for this bank: ' + bank + '. This looks like an unknown bank')

    return detector_ids


def generate_output_param_table(name, difa, difc, tzero):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggCalibrate, EnggFitTOFFromPeaks)

    Produces a table workspace with the two fitted calibration parameters

    @param name :: the name to use for the table workspace that is created here
    @param difa :: DIFA calibration parameter (GSAS parameter)
    @param difc :: DIFC calibration parameter
    @param tzero :: TZERO calibration parameter
    """
    tbl = mantid.CreateEmptyTableWorkspace(OutputWorkspace=name)
    tbl.addColumn('double', 'DIFA')
    tbl.addColumn('double', 'DIFZ')
    tbl.addColumn('double', 'TZERO')
    tbl.addRow([float(difa), float(difc), float(tzero)])


def apply_vanadium_corrections(parent, ws, indices, vanadium_ws, van_integration_ws, van_curves_ws,
                               progress_range=None):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggCalibrateFull, EnggVanadiumCorrections and EnggFocus)

    Apply the EnggVanadiumCorrections algorithm on the workspace given, by using the algorithm
    EnggVanadiumCorrections

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to correct (modified in place)
    @param indices :: workspace indices that are being processed (those not included will be ignored)
    @param vanadium_ws :: workspace with data from a Vanadium run
    @param van_integration_ws :: alternatively to vanWS, pre-calculated integration from Vanadium data
    @param van_curves_ws :: alternatively to vanWS, pre-calculated bank curves from Vanadium data
    @param progress_range :: pair for (startProgress, endProgress) with respect to the parent algorithm
    """
    if vanadium_ws and vanadium_ws.getNumberHistograms() < len(indices):
        raise ValueError("Inconsistency in inputs: the Vanadium workspace has less spectra (%d) than "
                         "the number of workspace indices to process (%d)" %
                         (vanadium_ws.getNumberHistograms(), len(indices)))
    elif van_integration_ws and van_curves_ws:
        # filter only indices from vanIntegWS (crop the table)
        tbl = mantid.CreateEmptyTableWorkspace(OutputWorkspace="__vanadium_integration_ws")
        tbl.addColumn('double', 'Spectra Integration')
        for i in indices:
            tbl.addRow([van_integration_ws.cell(i, 0)])
        van_integration_ws = tbl

    # These corrections rely on ToF<->Dspacing conversions, so they're done after the calibration step
    progress_params = dict()
    if progress_range:
        progress_params["startProgress"] = progress_range[0]
        progress_params["endProgress"] = progress_range[1]

    alg = parent.createChildAlgorithm('EnggVanadiumCorrections', **progress_params)
    if ws:
        alg.setProperty('Workspace', ws)
    if vanadium_ws:
        alg.setProperty('VanadiumWorkspace', vanadium_ws)
    if van_integration_ws:
        alg.setProperty('IntegrationWorkspace', van_integration_ws)
    if van_curves_ws:
        alg.setProperty('CurvesWorkspace', van_curves_ws)

    alg.execute()


def convert_to_d_spacing(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Converts a workspace to dSpacing using 'ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in ToF units) to convert (not modified)

    @returns workspace converted to d-spacing units
    """
    # A check to catch possible errors in an understandable way
    expected_dimension = 'Time-of-flight'
    dimension = ws.getXDimension().name
    if expected_dimension != dimension:
        raise ValueError("This function expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'. This is an internal logic "
                         "error. " % (expected_dimension, dimension))

    alg = parent.createChildAlgorithm('ConvertUnits')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('Target', 'dSpacing')
    alg.setProperty('AlignBins', True)
    alg.execute()
    return alg.getProperty('OutputWorkspace').value


def convert_to_TOF(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Converts workspace to Time-of-Flight using 'ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in d-spacing units) to convert to ToF

    @returns workspace with data converted to ToF units
    """
    alg = parent.createChildAlgorithm('ConvertUnits')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('Target', 'TOF')
    alg.execute()
    return alg.getProperty('OutputWorkspace').value


def crop_data(parent, ws, indices):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Produces a cropped workspace from the input workspace so that only
    data for the specified bank (given as a list of indices) is left.

    NB: This assumes spectra for a bank are consequent.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to crop (not modified in-place)
    @param indices :: workspace indices to keep in the workpace returned

    @returns cropped workspace, with only the spectra corresponding to the indices requested
    """
    # Leave only spectra between min and max
    alg = parent.createChildAlgorithm('ExtractSpectra')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('WorkspaceIndexList', indices)
    alg.execute()

    return alg.getProperty('OutputWorkspace').value


def sum_spectra(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Focuses/sums up all the spectra into a single one (calls the SumSpectra algorithm)

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to sum up

    @return single-spectrum workspace resulting from the sum
    """
    alg = parent.createChildAlgorithm('SumSpectra')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('RemoveSpecialValues', True)
    alg.execute()

    return alg.getProperty('OutputWorkspace').value
