#!/usr/bin/env python
import os

from distutils.core import setup

def make_cmd(ui_filename):
    pyuic_dir = os.path.dirname(ui_filename)
    pyuic_filename = "ui_%s.py" % os.path.basename(ui_filename).split('.')[0]
    return "pyuic4 -o %s/%s %s" % (pyuic_dir, pyuic_filename, ui_filename)

# Compile resource files for DGS instruments
try:
    os.system(make_cmd("ui/inelastic/dgs_sample_setup.ui"))
    os.system(make_cmd("ui/inelastic/dgs_data_corrections.ui"))

except:
    print "Could not compile resource file"