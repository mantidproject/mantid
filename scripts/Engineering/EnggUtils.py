# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
import mantid.simpleapi as mantid

ENGINX_BANKS = ['', 'North', 'South', 'Both: North, South', '1', '2']

ENGINX_MASK_BIN_MINS = [0, 19930, 39960, 59850, 79930]
ENGINX_MASK_BIN_MAXS = [5300, 20400, 40450, 62000, 82670]


def default_ceria_expected_peaks():
    """
    Get the list of expected Ceria peaks, which can be a good default for the expected peaks
    properties of algorithms like EnggCalibrate and EnggCalibrateFull

    @Returns :: a list of peaks in d-spacing as a float list
    """
    _CERIA_EXPECTED_PEAKS = [3.124277511, 2.705702376, 1.913220892, 1.631600313,
                             1.562138267, 1.352851554, 1.241461538, 1.210027059,
                             1.104598643, 1.04142562, 0.956610446, 0.914694494,
                             0.901900955, 0.855618487, 0.825231622, 0.815800156,
                             0.781069134, 0.757748432, 0.750426918, 0.723129589,
                             0.704504971, 0.676425777, 0.66110842, 0.656229382,
                             0.637740216, 0.624855346, 0.620730846, 0.605013529]

    return _CERIA_EXPECTED_PEAKS


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
                raise ValueError("Could not read any peaks from the file given in 'ExpectedPeaksFromFile: '" +
                                 filename + "', and no expected peaks were given in the property "
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
    indices = [idx for s in segments for idx in range(int(s[0]), int(s[-1])+1)]
    # remove duplicates and sort
    indices = sorted(set(indices))
    max_index = workspace.getNumberHistograms()
    if indices[-1] >= max_index:
        raise ValueError("A workspace index equal or bigger than the number of histograms available in the "
                         "workspace '" + workspace.name() + "' (" + str(workspace.getNumberHistograms()) +
                         ") has been given. Please check the list of indices.")
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
    if not AnalysisDataService.doesExist(group_name):
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
        raise ValueError('Could not find any detector for this bank: ' + bank +
                         '. This looks like an unknown bank')

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
    alg = parent.createChildAlgorithm('CropWorkspace')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('StartWorkspaceIndex', min(indices))
    alg.setProperty('EndWorkspaceIndex', max(indices))
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


def write_ENGINX_GSAS_iparam_file(output_file, difc, tzero, bank_names=None, ceria_run=241391, vanadium_run=236516,
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
    @param difc :: list of DIFC values, one per bank, to pass on to GSAS
                   (as produced by EnggCalibrate)
    @param tzero :: list of TZERO values, one per bank, to pass on to GSAS
                    (also from EnggCalibrate)
    @param ceria_run :: number of the ceria (CeO2) run used for this calibration.
                        this number goes in the file and should also be used to
                        name the file
    @param vanadium_run :: number of the vanadium (VNb) run used for this
                           calibration. This number goes in the file and should
                           also be used to name the file.
    @param template_file :: file to use as template (with relative or full path)

    @returns

    """
    if not isinstance(difc, list) or not isinstance(tzero, list):
        raise ValueError("The parameters difc and tzero must be lists, with as many elements as "
                         "banks")

    if len(difc) != len(tzero):
        raise ValueError("The lengths of the difc and tzero lists must be the same")

    # Defaults for a "both banks" file
    if not template_file:
        template_file = 'template_ENGINX_241391_236516_North_and_South_banks.prm'
    import os
    template_file = os.path.join(os.path.dirname(__file__), template_file)
    if not bank_names:
        bank_names = ["North", "South"]

    with open(template_file) as tf:
        temp_lines = tf.readlines()

    def replace_patterns(line, patterns, replacements):
        """
        If line starts with any of the strings passed in the list 'pattern', return the
        corresponding 'replacement'
        """
        for idx, pat in enumerate(patterns):
            if line[0:len(pat)] == pat:
                return replacements[idx]

        return line

    # need to replace two types of lines/patterns:
    # - instrument constants/parameters (ICONS)
    # - instrument calibration comment with run numbers (CALIB)
    output_lines = []
    for b_idx, _bank_name in enumerate(bank_names):
        patterns = ["INS  %d ICONS" % (b_idx + 1),  # bank calibration parameters: DIFC, DIFA, TZERO
                    "INS    CALIB",  # calibration run numbers (Vanadium and Ceria)
                    "INS    INCBM"   # A his file for open genie (with ceria run number in the name)
                    ]
        difa = 0.0
        # the ljust(80) ensures a length of 80 characters for the lines (GSAS rules...)
        replacements = [("INS  {0} ICONS  {1:.2f}    {2:.2f}    {3:.2f}".
                         format(b_idx + 1, difc[b_idx], difa, tzero[b_idx])).ljust(80) + '\n',
                        ("INS    CALIB   {0}   {1} ceo2".
                         format(ceria_run, vanadium_run)).ljust(80) + '\n',
                        ("INS    INCBM  ob+mon_{0}_North_and_South_banks.his".
                         format(ceria_run)).ljust(80) + '\n']

        output_lines = [replace_patterns(line, patterns, replacements) for line in temp_lines]

    with open(output_file, 'w') as of:
        of.writelines(output_lines)
