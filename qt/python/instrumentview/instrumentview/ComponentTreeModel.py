# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.dataobjects import Workspace2D

import numpy as np


class ComponentTreeModel:
    def __init__(self, workspace: Workspace2D) -> None:
        self._component_info = workspace.componentInfo()

    def root_index(self) -> int:
        return int(self._component_info.root())

    def root_name(self) -> str:
        return self._component_info.name(self.root_index())

    def get_children(self, component_index: int) -> list[tuple[str, int]]:
        children = self._component_info.children(component_index)
        return [(self._component_info.name(int(c)), int(c)) for c in children]

    def has_children(self, component_index: int) -> bool:
        return len(self._component_info.children(component_index)) > 0

    def get_all_sub_component_indices(self, component_indices: list[int]) -> np.ndarray:
        if len(component_indices) == 0:
            return np.array([], dtype=int)
        return np.concatenate([self._component_info.componentsInSubtree(idx) for idx in component_indices], dtype=int)
