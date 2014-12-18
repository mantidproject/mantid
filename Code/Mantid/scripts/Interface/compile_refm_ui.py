#!/usr/bin/env python
from compile_util import compile_ui
from distutils.core import setup

# Compile resource file
try:

    # Common widgets
    #compile_ui("ui/reduction_main.ui")
    #compile_ui("ui/hfir_output.ui")
    #compile_ui("ui/instrument_dialog.ui")

    # REF
    compile_ui("ui/reflectometer/refm_reduction.ui")
    compile_ui("ui/reflectometer/refl_stitching.ui")

except:
    print "Could not compile resource file"