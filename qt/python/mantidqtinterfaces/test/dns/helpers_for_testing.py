# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mantidqtinterfaces.dns.data_structures.object_dict import \
    ObjectDict

datasetdic = [{
    'filenumber': '774714',
    'det_rot': -7.01,
    'sample_rot': 123.0,
    'field': 'z7_sf',
    'temp_samp': 288.771,
    'sample': 'vana',
    'endtime': '2020-03-02 17:55:12',
    'tofchannels': 1,
    'channelwidth': 1.6,
    'filename': 'service_774714.d_dat',
    'wavelength': 4.74,
    'selector_speed': 7032.0,
    'scannumber': '14743',
    'scancommand': (
        "scan([det_rot, sample_rot], [-5.0000, 125.0000], "
        "[-0.5000, -0.5000], 10, tsf=120.0, field=['z7_sf', 'z7_nsf'],"
        " tnsf=240.0)"),
    'scanpoints': '20',
    'new_format': True
}, {
    'filenumber': '787463',
    'det_rot': -8.0,
    'sample_rot': 169.0,
    'field': 'z7_sf',
    'temp_samp': 4.1,
    'sample': '4p1K_map',
    'endtime': '2020-03-12 05:47:15',
    'tofchannels': 1,
    'channelwidth': 2.0,
    'filename': 'service_787463.d_dat',
    'wavelength': 4.74,
    'selector_speed': 7030.0,
    'scannumber': '14932',
    'scancommand': (
        "scan([det_rot, sample_rot], [-8.0000, 127.0000], "
        "[0, 1.0000], 170, tsf=30.0, field=['z7_sf', 'z7_nsf'], tnsf=30.0)"),
    'scanpoints': '340',
    'new_format': True
}, {
    'filenumber': '788058',
    'det_rot': -9.0,
    'sample_rot': 295.0,
    'field': 'z7_nsf',
    'temp_samp': 4.1,
    'sample': '4p1K_map',
    'endtime': '2020-03-12 11:27:20',
    'tofchannels': 1000,
    'channelwidth': 1.6,
    'filename': 'service_788058.d_dat',
    'wavelength': 4.74,
    'selector_speed': 7032.0,
    'scannumber': '14933',
    'scancommand': (
        "scan([det_rot, sample_rot], [-9.0000, 126.0000], "
        "[0, 1.0000], 170, tsf=30.0, field=['z7_sf', 'z7_nsf'], tnsf=30.0)"),
    'scanpoints': '1',
    'new_format': True
}]

fselector_fulldat = {
    'full_data': [{
        'filenumber': 788058,
        'det_rot': -9.0,
        'sample_rot': 295.0,
        'field': 'z7_nsf',
        'temperature': 4.1,
        'samplename': '4p1K_map',
        'tofchannels': 1000,
        'channelwidth': 1.6,
        'filename': 'service_788058.d_dat',
        'wavelength': 47.400000000000006,
        'sampletype': '4p1K_map',
        'selector_speed': 7032.0
    }],
    'standard_data': [{
        'filenumber': 788058,
        'det_rot': -9.0,
        'sample_rot': 295.0,
        'field': 'x7_nsf',
        'temperature': 4.1,
        'samplename': '_empty',
        'tofchannels': 1000,
        'channelwidth': 1.6,
        'filename': 'test_empty.d_dat',
        'wavelength': 47.400000000000006,
        'sampletype': 'empty',
        'selector_speed': 7032.0
    }, {
        'filenumber': 788058,
        'det_rot': -9.04,
        'sample_rot': 295.0,
        'field': 'z7_nsf',
        'temperature': 4.1,
        'samplename': '_vana',
        'tofchannels': 1000,
        'channelwidth': 1.6,
        'filename': 'test_vana.d_dat',
        'wavelength': 47.400000000000006,
        'sampletype': 'vana',
        'selector_speed': 7032.0
    }, {
        'filenumber': 788059,
        'det_rot': -10.0,
        'sample_rot': 295.0,
        'field': 'z7_nsf',
        'temperature': 4.1,
        'samplename': '_vana',
        'tofchannels': 1000,
        'channelwidth': 1.6,
        'filename': 'test_vana.d_dat',
        'wavelength': 47.400000000000006,
        'sampletype': 'vana',
        'selector_speed': 7032.0
    }]
}


def get_filepath():
    return 'C:/123'


def get_paths():
    return {
        'ascii': True,
        'nexus': True,
        'export': False,
        'export_dir': '123',
        'data_dir': '123',
        'standards_dir': '123'
    }


def get_fselector_fulldat():
    return fselector_fulldat


# fake SC dataset with 2 scans for treeview tests
def get_dataset():
    newlist = []
    for elm in datasetdic[1:]:
        dataset = ObjectDict()
        for key, value in elm.items():
            dataset[key] = value
        newlist.append(dataset)
    return newlist


def dns_file(_dummy, filename):
    elm = [
        dataset for dataset in datasetdic if dataset['filename'] == filename
    ][0]
    dataset = ObjectDict()
    for key, value in elm.items():
        dataset[key] = value
    return dataset


def get_fake_param_dict():
    return {'file_selector': {'full_data': get_dataset()}}


def get_fake_empty_param_dict():
    return {'file_selector': {'full_data': []}}


def get_first_scan_command():
    return ("14932 4p1K_map scan([det_rot, sample_rot], "
            "[-8.0000, 127.0000], [0, 1.0000], 170, tsf=30.0, field=['z7_sf', "
            "'z7_nsf'], tnsf=30.0) #340")


def get_3filenames():
    return [x['filename'] for x in datasetdic]


def get_3filenumbers():
    return [x['filenumber'] for x in datasetdic]


def get_fake_tof_binning():  # matches data 1,2 above
    return {
        'dEmin': -3.1409856698897682,
        'dEmax': 3.1409856698897682,
        'dEstep': 0.07281971339779537,
        'qmax': 2.837342973507243,
        'qmin': 0.1849336923669811,
        'qstep': 0.025
    }


def get_fileselector_param_dict():
    return {
        'paths': {
            'data_dir': 'C:/data',
            'standards_dir': 'C:/stand',
        },
        'file_selector': {
            'selected_filenumbers': [796640, 796640]
        }
    }


def get_fake_tof_options():
    tof_opt = get_fake_tof_binning()
    tof_opt.update({
        'corrections': True,
        'substract_vana_back': True,
        'substract_sample_back': True,
        'det_efficency': True,
        'vanadium_temperature': 295,
        'vana_back_factor': 1,
        'sample_back_factor': 1,
        'epp_channel': 0,
        'wavelength': 4.74,
        'delete_raw': True,
        'norm_monitor': True,
        'correct_elastic_peak_position': True,
        'mask_bad_detectors': True
    })
    return tof_opt


def get_fake_elastic_sc_options():
    el_opt = {
        'a': 2,
        'b': 3,
        'c': 4,
        'alpha': 78,
        'beta': 86,
        'gamma': 85,
        'hkl1': '1,2,3',
        'hkl2': '2,3,4',
        'omega_offset': 0,
        'dx': 1,
        'dy': 2,
        'wavelength': 4.74
    }
    return el_opt


def get_fake_tof_errors():  # matches data 1,2 above
    return {
        'channelwidths': [2.0, 1.6],
        'chan_error': True,
        'tofchannels': [1, 1000],
        'tof_error': True
    }


def get_fake_elastic_datadic():
    return {
        'knso': {
            'path': 'C:/data',
            'x_nsf': range(554574, 554634, 6),
            'x_sf': range(554573, 554633, 6),
            'y_nsf': range(554576, 554636, 6),
            'y_sf': range(554575, 554635, 6),
            'z_nsf': range(554578, 554638, 6),
            'z_sf': range(554577, 554637, 6)
        },
    }


def get_fake_tof_datadic():
    return {
        'knso': {
            'path': 'C:/data',
            -6.0: [0, 1, 2, 3, 4, 5, 6, 7, 8],
            -5.0: [2, 3, 4]
        }
    }


def get_elastic_standard_datadic():
    return {
        'vana': {
            'path': 'C:/_knso_554573_to_554632_ip_vana',
            'z_nsf': range(10, 20, 1),
            'z_sf': range(0, 10, 1)
        },
        'nicr': {
            'path': 'C:/_knso_554573_to_554632_ip_nicr',
            'x_nsf': range(10, 20, 1),
            'x_sf': range(0, 10, 1),
            'y_nsf': range(30, 40, 1),
            'y_sf': range(20, 30, 1),
            'z_nsf': range(50, 60, 1),
            'z_sf': range(40, 50, 1)
        },
        'empty': {
            'path': 'C:/_knso_554573_to_554632_ip_empty',
            'x_nsf': range(10, 20, 1),
            'x_sf': range(0, 10, 1),
            'y_nsf': range(30, 40, 1),
            'y_sf': range(20, 30, 1),
            'z_nsf': range(50, 60, 1),
            'z_sf': range(40, 50, 1)
        },
    }


def get_fake_elastic_sc_dataset():
    return {
        'ttheta': [0, 1, 2],
        'omega': [4, 5],
        'intensity': np.transpose(
            np.asarray([[8.0, 9.0, 10.0], [11.0, 12.0, 13.0]])),
        'error': np.transpose(
            np.asarray([[14.0, 15.0, 16.0], [17.0, 18.0, 19.0]]))
    }
