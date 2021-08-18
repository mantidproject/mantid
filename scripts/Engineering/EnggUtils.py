# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path
from mantid.api import *
from mantid.kernel import IntArrayProperty, UnitConversion, DeltaEModeType, logger
import mantid.simpleapi as mantid
from mantid.simpleapi import AnalysisDataService as ADS
from matplotlib import gridspec
from Engineering.common import path_handling

ENGINX_BANKS = ['', 'North', 'South', 'Both: North, South', '1', '2']

ENGINX_MASK_BIN_MINS = [0, 19930, 39960, 59850, 79930]
ENGINX_MASK_BIN_MAXS = [5300, 20400, 40450, 62000, 82670]


def determine_roi_from_prm_fname(file_path: str) -> str:
    """
    Determine the region of interest from the .prm calibration file that is being loaded
    :param file_path: Path of the .prm file being loaded
    :return: String describing the region of interest
    """
    # fname has form INSTRUMENT_VanadiumRunNo_ceriaRunNo_BANKS
    # BANKS can be "all_banks, "bank_1", "bank_2", "Cropped", "Custom"
    basepath, fname = path.split(file_path)
    fname_words = fname.split('_')
    suffix = fname_words[-1]
    if "banks" in suffix:
        return "BOTH"
    elif "1" in suffix:
        return "bank_1"
    elif "2" in suffix:
        return "bank_2"
    elif "Custom" in suffix:
        return "Custom"
    elif "Cropped" in suffix:
        return "Cropped"
    else:
        raise ValueError("Region of interest not recognised from .prm file name")


def load_relevant_calibration_files(file_path, output_prefix="engggui") -> list:
    """
    Determine which pdcal output .nxs files to Load from the .prm file selected, and Load them
    :param file_path: path to the calibration .prm file selected
    :param output_prefix: Prefix to use when defining the output workspace name
    :return bank if region of interest is one or both banks, None if not
    """
    basepath, fname = path.split(file_path)
    fname_words = fname.split('_')
    prefix = '_'.join(fname_words[0:2])
    roi = determine_roi_from_prm_fname(fname)
    bank = None
    if roi == "BOTH":
        path_to_load = path.join(basepath, prefix + "_bank_1.nxs")
        mantid.Load(Filename=path_to_load, OutputWorkspace=output_prefix + "_calibration_bank_1")
        path_to_load = path.join(basepath, prefix + "_bank_2.nxs")
        mantid.Load(Filename=path_to_load, OutputWorkspace=output_prefix + "_calibration_bank_2")
        bank = ['1', '2']
        return bank
    elif roi == "bank_1":
        path_to_load = path.join(basepath, prefix + "_bank_1.nxs")
        bank = ['1']
    elif roi == "bank_2":
        path_to_load = path.join(basepath, prefix + "_bank_2.nxs")
        bank = ['2']
    elif roi == "Custom":  # custom calfile case, need to load grouping workspace as well
        path_to_load = path.join(basepath, prefix + "_Custom.nxs")
    else:  # custom spectra numbers case, need to load grouping workspace as well
        path_to_load = path.join(basepath, prefix + "_Cropped.nxs")
    mantid.Load(Filename=path_to_load, OutputWorkspace=output_prefix + "_calibration_" + roi)
    return bank


def load_custom_grouping_workspace(file_path: str) -> (str, str):
    """
    Determine the grouping workspace that corresponds to the .prm calibration file being loaded, and if this is a
    non-bank based region, load and return the saved grouping workspace corresponding to the calibration being loaded.
    :param file_path: Path to the .prm file being loaded
    :return: Name of the grouping workspace IF custom, and description of roi for use as display text on the Focus tab
    """
    basepath, fname = path.split(file_path)
    fname_no_ext = fname[:-4]
    roi = determine_roi_from_prm_fname(fname)
    if roi == "Cropped":
        ws_name = "Cropped_spectra_grouping"
        roi_text = "Custom spectrum numbers"
    elif roi == "Custom":
        ws_name = "Custom_calfile_grouping"
        roi_text = "Custom calfile"
    elif roi == "BOTH":
        # no need to load grouping workspaces for single/both bank calibrations at this time
        return None, "North and South Banks"
    elif roi == "bank_1":
        return None, "North Bank"
    elif roi == "bank_2":
        return None, "South Bank"
    else:
        raise ValueError("Region not recognised")
    load_path = path.join(basepath, fname_no_ext + '.xml')
    mantid.LoadDetectorsGroupingFile(InputFile=load_path, OutputWorkspace=ws_name)
    return ws_name, roi_text


def save_grouping_workspace(grp_ws, directory: str, ceria_path: str, instrument: str,
                            calfile: str = None, spec_nos=None) -> None:
    if calfile:
        name = generate_output_file_name(ceria_path, instrument, "Custom", '.xml')
    elif spec_nos:
        name = generate_output_file_name(ceria_path, instrument, "Cropped", '.xml')
    else:
        logger.warning("No Calfile or Spectra given, no grouping workspace saved")
        return
    save_path = path.join(directory, name)
    mantid.SaveDetectorsGrouping(InputWorkspace=grp_ws, OutputFile=save_path)


def generate_output_file_name(ceria_path, instrument, bank, ext='.prm'):
    """
    Generate an output filename in the form INSTRUMENT_ceriaRunNo_BANKS
    :param vanadium_path: Path to vanadium data file
    :param ceria_path: Path to ceria data file
    :param instrument: The instrument in use.
    :param bank: The bank being saved.
    :param ext: Extension to be used on the saved file
    :return: The filename, the vanadium run number, and ceria run number.
    """
    ceria_no = path_handling.get_run_number_from_path(ceria_path, instrument)
    filename = instrument + "_" + ceria_no + "_"
    if bank == "all":
        filename = filename + "all_banks" + ext
    elif bank == "north":
        filename = filename + "bank_1" + ext
    elif bank == "south":
        filename = filename + "bank_2" + ext
    elif bank == "Cropped":
        filename = filename + "Cropped" + ext
    elif bank == "Custom":
        filename = filename + "Custom" + ext
    else:
        raise ValueError("Invalid bank name entered")
    return filename


def get_diffractometer_constants_from_workspace(ws):
    """
    Get diffractometer constants from workspace
    TOF = difc*d + difa*(d^2) + tzero
    """
    si = ws.spectrumInfo()
    diff_consts = si.diffractometerConstants(0)  # output is a UnitParametersMap
    return diff_consts


def generate_tof_fit_dictionary(cal_name=None) -> dict:
    """
    Generate a dictionary of data to plot showing the results of the calibration
    :param cal_name: Name of the region of interest of the calibration
    :return: dict, keys: x = expected peaks (dSpacing), y = fitted peaks (TOF), e = y error data,
                         y2 = calculated peaks (TOF), r = residuals (y - y2)
    """
    if not cal_name:
        generate_tof_fit_dictionary("bank_1")
        generate_tof_fit_dictionary("bank_2")
    if cal_name[-1:] == '1':  # bank_1
        diag_ws_name = "diag_bank_1"
    elif cal_name[-1:] == '2':
        diag_ws_name = "diag_bank_2"
    else:
        diag_ws_name = "diag_" + cal_name
    fitparam_ws_name = diag_ws_name + "_fitparam"
    fitted_ws_name = diag_ws_name + "_fitted"
    fiterror_ws_name = diag_ws_name + "_fiterror"
    fitparam_ws = ADS.retrieve(fitparam_ws_name)
    fitted_ws = ADS.retrieve(fitted_ws_name)
    fiterror_ws = ADS.retrieve(fiterror_ws_name)

    expected_dspacing_peaks = default_ceria_expected_peaks(final=True)

    expected_d_peaks_x = []
    fitted_tof_peaks_y = []
    tof_peaks_error_e = []
    calculated_tof_peaks_y2 = []
    residuals = []
    for irow in range(0, fitparam_ws.rowCount()):
        expected_d_peaks_x.append(expected_dspacing_peaks[-(irow + 1)])
        fitted_tof_peaks_y.append(fitparam_ws.cell(irow, 5))
        tof_peaks_error_e.append(fiterror_ws.cell(irow, 5))
        calculated_tof_peaks_y2.append(convert_single_value_dSpacing_to_TOF(expected_d_peaks_x[irow], fitted_ws))
        residuals.append(fitted_tof_peaks_y[irow] - calculated_tof_peaks_y2[irow])

    return {'x': expected_d_peaks_x, 'y': fitted_tof_peaks_y, 'e': tof_peaks_error_e, 'y2': calculated_tof_peaks_y2,
            'r': residuals}


def plot_tof_fit(plot_dicts: list, regions: list) -> None:
    """
    Plot fitted tof peaks against calculated tof peaks to show quality of calibration. Residuals also plotted.
    :param regions: list of string names of regions of interest calibrated
    :param plot_dicts: list of dictionaries containing data to plot, see EnggUtils.generate_tof_fit_dictionary
    :return: None
    """
    def _add_plot_to_axes(ax, plot_dict, bank):
        ax.errorbar(plot_dict['x'], plot_dict['y'], yerr=plot_dict['e'], capsize=2, marker=".", color='b',
                    label="Peaks Fitted", ls="None")
        ax.plot(plot_dict['x'], plot_dict['y2'], linestyle="-", marker="None", color='r', label="TOF Quadratic Fit")
        ax.set_title("Engg Gui TOF Peaks " + str(bank))
        ax.legend()
        ax.set_xlabel("")  # hide here as set automatically
        ax.set_ylabel("Fitted Peaks Centre (TOF, \u03BCs)")

    def _add_residuals_to_axes(ax, plot_dict):
        ax.errorbar(plot_dict['x'], plot_dict['r'], yerr=plot_dict['e'], color='b', marker='.', capsize=2, ls="None")
        ax.axhline(color='r')
        ax.set_xlabel("Expected Peaks Centre(dSpacing, A)")
        ax.set_ylabel("Residuals (TOF, \u03BCs)")

    n_plots = len(regions)

    # Create plot
    # import pyplot here to stop FindPeakAutomaticTest picking it up
    from matplotlib.pyplot import figure
    fig = figure()
    gs = gridspec.GridSpec(2, n_plots)
    bank_axes = [fig.add_subplot(gs[0, n], projection="mantid") for n in range(n_plots)]
    residuals_axes = [fig.add_subplot(gs[1, n], projection="mantid") for n in range(n_plots)]

    for ax, plot_dict, bank in zip(bank_axes, plot_dicts, regions):
        _add_plot_to_axes(ax, plot_dict, bank)
    for ax, plot_dict in zip(residuals_axes, plot_dicts):
        _add_residuals_to_axes(ax, plot_dict)
    fig.tight_layout()
    fig.show()


def convert_single_value_dSpacing_to_TOF(d, diff_consts_ws):
    diff_consts = get_diffractometer_constants_from_workspace(diff_consts_ws)
    # L1 = 0 is ignored when diff constants supplied
    tof = UnitConversion.run("dSpacing", "TOF", d, 0, DeltaEModeType.Elastic, diff_consts)
    return tof


def create_spectrum_list_from_string(str_list):
    array = IntArrayProperty('var', str_list).value
    int_list = [int(i) for i in array]  # cast int32 to int
    return int_list


def get_bank_grouping_workspace(bank: int, sample_raw):  # -> GroupingWorkspace
    """
    Retrieve the grouping workspace for the North/South bank from the user directories, or create a new one from the
    sample workspace instrument data if not found
    :param bank: integer denoting the bank, 1 or 2 for North/South respectively
    :param sample_raw: Workspace containing the instrument data that can be used to create a new grouping workspace
    :return: The loaded or created grouping workspace
    """
    if bank == 1:
        try:
            if ADS.doesExist("NorthBank_grouping"):
                return ADS.retrieve("NorthBank_grouping")
            grp_ws = mantid.LoadDetectorsGroupingFile(InputFile="ENGINX_North_grouping.xml",
                                                      OutputWorkspace="NorthBank_grouping")
            return grp_ws
        except ValueError:
            logger.notice("NorthBank grouping file not found in user directories - creating one")
        bank_name = "NorthBank"
    elif bank == 2:
        try:
            if ADS.doesExist("SouthBank_grouping"):
                return ADS.retrieve("SouthBank_grouping")
            grp_ws = mantid.LoadDetectorsGroupingFile(InputFile="ENGINX_South_grouping.xml",
                                                      OutputWorkspace="SouthBank_grouping")
            return grp_ws
        except ValueError:
            logger.notice("SouthBank grouping file not found in user directories - creating one")
        bank_name = "SouthBank"
    else:
        raise ValueError("Invalid bank number given")
    ws_name = bank_name + "_grouping"
    grp_ws, _, _ = mantid.CreateGroupingWorkspace(InputWorkspace=sample_raw, GroupNames=bank_name,
                                                  OutputWorkspace=ws_name)
    return grp_ws


def create_grouping_workspace_from_calfile(calfile, sample_raw):  # -> GroupingWorkspace
    ws_name = "Custom_calfile_grouping"
    grp_ws, _, _ = mantid.CreateGroupingWorkspace(InputWorkspace=sample_raw, OldCalFilename=calfile,
                                                  OutputWorkspace=ws_name)
    return grp_ws


def create_grouping_workspace_from_spectra_list(str_list, sample_raw):
    grp_ws, _, _ = mantid.CreateGroupingWorkspace(InputWorkspace=sample_raw, OutputWorkspace="Custom_spectra_grouping")
    int_spectrum_numbers = create_spectrum_list_from_string(str_list)
    for spec in int_spectrum_numbers:
        ws_ind = int(spec - 1)
        det_ids = grp_ws.getDetectorIDs(ws_ind)
        grp_ws.setValue(det_ids[0], 1)
    return grp_ws


def default_ceria_expected_peaks(final=False):
    """
    Get the list of expected Ceria peaks, which can be a good default for the expected peaks
    properties of algorithms like PDCalibration

    @param :: final - if true, returns a list better suited to a secondary fitting
    @Returns :: a list of peaks in d-spacing as a float list
    """
    _CERIA_EXPECTED_PEAKS = [2.705702376, 1.913220892, 1.631600313, 1.352851554, 1.104598643]
    _CERIA_EXPECTED_PEAKS_FINAL = [2.705702376, 1.913220892, 1.631600313, 1.562138267, 1.352851554,
                                   1.241461538, 1.210027059, 1.104598643, 1.04142562, 0.956610446,
                                   0.914694494, 0.901900955, 0.855618487]

    return _CERIA_EXPECTED_PEAKS_FINAL if final else _CERIA_EXPECTED_PEAKS


def get_first_unmasked_specno_from_mask_ws(ws) -> int:
    num_spectra = ws.getNumberHistograms()
    for specno in range(num_spectra):
        detid = ws.getDetectorIDs(specno)[0]
        val = ws.getValue(detid)
        if val == 0:
            return specno
    # if no 0 values, no values have been masked so fits have failed
    # return first specno to avoid error
    return 0


def read_in_expected_peaks(filename, expected_peaks):
    """
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


def write_ENGINX_GSAS_iparam_file(output_file, difa, difc, tzero, bk2bk_params=None, bank_names=None, ceria_run=241391,
                                  template_file=None):
    """
    Produces and writes an ENGIN-X instrument parameter file for GSAS
    (in the GSAS iparam format, as partially described in the GSAS
    manual). It simply uses a template (found in template_path) where
    some values are replaced with the values (difc, tzero) passed to
    this function. DIFA is fixed to 0.

    Possible extensions for the file are .par (used here as default),
    .prm, .ipar, etc.

    @param output_file :: name of the file where to write the output
    @param difa :: list of DIFA values, one per bank, to pass on to GSAS
    @param difc :: list of DIFC values, one per bank, to pass on to GSAS
                   (as produced by EnggCalibrate)
    @param tzero :: list of TZERO values, one per bank, to pass on to GSAS
                    (also from EnggCalibrate)
    @param bk2bk_params :: list of BackToBackExponential parameters from
                        Parameters.xml file, one per bank, to pass on to GSAS
    @param bank_names :: Names of each bank to be added to the file
    @param ceria_run :: number of the ceria (CeO2) run used for this calibration.
                        this number goes in the file and should also be used to
                        name the file
    @param template_file :: file to use as template (with relative or full path)

    @returns

    """
    if not isinstance(difc, list) or not isinstance(tzero, list):
        raise ValueError("The parameters difc and tzero must be lists, with as many elements as "
                         "banks")

    if len(difc) != len(tzero) and len(difc) != len(difa):
        raise ValueError("The lengths of the difa, difc and tzero lists must be the same")

    # Defaults for a "both banks" file
    if not template_file:
        template_file = 'template_ENGINX_241391_North_and_South_banks.prm'
    import os
    template_file = os.path.join(os.path.dirname(__file__), template_file)

    if not bank_names:
        bank_names = ["North", "South"]

    with open(template_file) as tf:
        output_lines = tf.readlines()

    def replace_patterns(line, patterns, replacements):
        """
        If line starts with any of the strings passed in the list 'pattern', return the
        corresponding 'replacement'
        """
        for idx, pat in enumerate(patterns):
            if line[0:len(pat)] == pat:
                return replacements[idx]

        return line

    # need to replace these types of lines/patterns:
    # - instrument constants/parameters (ICONS)
    # - instrument calibration comment with run numbers (CALIB)
    # - .his file name for open genie (INCBM)
    # - BackToBackExponential parameters (PRCF11+12)
    for b_idx, _bank_name in enumerate(bank_names):
        patterns = ["INS  %d ICONS" % (b_idx + 1),  # bank calibration parameters: DIFC, DIFA, TZERO
                    "INS    CALIB",  # calibration run number (Ceria)
                    "INS    INCBM"   # A his file for open genie (with ceria run number in the name)
                    ]
        # the ljust(80) ensures a length of 80 characters for the lines (GSAS rules...)
        replacements = [("INS  {0} ICONS  {1:.2f}    {2:.2f}    {3:.2f}".
                         format(b_idx + 1, difc[b_idx], difa[b_idx], tzero[b_idx])).ljust(80) + '\n',
                        ("INS    CALIB   {0}   ceo2".
                         format(ceria_run)).ljust(80) + '\n',
                        ("INS    INCBM  ob+mon_{0}_North_and_South_banks.his".
                         format(ceria_run)).ljust(80) + '\n']

        if bk2bk_params:  # template params not overwritten if none provided

            patterns += ["INS  {}PRCF11".format(b_idx + 1), "INS  {}PRCF12".format(b_idx + 1)]

            replacements += [("INS  {0}PRCF11   {1:.6E}   {2:.6E}   {3:.6E}   {4:.6E}".
                              format(b_idx + 1,
                                     bk2bk_params[b_idx][0],  # alpha
                                     bk2bk_params[b_idx][1],  # beta_0
                                     bk2bk_params[b_idx][2],  # beta_1
                                     bk2bk_params[b_idx][3]   # sigma_0_sq
                                     )).ljust(80) + '\n',
                             ("INS  {0}PRCF12   {1:.6E}   {2:.6E}   0.000000E+00   0.000000E+00 ".
                              format(b_idx + 1,
                                     bk2bk_params[b_idx][4],  # sigma_1_sq
                                     bk2bk_params[b_idx][5]   # sigma_2_sq
                                     )).ljust(80) + '\n'
                             ]

        output_lines = [replace_patterns(line, patterns, replacements) for line in output_lines]

    with open(output_file, 'w') as of:
        of.writelines(output_lines)
