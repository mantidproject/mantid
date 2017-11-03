from __future__ import (absolute_import, division, print_function)

import numpy as np
from mantid.kernel import Direction
from mantid.api import PythonAlgorithm, FileProperty, FileAction, MatrixWorkspaceProperty
from mantid.simpleapi import *

class PowderDiffILLCorrectionROC(PythonAlgorithm):

    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction;Diffraction\\Calibration"

    def summary(self):
        return "Performs radial oscilating collimator (ROC) correction calculation for powder diffraction instrument D20 at ILL."

    def name(self):
        return "PowderDiffILLCorrectionROC"

    def PyInit(self):
        self.declareProperty(FileProperty('RunWithoutROC', '', action=FileAction.Load, extensions=['nxs']),
                             doc='File path of the run without collimator.')

        self.declareProperty(FileProperty('RunWithROC', '', action=FileAction.Load, extensions=['nxs']),
                             doc='File path of the run with collimator.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Time', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts.')

        thetaRangeValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(FloatArrayProperty(name='ROI', values=[0, 153.6], validator=thetaRangeValidator),
                             doc='Regions of interest for normalisation [in scattering angle in degrees].')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the ROC corrections for each pixel.')

    def PyExec(self):
        run_no_roc = self.getPropertyValue('RunWithoutROC')
        run_roc = self.getPropertyValue('RunWithROC')
        output_ws = self.getPropertyValue('OutputWorkspace')
        no_roc_ws = LoadILLDiffraction(Filename=run_no_roc, StoreInADS=False)
        roc_ws = LoadILLDiffraction(Filename=run_roc, StoreInADS=False)
        corr = Divide(LHSWorkspace=no_roc_ws, RHSWorkspace=roc_ws, StoreInADS=False, EnableLogging=False)
        corr = ReplaceSpecialValues(InputWorkspace=corr, SmallNumberThreshold=1e-10, StoreInADS=False,
                                    NaNValue=0, InfinityValue=0, SmallNumberValue=0,
                                    NaNError=0, InfinityError=0, SmallNumberError=0)
        y = corr.extractY()
        good_bins = np.argwhere(y > 0)
        median = np.median(y[good_bins])
        self.log().information('The median ratio is '+str(median))
        corr = Scale(InputWorkspace=corr, Factor=1./median, StoreInADS=False)
        corr = ReplaceSpecialValues(InputWorkspace=corr, StoreInADS=False,
                                    SmallNumberThreshold=1e-10, SmallNumberValue=1., SmallNumberError=0.)
        corr = CropWorkspace(InputWorkspace=corr, StartWorkspaceIndex=1)
        self.setProperty('OutputWorkspace', corr)

# subscribe the algorithm with mantid
AlgorithmFactory.subscribe(PowderDiffILLCorrectionROC)
