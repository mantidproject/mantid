# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import Engineering.EngineeringCalibration as Cal
import Engineering.EngineeringFocus as Focus
import Engineering.EngineeringPreProcess as PreProcess
import os


# take the user, the run_number of the vanadium, and the directory to save to as arguments
def main(vanadium_run, user, focus_run, **kwargs):

    # Set all values at top,
    # get whether or not to force a vanadium, default value is false
    do_van = kwargs.get("force_vanadium", False)
    # get whether or not to pre-process the run before focusing, default is false
    do_pre_process = kwargs.get("pre_process_run", False)
    # get whether or not to force re-create the calibration, default is false
    do_cal = kwargs.get("force_cal", False)
    cropped = kwargs.get("crop_type", None)
    ceria_run = kwargs.get("ceria_run", "241391")
    crop_name = kwargs.get("crop_name", "cropped")
    crop_on = kwargs.get("crop_on", None)
    grouping_file = kwargs.get("grouping_file", None)
    if crop_on is None and cropped is not None:
        print("You must enter a value to crop_on if you enter a cropping method")
        return
    root = os.path.abspath(os.sep)
    if root == "/":
        path= os.path.expanduser("~/EnginX_Mantid")
    else:
        path= os.path.join(root, "EnginX_Mantid")
    directory = kwargs.get("directory", path)
    user_dir = os.path.join(directory, "User", user)
    calibration_directory = os.path.join(user_dir, "Calibration")
    calibration_general = os.path.join(directory, "Calibration")
    focus_directory = os.path.join(user_dir, "Focus")
    focus_general = os.path.join(directory, "Focus")

    run(calibration_directory, calibration_general, ceria_run, crop_name, crop_on, cropped, do_cal, do_pre_process,
        do_van, focus_directory, focus_general, focus_run, grouping_file, kwargs, vanadium_run)


# goes through and calls methods nessecary for this run
def run(calibration_directory, calibration_general, ceria_run, crop_name, crop_on, cropped, do_cal, do_pre_process,
        do_van, focus_directory, focus_general, focus_run, grouping_file, kwargs, van_run):

    vanadium = _gen_filename(van_run)
    if not os.path.isfile(vanadium) or do_van:
        create_vanadium(van_run, calibration_directory)

    cal_endings = {"banks": ["all_banks", "bank_{}".format(crop_on)],
                   "spectra": ["all_banks", "bank_{}".format(crop_name)],
                   None: ["all_banks", "bank_North", "bank_South"]}
    expected_cals = ["ENGINX_{0}_{1}_{2}.prm".format(van_run, ceria_run, i) for i in cal_endings.get(cropped)]
    expected_cals_present = [os.path.isfile(os.path.join(calibration_directory, cal_file)) for cal_file in
                             expected_cals]

    if not all(expected_cals_present) or do_cal:
        create_calibration(cropped, crop_on, crop_name, ceria_run, calibration_directory, van_run,
                           calibration_general)

    if do_pre_process:
        params = kwargs.get("params")
        time_period = kwargs.get("time_period", None)
        pre_process(params, time_period, focus_run)

    if focus_run is not None:
        focus(focus_run, grouping_file, crop_on, calibration_directory, focus_directory, focus_general, cropped, van_run)


# create the vanadium run for the run number set of the object
def create_vanadium(van_run, calibration_directory):
    van_file = _gen_filename(van_run)
    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)
    Cal.create_vanadium_workspaces(van_file, van_curves_file, van_int_file)


# create the calibration files from the vanadium run on the object, and the ceria run passed in or the default ceria
def create_calibration(cropped, crop_on, crop_name, ceria_run, calibration_directory, van_run, calibration_general):
    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)

    # Check if the calibration should be cropped, and if so what cropping method to use
    if cropped is not None:
        if cropped == "banks":
            Cal.create_calibration_cropped_file(False, crop_on, crop_name,
                                                van_curves_file, van_int_file, ceria_run,calibration_directory,
                                                van_run, calibration_general)
        elif cropped == "spectra":
            Cal.create_calibration_cropped_file(True, crop_on, crop_name,
                                                van_curves_file, van_int_file, ceria_run,calibration_directory,
                                                van_run, calibration_general)
    else:
        Cal.create_calibration_files(van_curves_file, van_int_file, ceria_run, calibration_directory, van_run,
                                     calibration_general)


# focus the run passed in useing the vanadium set on the object
def focus(run_no, grouping_file, crop_on,calibration_directory, focus_directory, focus_general, cropped, van_run):
    van_curves_file, van_int_file = _get_van_names(van_run, calibration_directory)
    # if a grouping file is passed in then focus in texture mode
    if grouping_file is not None:
        grouping_file = os.path.join(calibration_directory,grouping_file)
        Focus.focus_texture_mode(van_curves_file, van_int_file, run_no,
                                 focus_directory, grouping_file, focus_general)
    # if no texture mode was passed in check if the file should be cropped, and if so what cropping method to use
    elif cropped is not None:
        if cropped == "banks":
            Focus.focus_cropped(False, crop_on, van_curves_file, van_int_file, run_no,
                                focus_directory, focus_general)
        elif cropped == "spectra":
            Focus.focus_cropped(True, crop_on, van_curves_file, van_int_file, run_no,
                                focus_directory, focus_general)
    else:
        Focus.focus_whole(van_curves_file, van_int_file, run_no,
                          focus_directory, focus_general)


# pre_process the run passed in, usign the rebin parameters passed in
def pre_process(params, time_period, focus_run):
    # rebin based on pulse if a time period is sent in, otherwise just do a normal rebin
    if time_period is not None:
        PreProcess.rebin_pulse(focus_run, params, time_period)
    else:
        PreProcess.rebin_time(focus_run, params)


# generate the names of the vanadium workspaces from the vanadium set on the object
def _get_van_names(van_run, calibration_directory):
    van_file = _gen_filename(van_run)
    van_out = os.path.join(calibration_directory, van_file)
    van_int_file = van_out + "_precalculated_vanadium_run_integration.nxs"
    van_curves_file = van_out + "_precalculated_vanadium_run_bank_curves.nxs"
    return van_curves_file, van_int_file


# generate the file name based of just having a run number
def _gen_filename(run_number):
    return "ENGINX" + ("0" * (8 - len(run_number))) + run_number

