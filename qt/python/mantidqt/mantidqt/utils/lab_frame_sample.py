# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantid.kernel import logger


def lab_frame_sample(workspace):
    """The sample where the goniometer puts it, or as defined if it cannot be turned. Any environment is as defined."""
    try:
        return workspace.getLabFrameSample()
    except ValueError as err:  # a CSG shape built from surfaces, e.g. SaveHKL's sphere, has no XML to turn
        logger.warning(f"Showing the sample as defined, without the goniometer rotation: {err}")
        return workspace.sample()
