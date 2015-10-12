#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *
from mantid import config, logger
import os

class QLRun(PythonAlgorithm):

    def category(self):
        return "PythonAlgorithms"

    def summary(self):
        return "This algorithm runs the Fortran QLines programs which fits a Delta function of"
               " amplitude 0 and Lorentzians of amplitude A(j) and HWHM W(j) where j=1,2,3. The"
               " whole function is then convoled with the resolution function."

    def PyInit(self):
        self.declareProperty(name='program', defaultValue='QL',
                             validator=StringListValidator(['QL','QSe']),
                             doc='The type of program to run (either QL or QSe)')

        self.declareProperty(MatrixWorkspaceProperty('samWs', '', direction=Direction.Input),
                             doc='Name of the Sample input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('resWs', '', direction=Direction.Input),
                             doc='Name of the resolution input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('resNormWs', '',
                             optional=PropertyMod.Optional, direction=Direction.Input),
                             doc='Name of the ResNorm input Workspace')

        self.declareProperty(name='erange', '', validator=StringMandatoryValidator(),
                             doc='The range of the data to be fitted in the format of a python list [min,max]')

        self.declareProperty(name='nbins', '', validator=StringMandatoryValidator(),
                             doc='The number and type of binning to be used in the format of a python list'
                             ' [sampleBins,resolutionBins')

        self.declareProperty(name='Fit', '', validator=StringMandatoryValidator(),
                             doc='The features of the Fit in the form of a python list'
                             ' [elasticPeak(T/F),background,fixedWidth(T/F),useResNorm(T/F)')

        self.declareProperty(name='wfile' '', doc='The name of the fixedWidth file')

        self.declareProperty(name='Loop', defaultValue='True', doc='If the fit is sequential.')

        self.declareProperty(name='Plot', defaultValue='False', doc='If the result should be plotted.')

        self.declareProperty(name='Save', defaultValue='False', doc='If the result should be saved'
                             ' to the default save directory.')

