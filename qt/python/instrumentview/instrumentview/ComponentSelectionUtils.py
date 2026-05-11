# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np


def detector_indices_in_component_subtrees(component_indices: list[int], component_info) -> np.ndarray:
    """Return indices of detectors that belong to the given component subtrees.

    The returned indices are concatenated in the same order as ``component_indices``.
    If no components are provided, an empty integer array is returned.
    """
    if len(component_indices) == 0:
        return np.array([], dtype=int)
    return np.concatenate([component_info.detectorsInSubtree(idx) for idx in component_indices], dtype=int)


def detector_table_indices_for_parent_subtrees(
    selected_indices: np.ndarray,
    detector_ids: np.ndarray,
    component_idxs: np.ndarray,
    detector_info,
    component_info,
    pickable_mask: np.ndarray | None = None,
) -> np.ndarray:
    """Expand detector-table indices to all detectors in each picked detector's parent subtree.

    For each detector-table index in ``detector_table_indices``, this finds the detector's
    parent component and includes all detectors returned by
    ``component_info.detectorsInSubtree(parent)``.

    Returns:
        Sorted array of expanded detector-table indices, optionally filtered by
        ``pickable_mask``.
    """
    if selected_indices.size == 0:
        return np.array([], dtype=int)

    expanded_indices: set[int] = set()
    for i, cidx in enumerate(component_idxs[selected_indices]):
        if not component_info.hasParent(int(cidx)):
            expanded_indices.add(int(selected_indices[i]))
            continue

        parent_index = component_info.parent(int(cidx))
        subtree_detector_indices = component_info.detectorsInSubtree(parent_index)

        sorter = np.argsort(component_idxs)
        indices = sorter[np.searchsorted(component_idxs, subtree_detector_indices, sorter=sorter)]
        expanded_indices.update(indices.flatten())

    expanded_array = np.array(sorted(expanded_indices), dtype=int)
    if pickable_mask is None:
        return expanded_array
    return expanded_array[pickable_mask[expanded_array]]
