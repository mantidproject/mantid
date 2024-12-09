# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, MatrixWorkspace, MatrixWorkspaceProperty
from mantid.kernel import Direction, FloatArrayProperty, Property
from mantid.simpleapi import ConjoinWorkspaces, Rebin, DeleteWorkspace, ExtractSpectra
import numpy as np


class RebinRagged(PythonAlgorithm):
    def category(self):
        return "Transforms\\Splitting"

    def seeAlso(self):
        return ["Rebin", "ResampleX"]

    def name(self):
        return "RebinRagged"

    def summary(self):
        return "Rebin each spectrum of a workspace independently. There is only one delta allowed per spectrum"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            "input workspace",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            "output workspace",
        )
        self.declareProperty(FloatArrayProperty("XMin"), "minimum x values with NaN meaning no minimum")
        self.declareProperty(FloatArrayProperty("XMax"), "maximum x values with NaN meaning no maximum")
        self.declareProperty(FloatArrayProperty("Delta"), "step parameter for rebin")
        self.declareProperty("PreserveEvents", True, "False converts event workspaces to histograms")

    def validateInputs(self):
        errors = {}

        inputWS = self.getProperty("InputWorkspace").value
        xmins = self.getProperty("XMin").value
        xmaxs = self.getProperty("XMax").value
        deltas = self.getProperty("Delta").value

        numMin = len(xmins)
        numMax = len(xmaxs)
        numDelta = len(deltas)

        if np.any(np.invert(np.isfinite(xmins))):
            errors["Delta"] = "All must be finite"
        elif np.any(deltas == 0.0):
            errors["Delta"] = "All must be nonzero"

        if numMin == 0 and numMax == 0:
            errors["XMin"] = "Must specify minimums or maximums"
            errors["XMax"] = "Must specify minimums or maximums"

        if isinstance(inputWS, MatrixWorkspace):
            numSpec = inputWS.getNumberHistograms()

            if numDelta == 0:
                errors["Delta"] = "Must specify binning"
            elif not (numDelta == 1 or numDelta == numSpec):
                errors["Delta"] = "Must specify for each spetra ({}!={})".format(numDelta, numSpec)

            if numMin > 1 and numMin != numSpec:
                errors["XMin"] = "Must specify min for each spectra ({}!={})".format(numMin, numSpec)

            if numMax > 1 and numMax != numSpec:
                errors["XMax"] = "Must specify max for each spectra ({}!={})".format(numMax, numSpec)
        else:
            errors["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace."

        return errors

    def __use_simple_rebin(self, xmins, xmaxs, deltas):
        if len(xmins) == 1 and len(xmaxs) == 1 and len(deltas) == 1:
            return True
        if len(xmins) == 0 or len(xmaxs) == 1 or len(deltas) == 0:
            return False
        # is there (effectively) only one xmin?
        if not (len(xmins) == 1 or np.all(xmins == xmins[0])):
            return False
        # is there (effectively) only one xmin?
        if not (len(xmaxs) == 1 or np.all(xmaxs == xmaxs[0])):
            return False
        # is there (effectively) only one xmin?
        if not (len(deltas) == 1 or np.all(deltas == deltas[0])):
            return False

        # all of these point to 'just do rebin'
        return True

    def __extend_value(self, numSpec, array, replaceNan):
        # change it to be the right length
        if len(array) == 0:
            array = np.full((numSpec,), Property.EMPTY_DBL)
        elif len(array) == 1:
            array = np.full((numSpec,), array[0])
        # replace nan with EMPTY_DBL
        indices = np.where(np.invert(np.isfinite(array)))
        array[indices] = Property.EMPTY_DBL

        return array

    def PyExec(self):
        inputWS = self.getProperty("InputWorkspace").value
        outputWS = self.getProperty("OutputWorkspace").valueAsStr
        xmins = self.getProperty("XMin").value
        xmaxs = self.getProperty("XMax").value
        deltas = self.getProperty("Delta").value
        preserveEvents = self.getProperty("PreserveEvents").value

        if self.__use_simple_rebin(xmins, xmaxs, deltas):
            # plain old rebin should have been used
            name = "__{}_rebinned_".format(outputWS)
            params = (xmins[0], deltas[0], xmaxs[0])
            Rebin(InputWorkspace=inputWS, OutputWorkspace=name, Params=params, PreserveEvents=preserveEvents)
            self.setProperty("OutputWorkspace", mtd[name])
            DeleteWorkspace(name)
        else:
            numSpec = inputWS.getNumberHistograms()

            # fill out the values for min and max as appropriate
            xmins = self.__extend_value(numSpec, xmins, replaceNan=True)
            xmaxs = self.__extend_value(numSpec, xmaxs, replaceNan=True)
            deltas = self.__extend_value(numSpec, deltas, replaceNan=False)

            self.log().debug("MIN:  " + str(xmins))
            self.log().debug("DELTA:" + str(deltas))
            self.log().debug("MAX:  " + str(xmaxs))

            # temporary workspaces should be hidden
            names = ["__{}_spec_{}".format(outputWS, i) for i in range(len(xmins))]

            # how much the progress bar moves forward for each spectrum
            progStep = float(1) / float(3 * numSpec)

            # crop out each spectra and conjoin to a temporary workspace
            accumulationWS = None
            for i, (name, xmin, xmax, delta) in enumerate(zip(names, xmins, xmaxs, deltas)):
                # don't  go beyond the range of the data
                x = inputWS.readX(i)
                if xmin == Property.EMPTY_DBL:
                    xmin = x[0]
                if xmax == Property.EMPTY_DBL:
                    xmax = x[-1]

                progStart = 3 * i * progStep

                try:
                    # extract the range of the spectrum requested
                    ExtractSpectra(
                        InputWorkspace=inputWS,
                        OutputWorkspace=name,
                        StartWorkspaceIndex=i,
                        EndWorkspaceIndex=i,
                        XMin=xmin,
                        XMax=xmax,
                        startProgress=progStart,
                        endProgress=(progStart + progStep),
                        EnableLogging=False,
                    )

                    # rebin the data
                    Rebin(
                        InputWorkspace=name,
                        OutputWorkspace=name,
                        Params=(xmin, delta, xmax),
                        PreserveEvents=preserveEvents,
                        startProgress=progStart,
                        endProgress=(progStart + progStep),
                        EnableLogging=False,
                    )
                except Exception as e:
                    raise RuntimeError("for index={}: {}".format(i, e)) from e

                # accumulate
                if accumulationWS is None:
                    accumulationWS = name  # messes up progress during very first step
                else:
                    # this deletes both input workspaces
                    ConjoinWorkspaces(
                        InputWorkspace1=accumulationWS,
                        InputWorkspace2=name,
                        startProgress=(progStart + 2 * progStep),
                        endProgress=(progStart + 3 * progStep),
                        EnableLogging=False,
                        CheckMatchingBins=False,
                    )
            self.setProperty("OutputWorkspace", mtd[accumulationWS])
            DeleteWorkspace(accumulationWS, EnableLogging=False)


AlgorithmFactory.subscribe(RebinRagged)
