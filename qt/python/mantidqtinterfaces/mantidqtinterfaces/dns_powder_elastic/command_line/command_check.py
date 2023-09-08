# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict


class CommandLineReader(ObjectDict):
    """
    Allows running data reduction with command line arguments
    accepts the same syntax as dnsplot.
    """

    def __init__(self):
        super().__init__()
        self.files = []

    def read(self, cla):
        self.files = []
        path = os.path.abspath(os.path.dirname(cla[0]))
        for index, command in enumerate(cla):
            if command == "-files" and len(cla) >= index + 7:
                ffnmb, lfnmb, oof = self._parse_file_command(cla[index : index + 7], path)
                self.files.append({"path": path, "ffnmb": ffnmb, "lfnmb": lfnmb, "oof": oof})
            if command == "-new" and len(cla) >= index + 3:
                # new give only number and oof
                ffnmb, lfnmb, oof = cla[index + 1 : index + 4]
                self.files.append({"path": path, "ffnmb": ffnmb, "lfnmb": lfnmb, "oof": oof})
            if command[1:] in ["dx", "dy", "oof"] and len(cla) >= index + 1:
                self[command[1:]] = float(cla[index + 1])
            elif command[1:] in ["nx", "ny", "cz"] and len(cla) >= index + 1:
                self[command[1:]] = cla[index + 1]
            elif command[1:] in ["fr", "v", "b", "powder", "sep-nonmag", "xyz", "tof"]:
                self[command[1:]] = True
        self["omega_offset"] = self["omega_offset"] = self.files[0].get("oof", 0)
        self["hkl1"] = self.pop("nx", "1,0,0")
        self["hkl2"] = self.pop("ny", "0,1,0")
        self["det_efficiency"] = self.pop("v", False)
        self["flipping_ratio"] = self.pop("fr", False)
        self["separation_xyz"] = self.pop("xyz", False)
        self["separation_coh_inc"] = self.pop("sep-nonmag", False)

    def _parse_old_filenumbers(self, ffnmb, prefix, postfix, path):
        first_file_n = prefix + ffnmb + postfix
        first_file = DNSFile(path, first_file_n)
        real_fn = first_file["file_number"]
        pre, post = first_file_n.split(real_fn)
        fn_part_prefix = prefix[len(pre) :]
        fn_part_postfix = postfix[: -len(post)]
        return [fn_part_prefix, fn_part_postfix]

    def _get_fix_part_fnb(self, ffnmb, prefix, postfix, path):
        first_file_n = prefix + ffnmb + postfix
        if os.path.isfile(os.path.join(path, first_file_n)):
            return self._parse_old_filenumbers(ffnmb, prefix, postfix, path)
        return [prefix.split("_")[-1], postfix.split(".d_dat")[0]]

    def _parse_file_command(self, command, path):
        if not command[0] == "-files":
            return [None, None, 0, ""]
        arguments = command[1:]
        if len(arguments) < 6:
            return [None, None, 0, ""]
        prefix = arguments[0]
        oof = arguments[1]
        # inc = arguments[2]
        ffnmb = arguments[3]
        lfnmb = arguments[4]
        postfix = arguments[5]
        fn_pre, fn_post = self._get_fix_part_fnb(ffnmb, prefix, postfix, path)
        ffnmb = fn_pre + ffnmb + fn_post
        lfnmb = fn_pre + lfnmb + fn_post
        return [ffnmb, lfnmb, oof]
