# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np


def detector_component_indices_in_subtrees(component_indices: np.ndarray, component_info) -> list:
    """
    Return unique subtrees of the provided indices
    """
    if len(component_indices) == 0:
        return []
    seen_parents: set = set()
    result = []
    for cidx in component_indices:
        parent = component_info.parent(int(cidx))

        # Skip if already got that subtree
        if parent in seen_parents:
            continue

        seen_parents.add(parent)
        result.append(component_info.detectorsInSubtree(parent))
    return result


def detector_table_indices_for_parent_subtrees(
    selected_indices: np.ndarray,
    component_idxs: np.ndarray,
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

    expanded_array = np.concatenate(detector_component_indices_in_subtrees(component_idxs[selected_indices], component_info))

    if pickable_mask is None:
        return expanded_array
    return expanded_array[pickable_mask[expanded_array]]


def reflect_points_in_axis(points: np.ndarray, axis: np.ndarray) -> np.ndarray:
    """Return points reflected across a plane perpendicular to the given axis."""
    return points - 2 * (points @ axis)[..., np.newaxis] * axis


def get_beam_axis(workspace) -> np.ndarray:
    """Return the beam axis vector for the given workspace."""
    beam_axis = workspace.getInstrument().getReferenceFrame().vecPointingAlongBeam()
    beam_axis_norm = np.linalg.norm(beam_axis)
    if beam_axis_norm == 0.0:
        raise ValueError("Beam axis vector cannot be the zero vector.")
    return beam_axis / beam_axis_norm
