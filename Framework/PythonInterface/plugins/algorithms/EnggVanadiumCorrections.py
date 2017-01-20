#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as sapi

import numpy as np

import EnggUtils


class EnggVanadiumCorrections(PythonAlgorithm):
    # banks (or groups) to which the pixel-by-pixel correction should be applied
    _ENGINX_BANKS_FOR_PIXBYPIX_CORR = [1,2]

    def category(self):
        return ("Diffraction\\Engineering;CorrectionFunctions\\BackgroundCorrections;"
                "CorrectionFunctions\\EfficiencyCorrections;CorrectionFunctions\\NormalisationCorrections")

    def name(self):
        return "EnggVanadiumCorrections"

    def summary(self):
        return ("Calculates correction features and / or uses them to correct diffraction data "
                "with respect to reference Vanadium data.")

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("Workspace", "", Direction.InOut, PropertyMode.Optional),
                             "Workspace with the diffraction data to correct. The Vanadium corrections "
                             "will be applied on it.")

        self.declareProperty(MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input,
                                                     PropertyMode.Optional),
                             "Workspace with the reference Vanadium diffraction data.")

        self.declareProperty(ITableWorkspaceProperty("OutIntegrationWorkspace", "", Direction.Output,
                                                     PropertyMode.Optional),
                             'Output integration workspace produced when given an input Vanadium workspace')

        self.declareProperty(MatrixWorkspaceProperty("OutCurvesWorkspace", "", Direction.Output,
                                                     PropertyMode.Optional),
                             'Output curves workspace produced when given an input Vanadium workspace')

        # ~10 break points is still poor, there is no point in using less than that
        self.declareProperty("SplineBreakPoints", defaultValue=50,
                             validator=IntBoundedValidator(10),
                             doc="Number of break points used when fitting the bank profiles with a spline "
                             "function.")

        out_vana_grp = 'Output parameters (for when calculating corrections)'
        self.setPropertyGroup('OutIntegrationWorkspace', out_vana_grp)
        self.setPropertyGroup('OutCurvesWorkspace', out_vana_grp)
        self.setPropertyGroup('SplineBreakPoints', out_vana_grp)

        self.declareProperty(ITableWorkspaceProperty("IntegrationWorkspace", "", Direction.Input,
                                                     PropertyMode.Optional),
                             "Workspace with the integrated values for every spectra of the reference "
                             "Vanadium diffraction data. One row per spectrum.")

        self.declareProperty(MatrixWorkspaceProperty("CurvesWorkspace", "", Direction.Input,
                                                     PropertyMode.Optional),
                             'Workspace with the curves fitted on bank-aggregated Vanadium diffraction '
                             'data, one per bank. This workspace has three spectra per bank, as produced '
                             'by the algorithm Fit. This is meant to be used as an alternative input '
                             'VanadiumWorkspace')

        in_vana_grp = 'Input parameters (for when applying pre-calculated corrections)'
        self.setPropertyGroup('IntegrationWorkspace', in_vana_grp)
        self.setPropertyGroup('CurvesWorkspace', in_vana_grp)

    def PyExec(self):
        """
        Vanadium correction as done for the EnginX instrument. This is in principle meant to be used
        in EnginXFocus and EnginXCalibrateFull. The process consists of two steps:
        1. sensitivity correction
        2. pixel-by-pixel correction

        1. is performed for very pixel/detector/spectrum: scale every spectrum/curve by a
        scale factor proportional to the number of neutrons in that spectrum

        2. Correct every pixel by dividing by a curve (spline) fitted on the summed spectra
        (summing banks separately).

        The sums and fits are done in d-spacing.
        """

        ws = self.getProperty('Workspace').value
        vanWS = self.getProperty('VanadiumWorkspace').value
        integWS = self.getProperty('IntegrationWorkspace').value
        curvesWS = self.getProperty('CurvesWorkspace').value
        spline_breaks = self.getProperty('SplineBreakPoints').value

        preports = 1
        if ws:
            preports += 1
        prog = Progress(self, start=0, end=1, nreports=preports)

        prog.report('Checking availability of vanadium correction features')
        # figure out if we are calculating or re-using pre-calculated corrections
        if vanWS:
            self.log().information("A workspace with reference Vanadium data was passed. Calculating "
                                   "corrections")
            integWS, curvesWS = self._calcVanadiumCorrection(vanWS, spline_breaks)
            self.setProperty('OutIntegrationWorkspace', integWS)
            self.setProperty('OutCurvesWorkspace', curvesWS)

        elif not integWS or not curvesWS:
            raise ValueError('When a VanadiumWorkspace is not passed, both the IntegrationWorkspace and '
                             'the CurvesWorkspace are required inputs. One or both of them were not given')

        prog.report('Applying corrections on the input workspace')
        if ws:
            self._applyVanadiumCorrections(ws, integWS, curvesWS)
            self.setProperty('Workspace', ws)

    def _applyVanadiumCorrections(self, ws, integWS, curvesWS):
        """
        Applies the corrections on a workspace. The integration and curves workspaces may have
        been calculated from a Vanadium run or may have been passed from a previous calculation.

        @param ws :: workspace to correct (modified in-place)
        @param integWS :: table workspace with spectra integration values, one row per spectra
        @param curvesWS ::workspace with "Vanadium curves" for every bank
        """
        integSpectra = integWS.rowCount()
        spectra = ws.getNumberHistograms()
        if integSpectra < spectra:
            raise ValueError("The number of histograms in the input data workspace (%d) is bigger "
                             "than the number of spectra (rows) in the integration workspace (%d)"%
                             (spectra, integSpectra))

        prog = Progress(self, start=0, end=1, nreports=2)
        prog.report('Applying sensitivity correction')
        self._applySensitivityCorrection(ws, integWS, curvesWS)
        prog.report('Applying pixel-by-pixel correction')
        self._applyPixByPixCorrection(ws, curvesWS)

    def _applySensitivityCorrection(self, ws, integWS, curvesWS):
        """
        Applies the first step of the Vanadium corrections on the given workspace.
        Operations are done in ToF

        @param ws :: workspace (in/out)
        @param vanWS :: workspace with Vanadium data
        @param integWS :: pre-calculated integral of every spectrum for the Vanadium data
        @param curvesWS :: pre-calculated per-bank curves from the Vanadium data
        """
        for i in range(0, ws.getNumberHistograms()):
            scaleFactor = integWS.cell(i,0) / curvesWS.blocksize()
            ws.setY(i, np.divide(ws.dataY(i), scaleFactor))

    def _applyPixByPixCorrection(self, ws, curvesWS):
        """
        Applies the second step of the Vanadium correction on the given workspace: pixel by pixel
        divides by a curve fitted to the sum of the set of spectra of the corresponding bank.

        @param ws :: workspace to work on / correct
        @param curvesWS :: a workspace with the per-bank curves for Vanadium data,
                this will contain 3 histograms per instrument bank
        """
        curvesDict = self._precalcWStoDict(curvesWS)

        self._divideByCurves(ws, curvesDict)

    def _calcVanadiumCorrection(self, vanWS, spline_breaks):
        """
        Calculates the features that are required to perform vanadium corrections: integration
        of the vanadium data spectra, and per-bank curves fitted to the summed spectra

        @param vanWS :: workspace with data from a Vanadium run
        @param spline_breaks :: number of break points when fitting spline functions

        @returns two workspaces: the integration and the curves. The integration workspace is a
        matrix workspace as produced by the algotithm 'Integration'. The curves workspace is a
        matrix workspace as produced in the outputs of the algorithm 'Fit'
        """
        # Integration of every spectra, as a matrix workspace
        integWS = self._calcIntegrationSpectra(vanWS)

        # Have to calculate curves. get one curve per bank, in d-spacing
        curvesWS = self._fitCurvesPerBank(vanWS, self._ENGINX_BANKS_FOR_PIXBYPIX_CORR, spline_breaks)

        return integWS, curvesWS

    def _calcIntegrationSpectra(self, vanWS):
        """
        This does the real calculations behind _applySensitivityCorrection(), essentially a call to
        the 'Integration' algorithm, for when we are given raw data from a Vanadium run.

        @param vanWS :: workspace with data from a Vanadium run

        @returns Integration workspace with Vanadium spectra integration values, as a table workspace
        with one row per spectrum
        """
        expectedDim = 'Time-of-flight'
        dimType = vanWS.getXDimension().name
        if expectedDim != dimType:
            raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                             "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

        integWS = self._integrateSpectra(vanWS)
        if  1 != integWS.blocksize() or integWS.getNumberHistograms() < vanWS.getNumberHistograms():
            raise RuntimeError("Error while integrating vanadium workspace, the Integration algorithm "
                               "produced a workspace with %d bins and %d spectra. The workspace "
                               "being integrated has %d spectra."%
                               (integWS.blocksize(), integWS.getNumberHistograms(),
                                vanWS.getNumberHistograms()))

        integTbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace='__vanIntegTbl')
        integTbl.addColumn('double', 'Spectra Integration')
        for i in range(integWS.getNumberHistograms()):
            integTbl.addRow([integWS.readY(i)[0]])

        return integTbl

    def _fitCurvesPerBank(self, vanWS, banks, spline_breaks):
        """
        Fits one curve to every bank (where for every bank the data fitted is the result of
        summing up all the spectra of the bank). The fitting is done in d-spacing.

        @param vanWS :: Vanadium run workspace to fit, expected in TOF units as they are archived
        @param banks :: list of banks to consider which is normally all the banks of the instrument
        @param spline_breaks :: number of break points when fitting spline functions

        @returns a workspace with fitting results for all banks (3 spectra per bank). The spectra
        are in dSpacing units.
        """
        curves = {}
        for b in banks:
            indices = EnggUtils.getWsIndicesForBank(vanWS, b)
            if not indices:
                # no indices at all for this bank, not interested in it, don't add it to the dictionary
                # (as when doing Calibrate (not-full)) which does CropData() the original workspace
                continue

            wsToFit = EnggUtils.cropData(self, vanWS, indices)
            wsToFit = EnggUtils.convertToDSpacing(self, wsToFit)
            wsToFit = EnggUtils.sumSpectra(self, wsToFit)

            fitWS = self._fitBankCurve(wsToFit, b, spline_breaks)
            curves.update({b: fitWS})

        curvesWS = self._prepareCurvesWS(curves)

        return curvesWS

    def _fitBankCurve(self, vanWS, bank, spline_breaks):
        """
        Fits a spline to a single-spectrum workspace (in d-spacing)

        @param vanWS :: Vanadium workspace to fit (normally this contains spectra for a single bank)
        @param bank :: instrument bank this is fitting is done for
        @param spline_breaks :: number of break points when fitting spline functions

        @returns fit workspace (MatrixWorkspace), with the same number of bins as the input
        workspace, and the Y values simulated from the fitted curve
        """
        expectedDim = 'd-Spacing'
        dimType = vanWS.getXDimension().name
        if expectedDim != dimType:
            raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                             "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

        if 1 != vanWS.getNumberHistograms():
            raise ValueError("The workspace does not have exactly one histogram. Inconsistency found.")

        # without these min/max parameters 'BSpline' would completely misbehave
        xvec = vanWS.readX(0)
        startX = min(xvec)
        endX = max(xvec)
        functionDesc = ('name=BSpline, Order=3, StartX={0}, EndX={1}, NBreak={2}'.
                        format(startX, endX, spline_breaks))
        fitAlg = self.createChildAlgorithm('Fit')
        fitAlg.setProperty('Function', functionDesc)
        fitAlg.setProperty('InputWorkspace', vanWS)
        # WorkspaceIndex is left to default '0' for 1D function fits
        # StartX, EndX could in principle be left to default start/end of the spectrum, but apparently
        # not safe for 'BSpline'
        fitAlg.setProperty('CreateOutput', True)
        fitAlg.execute()

        success = fitAlg.getProperty('OutputStatus').value
        self.log().information("Fitting Vanadium curve for bank %s, using function '%s', result: %s" %
                               (bank, functionDesc, success))

        detailMsg = ("It seems that this algorithm failed to to fit a function to the summed "
                     "spectra of a bank. The function definiton was: '%s'") % functionDesc

        outParsPropName = 'OutputParameters'
        try:
            fitAlg.getProperty(outParsPropName).value
        except RuntimeError:
            raise RuntimeError("Could not find the parameters workspace expected in the output property " +
                               OutParsPropName + " from the algorithm Fit. It seems that this algorithm failed." +
                               detailMsg)

        outWSPropName = 'OutputWorkspace'
        fitWS = None
        try:
            fitWS = fitAlg.getProperty(outWSPropName).value
        except RuntimeError:
            raise RuntimeError("Could not find the data workspace expected in the output property " +
                               outWSPropName + ". " + detailMsg)

        mtd['engg_van_ws_dsp'] = vanWS
        mtd['engg_fit_ws_dsp'] = fitWS

        return fitWS

    def _prepareCurvesWS(self, curvesDict):
        """
        Simply concantenates or appends fitting output workspaces as produced by the algorithm Fit (with 3
        spectra each). The groups of 3 spectra are added sorted by the bank ID (number). This could also
        produce a workspace group with the individual workspaces in it, but the AppendSpectra solution
        seems simpler.

        @param curvesDict :: dictionary with fitting workspaces produced by 'Fit'

        @returns a workspace where all the input workspaces have been concatenated, with 3 spectra per
        workspace / bank
        """
        if 0 == len(curvesDict):
            raise RuntimeError("Expecting a dictionary with fitting workspaces from 'Fit' but got an "
                               "empty dictionary")
        if 1 == len(curvesDict):
            return list(curvesDict.values())[0]

        keys = sorted(curvesDict)
        ws = curvesDict[keys[0]]
        for idx in range(1, len(keys)):
            nextWS = curvesDict[keys[idx]]
            ws = self._appendSpec(ws, nextWS)

        return ws

    def _divideByCurves(self, ws, curves):
        """
        Expects a workspace in ToF units. All operations are done in-place (the workspace is
        input/output). For every bank-curve pair, divides the corresponding spectra in the
        workspace by the (simulated) fitted curve. The division is done in d-spacing (the
        input workspace is converted to d-spacing inside this method, but results are converted
        back to ToF before returning from this method). The curves workspace is expected in
        d-spacing units (since it comes from fitting a sum of spectra for a bank or group of
        detectors).

        This method is capable of dealing with workspaces with range and bin size different from
        the range and bin size of the curves. It will rebin the curves workspace to match the
        input 'ws' workspace (using the algorithm RebinToWorkspace).

        @param ws :: workspace with (sample) spectra to divide by curves fitted to Vanadium spectra

        @param curves :: dictionary of fitting workspaces (in d-spacing), one per bank. The keys are
        the bank identifier and the values are their fitting workspaces. The fitting workspaces are
        expected as returned by the algorithm 'Fit': 3 spectra: original data, simulated data with fit,
        difference between original and simulated data.
        """
        # Note that this division could use the algorithm 'Divide'
        # This is simple and more efficient than using divide workspace, which requires
        # cropping separate workspaces, dividing them separately, then appending them
        # with AppendSpectra, etc.
        ws = EnggUtils.convertToDSpacing(self, ws)
        for b in curves:
            # process all the spectra (indices) in one bank
            fittedCurve = curves[b]
            idxs = EnggUtils.getWsIndicesForBank(ws, b)

            if not idxs:
                pass

            # This RebinToWorkspace is required here: normal runs will have narrower range of X values,
            # and possibly different bin size, as compared to (long) Vanadium runs. Same applies to short
            # Ceria runs (for Calibrate -non-full) and even long Ceria runs (for Calibrate-Full).
            rebinnedFitCurve = self._rebinToMatchWS(fittedCurve, ws)

            for i in idxs:
                # take values of the second spectrum of the workspace (fit simulation - fitted curve)
                ws.setY(i, np.divide(ws.dataY(i), rebinnedFitCurve.readY(1)))

        # finally, convert back to ToF
        EnggUtils.convertToToF(self, ws)

    def _precalcWStoDict(self, ws):
        """
        Turn a workspace with one or more fitting results (3 spectra per curve fitted), that comes
        with all the spectra appended, into a dictionary of individual fitting results.

        @param ws workspace with fitting results (3 spectra per curve fitted) as provided by Fit for
        all banks.

        @returns dictionary with every individual fitting result (3 spectra)
        """
        curves = {}

        if 0 != (ws.getNumberHistograms() % 3):
            raise RuntimeError("A workspace without instrument definition has been passed, so it is "
                               "expected to have fitting results, but it does not have a number of "
                               "histograms multiple of 3. Number of hsitograms found: %d"%
                               ws.getNumberHistograms())

        for wi in range(0, int(ws.getNumberHistograms()/3)):
            indiv = EnggUtils.cropData(self, ws, [wi, wi+2])
            # the bank id is +1 because wi starts from 0
            bankid = wi + 1
            curves.update({bankid: indiv})

        return curves

    def _appendSpec(self, ws1, ws2):
        """
        Uses the algorithm 'AppendSpectra' to append the spectra of ws1 and ws2

        @param ws1 :: first workspace
        @param ws2 :: second workspace

        @returns workspace with the concatenation of the spectra ws1+ws2
        """
        alg = self.createChildAlgorithm('AppendSpectra')
        alg.setProperty('InputWorkspace1', ws1)
        alg.setProperty('InputWorkspace2', ws2)
        alg.execute()

        result = alg.getProperty('OutputWorkspace').value
        return result

    def _integrateSpectra(self, ws):
        """
        Integrates all the spectra or a workspace, and return the result.
        Simply uses 'Integration' as a child algorithm.

        @param ws :: workspace (MatrixWorkspace) with the spectra to integrate

        @returns integrated workspace, or result of integrating every spectra in the input workspace
        """
        intAlg = self.createChildAlgorithm('Integration')
        intAlg.setProperty('InputWorkspace', ws)
        intAlg.execute()
        ws = intAlg.getProperty('OutputWorkspace').value

        return ws

    def _rebinToMatchWS(self, ws, targetWS):
        """
        Rebins a workspace so that its bins match those of a 'target' workspace. This simply uses the
        algorithm RebinToWorkspace as a child. In principle this method does not care about the units
        of the input workspaces, as long as they are in the same units.

        @param ws :: input workspace (MatrixWorkspace) to rebin (all spectra will be rebinnded)
        @param targetWS :: workspace to match against, this fixes the data range, and number and width of the
        bins for the workspace returned

        @returns ws rebinned to resemble targetWS
        """
        reAlg = self.createChildAlgorithm('RebinToWorkspace')
        reAlg.setProperty('WorkspaceToRebin', ws)
        reAlg.setProperty('WorkspaceToMatch', targetWS)
        reAlg.execute()
        reWS = reAlg.getProperty('OutputWorkspace').value

        return reWS


AlgorithmFactory.subscribe(EnggVanadiumCorrections)
