# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#-------------------------------------------------------------------
# mantidplot.py
#
# Load 'pymantidplot' which cannot be called 'mantidplot' because of
# name conflict with the MantidPlot binary (especially on osx)
#-------------------------------------------------------------------
from __future__ import (absolute_import, division,
                        print_function)

import os.path as osp
import sys

import mantid
import pymantidplot
from pymantidplot import *

# and the old qtiplot stuff
import pymantidplot.qtiplot

# error early if PyQt4 cannot be used
import PyQt4

def load_ui(caller_filename, ui_relfilename, baseinstance=None):
    '''This is copied from mantidqt.utils.qt and should be deprecated as
    soon as possible.'''
    from qtpy.uic import loadUi, loadUiType  # noqa

    filepath = osp.join(osp.dirname(caller_filename), ui_relfilename)
    if not osp.exists(filepath):
        raise ImportError('File "{}" does not exist'.format(filepath))
    if not osp.isfile(filepath):
        raise ImportError('File "{}" is not a file'.format(filepath))
    if baseinstance is not None:
        return loadUi(filepath, baseinstance=baseinstance)
    else:
        return loadUiType(filepath)
