#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from compile_util import compile_ui

# Compile resource files for Diffraction instruments
try:
    compile_ui("ui/diffraction/diffraction_adv_setup.ui")
    compile_ui("ui/diffraction/diffraction_filter_setup.ui")
    compile_ui("ui/diffraction/diffraction_info.ui")
    compile_ui("ui/diffraction/diffraction_run_setup.ui")
    compile_ui("ui/diffraction/filter_info.ui")
except:
    print("Could not compile resource file")
