# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.widgets.filefinderwidget import FileFinderWidget


def FileFinder(*args):
    from warnings import warn
    warn("The name FileFinder is deprecated, please use FileFinderWidget instead.")
    return FileFinderWidget(*args)
