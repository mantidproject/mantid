# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#

# flake8: noqa
from mantidqt.utils.qt import import_qt

AlgorithmProgressWidget = import_qt('.._common', package='mantidqt.widgets', attr='AlgorithmProgressWidget')
