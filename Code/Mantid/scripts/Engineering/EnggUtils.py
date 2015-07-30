#pylint: disable=invalid-name
#pylint: disable=too-many-arguments
# R0913: Too many arguments - strictly for write_GSAS_iparam_file()
from mantid.api import *
import mantid.simpleapi as sapi

# numpy needed only for Vanadium calculations, at the moment
import numpy as np

ENGINX_BANKS = ['', 'North', 'South', '1', '2']
# banks (or groups) to which the pixel-by-pixel correction should be applied
ENGINX_BANKS_FOR_PIXBYPIX_CORR = [1,2]

def readInExpectedPeaks(filename, expectedGiven):
    """
    Reads in expected peaks from the .csv file if requested. Otherwise fall back to the list of
    peaks given (and check that it is not empty).

    @param :: filename name of the csv file to read from. If empty, we take the peaks given in an option.
    This is passed to Engg algorithms in the (optional) input property 'ExpectedPeaksFromFile'

    @param :: expectedGiven list of expected peaks given as an input property to an Engg algorithm
    (ExpectedPeaks)

    @returns the expected peaks either from a file or input list, sorted in ascending order
    """

    expectedPeaksD = None

    if filename != "":
        exPeakArray = []
        readInArray = []
        try:
            with open(filename) as f:
                for line in f:
                    readInArray.append([float(x) for x in line.split(',')])
            for a in readInArray:
                for b in a:
                    exPeakArray.append(b)
        except RuntimeError, ex:
            raise RuntimeError("Error while reading file of expected peaks '%s': %s" % (filename, ex))

        if not exPeakArray:
            # "File could not be read. Defaults in alternative option used."
            if [] == expectedGiven:
                raise ValueError("Could not read any peaks from the file given in 'ExpectedPeaksFromFile: '" +
                                 filename + "', and no expected peaks were given in the property "
                                 "'ExpectedPeaks' either. Cannot continue without a list of expected peaks.")
            expectedPeaksD = sorted(expectedGiven)

        else:
            expectedPeaksD = sorted(exPeakArray)

    else:
        if [] == expectedGiven:
            raise ValueError("No expected peaks were given in the property 'ExpectedPeaks', "
                             "could not get default expected peaks, and 'ExpectedPeaksFromFile' "
                             "was not given either. Cannot continout without a list of expected peaks.")
        expectedPeaksD = sorted(expectedGiven)

    return expectedPeaksD

def getWsIndicesFromInProperties(ws, bank, detIndices):
    """
    Get the detector indices that the user requests, either through the input property 'Bank' or
    'DetectorIndices'

    @param workspace :: input workspace (with instrument)
    @param bank :: value passed in the input property 'Bank' to an Engg algorithm
    @param detIndices :: value passed in the 'Det

    @returns list of workspace indices that can be used in mantid algorithms such as CropWorkspace.
    """

    indices = None
    if bank and detIndices:
        raise ValueError("It is not possible to use at the same time the input properties 'Bank' and "
                         "'DetectorIndices', as they overlap. Please use either of them. Got Bank: '%s', "
                         "and DetectorIndices: '%s'"%(bank, detIndices))
    elif bank:
        bankAliases = {'North': '1', 'South': '2'}
        bank = bankAliases.get(bank, bank)
        indices = getWsIndicesForBank(ws, bank)
        if not indices:
            raise RuntimeError("Unable to find a meaningful list of workspace indices for the "
                               "bank passed: %s. Please check the inputs." % bank)
        return indices
    elif detIndices:
        indices = parseSpectrumIndices(ws, detIndices)
        if not indices:
            raise RuntimeError("Unable to find a meaningful list of workspace indices for the "
                               "range(s) of detectors passed: %s. Please check the inputs." % detIndices)
        return indices
    else:
        raise ValueError("You have not given any value for the properties 'Bank' and 'DetectorIndices' "
                         "One of them is required")


def parseSpectrumIndices(ws, specNumbers):
    """
    Get a usable list of workspace indices from a user provided list of spectra that can look like:
    '8-10, 20-40, 100-110'. For that example this method will produce: [7,8,9,19, 20,... , 109]

    @param workspace :: input workspace (with instrument)
    @param specNumbers :: range of spectrum numbers (or list of ranges) as given to an algorithm

    @return list of workspace indices, ready to be used in mantid algorithms such as CropWorkspace
    """
    segments = [ s.split("-") for s in specNumbers.split(",") ]
    indices = [ idx for s in segments for idx in range(int(s[0]), int(s[-1])+1) ]
    # remove duplicates and sort
    indices = list(set(indices))
    indices.sort()
    maxIdx = ws.getNumberHistograms()
    if indices[-1] >= maxIdx:
        raise ValueError("A workspace index equal or bigger than the number of histograms available in the "
                         "workspace '" + ws.getName() +"' (" + str(ws.getNumberHistograms()) +
                         ") has been given. Please check the list of indices.")
    # and finally traslate from 'spectrum numbers' to 'workspace indices'
    return [ws.getIndexFromSpectrumNumber(sn) for sn in indices]

def getWsIndicesForBank(ws, bank):
    """
    Finds the workspace indices of all the pixels/detectors/spectra corresponding to a bank.

    ws :: workspace with instrument definition
    bank :: bank number as it appears in the instrument definition

    @returns :: list of workspace indices for the bank
    """
    detIDs = getDetIDsForBank(bank)

    def isIndexInBank(index):
        try:
            det = ws.getDetector(index)
            return det.getID() in detIDs
        except RuntimeError:
            return False

    return [i for i in range(0, ws.getNumberHistograms()) if isIndexInBank(i)]

def getDetIDsForBank(bank):
    """
    Find the detector IDs for an instrument bank. Note this is at this point specific to
    the ENGINX instrument.

    @param bank :: name/number as a string

    @returns list of detector IDs corresponding to the specified Engg bank number
    """
    import os
    groupingFilePath = os.path.join(sapi.config.getInstrumentDirectory(),
                                    'Grouping', 'ENGINX_Grouping.xml')

    alg = AlgorithmManager.create('LoadDetectorsGroupingFile')
    alg.setProperty('InputFile', groupingFilePath)
    alg.setProperty('OutputWorkspace', '__EnginXGrouping')
    alg.execute()

    grouping = mtd['__EnginXGrouping']

    detIDs = set()

    for i in range(grouping.getNumberHistograms()):
        if grouping.readY(i)[0] == int(bank):
            detIDs.add(grouping.getDetector(i).getID())

    sapi.DeleteWorkspace(grouping)

    if len(detIDs) == 0:
        raise ValueError('Could not find any detector for this bank: ' + bank +
                         '. This looks like an unknown bank')

    return detIDs

def  generateOutputParTable(name, difc, zero):
    """
    Produces a table workspace with the two fitted calibration parameters

    @param name :: the name to use for the table workspace that is created here
    @param difc :: difc calibration parameter
    @param zero :: zero calibration parameter
    """
    tbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace=name)
    tbl.addColumn('double', 'difc')
    tbl.addColumn('double', 'zero')
    tbl.addRow([float(difc), float(zero)])

def convertToDSpacing(parent, ws):
    """
    Converts a workspace to dSpacing using 'ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in ToF units) to convert (not modified)

    @returns workspace converted to d-spacing units
    """
    # A check to catch possible errors in an understandable way
    expectedDim = 'Time-of-flight'
    dimType = ws.getXDimension().getName()
    if expectedDim != dimType:
        raise ValueError("This function expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'. This is an internal logic "
                         "error. "% (expectedDim, dimType))

    alg = parent.createChildAlgorithm('ConvertUnits')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('Target', 'dSpacing')
    alg.setProperty('AlignBins', True)
    alg.execute()
    return alg.getProperty('OutputWorkspace').value

def convertToToF(parent, ws):
    """
    Converts workspace to Time-of-Flight using 'ConvertUnits' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (normally in d-spacing units) to convert to ToF

    @returns workspace with data converted to ToF units
    """
    alg = parent.createChildAlgorithm('ConvertUnits')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('Target', 'TOF')
    alg.execute()
    return alg.getProperty('OutputWorkspace').value

def cropData(parent, ws, indices):
    """
    Produces a cropped workspace from the input workspace so that only
    data for the specified bank (given as a list of indices) is left.

    NB: This assumes spectra for a bank are consequent.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to crop (not modified in-place)
    @param indices :: workspace indices to keep in the workpace returned

    @returns cropped workspace, with only the spectra corresponding to the indices requested
    """
    # Leave only spectra between min and max
    alg = parent.createChildAlgorithm('CropWorkspace')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('StartWorkspaceIndex', min(indices))
    alg.setProperty('EndWorkspaceIndex', max(indices))
    alg.execute()

    return alg.getProperty('OutputWorkspace').value

def sumSpectra(parent, ws):
    """
    Focuses/sums up all the spectra into a single one (calls the SumSpectra algorithm)

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to sum up

    @return single-spectrum workspace resulting from the sum
    """
    alg = parent.createChildAlgorithm('SumSpectra')
    alg.setProperty('InputWorkspace', ws)
    alg.execute()

    return alg.getProperty('OutputWorkspace').value

def write_ENGINX_GSAS_iparam_file(output_file, difc, zero, ceria_run=241391,
                                  vanadium_run=236516, template_file=None):
    """
    Produces and writes an ENGIN-X instrument parameter file for GSAS
    (in the GSAS iparam format, as partially described in the GSAS
    manual). It simply uses a template (found in template_path) where
    some values are replaced with the values (difc, zero) passed to
    this function.

    Possible extensions for the file are .par (used here as default),
    .prm, .ipar, etc.

    @param output_file :: name of the file where to write the output
    @param difc :: list of DIFC values, one per bank, to pass on to GSAS
                   (as produced by EnggCalibrate)
    @param zero :: list of TZERO values, one per bank, to pass on to GSAS
                   (also from EnggCalibrate)
    @param ceria_run :: number of the ceria (CeO2) run used for this calibration.
                        this number goes in the file and should also be used to
                        name the file
    @param vanadium_run :: number of the vanadium (VNb) run used for this
                           calibration. This number goes in the file and should
                           also be used to name the file.
    @param template_file :: file to use as template (with relative or full path)

    @returns

    """
    if not isinstance(difc, list) or not isinstance(zero, list):
        raise ValueError("The parameters difc and zero must be lists, with as many elements as "
                         "banks")

    if len(difc) != len(zero):
        raise ValueError("The lengths of the difc and zero lists must be the same")

    if not template_file:
        import os
        template_file = os.path.join(os.path.dirname(__file__),
                                     'template_ENGINX_241391_236516_North_and_South_banks.par')

    temp_lines = []
    with open(template_file) as tf:
        temp_lines = tf.readlines()


    def replace_patterns(line, patterns, replacements):
        """
        If line starts with any of the strings passed in the list 'pattern', return the
        corresponding 'replacement'
        """
        for idx, pat in enumerate(patterns):
            if line[0:len(pat)] == pat:
                return replacements[idx]

        return line

    # need to replace two types of lines/patterns:
    # - instrument constants/parameters (ICONS)
    # - instrument calibration comment with run numbers (CALIB)
    output_lines = []
    for bank_idx in range(0, len(difc)):
        patterns = ["INS  %d ICONS"%bank_idx,
                    "INS    CALIB"]
        difa = 0.0
        # the ljust(80) ensure a length of 80 characters for the lines (GSAS rules...)
        replacements = [ ("INS  %d ICONS  %.2f    %.2f    %.2f" %
                          (bank_idx, difc[bank_idx], difa, zero[bank_idx])).ljust(80) + '\r\n',
                         ("INS    CALIB   %d   %d ceo2" %
                          (ceria_run, vanadium_run)).ljust(80) + '\r\n']

        output_lines = [ replace_patterns(line, patterns, replacements) for line in temp_lines]

    with open(output_file, 'w') as of:
        of.writelines(output_lines)

# ----------------------------------------------------------------------------------------
# Functions for Vanadium corrections follow. These could be converted into an algorithm
# that would be used as a child from EnginXFocus and EnginXCalibrate
# ----------------------------------------------------------------------------------------

def applyVanadiumCorrection(parent, ws, vanWS, precalcInteg=None):
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

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to work on, must be a Vanadium (V-Nb) run (in/out)
    @param vanWS :: workspace with Vanadium data
    @param precalcIntegration :: pre-calculated integral of every spectrum for the vanadium data

    @param curvesWS :: workspace with the fitting workspace(s) corresponding to the bank(s)
    processed (3 spectra workspace for every bank). If several banks have been processed there
    will be 3 spectra for each bank.
    """
    if vanadiumWorkspaceIsPrecalculated(vanWS):
        if not precalcInteg:
            raise ValueError('A pre-calcuated vanadium curves workspace was passed instead of raw '
                             'Vanadium run data, but no spectra integration results were provided.')

        rows = precalcInteg.rowCount()
        spectra = ws.getNumberHistograms()
        if rows < spectra:
            raise ValueError("The number of histograms in the input data workspace (%d) is bigger "
                             "than the number of rows (spectra) in the integration workspace (%d)"%
                             (rows, spectra))

    applySensitivityCorrection(parent, ws, vanWS, precalcInteg)

    # apply this to both Engin-X banks, each with separate calculations
    curvesDict = applyPixByPixCorrection(parent, ws, vanWS, ENGINX_BANKS_FOR_PIXBYPIX_CORR)
    curvesWS = prepareCurvesWS(parent, curvesDict)
    return curvesWS

def applySensitivityCorrection(parent, ws, vanWS, precalcInteg):
    """
    Applies the first step of the Vanadium corrections on the given workspace.
    Operations are done in ToF

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (in/out)
    @param vanWS :: workspace with Vanadium data
    @param precalcIntegration :: pre-calculated integral of every spectrum for the vanadium data
    """
    if not vanadiumWorkspaceIsPrecalculated(vanWS):
        applySensitivityCorrectionFromRawData(parent, ws, vanWS)
    else:
        for i in range(0, ws.getNumberHistograms()):
            scaleFactor = precalcInteg.cell(i,0)/vanWS.blocksize()
            ws.setY(i, np.divide(ws.dataY(i), scaleFactor))


def applySensitivityCorrectionFromRawData(parent, ws, vanWS):
    """
    This does the real calculations behind applySensitivityCorrection() for when we are given raw
    data from a Vanadium run.
    """
    expectedDim = 'Time-of-flight'
    dimType = vanWS.getXDimension().getName()
    if expectedDim != dimType:
        raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

    integWS = integrateSpectra(parent, vanWS)
    if  1 != integWS.blocksize() or integWS.getNumberHistograms() < ws.getNumberHistograms():
        raise RuntimeError("Error while integrating vanadium workspace, the Integration algorithm "
                           "produced a workspace with %d bins and %d spectra. The workspace "
                           "being integrated has %d spectra."%
                           (integWS.blocksize(), integWS.getNumberHistograms(),
                            vanWS.getNumberHistograms()))

    for i in range(0, ws.getNumberHistograms()):
        scaleFactor = integWS.readY(i)[0] / vanWS.blocksize()
        ws.setY(i, np.divide(ws.dataY(i), scaleFactor)) # DivideSpec(ws, scaleFactor)

def applyPixByPixCorrection(parent, ws, vanWS, banks):
    """
    Applies the second step of the Vanadium correction on the given workspace: pixel by pixel
    divides by a curve fitted to the sum of the set of spectra of the corresponding bank.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace to work on / correct
    @param banks :: list of banks to correct - normally would expect all banks: [1,2]
    @param vanWS :: workspace with Vanadium data

    @param curves :: a dictionary with bank id's (numbers) as keys and fitting output workspace
    as values
    """
    if vanadiumWorkspaceIsPrecalculated(vanWS):
        # No instrument -> precalculated curve(s)
        curves = _precalcWStoDict(parent, vanWS)
    else:
        # Have to calculate curves. get one curve per bank, in d-spacing
        curves = fitCurvesPerBank(parent, vanWS, banks)

    # divide the spectra by their corresponding bank curve
    divideByCurves(parent, ws, curves)

    return curves

def divideByCurves(parent, ws, curves):
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

    @param parent :: parent (Mantid) algorithm that wants to run this
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
    ws = convertToDSpacing(parent, ws)
    for b in curves:
        # process all the spectra (indices) in one bank
        fittedCurve = curves[b]
        idxs = getWsIndicesForBank(ws, b)

        if not idxs:
            pass

        # This RebinToWorkspace is required here: normal runs will have narrower range of X values,
        # and possibly different bin size, as compared to (long) Vanadium runs. Same applies to short
        # Ceria runs (for Calibrate -non-full) and even long Ceria runs (for Calibrate-Full).
        rebinnedFitCurve = rebinToMatchWS(parent, fittedCurve, ws)

        for i in idxs:
            # take values of the second spectrum of the workspace (fit simulation - fitted curve)
            ws.setY(i, np.divide(ws.dataY(i), rebinnedFitCurve.readY(1)))

    # finally, convert back to ToF
    ws = convertToToF(parent, ws)

def fitCurvesPerBank(parent, vanWS, banks):
    """
    Fits one curve to every bank (where for every bank the data fitted is the result of
    summing up all the spectra of the bank). The fitting is done in d-spacing.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param vanWS :: Vanadium run workspace to fit, expected in TOF units as they are archived
    @param banks :: list of banks to consider which is normally all the banks of the instrument

    @returns dictionary of fit workspaces, with one per bank given in the inputs. These workspaces are
    in d-spacing units. The bank identifiers are the keys, and the workspaces are the values.
    """
    curves = {}
    for b in banks:
        indices = getWsIndicesForBank(vanWS, b)
        if not indices:
            # no indices at all for this bank, not interested in it, don't add it to the dictionary
            # (as when doing Calibrate (not-full)) which does CropData() the original workspace
            continue

        wsToFit = cropData(parent, vanWS, indices)
        wsToFit = convertToDSpacing(parent, wsToFit)
        wsToFit = sumSpectra(parent, wsToFit)

        fitWS = fitBankCurve(parent, wsToFit, b)
        curves.update({b: fitWS})

    return curves

def fitBankCurve(parent, vanWS, bank):
    """
    Fits a spline to a single-spectrum workspace (in d-spacing)

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param vanWS :: Vanadium workspace to fit (normally this contains spectra for a single bank)
    @param bank :: instrument bank this is fitting is done for

    @returns fit workspace (MatrixWorkspace), with the same number of bins as the input
    workspace, and the Y values simulated from the fitted curve
    """
    expectedDim = 'd-Spacing'
    dimType = vanWS.getXDimension().getName()
    if expectedDim != dimType:
        raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

    if 1 != vanWS.getNumberHistograms():
        raise ValueError("The workspace does not have exactly one histogram. Inconsistency found.")

    # without these min/max parameters 'BSpline' would completely misbehave
    xvec = vanWS.readX(0)
    startX = min(xvec)
    endX = max(xvec)
    functionDesc = 'name=BSpline, Order=3, StartX=' + str(startX) +', EndX=' + str(endX) + ', NBreak=12'
    fitAlg = parent.createChildAlgorithm('Fit')
    fitAlg.setProperty('Function', functionDesc)
    fitAlg.setProperty('InputWorkspace', vanWS)
    # WorkspaceIndex is left to default '0' for 1D function fits
    # StartX, EndX could in principle be left to default start/end of the spectrum, but apparently
    # not safe for 'BSpline'
    fitAlg.setProperty('CreateOutput', True)
    fitAlg.execute()

    success = fitAlg.getProperty('OutputStatus').value
    parent.log().information("Fitting Vanadium curve for bank %s, using function '%s', result: %s" %
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

def prepareCurvesWS(parent, curvesDict):
    """
    Simply concantenates or appends fitting output workspaces as produced by the algorithm Fit (with 3
    spectra each). The groups of 3 spectra are added sorted by the bank ID (number). This could also
    produce a workspace group with the individual workspaces in it, but the AppendSpectra solution
    seems simpler.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param curvesDict :: dictionary with fitting workspaces produced by 'Fit'

    @returns a workspace where all the input workspaces have been concatenated, with 3 spectra per
    workspace / bank
    """
    if 0 == len(curvesDict):
        raise RuntimeError("Expecting a dictionary with fitting workspaces from 'Fit' but got an "
                           "empty dictionary")
    if 1 == len(curvesDict):
        return curvesDict.values()[0]

    keys = sorted(curvesDict)
    ws = curvesDict[keys[0]]
    for idx in range(1, len(keys)):
        nextWS = curvesDict[keys[idx]]
        ws = appendSpec(parent, ws, nextWS)

    return ws

def vanadiumWorkspaceIsPrecalculated(ws):
    """
    Is the workspace precalculated curve(s)? If not, it must be raw data from a Vanadium run

    @param ws :: workspace passed in an Engg algorithm property

    @returns True if the workspace seems to contain precalculated bank curves for Vanadium data
    """
    inst = ws.getInstrument()
    return not inst or not inst.getName()

def _precalcWStoDict(parent, ws):
    """
    Turn a workspace with one or more fitting results (3 spectra per curve fitted), that comes
    with all the spectra appended, into a dictionary of individual fitting results.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws workspace with fitting results (3 spectra per curve fitted) as provided by Fit

    @returns dictionary with every individual fitting result (3 spectra)

    """
    curves = {}

    if 0 != (ws.getNumberHistograms() % 3):
        raise RuntimeError("A workspace without instrument definition has ben passed, so it is "
                           "expected to have fitting results, but it does not have a number of "
                           "histograms multiple of 3. Number of hsitograms found: %d"%
                           ws.getNumberHistograms())

    for wi in range(0, ws.getNumberHistograms()/3):
        indiv = cropData(parent, ws, [wi, wi+2])
        curves.update({wi: indiv})

    return curves

def appendSpec(parent, ws1, ws2):
    """
    Uses the algorithm 'AppendSpectra' to append the spectra of ws1 and ws2

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws1 :: first workspace
    @param ws2 :: second workspace

    @returns workspace with the concatenation of the spectra ws1+ws2
    """
    alg = parent.createChildAlgorithm('AppendSpectra')
    alg.setProperty('InputWorkspace1', ws1)
    alg.setProperty('InputWorkspace2', ws2)
    alg.execute()

    result = alg.getProperty('OutputWorkspace').value
    return result

def integrateSpectra(parent, ws):
    """
    Integrates all the spectra or a workspace, and return the result.
    Simply uses 'Integration' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (MatrixWorkspace) with the spectra to integrate

    @returns integrated workspace, or result of integrating every spectra in the input workspace
    """
    intAlg = parent.createChildAlgorithm('Integration')
    intAlg.setProperty('InputWorkspace', ws)
    intAlg.execute()
    ws = intAlg.getProperty('OutputWorkspace').value

    return ws

def rebinToMatchWS(parent, ws, targetWS):
    """
    Rebins a workspace so that its bins match those of a 'target' workspace. This simply uses the
    algorithm RebinToWorkspace as a child. In principle this method does not care about the units
    of the input workspaces, as long as they are in the same units.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: input workspace (MatrixWorkspace) to rebin (all spectra will be rebinnded)
    @param targetWS :: workspace to match against, this fixes the data range, and number and width of the
    bins for the workspace returned

    @returns ws rebinned to resemble targetWS
    """
    reAlg = parent.createChildAlgorithm('RebinToWorkspace')
    reAlg.setProperty('WorkspaceToRebin', ws)
    reAlg.setProperty('WorkspaceToMatch', targetWS)
    reAlg.execute()
    reWS = reAlg.getProperty('OutputWorkspace').value

    return reWS
