# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import csv
import matplotlib.pyplot as plt
import numpy as np
import os
from platform import system
from shutil import copy2
from six import u

import mantid.plots # noqa
import Engineering.EnggUtils as Utils
import mantid.simpleapi as simple


def main(vanadium_run, user, focus_run, **kwargs):
    """
    sets up all defaults, then calls run


    @param vanadium_run :: the run number of the vanadium to be used
    @param user :: the username to save under
    @param focus_run :: the run number to focus
    Kwargs:
        ceria_run (string): the run number of the ceria to use
        force_vanadium (bool): forces creation of vanadium even if one for the run already exists
        force_cal (bool) : forces creation of calibration files
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
    do_van = kwargs.get("force_vanadium", False)
    do_cal = kwargs.get("force_cal", False)

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
    run(ceria_run, do_cal, do_van, vanadium_run, calibration_directory, calibration_general, cropped, crop_name,
        crop_on, focus_directory, focus_general, do_pre_process, params, time_period, focus_run, grouping_file)


def run(ceria_run, do_cal, do_van, van_run, calibration_directory, calibration_general, cropped, crop_name, crop_on,
        focus_directory, focus_general, do_pre_process, params, time_period, focus_run, grouping_file):
    """
    calls methods needed based off of inputs

    @param ceria_run :: the run number of the ceria to use
    @param do_cal :: whether or not to force running calibration
    @param do_van :: whether or not to force calculating the vanadium
    @param van_run :: run number to use for the vanadium
    @param calibration_directory :: the users calibration directory
    @param calibration_general :: the non-user specific calibration directory
    @param crop_on :: where to crop using the cropping method
    @param crop_name :: how to name cropped banks
    @param cropped :: the cropping method to use
    @param focus_directory :: the users focus directory
    @param focus_general :: the non-user specificfocus directory
    @param do_pre_process:: whether or not to pre-process before focussing
    @param params :: list of pararmeters to use for rebinning
    @param time_period :: time perriod to old binning
    @param focus_run :: run number to focus
    @param grouping_file :: grouping file to use with texture mode


    """

    # check whether creating a vanadium is required or requested
    if (not os.path.isfile(_get_van_names(van_run, calibration_directory)[0])) or do_van:
        create_vanadium(van_run, calibration_directory)

    # find the file names of calibration files that would be created by this run
    cal_endings = {"banks": ["all_banks", "bank_{}".format(crop_on)],
                   "spectra": ["all_banks", "bank_{}".format(crop_name)],
                   None: ["all_banks", "bank_North", "bank_South"]}
    expected_cals = ["ENGINX_{0}_{1}_{2}.prm".format(van_run, ceria_run, i) for i in cal_endings.get(cropped)]
    expected_cals_present = [os.path.isfile(os.path.join(calibration_directory, cal_file)) for cal_file in
                             expected_cals]

    # if the calibration files that this run would create are not present, or the user has requested it, create the
    # calibration files
    if not all(expected_cals_present) or do_cal:
        create_calibration(ceria_run, van_run, calibration_directory, calibration_general, cropped, crop_name, crop_on)

    # if a focus is requested, run the focus
    if focus_run is not None:
        focus(focus_run, van_run, calibration_directory, focus_directory, focus_general, do_pre_process, params,
              time_period, grouping_file, cropped, crop_on)


def create_vanadium(van_run, calibration_directory):
    """
    create the vanadium run for the run number set of the object

    @param van_run :: the run number for the vanadium
    @param calibration_directory :: the directory to save the output to
    """
    # find and load the vanadium
    van_file = _gen_filename(van_run)
    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)
    van_name = "eng_vanadium_ws"
    simple.Load(van_file, OutputWorkspace=van_name)

    # make the vanadium corrections
    simple.EnggVanadiumCorrections(VanadiumWorkspace=van_name,
                                   OutIntegrationWorkspace="eng_vanadium_integration",
                                   OutCurvesWorkspace="eng_vanadium_curves")

    # save out the vanadium and delete the original workspace
    simple.SaveNexus("eng_vanadium_integration", van_int_file)
    simple.SaveNexus("eng_vanadium_curves", van_curves_file)
    simple.DeleteWorkspace(van_name)


def create_calibration(ceria_run, van_run, calibration_directory, calibration_general, cropped, crop_name, crop_on):
    """
    create the calibration files

    @param ceria_run :: the run number of the ceria to use
    @param van_run :: the run number of the vanadium to use
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
            create_calibration_cropped_file(ceria_run, van_run, van_curves_file, van_int_file, calibration_directory,
                                            calibration_general, False, crop_name, crop_on)
        elif cropped == "spectra":
            create_calibration_cropped_file(ceria_run, van_run, van_curves_file, van_int_file, calibration_directory,
                                            calibration_general, True, crop_name, crop_on)
    else:
        create_calibration_files(ceria_run, van_run, van_curves_file, van_int_file, calibration_directory,
                                 calibration_general)


def create_calibration_cropped_file(ceria_run, van_run, curve_van, int_van, calibration_directory, calibration_general,
                                    use_spectrum_number, crop_name, spec_nos):
    """
    create and save a cropped calibration file

    @param ceria_run :: run number for the ceria used
    @param van_run :: the run number of the vanadium to use
    @param curve_van :: name of the vanadium curves workspace
    @param int_van :: name of the integrated vanadium workspace
    @param calibration_directory :: the user specific calibration directory to save to
    @param calibration_general :: the general calibration dirrecory
    @param use_spectrum_number :: whether or not to crop using spectrum numbers  or banks
    @param crop_name :: name of the output workspace
    @param spec_nos :: the value to crop on, either a spectra number, or a bank

    """
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX" + ceria_run, OutputWorkspace="eng_calib")
    # check which cropping method to use
    if use_spectrum_number:
        param_tbl_name = crop_name if crop_name is not None else "cropped"
        # run the calibration cropping on spectrum number
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=str(spec_nos),
                                      FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        # get the values needed for saving out the .prm files
        difc = [output.DIFC]
        tzero = [output.TZERO]
        difa = [output.DIFA]
        save_calibration(ceria_run, van_run, calibration_directory, calibration_general, "all_banks", [param_tbl_name],
                         tzero, difc)
        save_calibration(ceria_run, van_run, calibration_directory, calibration_general,
                         "bank_{}".format(param_tbl_name), [param_tbl_name], tzero, difc)
    else:
        # work out which bank number to crop on, then calibrate
        if spec_nos.lower() == "north":
            param_tbl_name = "engg_calibration_bank_1"
            bank = 1
        else:
            param_tbl_name = "engg_calibration_bank_2"
            bank = 2
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, Bank=str(bank), FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        # get the values needed for saving out the .prm files
        difc = [output.DIFC]
        tzero = [output.TZERO]
        difa = [output.DIFA]
        save_calibration(ceria_run, van_run, calibration_directory, calibration_general, "all_banks", [spec_nos], tzero,
                         difc)
        save_calibration(ceria_run, van_run, calibration_directory, calibration_general, "bank_{}".format(spec_nos),
                         [spec_nos], tzero, difc)
    # create the table workspace containing the parameters
    create_params_table(difc, tzero, difa)
    create_difc_zero_workspace(difc, tzero, spec_nos, param_tbl_name)


def create_calibration_files(ceria_run, van_run, curve_van, int_van, calibration_directory, calibration_general):
    """
    create the calibration files for an uncropped run

    @param ceria_run :: the run number of the ceria
    @param van_run :: the run number of the vanadium
    @param curve_van :: the vanadium curves workspace
    @param int_van :: the integrated vanadium workspace
    @param calibration_directory :: the user specific calibration directory to save to
    @param calibration_general :: the general calibration directory

    """
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX" + ceria_run, OutputWorkspace="eng_calib")
    difcs = []
    tzeros = []
    difa = []
    banks = 3
    bank_names = ["North", "South"]
    # loop through the banks, calibrating both
    for i in range(1, banks):
        param_tbl_name = "engg_calibration_bank_{}".format(i)
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, Bank=str(i), FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        # add the needed outputs to a list
        difcs.append(output.DIFC)
        tzeros.append(output.TZERO)
        difa.append(output.DIFA)
        # save out the ones needed for this loop
        save_calibration(ceria_run, van_run, calibration_directory, calibration_general,
                         "bank_{}".format(bank_names[i - 1]), [bank_names[i - 1]],
                         [tzeros[i - 1]], [difcs[i - 1]])
    # save out the total version, then create the table of params
    save_calibration(ceria_run, van_run, calibration_directory, calibration_general, "all_banks", bank_names, tzeros,
                     difcs)
    create_params_table(difcs, tzeros, difa)
    create_difc_zero_workspace(difcs, tzeros, "", None)


def load_van_files(curves_van, ints_van):
    """
    load the vanadium files passed in

    @param curves_van :: the path to the vanadium curves file
    @param ints_van:: the path to the integrated vanadium file

    """
    van_curves_ws = simple.Load(curves_van, OutputWorkspace="curves_van")
    van_integrated_ws = simple.Load(ints_van, OutputWorkspace="int_van")
    return van_curves_ws, van_integrated_ws


def save_calibration(ceria_run, van_run, calibration_directory, calibration_general, name, bank_names, zeros, difcs):
    """
    save the calibration data

    @param ceria_run :: the run number of the ceria
    @param van_run :: the run number of the vanadium
    @param calibration_directory :: the user specific calibration directory to save to
    @param calibration_general :: the general calibration directory to save to
    @param name ::  the name of the banks being saved
    @param bank_names :: the list of banks to save
    @param difcs :: the list of difc values to save
    @param zeros :: the list of tzero values to save

    """

    gsas_iparm_fname = os.path.join(calibration_directory, "ENGINX_" + van_run + "_" + ceria_run + "_" + name + ".prm")
    # work out what template to use
    if name == "all_banks":
        template_file = None
    elif name == "bank_South":
        template_file = "template_ENGINX_241391_236516_South_bank.prm"
    else:
        template_file = "template_ENGINX_241391_236516_North_bank.prm"
    # write out the param file to the users directory

    Utils.write_ENGINX_GSAS_iparam_file(output_file=gsas_iparm_fname, bank_names=bank_names, difc=difcs, tzero=zeros,
                                        ceria_run=ceria_run, vanadium_run=van_run,
                                        template_file=template_file)
    if not calibration_general == calibration_directory:
        # copy the param file to the general directory
        if not os.path.exists(calibration_general):
            os.makedirs(calibration_general)
        copy2(gsas_iparm_fname, calibration_general)


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


def create_difc_zero_workspace(difc, tzero, crop_on, name):
    """
    create a workspace that can be used to plot the expected peaks against the fitted ones

    @param difc :: the list of difc values to add to the table
    @param tzero :: the list of tzero values to add to the table
    @param crop_on :: where the cropping occured, either a bank, a spectra, or empty
    @param name :: the name of a cropped workspace to use, if it is a non default name

    """
    plot_spec_num = False
    # check what banks to use
    banks = [1]
    correction = 0
    if crop_on == "":
        banks = [1, 2]
    elif crop_on.lower() == "south":
        correction = 1
    elif not crop_on == "":
        plot_spec_num = True

    # loop through used banks
    for i in banks:
        actual_i = correction+i
        # retrieve required workspace
        if not plot_spec_num:
            bank_ws = simple.AnalysisDataService.retrieve("engg_calibration_bank_{}".format(actual_i))
        else:
            bank_ws = simple.AnalysisDataService.retrieve(name)

        # get the data to be used
        x_val = []
        y_val = []
        y2_val = []
        for irow in range(0, bank_ws.rowCount()):
            x_val.append(bank_ws.cell(irow, 0))
            y_val.append(bank_ws.cell(irow, 5))
            y2_val.append(x_val[irow] * difc[i - 1] + tzero[i - 1])

        # create workspaces to temporary hold the data
        simple.CreateWorkspace(OutputWorkspace="ws1", DataX=x_val, DataY=y_val,
                               UnitX="Expected Peaks Centre(dSpacing, A)",
                               YUnitLabel="Fitted Peaks Centre(TOF, us)")
        simple.CreateWorkspace(OutputWorkspace="ws2", DataX=x_val, DataY=y2_val)

        # get correct name for output
        if not plot_spec_num:
            name = actual_i
        output_name = "Engg difc Zero Peaks Bank {}".format(name)

        # use the two workspaces to creat the output
        output = simple.AppendSpectra(InputWorkspace1="ws1", InputWorkspace2="ws2",
                                      OutputWorkspace=output_name)
        plot_calibration(output, output_name)

    # remove the left-over workspaces
    simple.DeleteWorkspace("ws1")
    simple.DeleteWorkspace("ws2")


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


def focus(run_no, van_run, calibration_directory, focus_directory, focus_general, do_pre_process, params, time_period,
          grouping_file, cropped, crop_on):
    """

    focus the run passed in useing the vanadium set on the object

    @param run_no :: the run number of the run to focus
    @param van_run :: the run number of the vanadium to use
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
        focus_texture_mode(run_no, van_curves_file, van_int_file, focus_directory, focus_general, do_pre_process,
                           params, time_period, grouping_file)
    # if no texture mode was passed in check if the file should be cropped, and if so what cropping method to use
    elif cropped is not None:
        if cropped == "banks":
            focus_cropped(run_no, van_curves_file, van_int_file, focus_directory, focus_general, do_pre_process, params,
                          time_period, crop_on, False)
        elif cropped == "spectra":
            focus_cropped(run_no, van_curves_file, van_int_file, focus_directory, focus_general, do_pre_process, params,
                          time_period, crop_on, True)
    else:
        focus_whole(run_no, van_curves_file, van_int_file, focus_directory, focus_general, do_pre_process, params,
                    time_period)


def focus_whole(run_number, van_curves, van_int, focus_directory, focus_general, do_pre_process, params, time_period):
    """
    focus a whole run with no cropping

    @param run_number :: the run nuumber to focus
    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
    @param focus_directory :: the user specific focus directory to save to
    @param focus_general :: the general focus directory to save to
    @param do_pre_process :: whether or not to pre-process the run before focussing it
    @param params :: the rebin parameters for pre-processing
    @param time_period :: the time period for pre-processing

    """
    van_curves_ws, van_integrated_ws, ws_to_focus = _prepare_focus(run_number, van_curves, van_int, do_pre_process,
                                                                   params, time_period)
    # loop through both banks, focus and save them
    for i in range(1, 3):
        output_ws = "engg_focus_output_bank_{}".format(i)
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=str(i))
        _save_out(run_number, focus_directory, focus_general, output_ws, "ENGINX_{}_{}{}", str(i))


def focus_cropped(run_number, van_curves, van_int, focus_directory, focus_general, do_pre_process, params, time_period,
                  crop_on,
                  use_spectra):
    """
    focus a partial run, cropping either on banks or on specific spectra

    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
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
    output_ws = "engg_focus_output{0}{1}"
    # check whether to crop on bank or spectra
    if not use_spectra:
        # get the bank to crop on, focus and save it out
        bank = {"North": "1",
                "South": "2"}
        output_ws = output_ws.format("_bank_", bank.get(crop_on))
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=bank.get(crop_on))
        _save_out(run_number, focus_directory, focus_general, output_ws, "ENGINX_{}_{}{}", crop_on)
    else:
        # crop on the spectra passed in, focus and save it out
        output_ws = output_ws.format("", "")
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws,
                         VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=crop_on)
        _save_out(run_number, focus_directory, focus_general, output_ws, "ENGINX_{}_bank_{}{}", "cropped")


def focus_texture_mode(run_number, van_curves, van_int, focus_directory, focus_general, do_pre_process, params,
                       time_period,
                       dg_file):
    """
    perform a texture mode focusing using the grouping csv file

    @param run_number :: the run nuumber to focus
    @param van_curves :: the path to the vanadium curves file
    @param van_int :: the path to the integrated vanadium file
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
    with open(dg_file) as grouping_file:
        group_reader = csv.reader(_decomment_csv(grouping_file), delimiter=',')

        for row in group_reader:
            banks.update({row[0]: ','.join(row[1:])})

    # loop through the banks described in the csv, focusing and saing them out
    for bank in banks:
        output_ws = "engg_focusing_output_ws_texture_bank_{}"
        output_ws = output_ws.format(bank)
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         SpectrumNumbers=banks.get(bank))
        _save_out(run_number, focus_directory, focus_general, output_ws, "ENGINX_{}_texture_{}{}", bank)


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
    van_curves_ws, van_integrated_ws = load_van_files(van_curves, van_int)
    if do_pre_process:
        ws_to_focus = pre_process(_gen_filename(run_number), params, time_period)
    else:
        ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    return van_curves_ws, van_integrated_ws, ws_to_focus


def _save_out(run_number, focus_directory, focus_general, output, join_string, bank_id):
    """
    save out the files required for the focus

    @param run_number :: the run number of the focused run
    @param focus_directory :: the user directory to save to
    @param focus_general :: the general folder to copy the saved out files to
    @param output :: the workspace to save
    @param join_string :: the nameing scheme of the files
    @param bank_id :: the bank being saved

    """
    # work out where to save the files
    dat_name, genie_filename, gss_name, hdf5_name, nxs_name = _find_focus_file_location(bank_id, focus_directory,
                                                                                       join_string, run_number)
    if not u(bank_id).isnumeric():
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


def _find_focus_file_location(bank_id, focus_directory, join_string, run_number):

    run_and_bank = join_string.format(run_number, bank_id, "{}")
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
