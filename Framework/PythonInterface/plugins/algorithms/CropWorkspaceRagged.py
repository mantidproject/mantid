#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.simpleapi import ConjoinWorkspaces, CropWorkspace, DeleteWorkspace
from mantid.kernel import *
import numpy as np


class CropWorkspaceRagged(PythonAlgorithm):

    def category(self):
        return 'Transforms\\Splitting;Workflow'

    def seeAlso(self):
        return [ "CropWorkspace" ]

    def name(self):
        return 'CropWorkspaceRagged'

    def summary(self):
        return 'Crop each spectrum of a workspace independently'

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('InputWorkspace', '',
                                               direction=Direction.Input), 'input workspace')
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output), 'output workspace')
        self.declareProperty(FloatArrayProperty('XMin'), 'minimum x values with NaN meaning no minimum')
        self.declareProperty(FloatArrayProperty('XMax'), 'maximum x values with NaN meaning no maximum')

    def validateInputs(self):
        inputWS = self.getProperty('InputWorkspace').value
        xmins = self.getProperty('XMin').value
        xmaxs = self.getProperty('XMax').value

        numSpec = inputWS.getNumberHistograms()
        numMin = len(xmins)
        numMax = len(xmaxs)

        errors = {}

        if numMin == 0 and numMax == 0:
            errors['XMin'] = 'Must specify minimums or maximums'
            errors['XMax'] = 'Must specify minimums or maximums'

        if numMin > 1 and numMin != numSpec:
            errors['XMin'] = 'Must specify min for each spectra ({}!={})'.format(numMin, numSpec)

        if numMax > 1 and numMax != numSpec:
            errors['XMax'] = 'Must specify max for each spectra ({}!={})'.format(numMax, numSpec)

        return errors

    def PyExec(self):
        inputWS = self.getProperty('InputWorkspace').value
        outputWS = self.getProperty('OutputWorkspace').valueAsStr
        xmins = self.getProperty('XMin').value
        xmaxs = self.getProperty('XMax').value

        if len(xmins) == 1 and len(xmaxs) == 1:
            name = '__{}_cropped_'.format(outputWS)
            CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=name, XMin=xmins[0], XMax=xmaxs[0])
            self.setProperty('OutputWorkspace', mtd[name])
            DeleteWorkspace(name)
        else:
            numSpec = inputWS.getNumberHistograms()

            # fill out the values for min and max as appropriate
            # numpy 1.7 (on rhel7) doesn't have np.full
            if len(xmins) == 0:
                xmins = np.array([Property.EMPTY_DBL]*numSpec)
            elif len(xmins) == 1:
                xmins = np.array([xmins[0]]*numSpec)
            if len(xmaxs) == 0:
                xmaxs = np.array([Property.EMPTY_DBL]*numSpec)
            elif len(xmaxs) == 1:
                xmaxs = np.array([xmaxs[0]]*numSpec)

            # replace nan with EMPTY_DBL in xmin/xmax
            indices = np.where(np.invert(np.isfinite(xmins)))
            xmins[indices] = Property.EMPTY_DBL
            indices = np.where(np.invert(np.isfinite(xmaxs)))
            xmaxs[indices] = Property.EMPTY_DBL

            self.log().information('MIN: ' + str(xmins))
            self.log().information('MAX: ' + str(xmaxs))

            # temporary workspaces should be hidden
            names = ['__{}_spec_{}'.format(outputWS, i) for i in range(len(xmins))]

            # how much the progress bar moves forward for each spectrum
            progStep = float(1)/float(2*numSpec)

            # crop out each spectra and conjoin to a temporary workspace
            accumulationWS = None
            for i, (name, xmin, xmax) in enumerate(zip(names, xmins, xmaxs)):
                # don't  go beyond the range of the data
                x = inputWS.readX(i)
                if xmin < x[0]:
                    xmin = Property.EMPTY_DBL
                if xmax > x[-1]:
                    xmax = Property.EMPTY_DBL

                progStart = 2 * i * progStep

                # extract the range of the spectrum requested
                CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=name,
                              StartWorkspaceIndex=i, EndWorkspaceIndex=i, XMin=xmin, XMax=xmax,
                              startProgress=progStart, endProgress=(progStart + progStep),
                              EnableLogging=False)

                # accumulate
                if accumulationWS is None:
                    accumulationWS = name # messes up progress during very first step
                else:
                    ConjoinWorkspaces(InputWorkspace1=accumulationWS,
                                      InputWorkspace2=name,
                                      startProgress=(progStart + progStep), endProgress=(progStart + 2 * progStep),
                                      EnableLogging=False)
            self.setProperty('OutputWorkspace', mtd[accumulationWS])
            DeleteWorkspace(accumulationWS)


AlgorithmFactory.subscribe(CropWorkspaceRagged)
