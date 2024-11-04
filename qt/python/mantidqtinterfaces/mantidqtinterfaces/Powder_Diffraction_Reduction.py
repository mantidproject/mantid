# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Script used to start the DGS reduction GUI from MantidPlot
"""

import os
import sys

from mantidqtinterfaces.reduction_application import ReductionGUI

if "workbench" in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None
reducer = ReductionGUI(parent, flags, instrument_list=["PG3", "NOM", "VULCAN"])
if reducer.setup_layout(load_last=True):
    # Set up reduction configuration from previous usage
    try:
        # Find home dir
        homedir = os.path.expanduser("~")
        mantidconfigdir = os.path.join(homedir, ".mantid")
        autopath = os.path.join(mantidconfigdir, "snspowderreduction.xml")
        # Load configuration
        reducer.open_file(autopath)
    except IOError as e:
        print("[Error] Unable to load previously reduction setup from file %s.\nReason: %s." % (autopath, str(e)))
    else:
        print("[Info] Load earlier reduction setup from auto-saved %s." % (autopath))

    # Show GUI
    reducer.show()
