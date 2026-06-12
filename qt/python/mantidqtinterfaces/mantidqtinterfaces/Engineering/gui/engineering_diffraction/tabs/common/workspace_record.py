# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from collections import OrderedDict
from typing import List, Dict
from mantid.api import MatrixWorkspace


class FittingWorkspaceRecord:
    """Record that maps the workspace the user has loaded to the derived background-subtracted workspace"""

    def __init__(self, **kwargs):
        self.loaded_ws = kwargs.get("loaded_ws", None)
        self.bgsub_ws_name = kwargs.get("bgsub_ws_name", None)
        self.bgsub_ws = kwargs.get("bgsub_ws", None)
        self.bg_params = kwargs.get("bg_params", [])

    def get_bg_active(self) -> bool:
        if self.bgsub_ws and self.bg_params[0]:
            # first element is isSub checkbox
            return True
        else:
            return False

    def get_active_ws(self) -> MatrixWorkspace | None:
        if self.get_bg_active():
            return self.bgsub_ws
        else:
            return self.loaded_ws

    def __setattr__(self, key: str, value: str | MatrixWorkspace | List):
        if key == "bg_params":
            if not isinstance(value, list):
                raise AttributeError("bg_params must be a list")

        self.__dict__[key] = value  # initialize self.key


class FittingWorkspaceRecordContainer:
    def __init__(self):
        self.dict = OrderedDict()

    def __getitem__(self, key: str) -> FittingWorkspaceRecord:
        return self.dict[key]

    def __len__(self) -> int:
        return len(self.dict)

    def __bool__(self) -> bool:
        return len(self.dict) > 0

    def get(self, key: str, default_value: FittingWorkspaceRecord | None) -> FittingWorkspaceRecord | None:
        return self.dict.get(key, default_value)

    def add(self, ws_name: str, **kwargs) -> None:
        self.dict[ws_name] = FittingWorkspaceRecord(**kwargs)

    def add_from_names_dict(self, names_dict: Dict) -> None:
        for key, value in names_dict.items():
            self.add(key, bgsub_ws_name=value[0], bg_params=value[1])

    def get_loaded_workpace_names(self) -> List[str]:
        return list(self.dict.keys())

    def get_bgsub_workpace_names(self) -> List[str]:
        return [w.bgsub_ws_name for w in self.dict.values()]

    # Set of methods that each return a two column dictionary for the various fields in FittingWorkspaceRecord
    def get_loaded_ws_dict(self) -> Dict[str, MatrixWorkspace | None]:
        return dict([(key, value.loaded_ws) for key, value in self.dict.items()])

    def get_bgsub_ws_dict(self) -> Dict[str, MatrixWorkspace | None]:
        return dict([(key, value.bgsub_ws) for key, value in self.dict.items()])

    def get_bgsub_ws_name_dict(self) -> Dict[str, str | None]:
        return dict([(key, value.bgsub_ws_name) for key, value in self.dict.items()])

    def get_bg_params_dict(self) -> Dict[str, List]:
        return dict([(key, value.bg_params) for key, value in self.dict.items()])

    def get_active_ws_name_list(self) -> List[str]:
        return [self.get_active_ws_name(key) for key, value in self.dict.items()]

    def get_active_ws_dict(self) -> Dict[str, MatrixWorkspace | None]:
        return dict([(self.get_active_ws_name(key), value.get_active_ws()) for key, value in self.dict.items()])

    def get_ws_names_dict(self) -> Dict[str, List[str | List]]:
        return dict([(key, [value.bgsub_ws_name, value.bg_params]) for key, value in self.dict.items()])

    def get_loaded_workspace_name_from_bgsub(self, bgsub_ws_name: str) -> str | None:
        return next((key for key, val in self.dict.items() if val.bgsub_ws_name == bgsub_ws_name), None)

    def get_active_ws_name(self, ws_name: str) -> str:
        ws_rec = self.dict[ws_name]
        if ws_rec.get_bg_active():
            return ws_rec.bgsub_ws_name
        else:
            return ws_name

    def rename(self, old_ws_name: str, new_ws_name: str) -> None:
        ws_loaded = self.dict.get(old_ws_name, None)
        if ws_loaded:
            self.dict[new_ws_name] = self.pop(old_ws_name)
        else:
            ws_loaded_name = self.get_loaded_workspace_name_from_bgsub(old_ws_name)
            if ws_loaded_name:
                self.dict[ws_loaded_name].bgsub_ws_name = new_ws_name

    def replace_workspace(self, name: str, workspace: MatrixWorkspace) -> None:
        ws_loaded = self.dict.get(name, None)
        if ws_loaded:
            self.dict[name].loaded_ws = workspace
        else:
            ws_loaded_name = self.get_loaded_workspace_name_from_bgsub(name)
            if ws_loaded_name:
                self.dict[ws_loaded_name].bgsub_ws = workspace

    def pop(self, ws_name: str) -> FittingWorkspaceRecord:
        return self.dict.pop(ws_name)

    def clear(self) -> None:
        self.dict.clear()
