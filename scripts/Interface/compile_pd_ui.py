#!/usr/bin/env python
from compile_util import compile_ui

# Compile resource files for Diffraction instruments
try:
    compile_ui("ui/diffraction/diffraction_adv_setup.ui")
    compile_ui("ui/diffraction/diffraction_filter_setup.ui")
    compile_ui("ui/diffraction/diffraction_info.ui")
    compile_ui("ui/diffraction/diffraction_run_setup.ui")
    compile_ui("ui/diffraction/filter_info.ui")
except:
    print "Could not compile resource file"
