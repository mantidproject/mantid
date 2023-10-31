# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores a single DNS datafile in a dictionary.
"""

import numpy as np
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import load_txt


class DNSFile(ObjectDict):
    """
    Class for reading, writing and storing data of a single DNS datafile.
    This is a dictionary, but can also be accessed like attributes.
    """

    def __init__(self, data_path, filename, dns_polarisation_table=[]):
        super().__init__()
        self.new_format = False
        self.legacy_format = False
        self.polarisation_table = dns_polarisation_table
        self.read(data_path, filename)

    def read(self, data_path, filename):
        txt = load_txt(filename, data_path)
        if txt[0].startswith("# DNS Data") and len(txt) == 138:
            self.new_format = True
            self.read_new_format(filename, txt)
        elif txt[0].startswith("# DNS Data") and len(txt) == 128:
            self.legacy_format = True
            # my_table = self.get_dns_legacy_polarisation_table()
            self.read_legacy_format(filename, txt)
        del txt

    def read_new_format(self, filename, txt):
        self["filename"] = filename
        line = txt[0]
        line = line.split("userid=")[1].split(",exp=")
        self["users"] = line[0]
        line = line[1].split(",file=")
        self["proposal"] = line[0]
        line = line[1].split(",sample=")
        self["file_number"] = line[0]
        self["sample"] = line[1][:-1]
        line = txt[7].split()
        self["mon_rot"] = float(line[3])
        self["wavelength"] = float(line[4]) * 10
        self["energy"] = float(line[5])
        self["speed"] = float(line[6])
        self["mon_rot"] = float(txt[12][25:-5])
        self["det_rot"] = float(txt[13][25:-5])
        self["sample_rot"] = float(txt[15][25:-5])
        self["flipper_precession_current"] = float(txt[35][25:-16])
        self["flipper_z_compensation_current"] = float(txt[36][27:-16])
        self["a_coil_current"] = float(txt[37][25:-16])
        self["b_coil_current"] = float(txt[38][25:-16])
        self["c_coil_current"] = float(txt[39][25:-16])
        self["z_coil_current"] = float(txt[40][25:-16])
        self["temp_sample"] = float(txt[44][25:-3])
        self["tof_channels"] = int(txt[48][25:-1])
        self["channel_width"] = float(txt[49][25:-11])
        self["tof_delay"] = float(txt[50][25:-11])
        self["timer"] = float(txt[56][15:-5])
        self["monitor"] = int(txt[57][15:-1])
        self["start_time"] = txt[59][21:-1]
        self["end_time"] = txt[60][21:-1]
        self["scan_number"] = txt[63][15:-1].strip()
        self["scan_command"] = txt[64][28:-1]
        self["scan_position"] = txt[65][15:-1].strip()
        self["field"] = txt[68][10:-1].strip()
        self["selector_speed"] = float(txt[70][17:-4])
        if "/" in self["scan_position"]:
            self["scan_points"] = self["scan_position"].split("/")[1]
        else:
            self["scan_points"] = ""
        self["counts"] = np.zeros((24, self["tof_channels"]), dtype=int)
        for ch in range(24):
            self["counts"][ch, :] = txt[74 + ch].split()[1:]

    def read_legacy_format(self, filename, txt):
        self["filename"] = filename
        line = txt[0]
        line = line.split("userid=")[1].split(",exp=")
        self["users"] = line[0]
        line = line[1].split(",file=")
        self["proposal"] = line[0]
        line = line[1].split(",sample=")
        self["file_number"] = line[0]
        self["sample"] = line[1][:-1]
        line = txt[7].split()
        self["mon_rot"] = float(line[3])
        self["wavelength"] = float(line[4]) * 10
        self["energy"] = float(line[5])
        self["speed"] = float(line[6])
        self["mon_rot"] = float(txt[12][25:-5])
        self["det_rot"] = float(txt[13][25:-5])
        self["sample_rot"] = float(txt[15][25:-5])
        self["flipper_precession_current"] = float(txt[35][25:-16])
        self["flipper_z_compensation_current"] = float(txt[36][27:-16])
        self["a_coil_current"] = float(txt[37][25:-16])
        self["b_coil_current"] = float(txt[38][25:-16])
        self["c_coil_current"] = float(txt[39][25:-16])
        self["z_coil_current"] = float(txt[40][25:-16])
        self["temp_sample"] = float(txt[44][25:-3])
        self["tof_channels"] = int(txt[48][25:-1])
        self["channel_width"] = float(txt[49][25:-11])
        self["tof_delay"] = float(txt[50][25:-11])
        self["timer"] = float(txt[56][15:-5])
        self["monitor"] = int(txt[57][15:-1])
        self["start_time"] = txt[59][21:-1]
        self["end_time"] = txt[60][21:-1]
        self["scan_number"] = txt[63][15:-1].strip()
        self["scan_command"] = ""
        self["scan_points"] = ""
        self["selector_speed"] = float(0.0)

        self["field"] = self.determine_polarisation(self.polarisation_table)
        self["counts"] = np.zeros((24, self["tof_channels"]), dtype=int)  # for python 2 use long
        for ch in range(24):
            self["counts"][ch, :] = txt[64 + ch].split()[1:]

    def determine_polarisation(self, polarisation_table, tolerance=0.1):
        polarisation = []
        if abs(self.flipper_z_compensation_current) > tolerance:
            flip_flag = "sf"
        else:
            flip_flag = "nsf"
        coil_currents = {
            "C_a": self.a_coil_current,
            "C_b": self.b_coil_current,
            "C_c": self.c_coil_current,
            "C_z": self.z_coil_current,
        }
        for row in polarisation_table:
            if self.currents_match(row, coil_currents, tolerance):
                if row["polarisation"] not in ["zero_field", "off"]:
                    return row["polarisation"] + "_" + flip_flag
                else:
                    return row["polarisation"]
        return polarisation

    def currents_match(self, dict1, dict2, tolerance):
        keys = ["C_a", "C_b", "C_c", "C_z"]
        for key in keys:
            if np.fabs(dict1[key] - dict2[key]) > tolerance:
                return False
        return True
