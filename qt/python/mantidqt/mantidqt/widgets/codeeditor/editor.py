# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
# 3rd party imports

# local imports
from mantidqt.utils.qt import import_qt

CodeEditor = import_qt('..._common', 'mantidqt.widgets.codeeditor', 'ScriptEditor')
