# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.dataobjects import Workspace2D

from dataclasses import dataclass
import numpy as np


@dataclass
class Node:
    name: str
    component_index: int
    children: list["Node"]


class ComponentTreeModel:
    def __init__(self, workspace: Workspace2D) -> None:
        self._component_info = workspace.componentInfo()
        self.tree = self._create_tree()

    def _create_tree(self) -> Node:
        root = int(self._component_info.root())
        children = self._create_children(root)
        return Node(self._component_info.name(root), root, children)

    def _create_children(self, component_index: int) -> list[Node]:
        children = self._component_info.children(component_index)
        children_nodes = [Node(self._component_info.name(int(child)), int(child), self._create_children(int(child))) for child in children]
        return children_nodes

    def get_all_sub_component_indices(self, component_indices: list[int]) -> np.ndarray:
        if len(component_indices) == 0:
            return np.array([], dtype=int)
        return np.concatenate([self._component_info.componentsInSubtree(idx) for idx in component_indices], dtype=int)
