# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import import_qt

FittingMode = import_qt("..._common", "mantidqt.widgets.fitscriptgenerator", "FittingMode")

FitScriptGeneratorView = import_qt("..._common", "mantidqt.widgets.fitscriptgenerator", "FitScriptGeneratorView")
FitScriptGeneratorModel = import_qt("..._common", "mantidqt.widgets.fitscriptgenerator", "FitScriptGeneratorModel")
FitScriptGeneratorPresenter = import_qt("..._common", "mantidqt.widgets.fitscriptgenerator", "FitScriptGeneratorPresenter")
