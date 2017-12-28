#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.simpleapi import ConjoinWorkspaces, CropWorkspace, RenameWorkspace
from mantid.kernel import *
import numpy as np


class CropWorkspaceRagged(PythonAlgorithm):

    __exts = None
    __loader = None

    def category(self): # TODO
        return "DataHandling\\Text"

    def name(self):
        return "CropWorkspaceRagged"

    def summary(self): # TODO
        return "This algorithm loads multiple gsas files from a single directory into mantid."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('InputWorkspace', '',
                                               direction=Direction.Input), 'input workspace') # TODO
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output), 'output workspace') # TODO
        self.declareProperty(FloatArrayProperty('Xmin'), 'minimum x values with NaN meaning no minimum')
        self.declareProperty(FloatArrayProperty('Xmax'), 'maximum x values with NaN meaning no maximum')

    def validateInputs(self):
        inputWS = self.getProperty('InputWorkspace').valueAsStr
        xmins = self.getProperty('XMin').value
        xmaxs = self.getProperty('XMax').value

        numSpec = mtd[inputWS].getNumberHistograms()
        numMin = len(xmins)
        numMax = len(xmaxs)

        errors = {}

        if numMin == 0:
            errors['XMin'] = 'Must specify minimums'
        elif numMin > 1 and numMin != numSpec:
            errors['XMin'] = 'Must specify min for each spectra ({}!={})'.format(numMin, numSpec)

        if numMin == 0:
            errors['XMax'] = 'Must specify maximums'
        elif numMax > 1 and numMax != numSpec:
            errors['XMax'] = 'Must specify max for each spectra ({}!={})'.format(numMax, numSpec)

        return errors

    def PyExec(self):
        inputWS = self.getProperty('InputWorkspace').value
        outputWS = self.getProperty('OutputWorkspace').valueAsStr
        xmins = self.getProperty('XMin').value
        xmaxs = self.getProperty('XMax').value

        if len(xmins) == 1 and len(xmaxs) == 1:
            CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=outputWS,
                          XMin=xmins[0], XMax=xmaxs[0])
        else:
            numSpec = inputWS.getNumberHistograms()

            if len(xmins) == 1:
                xmins = np.full((numSpec), xmins[0])
            if len(xmaxs) == 1:
                xmaxs = np.full((numSpec), xmaxs[0])

            # fix up the values of xmin/xmax
            indices = np.where(np.isnan(xmins))
            xmins[indices] = Property.EMPTY_DBL
            indices = np.where(np.isnan(xmaxs))
            xmaxs[indices] = Property.EMPTY_DBL

            self.log().information('MIN: ' + str(xmins))
            self.log().information('MAX: ' + str(xmaxs))

            names = ['spec_{}'.format(i) for i in range(len(xmins))]

            # crop out each spectra and conjoin to a temporary workspace
            accumulationWS = None
            for i, (name, xmin, xmax) in enumerate(zip(names, xmins, xmaxs)):
                # don't  go beyond the range of the data
                x = inputWS.readX(i)
                if xmin < x[0]:
                    xmin = Property.EMPTY_DBL
                if xmax > x[-1]:
                    xmax = Property.EMPTY_DBL

                # extract the range of the spectrum requested
                CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=name,
                              StartWorkspaceIndex=i, EndWorkspaceIndex=i, Xmin=xmin, XMax=xmax)

                # accumulate
                if accumulationWS is None:
                    accumulationWS = name
                else:
                    ConjoinWorkspaces(InputWorkspace1=accumulationWS,
                                      InputWorkspace2=name)

            RenameWorkspace(InputWorkspace=accumulationWS, OutputWorkspace=outputWS)

        # set the output property
        self.setProperty('OutputWorkspace', mtd[outputWS])


AlgorithmFactory.subscribe(CropWorkspaceRagged)
