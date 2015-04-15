#pylint: disable=invalid-name
"""
    Script used to start the DGS reduction GUI from MantidPlot
"""
import os

from reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument_list=["PG3", "NOM", "VULCAN"])
if reducer.setup_layout(load_last=True): 
  
    # Set up reduction configuration from previous usage
    try:
        # Find home dir
        homedir = os.path.expanduser("~") 
        mantidconfigdir = os.path.join(homedir, ".mantid")
        autopath = os.path.join(mantidconfigdir, 'snspowderreduction.xml')
        # Load configuration 
        reducer.open_file(autopath)
    except IOError as e:
        print "[Error] Unable to load previously reduction setup from file %s.\nReason: %s." % (
                autopath, str(e))
    else:
        print "[Info] Load earlier reduction setup from auto-saved %s." % (autopath)

    # Show GUI
    reducer.show()
