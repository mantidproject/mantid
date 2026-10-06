# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Shared helpers for resolving legacy-style component names against ComponentInfo.
"""

from collections import deque

from mantid.geometry import ComponentType


def resolve_component_index(component, component_info):
    r"""Resolve a component name, which may be a bare name or a (partial) full
    path such as 'CORELLI/A row/bank1/sixteenpack', to its ComponentInfo index.

    Mirrors the walk performed by the legacy Instrument.getComponentByName: the first
    path segment is located anywhere in the instrument, then each remaining segment is
    located within the subtree of the previous one. This matters because an intermediate
    path segment (e.g. a bank that is just a positioning frame) can sit at a different
    position/rotation than the leaf component the full path actually identifies.

    @param str component: (partial) full name of the component assembly
    @param mantid.geometry.componentInfo component_info: object holding information for the instrument components
    @return int: component-info index
    """
    parts = component.split("/")
    index = component_info.indexOfAny(parts[0])
    for part in parts[1:]:
        try:
            index = next(
                int(c) for c in component_info.componentsInSubtree(index) if int(c) != index and component_info.name(int(c)) == part
            )
        except StopIteration as exc:
            raise ValueError(f"No component named '{part}' found within '{parts[0]}' while resolving '{component}'") from exc
    return index


def _find_bank_indices(component_info, is_bank) -> list[int]:
    """Breadth-first search for banks, mirroring the legacy Instrument::findDetectorsOfType:
    starting from the children of the root, the source, the sample and any component named
    'chopper-position', 'supermirror' or starting with 'slit' are skipped along with their
    subtrees, and the search does not descend into a bank once found. The legacy search also
    skipped monitors, but as they are leaves they can never be (or contain) a bank.

    @param mantid.geometry.ComponentInfo component_info: object holding information for the instrument components
    @param callable is_bank: predicate on a component index
    @return list[int]: component-info indices of the banks, in the legacy search order
    """
    skipped = set()
    if component_info.hasSource():
        skipped.add(component_info.source())
    if component_info.hasSample():
        skipped.add(component_info.sample())

    bank_indices = []
    queue = deque(int(child) for child in component_info.children(component_info.root()))
    while queue:
        index = queue.popleft()
        name = component_info.name(index)
        is_excluded = index in skipped or name in ("chopper-position", "supermirror") or name.startswith("slit")
        if not is_excluded:
            if is_bank(index):
                bank_indices.append(index)
            else:
                queue.extend(int(child) for child in component_info.children(index))
    return bank_indices


def is_rectangular_grid(component_info, index) -> bool:
    """Predicate for whether the component at index is a RectangularDetector grid."""
    return component_info.isGridDetector(index) and component_info.componentType(index) == ComponentType.Rectangular


def find_rectangular_detector_indices(component_info) -> list[int]:
    """Component indices of the RectangularDetector banks, in the order of the deprecated Instrument.findRectDetectors()."""
    return _find_bank_indices(component_info, lambda index: is_rectangular_grid(component_info, index))


def find_grid_detector_indices(component_info) -> list[int]:
    """Component indices of the GridDetector (including RectangularDetector) banks, in the order of the deprecated
    Instrument.findGridDetectors()."""
    return _find_bank_indices(component_info, component_info.isGridDetector)


def get_assembly_children(component_info, index) -> list[int]:
    """Child component indices of the assembly at index.
    Raises RuntimeError for a component that is not an assembly.
    """
    if component_info.componentType(index) in (ComponentType.Generic, ComponentType.Infinite, ComponentType.Detector):
        raise RuntimeError(f"Component {component_info.fullName(index)} is not an assembly")
    return [int(child) for child in component_info.children(index)]


def get_detector_id(component_info, detector_info, index):
    """Detector ID of the component at index, which must be a detector.
    Raises RuntimeError otherwise.
    """
    if not component_info.isDetector(index):
        raise RuntimeError(f"Component {component_info.fullName(index)} is not a detector")
    return detector_info.detid(index)


def get_spectrum_detector_index(spectrum_info, workspace_index):
    """Detector index of the first detector of a spectrum, which is the detector whose ID the deprecated
    MatrixWorkspace.getDetector(workspace_index).getID() returned, also for a grouped spectrum (the spectrum definition
    is sorted, and detector indices are in detector ID order). A detector index is also its component index.
    Raises RuntimeError for a spectrum without detectors, as getDetector did.
    """
    if not spectrum_info.hasDetectors(workspace_index):
        raise RuntimeError(f"No detectors found for workspace index {workspace_index}")
    return spectrum_info.getSpectrumDefinition(workspace_index)[0][0]
