# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass
from SANS.sans.common.enums import SaveType


@dataclass
class SaveOptions:
    can_sas_1d: bool = False
    nxs_can_sas: bool = False
    rkh: bool = False

    def to_all_states(self):
        """
        Converts into a format accepted by AllStates
        :return: A list of enum settings chosen
        """
        ret_val = []
        if self.can_sas_1d:
            ret_val.append(SaveType.CAN_SAS)
        if self.nxs_can_sas:
            ret_val.append(SaveType.NX_CAN_SAS)
        if self.rkh:
            ret_val.append(SaveType.RKH)
        if not ret_val:
            ret_val.append(SaveType.NO_TYPE)
        return ret_val
