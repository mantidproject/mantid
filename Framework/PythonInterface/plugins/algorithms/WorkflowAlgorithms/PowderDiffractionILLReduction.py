from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from mantid import config, mtd, logger
from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, \
    WorkspaceGroupProperty, FileAction, Progress, MatrixWorkspaceProperty
from mantid.simpleapi import *  # noqa


class PowderDiffractionILLReduction(PythonAlgorithm):

    _runs = []
    _calibration_file = None
    _normalise_option = None
    _region_of_interest = None
    _observable = None
    _sort_x_axis = None


    def category(self):
        return "ILL\\Diffraction"

    def summary(self):
        return 'Performs powder diffraction data reduction for ILL instrument D20.'

    def name(self):
        return "PowderDiffractionILLReduction"

    def validateInputs(self):
        return dict()

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run (s).')

        self.declareProperty(FileProperty('CalibrationFile', '',
                                          action=FileAction.OptionalLoad, extensions=['.nxs']),
                             doc='File containing the detector efficiencies.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Time',
                             validator=StringListValidator(['Time', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts.')

        self.declareProperty(name='ROI', defaultValue='', doc='Region of interest.')

        self.declareProperty(name='Observable',
                             defaultValue='sample.temperature',
                             doc='Scanning observable, a Sample Log entry\n')

        self.declareProperty(name='SortXAxis',
                             defaultValue=False,
                             doc='Whether or not to sort the x-axis\n')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):
        pass

#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffractionILLReduction)
