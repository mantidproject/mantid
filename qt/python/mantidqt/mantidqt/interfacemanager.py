# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantidqt.resource_loader import register_resources
from mantidqt.utils.qt import import_qt

# register icon resources (:/...) used by the C++ widgets
register_resources()

InterfaceManager = import_qt("._common", "mantidqt", "InterfaceManager")
