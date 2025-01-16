# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty
from mantid.simpleapi import ConvertToMatrixWorkspace
import numpy as np


class MatchSpectra(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Reduction"

    # def seeAlso(self):
    #    return ['']

    def name(self):
        return "MatchSpectra"

    def summary(self):
        return "Calculate factors to most closely match all spectra to reference spectrum"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Workspace to match the spectra between")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Workspace with the spectra matched")
        self.declareProperty("ReferenceSpectrum", 1, doc="Spectrum to match other spectra to")
        self.declareProperty("CalculateOffset", True, doc="Calculate vertical shift")
        self.declareProperty("CalculateScale", True, doc="Calculate scale factor")

        self.declareProperty(FloatArrayProperty("Offset", values=[], direction=Direction.Output), "Additive factor from matching")
        self.declareProperty(FloatArrayProperty("Scale", values=[], direction=Direction.Output), "Multiplicitive factor from matching")
        self.declareProperty(
            FloatArrayProperty("ChiSq", values=[], direction=Direction.Output),
            "Unweighted ChiSq between the spectrum and the reference. NaN means that the spectrum was not matched",
        )

    def __getReferenceWsIndex(self):
        refSpectrum = self.getProperty("ReferenceSpectrum").value
        inputWS = self.getProperty("InputWorkspace").value

        for wkspIndex in range(inputWS.getNumberHistograms()):
            if inputWS.getSpectrum(wkspIndex).getSpectrumNo() == refSpectrum:
                return wkspIndex

        raise RuntimeError('Failed to find spectrum {} in workspace "{}"'.format(refSpectrum, inputWS))

    def __createOutputWS(self):
        """Convert to a Workspace2D despite what the algorithm is named"""
        outputWS = ConvertToMatrixWorkspace(
            InputWorkspace=self.getPropertyValue("InputWorkspace"), OutputWorkspace=self.getPropertyValue("OutputWorkspace")
        )
        return outputWS

    def __generateIndices(self, spectrumNum, reference, testing, binBoundaries):
        """Generates the indices for slicing by comparing x-axes

        A note about implementation: If numpy.searchsorted fails to find the
        value, it returns the last index of the array.
        """
        BAD_RESULT = (False, (0, 0), (0, 0))
        # find the lower bounds
        refLower = 0
        tstLower = 0
        if reference[0] == testing[0]:
            pass  # values are already set
        elif reference[0] < testing[0]:
            refLower = np.searchsorted(reference, testing[0])
            if refLower == reference.size:
                msg = "Falied to find {} in reference spectrum x-axis (spectrum={})".format(testing[0], spectrumNum)
                self.log().notice(msg)
                return BAD_RESULT
        else:
            tstLower = np.searchsorted(testing, reference[0])
            if tstLower == testing.size:
                msg = "Falied to find {} in the x-axis of the spectrum being matched (spectrum={})".format(reference[0], spectrumNum)
                self.log().notice(msg)
                return BAD_RESULT

        # find the upper bounds
        refUpper = reference.size - 1
        tstUpper = testing.size - 1
        if binBoundaries:
            refUpper -= 1
            tstUpper -= 1
        if reference[refUpper] == testing[tstUpper]:
            pass  # values are already set
        elif reference[refUpper] < testing[tstUpper]:
            tstUpper = np.searchsorted(testing, reference[refUpper])
            if reference[refUpper] != testing[tstUpper]:
                msg = "Falied to find {} in the x-axis of the spectrum being matched (spectrum={})".format(reference[-1], spectrumNum)
                self.log().notice(msg)
                return BAD_RESULT
        else:
            refUpper = np.searchsorted(reference, testing[tstUpper])
            if reference[refUpper] != testing[tstUpper]:
                msg = "Falied to find {} in reference spectrum x-axis (spectrum={})".format(testing[-1], spectrumNum)
                self.log().notice(msg)
                return BAD_RESULT

        if (reference[refLower:refUpper]).size != (testing[tstLower:tstUpper]).size:
            self.log().notice(msg)
            return BAD_RESULT

        return (True, (refLower, refUpper), (tstLower, tstUpper))

    def __residual(self, X, Y1, Y2):
        deltaX = np.diff(X)
        deltaX = np.append(deltaX, deltaX[-1])  # add the last value to the end
        return (np.square(Y1 - Y2) * deltaX).sum() / deltaX.sum()

    def PyExec(self):
        referenceWkspIndex = self.__getReferenceWsIndex()
        outputWS = self.__createOutputWS()

        # determine what to calculate
        doScale = self.getProperty("CalculateScale").value
        doOffset = self.getProperty("CalculateOffset").value

        referenceX = outputWS.readX(referenceWkspIndex)
        referenceY = outputWS.readY(referenceWkspIndex)
        referenceE = outputWS.readE(referenceWkspIndex)

        if not np.any(referenceE > 0.0):
            raise RuntimeError("None of the uncertainties in the reference spectrum is greater than zero. No data would be used.")

        resultOffset = []
        resultScale = []
        resultResidual = []

        # this is just gauss-markov theorem
        for wkspIndex in range(outputWS.getNumberHistograms()):  # in nb which appears to be number of banks
            spectrumNum = outputWS.getSpectrum(wkspIndex).getSpectrumNo()
            if wkspIndex == referenceWkspIndex:
                resultOffset.append(0.0)
                resultScale.append(1.0)
                resultResidual.append(0.0)
                self.log().information("spectrum {} is the reference".format(spectrumNum))
                continue

            X = outputWS.readX(wkspIndex)
            Y = outputWS.readY(wkspIndex)
            E = outputWS.readE(wkspIndex)

            if not np.any(E > 0.0):
                self.log().warning("None of the uncertainties in the reference spectrum {} is greater than zero".format(spectrumNum))
                resultOffset.append(0.0)
                resultScale.append(1.0)
                resultResidual.append(np.nan)
                continue

            hasOverlap, refIndices, tstIndices = self.__generateIndices(spectrumNum, referenceX, X, X.size == Y.size + 1)
            if not hasOverlap:
                resultOffset.append(0.0)
                resultScale.append(1.0)
                resultResidual.append(np.nan)
                continue

            mask = (E[tstIndices[0] : tstIndices[1]] > 0.0) * (referenceE[refIndices[0] : refIndices[1]] > 0.0)
            if not np.any(mask):
                resultOffset.append(0.0)
                resultScale.append(1.0)
                resultResidual.append(np.nan)
                self.log().warning("The overlap region of spectrum {} has no uncertainties greater than zero".format(spectrumNum))
                continue
            totalBins = mask.sum()  # number of bins being used

            # only calculate the terms that are needed
            if doOffset:
                sumRef = referenceY[refIndices[0] : refIndices[1]][mask].sum()
                sumSpec = Y[tstIndices[0] : tstIndices[1]][mask].sum()
            if doScale:
                sumSpecSq = (Y[tstIndices[0] : tstIndices[1]][mask] * Y[tstIndices[0] : tstIndices[1]][mask]).sum()
                sumRefSpec = (Y[tstIndices[0] : tstIndices[1]][mask] * referenceY[refIndices[0] : refIndices[1]][mask]).sum()

            # defaults are to do nothing
            scale = 1.0
            offset = 0.0

            if doScale and doOffset:  # use both
                # Cramar's rule for 2x2 matrix
                denominator = totalBins * sumSpecSq - sumSpec * sumSpec
                scale = (totalBins * sumRefSpec - sumRef * sumSpec) / denominator
                offset = (sumRef * sumSpecSq - sumSpec * sumRefSpec) / denominator
            elif doScale and not doOffset:  # only scale
                scale = sumRefSpec / sumSpecSq
            elif doOffset and not doScale:  # only shift
                offset = (sumRef - sumSpec) / totalBins

            # calculate the residual of the fit - must be done before updating values
            residual = self.__residual(
                X[tstIndices[0] : tstIndices[1]][mask],
                Y[tstIndices[0] : tstIndices[1]][mask] * scale + offset,
                referenceY[refIndices[0] : refIndices[1]][mask],
            )
            resultResidual.append(residual)

            msg = (
                "spectrum {} chisq ".format(spectrumNum)
                + "before={} ".format(
                    self.__residual(
                        X[tstIndices[0] : tstIndices[1]][mask],
                        Y[tstIndices[0] : tstIndices[1]][mask],
                        referenceY[refIndices[0] : refIndices[1]][mask],
                    )
                )
                + "after={}".format(residual)
            )
            self.log().information(msg)

            # update the values in the output workspace
            Ynew = np.copy(Y)
            Ynew[E > 0.0] = Ynew[E > 0.0] * scale + offset
            outputWS.setY(wkspIndex, Ynew)
            outputWS.setE(wkspIndex, E * scale)  # background doesn't matter because there isn't uncertainty

            resultOffset.append(offset)
            resultScale.append(scale)

        # set output properties
        self.setProperty("OutputWorkspace", outputWS)
        self.setProperty("Offset", resultOffset)
        self.setProperty("Scale", resultScale)
        self.setProperty("ChiSq", resultResidual)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(MatchSpectra)
