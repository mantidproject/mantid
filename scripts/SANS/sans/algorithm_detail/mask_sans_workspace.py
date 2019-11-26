# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from sans.algorithm_detail.mask_workspace import create_masker
from sans.common.enums import DetectorType
from sans.common.general_functions import append_to_sans_file_tag


def mask_workspace(state, component_as_string, workspace):
    assert (state is not dict)
    component = DetectorType(component_as_string)

    # Get the correct SANS masking strategy from create_masker
    masker = create_masker(state, component)

    # Perform the masking
    mask_info = state.mask
    workspace = masker.mask_workspace(mask_info, workspace, component)

    append_to_sans_file_tag(workspace, "_masked")
    return workspace
