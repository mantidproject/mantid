# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores a single DNS datafile in a dictionary.
"""

import re
from dateutil.parser import parse
import numpy as np
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import load_txt


class DNSFile(ObjectDict):
    """
    Class for reading and storing data of a single DNS datafile.
    This is a dictionary, but can also be accessed like attributes.
    """

    def __init__(self, data_path, filename, dns_polarisation_table):
        super().__init__()
        self.new_format = False
        self.legacy_format = False
        self.polarisation_table = dns_polarisation_table
        self["users"] = ""
        self["sample"] = ""
        self["scan_number"] = ""
        self["scan_command"] = ""
        self["scan_position"] = ""
        self["scan_points"] = ""
        self["selector_speed"] = float(0.0)
        self.read_dns_file(data_path, filename)

    def read_dns_file(self, data_path, filename):
        self["filename"] = filename
        txt = load_txt(filename, data_path)
        block_splitter = "#--------------------------------------------------------------------------"
        blocks = txt.split(block_splitter)
        number_blocks = len(blocks)

        if number_blocks == 9:
            self.legacy_format = True
        elif number_blocks == 10:
            self.new_format = True
        else:
            raise RuntimeError("The file %s is not complete or has an unknown structure!" % filename)

        # block 0: header
        block_0 = blocks[0]
        # if header does not start with # DNS: raise Exception "wrong file format"
        if not block_0.startswith("# DNS"):
            raise RuntimeError("The file %s does not contain valid DNS data format." % filename)
        header_line_dict = parse_header(block_0)
        # try to parse parameters, perform nothing if not successful: sample and userid may be empty
        if "userid" in header_line_dict:
            self["users"] = header_line_dict["userid"]
        self["proposal"] = header_line_dict["exp"]
        self["file_number"] = header_line_dict["file"]
        if "sample" in header_line_dict:
            self["sample"] = header_line_dict["sample"].strip()

        # block 2: monochromator
        block_2 = [s.strip() for s in blocks[2].split("#")]
        info_line = block_2[2].split()
        # monochromator angle in degrees
        self["mon_rot"] = float(info_line[2])
        # wavelength in Angstrom
        self["wavelength"] = float(info_line[3]) * 10.0
        # energy in meV
        self["energy"] = float(info_line[4])
        # speed in m/s
        self["speed"] = float(info_line[5])

        # block 3: motors position
        block_3 = [s.strip() for s in blocks[3].split("#")]
        self["mon_rot"] = float(block_3[2].split()[1])
        # rotation angle of detector bank
        self["det_rot"] = float(block_3[3].split()[1])
        # Huber angle (default units degree)
        self["sample_rot"] = float(block_3[5].split()[1])

        # block 4: B-fields
        block_4 = [s.strip() for s in blocks[4].split("#")]
        self["flipper_precession_current"] = float(block_4[2].split()[1])
        self["flipper_z_compensation_current"] = float(block_4[3].split()[1])
        self["a_coil_current"] = float(block_4[4].split()[1])
        self["b_coil_current"] = float(block_4[5].split()[1])
        self["c_coil_current"] = float(block_4[6].split()[1])
        self["z_coil_current"] = float(block_4[7].split()[1])

        # block 5: temperatures
        # assume: T1=cold_head_temperature, T2=sample_temperature
        block_5 = [s.strip() for s in blocks[5].split("#")]
        self["temp_sample"] = float(block_5[3].split()[1])

        # block 6: TOF parameters
        block_6 = [s.strip() for s in blocks[6].split("#")]
        self["tof_channels"] = int(block_6[2].split()[2])
        self["channel_width"] = float(block_6[3].split()[3])
        self["tof_delay"] = float(block_6[4].split()[2])

        # block 7: time and monitor
        block_7 = [s.strip() for s in blocks[7].split("#")]
        # duration, timer value is assumed to be provided in seconds
        timer_line = block_7[2].split()
        self["timer"] = float(timer_line[1])
        # monitor data
        monitor_line = block_7[3].split()
        self["monitor"] = int(monitor_line[1])
        # for transition period data some other timer can be used
        if "timer" in block_7[4]:
            timer_line = block_7[4].split()
            self["timer"] = float(timer_line[1])
        # start_time and end_time (if specified)
        time_format = "%Y-%m-%dT%H:%M:%S"
        try:
            self["start_time"] = parse(block_7[5][10:].strip()).strftime(time_format)
            self["end_time"] = parse(block_7[6][10:].strip()).strftime(time_format)
        except ValueError:
            # if start and end time are not given, let them empty
            pass

        # legacy format, "Extended data" block is not provided
        if number_blocks == 9:
            self["field"] = self.determine_polarisation(self.polarisation_table)
            block_data = [s.strip() for s in blocks[8].split("#")]

        if number_blocks == 10:
            block_8 = [s.strip() for s in blocks[8].split("#")]
            scan_number_line = block_8[2].split()
            scan_command_line = block_8[3].split()
            scan_position_line = block_8[4].split()
            if len(scan_number_line) > 1:
                self["scan_number"] = scan_number_line[1]
            if len(scan_command_line) > 1:
                self["scan_command"] = " ".join(scan_command_line[1:])
            if len(scan_position_line) > 1:
                self["scan_position"] = scan_position_line[1]
            if "/" in self["scan_position"]:
                self["scan_points"] = self["scan_position"].split("/")[1]
            self["field"] = block_8[7].split()[1]
            self["selector_speed"] = float(block_8[9].split()[1])
            block_data = [s.strip() for s in blocks[9].split("#")]

        # last block: data
        channels = block_data[2].splitlines()[1:]
        self["counts"] = np.zeros((24, self["tof_channels"]), dtype=int)
        for ch in range(24):
            self["counts"][ch, :] = channels[ch].split()[1:]
        del txt

    def determine_polarisation(self, polarisation_table, tolerance=0.1):
        polarisation = "unknown"
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
        # this part is needed when polarisation table is loaded from an xml file
        table_element = polarisation_table[0]
        if isinstance(table_element, str):
            polarisation_table = self.format_xml_input_table(polarisation_table)

        for row in polarisation_table:
            if self.currents_match(row, coil_currents, tolerance):
                return row["polarisation"] + "_" + flip_flag
        return polarisation

    def currents_match(self, dict1, dict2, tolerance):
        keys = ["C_a", "C_b", "C_c", "C_z"]
        for key in keys:
            if np.fabs(dict1[key] - dict2[key]) > tolerance:
                return False
        return True

    def format_xml_input_table(self, table):
        formatted_table = [",".join(x) for x in zip(table[0::5], table[1::5], table[2::5], table[3::5], table[4::5])]
        stripped_table = [eval(x) for x in formatted_table]
        return stripped_table


def parse_header(header):
    """
    Parses the header string and returns the parsed dictionary.
    """
    header_dict = {}
    regexp = re.compile(r"(\w+)=([^,|\n]+)")
    result = regexp.finditer(header)
    for res in result:
        header_dict[res.groups()[0]] = res.groups()[1]
    return header_dict
