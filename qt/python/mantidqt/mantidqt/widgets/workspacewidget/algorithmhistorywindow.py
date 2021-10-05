# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from mantidqt.utils.qt import import_qt


AlgorithmHistoryWindow = import_qt('..._common',
                                   'mantidqt.widgets.workspacewidget',
                                   'AlgorithmHistoryWindow')
