# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Shared helpers for resolving legacy-style component names against ComponentInfo.
"""


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
