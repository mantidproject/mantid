# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import BinMD, FakeMDEventData, CreateMDWorkspace

from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict

dataset_dic = [
    {
        "file_number": "774714",
        "det_rot": -7.01,
        "sample_rot": 123.0,
        "field": "z7_sf",
        "temp_sample": 288.771,
        "sample": "vana",
        "end_time": "2020-03-02 17:55:12",
        "tof_channels": 1,
        "channel_width": 1.6,
        "filename": "service_774714.d_dat",
        "wavelength": 4.74,
        "selector_speed": 7032.0,
        "scan_number": "14743",
        "scan_command": (
            "scan([det_rot, sample_rot], [-5.0000, 125.0000], [-0.5000, -0.5000], 10, tsf=120.0, field=['z7_sf', 'z7_nsf'], tnsf=240.0)"
        ),
        "scan_points": "20",
        "new_format": True,
    },
    {
        "file_number": "787463",
        "det_rot": -8.0,
        "sample_rot": 169.0,
        "field": "z7_sf",
        "temp_sample": 4.1,
        "sample": "4p1K_map",
        "end_time": "2020-03-12 05:47:15",
        "tof_channels": 1,
        "channel_width": 2.0,
        "filename": "service_787463.d_dat",
        "wavelength": 4.74,
        "selector_speed": 7030.0,
        "scan_number": "14932",
        "scan_command": (
            "scan([det_rot, sample_rot], [-8.0000, 127.0000], [0, 1.0000], 170, tsf=30.0, field=['z7_sf', 'z7_nsf'], tnsf=30.0)"
        ),
        "scan_points": "340",
        "new_format": True,
    },
    {
        "file_number": "788058",
        "det_rot": -9.0,
        "sample_rot": 295.0,
        "field": "z7_nsf",
        "temp_sample": 4.1,
        "sample": "4p1K_map",
        "end_time": "2020-03-12 11:27:20",
        "tof_channels": 1000,
        "channel_width": 1.6,
        "filename": "service_788058.d_dat",
        "wavelength": 4.74,
        "selector_speed": 7032.0,
        "scan_number": "14933",
        "scan_command": (
            "scan([det_rot, sample_rot], [-9.0000, 126.0000], [0, 1.0000], 170, tsf=30.0, field=['z7_sf', 'z7_nsf'], tnsf=30.0)"
        ),
        "scan_points": "1",
        "new_format": True,
    },
]

file_selector_full_data = {
    "full_data": [
        {
            "file_number": 788058,
            "det_rot": -9.0,
            "sample_rot": 295.0,
            "field": "z7_nsf",
            "temperature": 4.1,
            "sample_name": "4p1K_map",
            "tof_channels": 1000,
            "channel_width": 1.6,
            "filename": "service_788058.d_dat",
            "wavelength": 47.400000000000006,
            "sample_type": "4p1K_map",
            "selector_speed": 7032.0,
        }
    ],
    "standard_data_tree_model": [
        {
            "file_number": 788058,
            "det_rot": -9.0,
            "sample_rot": 295.0,
            "field": "x7_nsf",
            "temperature": 4.1,
            "sample_name": "_empty",
            "tof_channels": 1000,
            "channel_width": 1.6,
            "filename": "test_empty.d_dat",
            "wavelength": 47.400000000000006,
            "sample_type": "empty",
            "selector_speed": 7032.0,
        },
        {
            "file_number": 788058,
            "det_rot": -9.04,
            "sample_rot": 295.0,
            "field": "z7_nsf",
            "temperature": 4.1,
            "sample_name": "_vana",
            "tof_channels": 1000,
            "channel_width": 1.6,
            "filename": "test_vana.d_dat",
            "wavelength": 47.400000000000006,
            "sample_type": "vana",
            "selector_speed": 7032.0,
        },
        {
            "file_number": 788059,
            "det_rot": -10.0,
            "sample_rot": 295.0,
            "field": "z7_nsf",
            "temperature": 4.1,
            "sample_name": "_vana",
            "tof_channels": 1000,
            "channel_width": 1.6,
            "filename": "test_vana.d_dat",
            "wavelength": 47.400000000000006,
            "sample_type": "vana",
            "selector_speed": 7032.0,
        },
    ],
}


def get_filepath():
    return "C:/123"


def get_paths():
    return {"ascii": True, "nexus": True, "export": False, "export_dir": "123", "data_dir": "123", "standards_dir": "123"}


def get_file_selector_full_data():
    return file_selector_full_data


# fake SC dataset with 2 scans for treeview tests
def get_dataset():
    new_list = []
    for elm in dataset_dic[1:]:
        dataset = ObjectDict()
        for key, value in elm.items():
            dataset[key] = value
        new_list.append(dataset)
    return new_list


def dns_file(_dummy, filename, _pol_table):
    elm = [dataset for dataset in dataset_dic if dataset["filename"] == filename][0]
    dataset = ObjectDict()
    for key, value in elm.items():
        dataset[key] = value
    return dataset


def get_fake_param_dict():
    return {"file_selector": {"full_data": get_dataset()}}


def get_fake_empty_param_dict():
    return {"file_selector": {"full_data": []}}


def get_first_scan_command():
    return (
        "14932 4p1K_map scan([det_rot, sample_rot], "
        "[-8.0000, 127.0000], [0, 1.0000], 170, tsf=30.0, field=['z7_sf', "
        "'z7_nsf'], tnsf=30.0) #340"
    )


def get_3_filenames():
    return [x["filename"] for x in dataset_dic]


def get_fake_tof_binning():  # matches data 1,2 above
    return {
        "dE_min": -3.1409856698897682,
        "dE_max": 3.1409856698897682,
        "dE_step": 0.07281971339779537,
        "q_max": 2.837342973507243,
        "q_min": 0.1849336923669811,
        "q_step": 0.025,
    }


def get_file_selector_param_dict():
    return {
        "paths": {
            "data_dir": "C:/data",
            "standards_dir": "C:/stand",
        },
        "file_selector": {"selected_file_numbers": [796640, 796640]},
    }


def get_fake_tof_options():
    tof_opt = get_fake_tof_binning()
    tof_opt.update(
        {
            "corrections": True,
            "subtract_vana_back": True,
            "subtract_sample_back": True,
            "det_efficiency": True,
            "vanadium_temperature": 295,
            "vana_back_factor": 1,
            "sample_back_factor": 1,
            "epp_channel": 0,
            "wavelength": 4.74,
            "delete_raw": True,
            "norm_monitor": True,
            "correct_elastic_peak_position": True,
            "mask_bad_detectors": True,
        }
    )
    return tof_opt


# OKcomment: not used anywhere
def get_fake_elastic_sc_options():
    el_opt = {
        "a": 2,
        "b": 3,
        "c": 4,
        "alpha": 78,
        "beta": 86,
        "gamma": 85,
        "hkl1": "1,2,3",
        "hkl2": "2,3,4",
        "omega_offset": 0,
        "dx": 1,
        "dy": 2,
        "wavelength": 4.74,
    }
    return el_opt


def get_fake_tof_errors():  # matches data 1,2 above
    return {"channel_widths": [2.0, 1.6], "chan_error": True, "tof_channels": [1, 1000], "tof_error": True}


def get_fake_tof_data_dic():
    return {"knso": {"path": "C:/data", -6.0: [0, 1, 2, 3, 4, 5, 6, 7, 8], -5.0: [2, 3, 4]}}


def get_fake_elastic_data_dic():
    return {
        "knso": {
            "path": "C:/data",
            "x_nsf": range(554574, 554634, 6),
            "x_sf": range(554573, 554633, 6),
            "y_nsf": range(554576, 554636, 6),
            "y_sf": range(554575, 554635, 6),
            "z_nsf": range(554578, 554638, 6),
            "z_sf": range(554577, 554637, 6),
        },
    }


def get_elastic_standard_data_dic():
    return {
        "vana": {"path": "C:/_knso_554573_to_554632_ip_vana", "z_nsf": range(10, 20, 1), "z_sf": range(0, 10, 1)},
        "nicr": {
            "path": "C:/_knso_554573_to_554632_ip_nicr",
            "x_nsf": range(10, 20, 1),
            "x_sf": range(0, 10, 1),
            "y_nsf": range(30, 40, 1),
            "y_sf": range(20, 30, 1),
            "z_nsf": range(50, 60, 1),
            "z_sf": range(40, 50, 1),
        },
        "empty": {
            "path": "C:/_knso_554573_to_554632_ip_empty",
            "x_nsf": range(10, 20, 1),
            "x_sf": range(0, 10, 1),
            "y_nsf": range(30, 40, 1),
            "y_sf": range(20, 30, 1),
            "z_nsf": range(50, 60, 1),
            "z_sf": range(40, 50, 1),
        },
    }


def get_fake_MD_workspace_unique(name="test", factor=1):
    ws = CreateMDWorkspace(
        Dimensions="3",
        EventType="MDEvent",
        Extents="0,150,-10,110,0,20",
        Names="Scattering Angle,Omega,TOF",
        Units="degree,degree,us",
        OutputWorkspace="test",
    )
    FakeMDEventData(ws, UniformParams=str(-15 * 12 * 2 * factor))
    bws = BinMD(
        InputWorkspace=ws,
        AlignedDim0="Scattering Angle,0,150,5",
        AlignedDim1="Omega,-10,110,4",
        AlignedDim2="TOF,0,20,1",
        OutputWorkspace=name,
    )
    return bws
