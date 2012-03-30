#!/usr/bin/env python
import os

from distutils.core import setup

# Compile resource file
try:
    
    # Common widgets
    os.system("pyuic4 -o ui/ui_reduction_main.py ui/reduction_main.ui")
    os.system("pyuic4 -o ui/ui_hfir_output.py ui/hfir_output.ui")
    os.system("pyuic4 -o ui/ui_instrument_dialog.py ui/instrument_dialog.ui")
    
    # REF - new
    #os.system("pyuic4 -o ui/reflectometer/ui_advanced.py ui/reflectometer/advanced.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_data_refl.py ui/reflectometer/data_refl.ui")
    os.system("pyuic4 -o ui/reflectometer/ui_data_refl_simple.py ui/reflectometer/data_refl_simple.ui")
    os.system("pyuic4 -o ui/reflectometer/ui_refm_reduction.py ui/reflectometer/refm_reduction.ui")
    os.system("pyuic4 -o ui/reflectometer/ui_refl_stitching.py ui/reflectometer/refl_stitching.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_merging_reflm.py ui/reflectometer/merging_refl.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_merging_reflm.py ui/reflectometer/merging_refm.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_norm_refl.py ui/reflectometer/norm_refl.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_parameters_refl.py ui/reflectometer/parameters_refl.ui")
    #os.system("pyuic4 -o ui/reflectometer/ui_parameters_refm.py ui/reflectometer/parameters_refm.ui")
    
    # Example
    #os.system("pyuic4 -o ui/ui_example.py ui/example.ui")
except:
    print "Could not compile resource file"