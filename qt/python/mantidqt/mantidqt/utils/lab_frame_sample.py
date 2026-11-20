# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantid.kernel import logger


def _turned_or_as_defined(turn, as_defined):
    try:
        return turn()
    except ValueError as err:  # a CSG shape built from surfaces, e.g. SaveHKL's sphere, has no XML to turn
        logger.warning(f"Showing the sample as defined, without the goniometer rotation: {err}")
        return as_defined()


def lab_frame_sample(workspace):
    """The sample and its goniometer-mounted environment parts where the goniometer puts them, or all as defined if any cannot be turned."""
    return _turned_or_as_defined(workspace.getLabFrameSample, workspace.sample)


def lab_frame_sample_shape(workspace):
    """The sample shape where the goniometer puts it, or as defined if it cannot be turned."""
    return _turned_or_as_defined(workspace.getLabFrameSampleShape, lambda: workspace.sample().getShape())
