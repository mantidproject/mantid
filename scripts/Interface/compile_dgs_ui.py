#!/usr/bin/env python
from __future__ import (absolute_import, division, print_function)
from compile_util import compile_ui

# Compile resource files for DGS instruments
try:
    compile_ui("ui/inelastic/dgs_sample_setup.ui")
    compile_ui("ui/inelastic/dgs_data_corrections.ui")
    compile_ui("ui/inelastic/dgs_diagnose_detectors.ui")
    compile_ui("ui/inelastic/dgs_absolute_units.ui")
    compile_ui("ui/inelastic/dgs_pd_sc_conversion.ui")
except:
    print("Could not compile resource file")
