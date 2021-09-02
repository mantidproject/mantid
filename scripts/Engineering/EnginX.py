# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import csv
import matplotlib.pyplot as plt
import numpy as np
import os
from platform import system
from shutil import copy2
import mantid.plots  # noqa
import Engineering.EnggUtils as Utils
import mantid.simpleapi as simple

NORTH_BANK_CAL = "EnginX_NorthBank.cal"
SOUTH_BANK_CAL = "EnginX_SouthBank.cal"


def main(vanadium_run, user, focus_run, **kwargs):
    """
    This is the main method to be called by an external user wishing to use this library. Based on user input, this
    can run the whole workflow of vanadium correction calculations, calibration and focussing of ENGINX data, or make
    use of preexisting workspaces to perform only a part of the workflow. See keyword argument descriptions below for
    information on configuring the flow of execution of these scripts.

    @param vanadium_run :: the run number of the vanadium to be used
    @param user :: the username to save under
    @param focus_run :: the run number to focus
    Kwargs:
        ceria_run (string): the run number of the ceria to use
        force_vanadium (bool): forces creation of vanadium even if one for the run already exists
        force_cal (bool) : forces creation of calibration files
        full_inst_calib_path (string) : path to the full instrument calibration file used in calibration and focussing
        crop_type (string): what method of cropping to use
        crop_name (string): what to call the cropped bank workspace
        crop_on (string): the bank of spectrum to crop on if cropping
        pre_process_run (bool): set whether or not to pre-process run before focusing
        params (string): rebin parameters for pre-process
        time_period (string): time period for pre-process
        grouping_file (string): the path of the grouping file for texture focusing
        directory (string): the path of the directory to save to
        user_struct (bool): whether or not to use the enginx file structure (defaults to true)
    """
    # Set all values at top,
    ceria_run = kwargs.get("ceria_run", "241391")

    # force parameters
    force_van = kwargs.get("force_vanadium", False)
    force_cal = kwargs.get("force_cal", False)

    # full instrument_calibration
    try:
        full_inst_calib = simple.Load(Filename=kwargs.get("full_inst_calib_path", None),
                                      OutputWorkspace="full_inst_calib")
    except ValueError as e:
        print("Failed to load full_instrument_calibration, please provide a valid path. Error: " + str(e))
        return

    # cropping parameters
    cropped = kwargs.get("crop_type", None)
    crop_name = kwargs.get("crop_name", "cropped")
    crop_on = kwargs.get("crop_on", None)
    if crop_on is None and cropped is not None:
        print("You must enter a value to crop_on if you enter a cropping method")
        return

    # pre-processing parameters
    do_pre_process = kwargs.get("pre_process_run", False)
    params = kwargs.get("params", None)
    time_period = kwargs.get("time_period", None)

    # focus parameters
    grouping_file = kwargs.get("grouping_file", None)

    # save directory
    if system() == "Windows":
        path = os.path.join("/", "EnginX_Mantid")
    else:
        path = os.path.expanduser("~/EnginX_Mantid")
    directory = kwargs.get("directory", path)

    # path setup
    create_user_dir = kwargs.get("user_struct", True)
    calibration_general = os.path.join(directory, "Calibration")
    focus_general = os.path.join(directory, "Focus")
    if create_user_dir:
        user_dir = os.path.join(directory, "User", user)
        calibration_directory = os.path.join(user_dir, "Calibration")
        focus_directory = os.path.join(user_dir, "Focus")
    else:
        calibration_directory = calibration_general
        focus_directory = focus_general

    # call methods with set parameters
    run(ceria_run, force_cal, force_van, full_inst_calib, vanadium_run, calibration_directory, calibration_general, cropped,
        crop_name, crop_on, focus_directory, focus_general, do_pre_process, params, time_period, focus_run,
        grouping_file)


def run(ceria_run, do_cal, do_van, full_inst_calib, van_run, calibration_directory, calibration_general, cropped,
        crop_name, crop_on, focus_directory, focus_general, do_pre_process, params, time_period, focus_run,
        grouping_file):
    """
    calls methods needed based off of inputs

    @param ceria_run :: the run number of the ceria to use
    @param do_cal :: whether or not to force running calibration
    @param do_van :: whether or not to force calculating the vanadium
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param van_run :: run number to use for the vanadium
    @param calibration_directory :: the users calibration directory
    @param calibration_general :: the non-user specific calibration directory
    @param crop_on :: where to crop using the cropping method
    @param crop_name :: how to name cropped banks
    @param cropped :: the cropping method to use
    @param focus_directory :: the users focus directory
    @param focus_general :: the non-user specific focus directory
    @param do_pre_process:: whether or not to pre-process before focussing
    @param params :: list of parameters to use for rebinning
    @param time_period :: time period to old binning
    @param focus_run :: run number to focus
    @param grouping_file :: grouping file to use with texture mode


    """

    # check whether creating a vanadium is required or requested
    if (not os.path.isfile(_get_van_names(van_run, calibration_directory)[0])) or do_van:
        create_vanadium_integration(van_run, calibration_directory)

    # find the file names of calibration files that would be created by this run
    cal_endings = {"banks": ["all_banks", f"bank_{crop_on}"],
                   "spectra": ["all_banks", f"bank_{crop_name}"],
                   None: ["all_banks", "bank_North", "bank_South"]}
    expected_cals = [f"ENGINX_{van_run}_{ceria_run}_{ending}.prm" for ending in cal_endings.get(cropped)]
    expected_cals_present = [os.path.isfile(os.path.join(calibration_directory, cal_file)) for cal_file in
                             expected_cals]
    pdcal_table_endings = {"banks": [f"bank_{crop_on}"],
                           "spectra": ["cropped"],
                           None: ["bank_North", "bank_South"]}
    expected_pdcal_tables = [f"ENGINX_{van_run}_{ceria_run}_{ending}.nxs" for ending in
                             pdcal_table_endings.get(cropped)]
    expected_tables_present = [os.path.isfile(os.path.join(calibration_directory, cal_file)) for cal_file in
                               expected_pdcal_tables]

    # if the calibration files that this run would create are not present, or the user has requested it, create the
    # calibration files
    if not all(expected_cals_present) or not all(expected_tables_present) or do_cal:
        create_calibration(ceria_run, van_run, full_inst_calib, calibration_directory, calibration_general, cropped,
                           crop_name, crop_on)
    else:
        ending_to_load = {"banks": f"bank_{crop_on}",
                          "spectra": crop_name,
                          None: "all_banks"}
        file_name = os.path.join(calibration_directory,
                                 f"ENGINX_{van_run}_{ceria_run}_{ending_to_load.get(cropped)}.prm")
        Utils.load_relevant_calibration_files(file_name, "engg")

    # if a focus is requested, run the focus
    if focus_run is not None:
        focus(focus_run, van_run, full_inst_calib, calibration_directory, focus_directory, focus_general,
              do_pre_process, params, time_period, grouping_file, cropped, crop_on)


def create_vanadium_integration(van_run, calibration_directory):
    """
    create the vanadium integration for the run number set of the object

    @param van_run :: the run number for the vanadium
    @param calibration_directory :: the directory to save the output to
    """
    # find and load the vanadium
    van_file = _gen_filename(van_run)
    _, van_int_file = _get_van_names(van_run, calibration_directory)
    van_name = "eng_vanadium_ws"
    van_ws = simple.Load(van_file, OutputWorkspace=van_name)

    # make the integration workspace
    simple.NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=van_ws)
    # sensitivity correction for van
    ws_van_int = simple.Integration(InputWorkspace=van_ws)
    ws_van_int /= van_ws.blocksize()
    # save out the vanadium and delete the original workspace
    simple.SaveNexus(ws_van_int, van_int_file)
    simple.DeleteWorkspace(van_name)
    simple.DeleteWorkspace(ws_van_int)


def handle_van_curves(van_curves, van_path):
    if len(van_curves) == 2:
        curves_ws = simple.AppendSpectra(InputWorkspace1=van_curves[0], InputWorkspace2=van_curves[1])
        simple.DeleteWorkspace(van_curves[1])
    else:
        curves_ws = van_curves[0]
    simple.SaveNexus(curves_ws, van_path)
    simple.DeleteWorkspace(van_curves[0])
    simple.DeleteWorkspace(curves_ws)


def create_calibration(ceria_run, van_run, full_inst_calib, calibration_directory, calibration_general, cropped,
                       crop_name, crop_on):
    """
    create the calibration files

    @param ceria_run :: the run number of the ceria to use
    @param van_run :: the run number of the vanadium to use
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param calibration_directory :: the user specific directory to save to
    @param calibration_general :: the general directory to save to
    @param cropped :: the cropping method to use
    @param crop_on :: where to crop on the cropping method
    @param crop_name :: the name of the cropped workspace

    """

    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)

    # Check if the calibration should be cropped, and if so what cropping method to use
    if cropped is not None:
        if cropped == "banks":
            create_calibration_files(ceria_run, van_run, full_inst_calib, van_int_file, van_curves_file,
                                     calibration_directory, calibration_general, False, crop_name, crop_on)
        elif cropped == "spectra":
            create_calibration_files(ceria_run, van_run, full_inst_calib, van_int_file, van_curves_file,
                                     calibration_directory, calibration_general, True, crop_name, crop_on)
    else:
        create_calibration_files(ceria_run, van_run, full_inst_calib, van_int_file, van_curves_file,
                                 calibration_directory, calibration_general, False, crop_name, 'both')


def create_calibration_files(ceria_run, van_run, full_inst_calib, int_van, van_curves_file, calibration_directory,
                             calibration_general, use_spectrum_number, crop_name, spec_nos):
    """
    create and save a calibration

    @param ceria_run :: run number for the ceria used
    @param van_run :: the run number of the vanadium to use
    @param int_van :: name of the integrated vanadium workspace
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param van_curves_file :: path to save vanadium curves to
    @param calibration_directory :: the user specific calibration directory to save to
    @param calibration_general :: the general calibration directory
    @param use_spectrum_number :: whether or not to crop using spectrum numbers or banks
    @param crop_name :: name of the output workspace
    @param spec_nos :: the value to crop on, either a spectra number, or a bank
    """
    van_integrated_ws = load_van_integration_file(int_van)
    ceria_ws = simple.Load(Filename="ENGINX" + ceria_run, OutputWorkspace="engg_calib")
    van_file = _gen_filename(van_run)
    van_ws = simple.Load(Filename=van_file)
    bank_names = ["North", "South"]
    # check which cropping method to use

    if use_spectrum_number:
        spectrum_numbers = spec_nos
        bank = None
    else:
        if spec_nos.lower() == "north" or spec_nos == '1':
            spectrum_numbers = None
            bank = '1'
        elif spec_nos.lower() == "south" or spec_nos == '2':
            spectrum_numbers = None
            bank = '2'
        else:
            spectrum_numbers = None
            bank = None

    output, ceria_raw, curves = run_calibration(sample_ws=ceria_ws,
                                                vanadium_workspace=van_ws,
                                                van_integration=van_integrated_ws,
                                                bank=bank,
                                                spectrum_numbers=spectrum_numbers,
                                                full_inst_calib=full_inst_calib)
    handle_van_curves(curves, van_curves_file)
    bk2bk_params = extract_b2b_params(ceria_raw)
    if len(output) == 1:
        # get the values needed for saving out the .prm files
        difa = [output[0]['difa']]
        difc = [output[0]['difc']]
        tzero = [output[0]['tzero']]
        if bank is None and spectrum_numbers is None:
            save_calibration(ceria_run, calibration_directory, calibration_general, "all_banks", [crop_name],
                             tzero, difc, difa, bk2bk_params)
        if spectrum_numbers is not None:
            save_calibration(ceria_run, calibration_directory, calibration_general,
                             "bank_cropped", [crop_name], tzero, difc, difa, bk2bk_params)
        else:
            save_calibration(ceria_run, calibration_directory, calibration_general,
                             "bank_{}".format(spec_nos), [crop_name], tzero, difc, difa, bk2bk_params)
        # create the table workspace containing the parameters
        param_tbl_name = crop_name if crop_name is not None else "Cropped"
        create_params_table(difc, tzero, difa)
        plot_dict = Utils.generate_tof_fit_dictionary(param_tbl_name)
        Utils.plot_tof_fit([plot_dict], [param_tbl_name])
    else:
        difas = [row['difa'] for row in output]
        difcs = [row['difc'] for row in output]
        tzeros = [row['tzero'] for row in output]
        plot_dicts = list()
        for i in range(1, 3):
            save_calibration(ceria_run, calibration_directory, calibration_general,
                             f"bank_{i}", [bank_names[i - 1]],
                             [tzeros[i - 1]], [difcs[i - 1]], [difas[i - 1]], bk2bk_params)
            plot_dicts.append(Utils.generate_tof_fit_dictionary(f"bank_{i}"))
        save_calibration(ceria_run, calibration_directory, calibration_general, "all_banks", bank_names,
                         tzeros, difcs, difas, bk2bk_params)
        # create the table workspace containing the parameters
        create_params_table(difcs, tzeros, difas)
        Utils.plot_tof_fit(plot_dicts, ["bank_1", "bank_2"])


def run_calibration(sample_ws,
                    vanadium_workspace,
                    van_integration,
                    bank,
                    spectrum_numbers,
                    full_inst_calib):
    """
    Creates Engineering calibration files with PDCalibration
    :param sample_ws: The workspace with the sample data.
    :param vanadium_workspace: The workspace with the vanadium data
    :param van_integration: The integration values from the vanadium corrections
    :param bank: The bank to crop to, both if none.
    :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
    :param full_inst_calib : workspace containing the full instrument calibration
    :return: The calibration output files, the vanadium curves workspace(s), and a clone of the sample file
    """

    def run_pd_calibration(kwargs_to_pass):
        return simple.PDCalibration(**kwargs_to_pass)

    def focus_and_make_van_curves(ceria_d, vanadium_d, grouping_kwarg):
        # focus ceria
        focused_ceria = simple.DiffractionFocussing(InputWorkspace=ceria_d, **grouping_kwarg)
        simple.ApplyDiffCal(InstrumentWorkspace=focused_ceria, ClearCalibration=True)
        tof_focused = simple.ConvertUnits(InputWorkspace=focused_ceria, Target='TOF')

        # focus van data
        focused_van = simple.DiffractionFocussing(InputWorkspace=vanadium_d, **grouping_kwarg)

        background_van = simple.EnggEstimateFocussedBackground(InputWorkspace=focused_van, NIterations='15',
                                                               XWindow=0.03)

        simple.DeleteWorkspace(focused_ceria)
        simple.DeleteWorkspace(focused_van)

        return tof_focused, background_van

    def ws_initial_process(ws):
        """Run some processing common to both the sample and vanadium workspaces"""
        simple.NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws)
        simple.ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_inst_calib)
        simple.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='dSpacing')
        return ws

    def calibrate_region_of_interest(roi, df_kwarg):
        focused_roi, curves_roi = focus_and_make_van_curves(ws_d, ws_van_d, df_kwarg)
        simple.RenameWorkspace(curves_roi, ("curves_" + roi))
        curves_output.append(curves_roi)

        # final calibration of focused data
        kwargs["InputWorkspace"] = focused_roi
        kwargs["OutputCalibrationTable"] = "engg_calibration_" + roi
        kwargs["DiagnosticWorkspaces"] = "diag_" + roi

        cal_roi = run_pd_calibration(kwargs)[0]
        cal_output[roi] = cal_roi

    # need to clone the data as PDCalibration rebins
    sample_raw = simple.CloneWorkspace(InputWorkspace=sample_ws)

    ws_van = simple.CloneWorkspace(vanadium_workspace)
    ws_van_d = ws_initial_process(ws_van)
    # sensitivity correction
    ws_van_d /= van_integration
    simple.ReplaceSpecialValues(InputWorkspace=ws_van_d, OutputWorkspace=ws_van_d, NaNValue=0, InfinityValue=0)

    ws_d = ws_initial_process(sample_ws)

    simple.DeleteWorkspace(van_integration)

    kwargs = {
        "PeakPositions": Utils.default_ceria_expected_peaks(final=True),
        "TofBinning": [15500, -0.0003, 52000],  # using a finer binning now have better stats
        "PeakWindow": 0.04,
        "MinimumPeakHeight": 0.5,
        "PeakFunction": 'BackToBackExponential',
        "CalibrationParameters": 'DIFC+TZERO+DIFA',
        "UseChiSq": True
    }
    cal_output = dict()
    curves_output = list()

    if spectrum_numbers is None:
        if bank == '1' or bank is None:
            df_kwarg = {"GroupingFileName": NORTH_BANK_CAL}
            calibrate_region_of_interest("bank_1", df_kwarg)

        if bank == '2' or bank is None:
            df_kwarg = {"GroupingFileName": SOUTH_BANK_CAL}
            calibrate_region_of_interest("bank_2", df_kwarg)
    else:
        grp_ws = Utils.create_grouping_workspace_from_spectra_list(spectrum_numbers, sample_raw)
        df_kwarg = {"GroupingWorkspace": grp_ws}
        calibrate_region_of_interest("Cropped", df_kwarg)

    simple.DeleteWorkspace(ws_van)
    simple.DeleteWorkspace("tof_focused")

    cal_params = list()
    # in the output calfile, rows are present for all detids, only read one from the region of interest
    bank_1_read_row = 0
    bank_2_read_row = 1200
    for bank_cal in cal_output:
        if bank_cal == "bank_1":
            read = bank_1_read_row
        elif bank_cal == "bank_2":
            read = bank_2_read_row
        else:
            read = int(Utils.create_spectrum_list_from_string(spectrum_numbers)[0])  # this can be int64
        row = cal_output[bank_cal].row(read)
        current_fit_params = {'difc': row['difc'], 'difa': row['difa'], 'tzero': row['tzero']}
        cal_params.append(current_fit_params)
    return cal_params, sample_raw, curves_output


def extract_b2b_params(workspace):

    ws_inst = workspace.getInstrument()
    NorthBank = ws_inst.getComponentByName("NorthBank")
    SouthBank = ws_inst.getComponentByName("SouthBank")
    params_north = []
    params_south = []
    for param_name in ["alpha_0", "beta_0", "beta_1", "sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]:
        params_north += [NorthBank.getNumberParameter(param_name)[0]]
        params_south += [SouthBank.getNumberParameter(param_name)[0]]

    return [params_north, params_south]


def load_van_integration_file(ints_van):
    """
    load the vanadium integration file passed in

    @param ints_van:: the path to the integrated vanadium file

    """
    van_integrated_ws = simple.Load(ints_van, OutputWorkspace="int_van")
    return van_integrated_ws


def load_van_curves_file(curves_van):
    """
    load the vanadium curves file passed in

    @param curves_van:: the path to the integrated vanadium file

    """
    van_curves_ws = simple.Load(curves_van, OutputWorkspace="curves_van")
    return van_curves_ws


def save_calibration(ceria_run, calibration_directory, calibration_general, name, bank_names, zeros, difcs,
                     difas, bk2bk_params):
    """
    save the calibration data

    @param ceria_run :: the run number of the ceria
    @param calibration_directory :: the user specific calibration directory to save to
    @param calibration_general :: the general calibration directory to save to
    @param name ::  the name of the banks being saved
    @param bank_names :: the list of banks to save
    @param difas :: the list of difa values to save
    @param difcs :: the list of difc values to save
    @param zeros :: the list of tzero values to save

    """
    file_save_prefix = os.path.join(calibration_directory, "ENGINX_" + ceria_run + "_")
    gsas_iparm_fname = file_save_prefix + name + ".prm"
    pdcals_to_save = dict()  # fname: workspace
    # work out what template to use, and which PDCalibration files to save
    if name == "all_banks":
        template_file = None
        pdcals_to_save[file_save_prefix + "bank_North.nxs"] = 'engg_calibration_bank_1'
        pdcals_to_save[file_save_prefix + "bank_South.nxs"] = 'engg_calibration_bank_2'
    elif name == "bank_2":
        template_file = "template_ENGINX_241391_South_bank.prm"
        pdcals_to_save[file_save_prefix + "bank_South.nxs"] = 'engg_calibration_bank_2'
    elif name == "bank_1":
        template_file = "template_ENGINX_241391_North_bank.prm"
        pdcals_to_save[file_save_prefix + "bank_North.nxs"] = 'engg_calibration_bank_1'
    else:  # cropped uses North bank template
        template_file = "template_ENGINX_241391_North_bank.prm"
        pdcals_to_save[file_save_prefix + "cropped.nxs"] = 'engg_calibration_cropped'
    # write out the param file to the users directory

    Utils.write_ENGINX_GSAS_iparam_file(output_file=gsas_iparm_fname, bank_names=bank_names, difa=difas, difc=difcs,
                                        tzero=zeros, ceria_run=ceria_run,
                                        template_file=template_file, bk2bk_params=bk2bk_params)
    for fname, ws in pdcals_to_save.items():
        simple.SaveNexus(InputWorkspace=ws, Filename=fname)
    if not calibration_general == calibration_directory:
        # copy the param file to the general directory
        if not os.path.exists(calibration_general):
            os.makedirs(calibration_general)
        copy2(gsas_iparm_fname, calibration_general)
        for fname in pdcals_to_save.keys():
            copy2(fname, calibration_general)


def create_params_table(difc, tzero, difa):
    """
    create the params table from the output from the calibration


    @param difc :: the list of difc values to add to the table
    @param tzero :: the list of tzero values to add to the table
    @param difa :: the list of difa values to add to the table

    """
    param_table = simple.CreateEmptyTableWorkspace(OutputWorkspace="engg_calibration_banks_parameters")
    # setup table
    param_table.addColumn("int", "bankid")
    param_table.addColumn("double", "difc")
    param_table.addColumn("double", "difa")
    param_table.addColumn("double", "tzero")
    # add data to table
    for i in range(len(difc)):
        next_row = {"bankid": i, "difc": difc[i], "difa": difa[i], "tzero": tzero[i]}
        param_table.addRow(next_row)


def plot_calibration(workspace, name):
    """
    Plot the fitted peaks and expected peaks of a bank
    @param workspace :: The workspace object to be plotted
    @param name :: The name to be plotted, should be a variation of "Engg difc Zero Peaks Bank..."

    """
    fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
    fig.canvas.set_window_title(name)

    # plot lines based off of old gui plot
    ax.plot(workspace, wkspIndex=0, label="Peaks Fitted", linestyle=":", color="black", marker='o', markersize=2,
            linewidth=1.5)
    ax.plot(workspace, wkspIndex=1, label="Expected Peaks Centre(dspacing, A)", color="orange", marker='o',
            markersize=2)

    # set the plot and axes titles
    ax.set_title(name, fontweight="bold")
    ax.set_xlabel("Expected Peaks Centre(dspacing, A)", fontweight="bold")
    ax.set_ylabel("Fitted Peaks Centre(TOF, us)", fontweight="bold")

    # set the ticks on the axes
    ax.set_xticks(np.arange(0.5, 3.6, step=0.1), True)
    ax.set_xticks(np.arange(0.5, 4, step=0.5))

    ax.set_yticks(np.arange(1e4, 7e4, step=1e4))
    ax.set_yticks(np.arange(1e4, 6.2e4, step=0.2e4), True)

    ax.legend()
    fig.show()


def focus(run_no, van_run, full_inst_calib, calibration_directory, focus_directory, focus_general, do_pre_process, params, time_period,
          grouping_file, cropped, crop_on):
    """

    focus the run passed in useing the vanadium set on the object

    @param run_no :: the run number of the run to focus
    @param van_run :: the run number of the vanadium to use
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param calibration_directory :: the user specific calibration directory to load from
    @param focus_directory :: the user specific focus directory to save to
    @param focus_general :: the general focus directory to save to
    @param do_pre_process :: whether or not to pre-process the run before focussing it
    @param params :: the rebin parameters for pre-processing
    @param time_period :: the time period for pre-processing
    @param grouping_file :: the grouping file for texture mode focussing
    @param cropped :: what cropping method to use
    @param crop_on :: where to crop, either a bank or spectrum numbers

    """
    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)
    # if a grouping file is passed in then focus in texture mode
    if grouping_file is not None:
        grouping_file = os.path.join(calibration_directory, grouping_file)
        focus_texture_mode(run_no, van_curves_file, van_int_file, full_inst_calib, focus_directory, focus_general, do_pre_process,
                           params, time_period, grouping_file)
    # if no texture mode was passed in check if the file should be cropped, and if so what cropping method to use
    elif cropped is not None:
        if cropped == "banks":
            focus_cropped(run_no, van_curves_file, van_int_file, full_inst_calib, focus_directory, focus_general, do_pre_process, params,
                          time_period, crop_on, False)
        elif cropped == "spectra":
            focus_cropped(run_no, van_curves_file, van_int_file, full_inst_calib, focus_directory, focus_general, do_pre_process, params,
                          time_period, crop_on, True)
    else:
        focus_whole(run_no, van_curves_file, van_int_file, full_inst_calib, focus_directory, focus_general, do_pre_process, params,
                    time_period)


def _run_focus(input_workspace,
               tof_output_name,
               vanadium_integration_ws,
               vanadium_curves_ws,
               df_kwarg,
               full_calib,
               region_calib):
    simple.NormaliseByCurrent(InputWorkspace=input_workspace, OutputWorkspace=input_workspace)
    input_workspace /= vanadium_integration_ws
    simple.ReplaceSpecialValues(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, NaNValue=0,
                                InfinityValue=0)
    simple.ApplyDiffCal(InstrumentWorkspace=input_workspace, CalibrationWorkspace=full_calib)
    ws_d = simple.ConvertUnits(InputWorkspace=input_workspace, Target='dSpacing')
    focused_sample = simple.DiffractionFocussing(InputWorkspace=ws_d, **df_kwarg)
    curves_rebinned = simple.RebinToWorkspace(WorkspaceToRebin=vanadium_curves_ws, WorkspaceToMatch=focused_sample)
    normalised = simple.Divide(LHSWorkspace=focused_sample, RHSWorkspace=curves_rebinned,
                               AllowDifferentNumberSpectra=True)
    simple.ApplyDiffCal(InstrumentWorkspace=normalised, CalibrationWorkspace=region_calib)
    dspacing_output_name = tof_output_name + "_dSpacing"
    simple.CloneWorkspace(InputWorkspace=normalised, OutputWorkspace=dspacing_output_name)
    simple.ConvertUnits(InputWorkspace=normalised, OutputWorkspace=tof_output_name, Target='TOF')
    simple.DeleteWorkspace(curves_rebinned)
    simple.DeleteWorkspace(focused_sample)
    simple.DeleteWorkspace(normalised)
    simple.DeleteWorkspace(ws_d)


def focus_whole(run_number, van_curves, van_int, full_inst_calib, focus_directory, focus_general, do_pre_process, params, time_period):
    """
    focus a whole run with no cropping

    @param run_number :: the run nuumber to focus
    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param focus_directory :: the user specific focus directory to save to
    @param focus_general :: the general focus directory to save to
    @param do_pre_process :: whether or not to pre-process the run before focussing it
    @param params :: the rebin parameters for pre-processing
    @param time_period :: the time period for pre-processing

    """
    van_curves_ws, van_integrated_ws, ws_to_focus = _prepare_focus(run_number, van_curves, van_int, do_pre_process,
                                                                   params, time_period)
    # loop through both banks, focus and save them
    for bank_no in range(1, 3):
        sample_ws_clone = simple.CloneWorkspace(ws_to_focus)
        curves_ws_clone = simple.CloneWorkspace(van_curves_ws)
        tof_output_name = "engg_focus_output_bank_{}".format(bank_no)
        dspacing_output_name = tof_output_name + "_dSpacing"
        cal_file = NORTH_BANK_CAL if bank_no == 1 else SOUTH_BANK_CAL
        region_calib = 'engg_calibration_bank_1' if bank_no == 1 else 'engg_calibration_bank_2'
        df_kwarg = {"GroupingFileName": cal_file}
        _run_focus(input_workspace=sample_ws_clone, tof_output_name=tof_output_name, region_calib=region_calib,
                   vanadium_integration_ws=van_integrated_ws, vanadium_curves_ws=curves_ws_clone, df_kwarg=df_kwarg,
                   full_calib=full_inst_calib)
        _save_out(run_number, focus_directory, focus_general, tof_output_name, "ENGINX_{}_{}{{}}", str(bank_no))
        _save_out(run_number, focus_directory, focus_general, dspacing_output_name, "ENGINX_{}_{}{{}}", str(bank_no))
        simple.DeleteWorkspace(sample_ws_clone)
        simple.DeleteWorkspace(curves_ws_clone)


def focus_cropped(run_number, van_curves, van_int, full_inst_calib, focus_directory, focus_general, do_pre_process, params, time_period,
                  crop_on,
                  use_spectra):
    """
    focus a partial run, cropping either on banks or on specific spectra

    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param run_number :: the run nuumber to focus
    @param focus_directory :: the user specific focus directory to save to
    @param focus_general :: the general focus directory to save to
    @param do_pre_process :: whether or not to pre-process the run before focussing it
    @param params :: the rebin parameters for pre-processing
    @param time_period :: the time period for pre-processing
    @param crop_on :: the bank or spectra to crop on
    @param use_spectra :: whether to focus by spectra or banks

    """
    van_curves_ws, van_integrated_ws, ws_to_focus = _prepare_focus(run_number, van_curves, van_int, do_pre_process,
                                                                   params, time_period)
    tof_output_name = "engg_focus_output{0}{1}"
    sample_ws_clone = simple.CloneWorkspace(ws_to_focus)
    curves_ws_clone = simple.CloneWorkspace(van_curves_ws)
    # check whether to crop on bank or spectra
    if not use_spectra:
        # get the bank to crop on, focus and save it out
        bank = {"North": "1",
                "South": "2"}
        bank_no = bank.get(crop_on)
        cal_file = NORTH_BANK_CAL if bank_no == 1 else SOUTH_BANK_CAL
        region_calib = 'engg_calibration_bank_1' if bank_no == 1 else 'engg_calibration_bank_2'
        df_kwarg = {"GroupingFileName": cal_file}
        tof_output_name = tof_output_name.format("_bank_", bank_no)
        dspacing_output_name = tof_output_name + "_dSpacing"
        _run_focus(input_workspace=sample_ws_clone, tof_output_name=tof_output_name, vanadium_integration_ws=van_integrated_ws,
                   vanadium_curves_ws=curves_ws_clone, df_kwarg=df_kwarg, full_calib=full_inst_calib,
                   region_calib=region_calib)
        _save_out(run_number, focus_directory, focus_general, tof_output_name, "ENGINX_{}_{}{{}}", crop_on)
        _save_out(run_number, focus_directory, focus_general, dspacing_output_name, "ENGINX_{}_{}{{}}", crop_on)
    else:
        # crop on the spectra passed in, focus and save it out
        tof_output_name = tof_output_name.format("_", "cropped")
        dspacing_output_name = tof_output_name + "_dSpacing"
        grp_ws = Utils.create_grouping_workspace_from_spectra_list(crop_on, ws_to_focus)
        df_kwarg = {"GroupingWorkspace": grp_ws}
        region_calib = 'engg_calibration_cropped'
        _run_focus(input_workspace=sample_ws_clone, tof_output_name=tof_output_name, vanadium_integration_ws=van_integrated_ws,
                   vanadium_curves_ws=curves_ws_clone, df_kwarg=df_kwarg, region_calib=region_calib,
                   full_calib=full_inst_calib)
        _save_out(run_number, focus_directory, focus_general, tof_output_name, "ENGINX_{}_bank_{}{{}}", "cropped")
        _save_out(run_number, focus_directory, focus_general, dspacing_output_name, "ENGINX_{}_bank_{}{{}}", "cropped")
    simple.DeleteWorkspace(sample_ws_clone)
    simple.DeleteWorkspace(curves_ws_clone)


def focus_texture_mode(run_number, van_curves, van_int, full_inst_calib, focus_directory, focus_general, do_pre_process, params,
                       time_period, dg_file):
    """
    perform a texture mode focusing using the grouping csv file

    @param run_number :: the run number to focus
    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
    @param full_inst_calib :: workspace containing the full instrument calibration
    @param focus_directory :: the user specific focus directory to save to
    @param focus_general :: the general focus directory to save to
    @param do_pre_process :: whether or not to pre-process the run before focussing it
    @param params :: the rebin parameters for pre-processing
    @param time_period :: the time period for pre-processing
    @param dg_file :: the grouping file to use for texture mode

    """
    van_curves_ws, van_integrated_ws, ws_to_focus = _prepare_focus(run_number, van_curves, van_int, do_pre_process,
                                                                   params, time_period)
    banks = {}
    # read the csv file to work out the banks
    # ensure csv reading works on python 2 or 3
    with open(dg_file, 'r', newline='', encoding='utf-8') as grouping_file:
        group_reader = csv.reader(_decomment_csv(grouping_file), delimiter=',')

        for row in group_reader:
            banks.update({row[0]: ','.join(row[1:])})

    # loop through the banks described in the csv, focusing and saving them out
    for bank in banks:
        sample_ws_clone = simple.CloneWorkspace(ws_to_focus)
        curves_ws_clone = simple.CloneWorkspace(van_curves_ws)
        tof_output_name = "engg_focusing_output_ws_texture_bank_{}"
        tof_output_name = tof_output_name.format(bank)
        dspacing_output_name = tof_output_name + "_dSpacing"
        grp_ws = Utils.create_grouping_workspace_from_spectra_list(banks[bank], ws_to_focus)
        df_kwarg = {"GroupingWorkspace": grp_ws}
        _run_focus(input_workspace=sample_ws_clone, tof_output_name=tof_output_name, region_calib=full_inst_calib,
                   vanadium_curves_ws=curves_ws_clone, full_calib=full_inst_calib, df_kwarg=df_kwarg,
                   vanadium_integration_ws=van_integrated_ws)
        _save_out(run_number, focus_directory, focus_general, tof_output_name, "ENGINX_{}_texture_{}{{}}", bank)
        _save_out(run_number, focus_directory, focus_general, dspacing_output_name, "ENGINX_{}_texture_{}{{}}", bank)
        simple.DeleteWorkspace(sample_ws_clone)
        simple.DeleteWorkspace(curves_ws_clone)


def _prepare_focus(run_number, van_curves, van_int, do_pre_process, params, time_period):
    """
    perform some universal setup for focusing

    @param run_number :: the run number to focus
    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
    @param do_pre_process :: whether or not to pre-process the run
    @param params :: the rebin paramters for pre-processing
    @param time_period :: the time period for pre-processing

    """
    van_curves_ws = load_van_curves_file(van_curves)
    van_integrated_ws = load_van_integration_file(van_int)
    if do_pre_process:
        ws_to_focus = pre_process(_gen_filename(run_number), params, time_period)
    else:
        ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    return van_curves_ws, van_integrated_ws, ws_to_focus


def _save_out(run_number, focus_directory, focus_general, output, enginx_file_name_format, bank_id):
    """
    save out the files required for the focus

    @param run_number :: the run number of the focused run
    @param focus_directory :: the user directory to save to
    @param focus_general :: the general folder to copy the saved out files to
    @param output :: the workspace to save
    @param enginx_file_name_format :: the naming scheme of the files
    @param bank_id :: the bank being saved

    """
    # work out where to save the files
    dat_name, genie_filename, gss_name, hdf5_name, nxs_name = _find_focus_file_location(bank_id, focus_directory,
                                                                                        enginx_file_name_format,
                                                                                        run_number)
    if not str(bank_id).isnumeric():
        bank_id = 0
    # save the files out to the user directory
    simple.SaveFocusedXYE(InputWorkspace=output, Filename=dat_name, SplitFiles=False,
                          StartAtBankNumber=bank_id)
    simple.SaveGSS(InputWorkspace=output, Filename=gss_name, SplitFiles=False, Bank=bank_id)
    simple.SaveOpenGenieAscii(InputWorkspace=output, Filename=genie_filename, OpenGenieFormat="ENGIN-X Format")
    simple.SaveNexus(InputWorkspace=output, Filename=nxs_name)
    simple.ExportSampleLogsToHDF5(InputWorkspace=output, Filename=hdf5_name, Blacklist="bankid")
    if not focus_general == focus_directory:
        if not os.path.exists(focus_general):
            os.makedirs(focus_general)
        # copy the files to the general directory
        copy2(dat_name, focus_general)
        copy2(gss_name, focus_general)
        copy2(genie_filename, focus_general)
        copy2(nxs_name, focus_general)
        copy2(hdf5_name, focus_general)


def _find_focus_file_location(bank_id, focus_directory, enginx_file_name_format, run_number):
    # Leave final {} in string so that file extension can be set.
    run_and_bank = enginx_file_name_format.format(run_number, bank_id)
    filename = os.path.join(focus_directory, run_and_bank)
    genie_filename = os.path.join(focus_directory, run_and_bank.replace("_", "", 1).format(".his"))

    dat_name = filename.format(".dat")
    gss_name = filename.format(".gss")
    nxs_name = filename.format(".nxs")
    hdf5_name = os.path.join(focus_directory, run_number + ".hdf5")
    return dat_name, genie_filename, gss_name, hdf5_name, nxs_name


def _decomment_csv(csvfile):
    """
    remove commented rows from the csv file

    @param csvfile :: the file to remove comments from

    """
    for row in csvfile:
        raw = row.split('#')[0].strip()
        if raw:
            yield raw


def pre_process(focus_run, params, time_period):
    """
    pre_process the run passed in, usign the rebin parameters passed in

    @param focus_run :: the run to focus
    @param params :: the rebin parameters
    @param time_period :: the time period for rebin by pulse

    @return: pre-processed workspace

    """
    # rebin based on pulse if a time period is sent in, otherwise just do a normal rebin
    wsname = "engg_preproc_input_ws"
    simple.Load(Filename=focus_run, OutputWorkspace=wsname)
    if time_period is not None:
        return rebin_pulse(params, wsname, time_period)
    else:
        return rebin_time(params, wsname)


def rebin_time(bin_param, wsname):
    """
    rebin by time

    @param bin_param:: the parameters to rebin to
    @param wsname:: the workspace to rebin
    @return: the rebinned workspace
    """
    output = "engg_focus_input_ws"
    ws = simple.Rebin(InputWorkspace=wsname, Params=bin_param, OutputWorkspace=output)
    return ws


def rebin_pulse(bin_param, wsname, time_period):
    """
    rebin by pulse

    @param bin_param:: the parameters to rebin to
    @param wsname:: the workspace to rebin
    @param time_period:: currently unused to match gui

    @return: the rebinned workspace
    """
    print(time_period)  # time period unused to match gui, print to prevent warnings
    output = "engg_focus_input_ws"
    ws = simple.RebinByPulseTimes(InputWorkspace=wsname, Params=bin_param, OutputWorkspace=output)
    return ws


def _get_van_names(van_run, calibration_directory):
    """
    generate the names of the vanadium workspaces

    @param van_run :: the run number of the vanadium
    @param calibration_directory :: the user specific calibration directory to load from

    @return: the file paths for the curved and integrated vanadium files

    """
    van_file = _gen_filename(van_run)
    van_out = os.path.join(calibration_directory, van_file)
    van_int_file = van_out + "_precalculated_vanadium_run_integration.nxs"
    van_curves_file = van_out + "_precalculated_vanadium_run_bank_curves.nxs"
    return van_curves_file, van_int_file


def _gen_filename(run_number):
    """
    generate the file name based of just having a run number

    @param run_number:: the run number to generate the name for

    @return: the generated name

    """
    return "ENGINX" + ("0" * (8 - len(run_number))) + run_number
