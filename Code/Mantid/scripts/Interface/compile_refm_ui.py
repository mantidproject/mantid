#!/usr/bin/env python
import os

from distutils.core import setup

# Compile resource file
try:
    
    # Common widgets
    #os.system("pyuic4 -o ui/ui_reduction_main.py ui/reduction_main.ui")
    #os.system("pyuic4 -o ui/ui_hfir_output.py ui/hfir_output.ui")
    #os.system("pyuic4 -o ui/ui_instrument_dialog.py ui/instrument_dialog.ui")
    
    # REF
    os.system("pyuic4 -o ui/reflectometer/ui_refm_reduction.py ui/reflectometer/refm_reduction.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_refl_stitching.py ui/reflectometer/refl_stitching.ui")
    
except:
    print "Could not compile resource file"