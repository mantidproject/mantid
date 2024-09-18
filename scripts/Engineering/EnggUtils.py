# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum
from numpy import array, degrees, isfinite, reshape
from os import path, makedirs
from shutil import copy2

from mantid.api import AnalysisDataService as ADS, AlgorithmManager
from mantid.kernel import IntArrayProperty, UnitConversion, DeltaEModeType, logger, UnitParams
import mantid.simpleapi as mantid  # required to call EnggUtils funcs from algorithms to avoid simpleapi error
from Engineering.common import path_handling

ENGINX_BANKS = ["", "North", "South", "Both: North, South", "1", "2"]  # used in EnggCalibrate, EnggVanadiumCorrections
ENGINX_MASK_BIN_MINS = [0, 19930, 39960, 59850, 79930]  # used in EnggFocus
ENGINX_MASK_BIN_MAXS = [5300, 20400, 40450, 62000, 82670]  # used in EnggFocus
# calibration tab variables
CALIB_DIR = path.join(path.dirname(path.realpath(__file__)), "calib")  # dirof default full instrument cal and .xml
DIFF_CONSTS_TABLE_NAME = "diffractometer_consts_table"
# focus tab variables
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"
CURVES_PREFIX = "engggui_curves_"
VAN_CURVE_REBINNED_NAME = "van_ws_foc_rb"
XUNIT_SUFFIXES = {"d-Spacing": "dSpacing", "Time-of-flight": "TOF"}  # to put in saved focused data filename


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
    TEXTURE20 = "Texture20", [1, 2]
    TEXTURE30 = "Texture30", [1, 2]


def plot_tof_vs_d_from_calibration(diag_ws, ws_foc, dspacing, calibration):
    """
    Plot fitted TOF vs expected d-spacing from diagnostic workspaces output from mantid.PDCalibration
    :param diag_ws: workspace object of group of diagnostic workspaces
    :param ws_foc: workspace object of focused data (post mantid.ApplyDiffCal with calibrated diff_consts)
    :param calibration: CalibrationInfo object used to determine subplot axes titles
    :return:
    """
    # import inside func otherwise causes FindPeaksAutomaticTest to fail
    # as it checks that pyplot isn't in sys.modules
    from matplotlib.pyplot import subplots

    fitparam = ADS.retrieve(diag_ws.name() + "_fitparam").toDict()
    fiterror = ADS.retrieve(diag_ws.name() + "_fiterror").toDict()
    d_table = ADS.retrieve(diag_ws.name() + "_dspacing").toDict()
    dspacing = array(sorted(dspacing))  # PDCal sorts the dspacing list passed
    x0 = array(fitparam["X0"])
    x0_er = array(fiterror["X0"])
    ws_index = array(fitparam["wsindex"])
    nspec = len(set(ws_index))
    si = ws_foc.spectrumInfo()

    ncols_per_fig = 4  # max number of spectra per figure window
    figs = []
    for ispec in range(nspec):
        # extract data from tables
        detid = ws_foc.getSpectrum(ispec).getDetectorIDs()[0]
        irow = d_table["detid"].index(detid)
        valid = [isfinite(d_table[key][irow]) for key in d_table if "@" in key]  # nan if fit unsuccessful
        ipks = ws_index == ispec  # peak centres for this spectrum
        x, y, e = dspacing[valid], x0[ipks][valid], x0_er[ipks][valid]
        # get poly fit
        diff_consts = si.diffractometerConstants(ispec)  # output is a UnitParametersMap
        yfit = array([UnitConversion.run("dSpacing", "TOF", xpt, 0, DeltaEModeType.Elastic, diff_consts) for xpt in x])
        # plot polynomial fit to TOF vs dSpacing
        if ispec + 1 > len(figs) * ncols_per_fig:
            # create new figure
            ncols = ncols_per_fig if not nspec - ispec < ncols_per_fig else nspec % ncols_per_fig
            fig, ax = subplots(2, ncols, sharex=True, sharey="row", subplot_kw={"projection": "mantid"})
            ax = reshape(ax, (-1, 1)) if ax.ndim == 1 else ax  # to ensure is 2D matrix even if ncols==1
            ax[0, 0].set_ylabel("Fitted TOF (\u03BCs)")
            ax[1, 0].set_ylabel("Residuals (\u03BCs)")
            figs.append(fig)
            icol = 0
        # plot TOF vs d
        ax[0, icol].set_title(calibration.get_subplot_title(ispec))
        ax[0, icol].errorbar(x, y, yerr=e, marker="o", markersize=3, capsize=2, ls="", color="b", label="Peak centres")
        ax[0, icol].plot(x, yfit, "-r", label="quadratic fit")
        # plot residuals
        ax[1, icol].errorbar(x, y - yfit, yerr=e, marker="o", markersize=3, capsize=2, ls="", color="b", label="resids")
        ax[1, icol].axhline(color="r", ls="-")
        ax[1, icol].set_xlabel("d-spacing (Ang)")
        icol += 1
    # Format figs and show
    for fig in figs:
        fig.tight_layout()
        fig.show()


def create_spectrum_list_from_string(str_list):
    array = IntArrayProperty("var", str_list).value
    int_list = [int(i) for i in array]  # cast int32 to int
    return int_list


def default_ceria_expected_peaks(final=False):
    """
    Get the list of expected Ceria peaks, which can be a good default for the expected peaks
    properties of algorithms like mantid.PDCalibration
    @param :: final - if true, returns a list better suited to a secondary fitting of focused banks
    @Returns :: a list of peaks in d-spacing as a float list
    """
    _CERIA_EXPECTED_PEAKS = [1.104598643, 1.352851554, 1.631600313, 1.913220892, 2.705702376]
    _CERIA_EXPECTED_PEAKS_FINAL = [
        0.781069,
        0.855618487,
        0.901900955,
        0.914694494,
        0.956610446,
        1.04142562,
        1.104598643,
        1.210027059,
        1.241461538,
        1.352851554,
        1.562138267,
        1.631600313,
        1.913220892,
        2.705702376,
    ]

    return _CERIA_EXPECTED_PEAKS_FINAL if final else _CERIA_EXPECTED_PEAKS


def default_ceria_expected_peak_windows(final=False):
    """
    Get the list of windows over which to fit ceria peaks in calls to mantid.PDCalibration
    @param :: final - if true, returns a list better suited to a secondary fitting of focused banks
    @Returns :: a list of peak windows in d-spacing as a float list
    """
    _CERIA_EXPECTED_WINDOW = [1.06515, 1.15210, 1.30425, 1.41292, 1.59224, 1.68462, 1.84763, 1.98891, 2.64097, 2.77186]
    _CERIA_EXPECTED_WINDOW_FINAL = [
        0.77,
        0.805,
        0.83702,
        0.88041,
        0.88041,
        0.90893,
        0.90893,
        0.93474,
        0.93474,
        0.98908,
        1.01625,
        1.06515,
        1.06515,
        1.15210,
        1.16297,
        1.22817,
        1.22817,
        1.29338,
        1.30425,
        1.41292,
        1.53242,
        1.59224,
        1.59224,
        1.68462,
        1.84763,
        1.98891,
        2.64097,
        2.77186,
    ]

    return _CERIA_EXPECTED_WINDOW_FINAL if final else _CERIA_EXPECTED_WINDOW


# Functions in calibration model


def create_new_calibration(calibration, rb_num, plot_output, save_dir, full_calib):
    """
    Create a new calibration from a ceria run
    :param calibration: CalibrationInfo object
    :param rb_num: RB number (or any string) used to determine save directory
    :param plot_output: create plot of TOF vs dSpacing (with residuals)
    :param save_dir: top level director for saving files
    :param full_calib: full instrument calibration workspace
    :return:
    """
    # load ceria data
    ceria_workspace = path_handling.load_workspace(calibration.get_ceria_path())

    # run mantid.PDCalibration
    focused_ceria, cal_table, diag_ws = run_calibration(ceria_workspace, calibration, full_calib)

    # extract diffractometer constants from calibration and write to table
    make_diff_consts_table(focused_ceria)

    if plot_output:
        plot_tof_vs_d_from_calibration(diag_ws, focused_ceria, default_ceria_expected_peaks(final=True), calibration)

    # save output
    calib_dirs = [path.join(save_dir, "Calibration", "")]
    if rb_num:
        calib_dirs.append(path.join(save_dir, "User", rb_num, "Calibration", ""))
        if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            calib_dirs.pop(0)  # only save to RB directory to limit number files saved

    for calib_dir in calib_dirs:
        prm_filepath = create_output_files(calib_dir, calibration, focused_ceria)

    mantid.DeleteWorkspace(ceria_workspace)

    return prm_filepath  # only from last calib_dir


def write_prm_file(ws_foc, prm_savepath, spec_nums=None):
    """
    Save GSAS prm file for ENGINX data - for specification see manual
    https://subversion.xray.aps.anl.gov/EXPGUI/gsas/all/GSAS%20Manual.pdf
    :param ws_foc: focused workspace (used to get detector positions and diff constants)
    :param prm_savepath: path to save prm to
    :param spec_index: list of indices to save (e.g. can specify a particular bank)
    """
    if not spec_nums:
        spec_nums = range(ws_foc.getNumberHistograms())  # one block per spectrum in ws
    nspec = len(spec_nums)
    # read header
    with open(path.join(CALIB_DIR, "template_ENGINX_prm_header.prm")) as fheader:
        lines = fheader.readlines()
    lines[1] = lines[1].replace("2", f"{nspec}")  # replace with nspectra in header
    lines[13] = lines[13].replace("241391", f'{ws_foc.run().get("run_number").value}')  # replace run num
    # add blocks
    si = ws_foc.spectrumInfo()
    inst = ws_foc.getInstrument()
    endl = lines[0][-1]  # new line char
    for iblock, ispec in enumerate(spec_nums):
        # detector parameters
        l2 = si.l2(ispec)
        phi, tth = degrees(si.geographicalAngles(ispec))
        dc = si.diffractometerConstants(ispec)
        difa, difc, tzero = [dc[param] for param in [UnitParams.difa, UnitParams.difc, UnitParams.tzero]]
        block = [f"INS  {iblock + 1}I ITYP\t0\t1.0000\t80.0000\t0{endl}"]
        block.extend(f"INS  {iblock + 1}BNKPAR\t{l2:.3f}\t{abs(tth):.3f}\t{phi:.3f}\t0.000\t0.000\t0\t0{endl}")
        block.extend(f"INS  {iblock + 1} ICONS\t{difc:.2f}\t{difa:.2f}\t{tzero:.2f}{endl}")
        # TOF peak profile parameters
        alpha0, beta0, beta1, sig0_sq, sig1_sq, sig2_sq = getParametersFromDetector(inst, ws_foc.getDetector(ispec))
        block.extend(f"INS  {iblock + 1}PRCF1 \t3\t21\t0.00050{endl}")
        block.extend(f"INS  {iblock + 1}PRCF11\t{alpha0:.6E}\t{beta0:.6E}\t{beta1:.6E}\t{sig0_sq:.6E}{endl}")
        block.extend(f"INS  {iblock + 1}PRCF12\t{sig1_sq:.6E}\t{sig2_sq:.6E}\t{0.0:.6E}\t{0.0:.6E}{endl}")
        for iblank in [3, 4, 5]:
            block.extend(f"INS  {iblock + 1}PRCF1{iblank}\t{0.0:.6E}\t{0.0:.6E}\t{0.0:.6E}\t{0.0:.6E}{endl}")
        block.extend(f"INS  {iblock + 1}PRCF16\t{0.0:.6E}{endl}")
        # append to lines
        lines.extend(block)
    # write lines to prm file
    with open(prm_savepath, "w") as fout:
        fout.writelines(lines)


def load_existing_calibration_files(calibration):
    # load prm
    prm_filepath = calibration.prm_filepath
    if not path.exists(prm_filepath):
        msg = f"Could not open GSAS calibration file: {prm_filepath}"
        logger.warning(msg)
        return None
    try:
        # read diff constants from prm
        write_diff_consts_to_table_from_prm(prm_filepath)
    except RuntimeError:
        logger.error(f"Invalid file selected: {prm_filepath}")
        return None
    calibration.load_relevant_calibration_files()
    return prm_filepath


def write_diff_consts_to_table_from_prm(prm_filepath):
    """
    read diff consntants from prm file and write in table workspace
    :param prm_filepath: path to prm file
    """
    diff_consts = read_diff_constants_from_prm(prm_filepath)
    # make table
    table = mantid.CreateEmptyTableWorkspace(OutputWorkspace=DIFF_CONSTS_TABLE_NAME)
    table.addColumn("int", "Index")
    table.addColumn("double", "DIFA")
    table.addColumn("double", "DIFC")
    table.addColumn("double", "TZERO")
    # add to row per spectrum to table
    for ispec in range(len(diff_consts)):
        table.addRow([ispec, *diff_consts[ispec, :]])


def make_diff_consts_table(ws_foc):
    """
    Summarise diff constants in table workspace (adapt from detector table)
    :param ws_foc: focused ceria workspace
    """
    table = mantid.CreateDetectorTable(InputWorkspace=ws_foc, DetectorTableWorkspace=DIFF_CONSTS_TABLE_NAME)
    col_names = ["Spectrum No", "Detector ID(s)", "Monitor", "DIFC - Uncalibrated"]
    for col in col_names:
        table.removeColumn(col)


def run_calibration(ceria_ws, calibration, full_instrument_cal_ws):
    """
    Creates Engineering calibration files with PDCalibration
    :param ceria_ws: The workspace with the ceria data.
    :param bank: The bank to crop to, both if none.
    :param calfile: The custom calibration file to crop to, not used if none.
    :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
    :return: dict containing calibrated diffractometer constants, and copy of the raw ceria workspace
    """

    # initial process of ceria ws
    mantid.NormaliseByCurrent(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws)
    mantid.ApplyDiffCal(InstrumentWorkspace=ceria_ws, CalibrationWorkspace=full_instrument_cal_ws)
    mantid.ConvertUnits(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws, Target="dSpacing")

    # get grouping workspace and focus
    grp_ws = calibration.get_group_ws()  # (creates if doesn't exist)
    focused_ceria = mantid.DiffractionFocussing(InputWorkspace=ceria_ws, GroupingWorkspace=grp_ws)
    mantid.ApplyDiffCal(InstrumentWorkspace=focused_ceria, ClearCalibration=True)  # DIFC of detector in middle of bank
    focused_ceria = mantid.ConvertUnits(InputWorkspace=focused_ceria, Target="TOF")

    # Run mantid.PDCalibration to fit peaks in TOF
    foc_name = focused_ceria.name()  # PDCal invalidates ptr during rebin so keep track of ws name
    cal_table_name = "engggui_calibration_" + calibration.get_group_suffix()
    diag_ws_name = "diag_" + calibration.get_group_suffix()
    cal_table, mask, diag_ws = mantid.PDCalibration(
        InputWorkspace=foc_name,
        OutputCalibrationTable=cal_table_name,
        MaskWorkspace=cal_table_name + "_mask",
        DiagnosticWorkspaces=diag_ws_name,
        PeakPositions=default_ceria_expected_peaks(final=True),
        TofBinning=[12000, -0.0003, 52000],
        PeakWindow=default_ceria_expected_peak_windows(final=True),
        MinimumPeakHeight=0.5,
        PeakFunction="BackToBackExponential",
        CalibrationParameters="DIFC+TZERO+DIFA",
        UseChiSq=True,
    )
    mantid.ApplyDiffCal(InstrumentWorkspace=foc_name, CalibrationWorkspace=cal_table)

    # warn user which spectra were unsuccessfully calibrated
    focused_ceria = ADS.retrieve(foc_name)
    masked_detIDs = mask.getMaskedDetectors()
    for ispec in range(focused_ceria.getNumberHistograms()):
        if focused_ceria.getSpectrum(ispec).getDetectorIDs()[0] in masked_detIDs:
            logger.warning(f"mantid.PDCalibration failed for spectrum index {ispec} - proceeding with uncalibrated DIFC.")

    # store cal_table in calibration
    calibration.set_calibration_table(cal_table_name)

    return focused_ceria, cal_table, diag_ws


def create_output_files(calibration_dir, calibration, ws_foc):
    """
    Create output files (.prm for GSAS and .nxs of calibration table) from the algorithms in the specified directory
    :param calibration_dir: The directory to save the files into.
    :param calibration: CalibrationInfo object with details of calibration and grouping
    :param ws_foc: focused ceria workspace
    """
    # create calibration dir of not exist
    if not path.exists(calibration_dir):
        makedirs(calibration_dir)

    # save grouping ws if custom or cropped
    if not calibration.group.banks:
        calibration.save_grouping_workspace(calibration_dir)

    # save prm file(s)
    prm_filepath = path.join(calibration_dir, calibration.generate_output_file_name())
    write_prm_file(ws_foc, prm_filepath)
    calibration.set_prm_filepath(prm_filepath)
    # save pdcal output as nexus
    filepath, ext = path.splitext(prm_filepath)
    nxs_filepath = filepath + ".nxs"
    mantid.SaveNexus(InputWorkspace=calibration.get_calibration_table(), Filename=nxs_filepath)

    # if both banks calibrated save individual banks separately as well
    if calibration.group == GROUP.BOTH:
        # output a separate prm for North and South when both banks included
        for ibank, bank in enumerate(calibration.group.banks):
            # get prm filename for individual banks by passing group enum as argument to generate_output_file_name
            prm_filepath_bank = path.join(calibration_dir, calibration.generate_output_file_name(GROUP(str(ibank + 1))))
            write_prm_file(ws_foc, prm_filepath_bank, spec_nums=[ibank])
            # copy pdcal output nxs for both banks
            filepath, ext = path.splitext(prm_filepath_bank)
            nxs_filepath_bank = filepath + ".nxs"
            copy2(nxs_filepath, nxs_filepath_bank)
    logger.notice(f'\n\nCalibration files saved to: "{calibration_dir}"\n\n')

    return prm_filepath  # if both banks, do not pass individual banks (prm_filepath_bank) to GSAS II tab


def getParametersFromDetector(instrument, detector):
    """
    Get BackToBackExponential parameters from highest level component in tree
    :param instrument:
    :param detector:
    :return: list of parameters
    """
    inst_tree = detector.getFullName().split(detector.getNameSeparator())[0].split("/")
    param_names = ["alpha_0", "beta_0", "beta_1", "sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]
    params = None
    for comp_name in inst_tree:
        comp = instrument.getComponentByName(comp_name)
        if comp.hasParameter(param_names[0]):
            params = [comp.getNumberParameter(param)[0] for param in param_names]
            break
    return params


def read_diff_constants_from_prm(file_path):
    """
    :param file_path: path to prm file
    :return: (nspec x 3) array with columns difa difc tzero (in that order - same as in detector table).
    """
    diff_consts = []  # one list per component (e.g. bank)
    with open(file_path) as f:
        for line in f.readlines():
            if "ICONS" in line:
                # If formatted correctly the line should be in the format INS bank ICONS difc difa tzero
                elements = line.split()
                diff_consts.append([float(elements[ii]) for ii in [4, 3, 5]])
    if not diff_consts:
        raise RuntimeError("Invalid file format.")
    return array(diff_consts)


def _generate_table_workspace_name(bank_num):
    return "engggui_calibration_bank_" + str(bank_num)


# Focus model functions


def focus_run(sample_paths, vanadium_path, plot_output, rb_num, calibration, save_dir, full_calib):
    """
    Focus some data using the current calibration.
    :param sample_paths: The paths to the data to be focused.
    :param vanadium_path: Path to the vanadium file from the current calibration
    :param plot_output: True if the output should be plotted.
    :param rb_num: Number to signify the user who is running this focus
    :param calibration: CalibrationInfo object that holds all info needed about ROI and instrument
    :param save_dir: top level directory in which to save output
    :param full_calib: full instrument calibration workspace
    :return focused_files_list: list of paths to focused nxs file
    """
    # check correct region calibration(s) and grouping workspace(s) exists
    if not calibration.is_valid():
        return

    # check if full instrument calibration exists, if not load it
    # load, focus and process vanadium (retrieve from ADS if exists)
    ws_van_foc, van_run = process_vanadium(vanadium_path, calibration, full_calib)

    # directories for saved focused data
    focus_dirs = [path.join(save_dir, "Focus")]
    if rb_num:
        focus_dirs.append(path.join(save_dir, "User", rb_num, "Focus"))
        if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            focus_dirs.pop(0)  # only save to RB directory to limit number files saved

    # Loop over runs and focus
    focused_files_list = []
    focused_files_gsas2_list = []
    output_workspaces = []  # List of focused workspaces to plot.
    for sample_path in sample_paths:
        ws_sample = _load_run_and_convert_to_dSpacing(sample_path, calibration.get_instrument(), full_calib)
        if ws_sample:
            # None returned if no proton charge
            ws_foc = _focus_run_and_apply_roi_calibration(ws_sample, calibration)
            ws_foc = _apply_vanadium_norm(ws_foc, ws_van_foc)
            _save_output_files(focus_dirs, ws_foc, calibration, van_run, rb_num)
            # convert units to TOF and save again
            ws_foc = mantid.ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target="TOF")
            nxs_paths, gss_path = _save_output_files(focus_dirs, ws_foc, calibration, van_run, rb_num)
            focused_files_list.extend(nxs_paths)  # list of .nsx paths for that sample using last dir in focus_dirs
            print(type(gss_path), len(gss_path))
            focused_files_gsas2_list.extend(gss_path)
            print(type(focused_files_gsas2_list), len(focused_files_gsas2_list))
            output_workspaces.append(ws_foc.name())

    # Plot the output
    if output_workspaces:
        mantid.DeleteWorkspace(VAN_CURVE_REBINNED_NAME)
        if plot_output:
            _plot_focused_workspaces(output_workspaces)

    return focused_files_list, focused_files_gsas2_list


def process_vanadium(vanadium_path, calibration, full_calib):
    van_run = path_handling.get_run_number_from_path(vanadium_path, calibration.get_instrument())
    van_foc_name = CURVES_PREFIX + calibration.get_group_suffix()
    if ADS.doesExist(van_foc_name):
        ws_van_foc = ADS.retrieve(van_foc_name)
    else:
        if ADS.doesExist(van_run):
            ws_van = ADS.retrieve(van_run)  # will exist if have only changed the ROI
        else:
            ws_van = _load_run_and_convert_to_dSpacing(vanadium_path, calibration.get_instrument(), full_calib)
            if not ws_van:
                raise RuntimeError(f"vanadium run {van_run} has no proton_charge - " f"please supply a valid vanadium run to focus.")
        ws_van_foc = _focus_run_and_apply_roi_calibration(ws_van, calibration, ws_foc_name=van_foc_name)
        ws_van_foc = _smooth_vanadium(ws_van_foc)
    return ws_van_foc, van_run


def _plot_focused_workspaces(ws_names):
    # import inside func otherwise causes FindPeaksAutomaticTest to fail
    # as it checks that pyplot isn't in sys.modules
    from matplotlib.pyplot import subplots

    for ws_name in ws_names:
        ws_foc = ADS.retrieve(ws_name)
        ws_label = "_".join([ws_foc.getInstrument().getName(), ws_foc.run().get("run_number").value])
        fig, ax = subplots(subplot_kw={"projection": "mantid"})
        for ispec in range(ws_foc.getNumberHistograms()):
            ax.plot(ws_foc, label=f"{ws_label} focused: spec {ispec + 1}", marker=".", wkspIndex=ispec)
        ax.legend()
        fig.show()


def _load_run_and_convert_to_dSpacing(filepath, instrument, full_calib):
    runno = path_handling.get_run_number_from_path(filepath, instrument)
    ws = mantid.Load(Filename=filepath, OutputWorkspace=str(runno))
    if ws.getRun().getProtonCharge() > 0:
        ws = mantid.NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws.name())
    else:
        logger.warning(f"Run {ws.name()} has invalid proton charge.")
        mantid.DeleteWorkspace(ws)
        return None
    mantid.ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib)
    ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target="dSpacing")
    return ws


def _focus_run_and_apply_roi_calibration(ws, calibration, ws_foc_name=None):
    if not ws_foc_name:
        ws_foc_name = ws.name() + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + calibration.get_foc_ws_suffix()
    ws_foc = mantid.DiffractionFocussing(InputWorkspace=ws, OutputWorkspace=ws_foc_name, GroupingWorkspace=calibration.get_group_ws())
    mantid.ApplyDiffCal(InstrumentWorkspace=ws_foc, ClearCalibration=True)
    ws_foc = mantid.ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target="TOF")
    mantid.ApplyDiffCal(InstrumentWorkspace=ws_foc, CalibrationWorkspace=calibration.get_calibration_table())
    ws_foc = mantid.ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target="dSpacing")
    return ws_foc


def _smooth_vanadium(van_ws_foc):
    return mantid.EnggEstimateFocussedBackground(InputWorkspace=van_ws_foc, OutputWorkspace=van_ws_foc, NIterations=1, XWindow=0.08)


def _apply_vanadium_norm(sample_ws_foc, van_ws_foc):
    # divide by curves - automatically corrects for solid angle, det efficiency and lambda dep. flux
    sample_ws_foc = mantid.CropWorkspace(InputWorkspace=sample_ws_foc, OutputWorkspace=sample_ws_foc.name(), XMin=0.45)
    van_ws_foc_rb = mantid.RebinToWorkspace(
        WorkspaceToRebin=van_ws_foc, WorkspaceToMatch=sample_ws_foc, OutputWorkspace=VAN_CURVE_REBINNED_NAME
    )  # copy so as not to lose data
    sample_ws_foc = mantid.Divide(
        LHSWorkspace=sample_ws_foc, RHSWorkspace=van_ws_foc_rb, OutputWorkspace=sample_ws_foc.name(), AllowDifferentNumberSpectra=False
    )
    sample_ws_foc = mantid.ReplaceSpecialValues(
        InputWorkspace=sample_ws_foc, OutputWorkspace=sample_ws_foc.name(), NaNValue=0, NaNError=0.0, InfinityValue=0, InfinityError=0.0
    )
    return sample_ws_foc


def _save_output_files(focus_dirs, sample_ws_foc, calibration, van_run, rb_num=None):
    # set bankid for use in fit tab
    foc_suffix = calibration.get_foc_ws_suffix()
    xunit = sample_ws_foc.getDimension(0).name
    xunit_suffix = XUNIT_SUFFIXES[xunit]
    sample_run_no = sample_ws_foc.run().get("run_number").value
    # save all spectra to single ASCII files
    ascii_fname = _generate_output_file_name(
        calibration.get_instrument(), sample_run_no, van_run, calibration.get_group_suffix(), xunit_suffix, ext=""
    )

    for focus_dir in focus_dirs:
        if not path.exists(focus_dir):
            makedirs(focus_dir)

        gss_path = path.join(focus_dir, ascii_fname + ".gss")
        gss_paths = [gss_path]
        mantid.SaveGSS(InputWorkspace=sample_ws_foc, Filename=gss_path, SplitFiles=False, UseSpectrumNumberAsBankID=True, Append=False)
        mantid.SaveFocusedXYE(
            InputWorkspace=sample_ws_foc, Filename=path.join(focus_dir, ascii_fname + ".abc"), SplitFiles=False, Format="TOPAS"
        )
        # Save nxs per spectrum
        nxs_paths = []
        mantid.AddSampleLog(Workspace=sample_ws_foc, LogName="Vanadium Run", LogText=van_run)
        for ispec in range(sample_ws_foc.getNumberHistograms()):
            # add a bankid and vanadium to log that is read by fitting model
            bankid = foc_suffix if sample_ws_foc.getNumberHistograms() == 1 else f"{foc_suffix}_{ispec + 1}"
            mantid.AddSampleLog(Workspace=sample_ws_foc, LogName="bankid", LogText=bankid.replace("_", " "))  # overwrites
            # save spectrum as nexus
            filename = _generate_output_file_name(calibration.get_instrument(), sample_run_no, van_run, bankid, xunit_suffix, ext=".nxs")
            nxs_path = path.join(focus_dir, filename)
            mantid.SaveNexus(InputWorkspace=sample_ws_foc, Filename=nxs_path, WorkspaceIndexList=[ispec])
            nxs_paths.append(nxs_path)
    return nxs_paths, gss_paths  # from last focus_dir only


def _generate_output_file_name(inst, sample_run_no, van_run_no, suffix, xunit, ext=""):
    return "_".join([inst, sample_run_no, van_run_no, suffix, xunit]) + ext


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
                    read_in_array.append([float(x) for x in line.split(",")])
            for a in read_in_array:
                for b in a:
                    ex_peak_array.append(b)
        except RuntimeError as exc:
            raise RuntimeError("Error while reading file of expected peaks '%s': %s" % (filename, exc))

        if not ex_peak_array:
            # "File could not be read. Defaults in alternative option used."
            if not expected_peaks:
                raise ValueError(
                    "Could not read any peaks from the file given in 'ExpectedPeaksFromFile: '"
                    + filename
                    + "', and no expected peaks were given in the property "
                    "'ExpectedPeaks' either. Cannot continue without a list of expected peaks."
                )
            expected_peaks_d = sorted(expected_peaks)

        else:
            expected_peaks_d = sorted(ex_peak_array)

    else:
        if 0 == len(expected_peaks):
            raise ValueError(
                "No expected peaks were given in the property 'ExpectedPeaks', "
                "could not get default expected peaks, and 'ExpectedPeaksFromFile' "
                "was not given either. Cannot continout without a list of expected peaks."
            )
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

    @returns list of workspace indices that can be used in mantid algorithms such as mantid.CropWorkspace.
    """

    if bank and detector_indices:
        raise ValueError(
            "It is not possible to use at the same time the input properties 'Bank' and "
            "'DetectorIndices', as they overlap. Please use either of them. Got Bank: '%s', "
            "and DetectorIndices: '%s'" % (bank, detector_indices)
        )
    elif bank:
        bank_aliases = {"North": "1", "South": "2", "Both: North, South": "-1"}
        bank = bank_aliases.get(bank, bank)
        indices = get_ws_indices_for_bank(workspace, bank)
        if not indices:
            raise RuntimeError(
                "Unable to find a meaningful list of workspace indices for the " "bank passed: %s. Please check the inputs." % bank
            )
        return indices
    elif detector_indices:
        indices = parse_spectrum_indices(workspace, detector_indices)
        if not indices:
            raise RuntimeError(
                "Unable to find a meaningful list of workspace indices for the "
                "range(s) of detectors passed: %s. Please check the inputs." % detector_indices
            )
        return indices
    else:
        raise ValueError("You have not given any value for the properties 'Bank' and 'DetectorIndices' " "One of them is required")


def parse_spectrum_indices(workspace, spectrum_numbers):
    """
    DEPRECATED: not used in UI, only in get_ws_indices_from_input_properties which is used in
    deprecated functions (EnggCalibrateFull, EnggCalibrate and EnggFitPeaks)

    Get a usable list of workspace indices from a user provided list of spectra that can look like:
    '8-10, 20-40, 100-110'. For that example this method will produce: [7,8,9,19, 20,... , 109]

    @param workspace :: input workspace (with instrument)
    @param spectrum_numbers :: range of spectrum numbers (or list of ranges) as given to an algorithm

    @return list of workspace indices, ready to be used in mantid algorithms such as mantid.CropWorkspace
    """
    segments = [s.split("-") for s in spectrum_numbers.split(",")]
    indices = [idx for s in segments for idx in range(int(s[0]), int(s[-1]) + 1)]
    # remove duplicates and sort
    indices = sorted(set(indices))
    max_index = workspace.getNumberHistograms()
    if indices[-1] >= max_index:
        raise ValueError(
            "A workspace index equal or bigger than the number of histograms available in the "
            "workspace '"
            + workspace.name()
            + "' ("
            + str(workspace.getNumberHistograms())
            + ") has been given. Please check the list of indices."
        )
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

    grouping_file_path = os.path.join(mantid.config.getInstrumentDirectory(), "Grouping", "ENGINX_Grouping.xml")

    alg = AlgorithmManager.create("LoadDetectorsGroupingFile")
    alg.initialize()
    alg.setLogging(False)
    alg.setProperty("InputFile", grouping_file_path)
    group_name = "__EnginXGrouping"
    alg.setProperty("OutputWorkspace", group_name)
    alg.execute()

    # LoadDetectorsGroupingFile produces a 'Grouping' workspace.
    # PropertyWithValue<GroupingWorkspace> not working (GitHub issue 13437)
    # => cannot run as child and get outputworkspace property properly
    if not ADS.doesExist(group_name):
        raise RuntimeError("LoadDetectorsGroupingFile did not run correctly. Could not " "find its output workspace: " + group_name)
    grouping = ADS.retrieve(group_name)

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
        raise ValueError("Could not find any detector for this bank: " + bank + ". This looks like an unknown bank")

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
    tbl.addColumn("double", "DIFA")
    tbl.addColumn("double", "DIFZ")
    tbl.addColumn("double", "TZERO")
    tbl.addRow([float(difa), float(difc), float(tzero)])


def apply_vanadium_corrections(parent, ws, indices, vanadium_ws, van_integration_ws, van_curves_ws, progress_range=None):
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
        raise ValueError(
            "Inconsistency in inputs: the Vanadium workspace has less spectra (%d) than "
            "the number of workspace indices to process (%d)" % (vanadium_ws.getNumberHistograms(), len(indices))
        )
    elif van_integration_ws and van_curves_ws:
        # filter only indices from vanIntegWS (crop the table)
        tbl = mantid.CreateEmptyTableWorkspace(OutputWorkspace="__vanadium_integration_ws")
        tbl.addColumn("double", "Spectra Integration")
        for i in indices:
            tbl.addRow([van_integration_ws.cell(i, 0)])
        van_integration_ws = tbl

    # These corrections rely on ToF<->Dspacing conversions, so they're done after the calibration step
    progress_params = dict()
    if progress_range:
        progress_params["startProgress"] = progress_range[0]
        progress_params["endProgress"] = progress_range[1]

    alg = parent.createChildAlgorithm("EnggVanadiumCorrections", **progress_params)
    if ws:
        alg.setProperty("Workspace", ws)
    if vanadium_ws:
        alg.setProperty("VanadiumWorkspace", vanadium_ws)
    if van_integration_ws:
        alg.setProperty("IntegrationWorkspace", van_integration_ws)
    if van_curves_ws:
        alg.setProperty("CurvesWorkspace", van_curves_ws)

    alg.execute()


def convert_to_d_spacing(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Converts a workspace to dSpacing using 'mantid.ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in ToF units) to convert (not modified)

    @returns workspace converted to d-spacing units
    """
    # A check to catch possible errors in an understandable way
    expected_dimension = "Time-of-flight"
    dimension = ws.getXDimension().name
    if expected_dimension != dimension:
        raise ValueError(
            "This function expects a workspace with %s X dimension, but "
            "the X dimension of the input workspace is: '%s'. This is an internal logic "
            "error. " % (expected_dimension, dimension)
        )

    alg = parent.createChildAlgorithm("ConvertUnits")
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("Target", "dSpacing")
    alg.setProperty("AlignBins", True)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def convert_to_TOF(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Converts workspace to Time-of-Flight using 'ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in d-spacing units) to convert to ToF

    @returns workspace with data converted to ToF units
    """
    alg = parent.createChildAlgorithm("ConvertUnits")
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("Target", "TOF")
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


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
    alg = parent.createChildAlgorithm("ExtractSpectra")
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("WorkspaceIndexList", indices)
    alg.execute()

    return alg.getProperty("OutputWorkspace").value


def sum_spectra(parent, ws):
    """
    DEPRECATED: not used in UI, only in deprecated functions (EnggVanadiumCorrections and EnggFocus)

    Focuses/sums up all the spectra into a single one (calls the SumSpectra algorithm)

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to sum up

    @return single-spectrum workspace resulting from the sum
    """
    alg = parent.createChildAlgorithm("SumSpectra")
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("RemoveSpecialValues", True)
    alg.execute()

    return alg.getProperty("OutputWorkspace").value
