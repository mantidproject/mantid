# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, invalid-name, bare-except, too-many-instance-attributes

# This file is just a shim as the module has moved to sans.common.
# This module needs investigating for general usage across isis sans scientist.
# If usage is necessary, it will need a warning to update to new path before removing the shim
from sans.common.isis_instrument import *  # noqa: F403
