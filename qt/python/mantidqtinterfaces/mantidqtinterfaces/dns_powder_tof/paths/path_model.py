# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Model for DNS path panel.
"""

import mantid.simpleapi as api
import glob
import os
from os.path import expanduser

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel


class DNSPathModel(DNSObsModel):
    @staticmethod
    def get_current_directory():
        return os.getcwd()

    def get_user_and_proposal_number(self, dir_name, load_polarisation_table):
        try:
            first_filename = next(glob.iglob(f"{dir_name}/*.d_dat"))
        except StopIteration:
            return ["", "", []]
        if load_polarisation_table:
            dns_polarisation_table = self.get_dns_legacy_polarisation_table()
            self.save_polarisation_table(dns_polarisation_table)
        else:
            dns_polarisation_table = self.get_preloaded_polarisation_table()
        dns_file = DNSFile("", first_filename, dns_polarisation_table)
        if dns_file["new_format"] or dns_file["legacy_format"]:
            return [dns_file["users"], dns_file["proposal"], dns_polarisation_table]
        return ["", "", []]

    @staticmethod
    def clear_cache(path):
        if path and os.path.isfile(path + "/last_filelist.txt"):
            os.remove(path + "/last_filelist.txt")

    @staticmethod
    def get_start_path_for_dialog(path):
        if path:
            return path
        return expanduser("~")

    @staticmethod
    def get_dns_legacy_polarisation_table():
        """
        Read the polarisation table from IDF.
        """
        polarisation_table = []
        tmp = api.LoadEmptyInstrument(InstrumentName="DNS")
        instrument = tmp.getInstrument()
        api.DeleteWorkspace(tmp)

        # polarisation names are taken from the IDF "DNS_parameters.xml"
        dns_polarisations_list = ["x", "y", "z", "off", "zero_field", "z_high", "minus_x", "minus_y", "minus_z"]

        for polarisation in dns_polarisations_list:
            currents = instrument.getStringParameter(f"{polarisation}_currents")[0].split(";")
            for current in currents:
                row = {"polarisation": f"{polarisation}"}
                row["C_a"], row["C_b"], row["C_c"], row["C_z"] = [float(c) for c in current.split(",")]
                polarisation_table.append(row)
        return polarisation_table

    def save_polarisation_table(self, table):
        self._polarisation_table = table

    def get_preloaded_polarisation_table(self):
        return self._polarisation_table
