#!/usr/bin/env python
import os

from distutils.core import setup

# Compile resource file for HFIR SANS and EQSANS
try:

    # HFIR - new
    os.system("pyuic4 -o ui/sans/ui_hfir_background.py ui/sans/hfir_background.ui")
    os.system("pyuic4 -o ui/sans/ui_hfir_sample_data.py ui/sans/hfir_sample_data.ui")
    os.system("pyuic4 -o ui/sans/ui_hfir_instrument.py ui/sans/hfir_instrument.ui")
    os.system("pyuic4 -o ui/sans/ui_hfir_detector.py ui/sans/hfir_detector.ui")
    os.system("pyuic4 -o ui/sans/ui_trans_direct_beam.py ui/sans/trans_direct_beam.ui")
    os.system("pyuic4 -o ui/sans/ui_trans_spreader.py ui/sans/trans_spreader.ui")

    # EQSANS - new
    os.system("pyuic4 -o ui/sans/ui_eqsans_instrument.py ui/sans/eqsans_instrument.ui")
    os.system("pyuic4 -o ui/sans/ui_eqsans_sample_data.py ui/sans/eqsans_sample_data.ui")
    os.system("pyuic4 -o ui/sans/ui_eqsans_info.py ui/sans/eqsans_info.ui")

    os.system("pyuic4 -o ui/ui_cluster_status.py ui/cluster_status.ui")
    os.system("pyuic4 -o ui/ui_cluster_details_dialog.py ui/cluster_details_dialog.ui")
    os.system("pyuic4 -o ui/ui_reduction_main.py ui/reduction_main.ui")
    os.system("pyuic4 -o ui/ui_hfir_output.py ui/hfir_output.ui")
    os.system("pyuic4 -o ui/ui_trans_direct_beam.py ui/trans_direct_beam.ui")
    os.system("pyuic4 -o ui/ui_trans_spreader.py ui/trans_spreader.ui")
    os.system("pyuic4 -o ui/ui_instrument_dialog.py ui/instrument_dialog.ui")
    os.system("pyuic4 -o ui/ui_data_catalog.py ui/data_catalog.ui")
    os.system("pyuic4 -o ui/ui_stitcher.py ui/stitcher.ui")
except:
    print "Could not compile resource file"
