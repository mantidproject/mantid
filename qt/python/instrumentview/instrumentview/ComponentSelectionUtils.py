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
    detector_table_indices: np.ndarray,
    detector_ids: np.ndarray,
    detector_ids_by_info_index: np.ndarray,
    detector_id_to_table_index: dict[int, int],
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
    detector_table_indices = np.asarray(detector_table_indices, dtype=int)
    if detector_table_indices.size == 0:
        return detector_table_indices

    expanded_indices: set[int] = set()
    for table_index in detector_table_indices:
        detector_id = int(detector_ids[table_index])
        detector_info_index = detector_info.indexOf(detector_id)
        if not component_info.hasParent(detector_info_index):
            expanded_indices.add(int(table_index))
            continue

        parent_index = component_info.parent(detector_info_index)
        subtree_detector_indices = component_info.detectorsInSubtree(parent_index)
        subtree_detector_ids = detector_ids_by_info_index[subtree_detector_indices]
        for subtree_detector_id in subtree_detector_ids:
            mapped_index = detector_id_to_table_index.get(int(subtree_detector_id))
            if mapped_index is not None:
                expanded_indices.add(mapped_index)

    expanded_array = np.array(sorted(expanded_indices), dtype=int)
    if pickable_mask is None:
        return expanded_array
    return expanded_array[pickable_mask[expanded_array]]
