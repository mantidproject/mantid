# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import Engineering.EnggUtils as Utils
import mantid.simpleapi as simple
from shutil import copy2
import os


def create_vanadium_workspaces(vanadium_run, curve_van, int_van):
    van_name = "eng_vanadium_ws"
    simple.Load(vanadium_run, OutputWorkspace=van_name)
    simple.EnggVanadiumCorrections(VanadiumWorkspace=van_name,
                                   OutIntegrationWorkspace="eng_vanadium_integration",
                                   OutCurvesWorkspace="eng_vanadium_curves")
    simple.SaveNexus("eng_vanadium_integration", int_van)
    simple.SaveNexus("eng_vanadium_curves", curve_van)
    simple.DeleteWorkspace(van_name)


def create_calibration_cropped_file(use_spectrum_number, spec_nos, crop_name, curve_van, int_van, ceria_run, cal_dir,
                                    van_run, cal_gen):
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX" + ceria_run, OutputWorkspace="eng_calib")
    param_tbl_name = None
    bank = None
    if use_spectrum_number:
        if crop_name is None:
            param_tbl_name = "cropped"
        else:
            param_tbl_name = crop_name
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=str(spec_nos),
                                      FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        difc = [output.DIFC]
        tzero = [output.TZERO]
        save_calibration(ceria_run, van_run, ".prm", cal_dir, [param_tbl_name], difc, tzero, "all_banks", cal_gen)
        save_calibration(ceria_run, van_run, ".prm", cal_dir, [param_tbl_name], difc, tzero,
                         "bank_{}".format(param_tbl_name), cal_gen)
    else:
        if spec_nos.lower() == "north":
            param_tbl_name = "engg_calibration_bank_1"
            bank = 1
        elif spec_nos.lower() == "south":
            param_tbl_name = "engg_calibration_bank_2"
            bank = 2
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, Bank=str(bank), FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        difc = [output.DIFC]
        tzero = [output.TZERO]
        save_calibration(ceria_run, van_run, ".prm", cal_dir, [spec_nos], difc, tzero, "all_banks",cal_gen)
        save_calibration(ceria_run, van_run, ".prm", cal_dir, [spec_nos], difc, tzero, "bank_{}".format(spec_nos),
                         cal_gen)
    create_params_table(difc, tzero)


def create_calibration_files(curve_van, int_van, ceria_run, cal_dir, van_run, cal_gen):
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX" + ceria_run, OutputWorkspace="eng_calib")
    difcs = []
    tzeros = []
    banks = 3
    bank_names = ["North", "South"]
    for i in range(1, banks):
        param_tbl_name = "engg_calibration_bank_{}".format(i)
        print(param_tbl_name)
        output = simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                      VanCurvesWorkspace=van_curves_ws, Bank=str(i), FittedPeaks=param_tbl_name,
                                      OutputParametersTableName=param_tbl_name)
        difcs.append(output.DIFC)
        tzeros.append(output.TZERO)
        save_calibration(ceria_run, van_run, ".prm", cal_dir, [bank_names[i-1]], [difcs[i-1]], [tzeros[i-1]],
                         "bank_{}".format(bank_names[i-1]), cal_gen)
    save_calibration(ceria_run, van_run, ".prm", cal_dir, bank_names, difcs, tzeros, "all_banks", cal_gen)
    create_params_table(difcs, tzeros)


def load_van_files(curves_van, ints_van):
    van_curves_ws = simple.Load(curves_van, OutputWorkspace="curves_van")
    van_integrated_ws = simple.Load(ints_van, OutputWorkspace="int_van")
    return van_curves_ws, van_integrated_ws


def save_calibration(ceria_run, van_run, ext, cal_dir, bank_names, difcs, zeros, name, cal_gen):
    gsas_iparm_fname = os.path.join(cal_dir, "ENGINX_"+van_run+"_"+ceria_run+"_"+name+ext)
    if name == "all_banks":
        template_file = None
    elif name == "bank_South":
        template_file = "template_ENGINX_241391_236516_South_bank.prm"
    else:
        template_file = "template_ENGINX_241391_236516_North_bank.prm"
    Utils.write_ENGINX_GSAS_iparam_file(output_file=gsas_iparm_fname, bank_names=bank_names, difc=difcs, tzero=zeros,
                                        ceria_run=ceria_run, vanadium_run=van_run,
                                        template_file=template_file)
    copy2(gsas_iparm_fname, cal_gen)


def create_params_table(difc, tzero):

    param_table = simple.CreateEmptyTableWorkspace(OutputWorkspace="engg_calibration_banks_parameters")
    param_table.addColumn("int", "bankid")
    param_table.addColumn("double", "difc")
    param_table.addColumn("double", "difa")
    param_table.addColumn("double", "tzero")
    for i in range(len(difc)):
        next_row = {"bankid": i, "difc": difc[i], "difa": 0, "tzero": tzero[i]}
        param_table.addRow(next_row)
