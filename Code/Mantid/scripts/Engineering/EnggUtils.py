#pylint: disable=invalid-name
import os

from mantid.api import *
import mantid.simpleapi as sapi
# numpy needed only for Vanadium calculations, at the moment
import numpy as np

ENGINX_BANKS = ['', 'North', 'South', '1', '2']

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
    alg = parent.createChildAlgorithm('ConvertUnits')
    alg.setProperty('InputWorkspace', ws)
    alg.setProperty('Target', 'dSpacing')
    alg.setProperty('AlignBins', True)
    alg.execute()
    return alg.getProperty('OutputWorkspace').value

def convertToTOF(parent, ws):
    """
    Converts workspace to TOF using 'ConvertUnits' as a child algorithm.

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

# ----------------------------------------------------------------------------------------
# Funcionts for Vanadium corrections follow. These could be converted into an algorithm
# that would be used as a child from EnginXFocus and EnginXCalibrate
# ----------------------------------------------------------------------------------------

def applyVanadiumCorrection(parent, ws, vanWS):
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
    @param ws :: workspace with Vanadium data
    """

    applySensitivityCorrection(parent, ws)

    # apply this to both Engin-X banks, each with separate calculations
    applyPixByPixCorrection(parent, ws, [1,2])

def applySensitivityCorrection(parent, ws, vanWS):
    """
    Applies the first step of the Vanadium corrections on the given workspace.
    Operations are done in ToF

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (in/out)
    @param vanWS :: workspace with Vanadium data
    """
    expectedDim = 'Time-of-flight'
    dimType = vanWS.getXDimension().getName()
    if expectedDim != dimType:
        raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

    integWS = integrateSpectra(parent, vanWS)
    if integWS.getNumberHistograms() != ws.getNumberHistograms():
        raise RuntimeError("Error while integrating workspace")

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
    """
    # get one curve per bank, in d-spacing
    curves = fitCurvesPerBank(parent, vanWS, banks)
    # divide the spectra by their corresponding bank curve
    divideByCurves(ws, banks, curves)

def divideByCurves(ws, banks, curves):
    """
    Expects a workspace in ToF units. All operations are done in-place (the workspace is
    input/output). For every bank-curve pair, divides the corresponding spectra in the
    workspace by the (simulated) fitted curve. The division is done in d-spacing (the
    input workspace is converted to d-spacing inside this method, but results are converted
    back to ToF before returning from this method).

    @param ws :: workspace with (sample) spectra to divide by curves fitted to Vanadium spectra

    @param banks :: list of banks to consider (normally all the banks of the instrument)

    @param curves :: fitting workspaces (in d-spacing), one per bank. Every of them is
    expected as a fitting workspace as returned by the algorithm 'Fit': 3 spectra: original
    data, simulated data with fit, difference
    """
    # Note that this division could use the algorithm 'Divide'
    # This is simple and more efficient than using divide workspace, which requires
    # cropping separate workspaces, dividing them separately, then appending them
    # with AppendSpectra, etc.
    ws = convertToDSpacing(ws)
    for bIdx, b in enumerate(banks):
        # process all the spectra (indices) in one bank
        fittedCurve = curves[bIdx]
        idxs = getWsIndicesForBank(b)
        for i in idxs:
            ws.setY(i, np.divide(ws.dataY(i), fittedCurve))

    ws = convertToTOF(ws)

def fitCurvesPerBank(parent, wsVan, banks):
    """
    Fits one curve to every bank (where for every bank the data fitted is the result of
    summing up all the spectra of the bank). The fitting is done in d-spacing.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: Vanadium workspace to fit
    @param banks :: list of banks to consider which is normally all the banks of the instrument

    @returns list of fit workspaces, with one per bank given in the inputs. These workspaces are
    in d-spacing units
    """
    curves = []
    for b in banks:
        indices = getWsIndicesForBank(vanWS, b)
        wsToFit = cropData(parent, vanWS, indices)
        wsToFit = convertToDSpacing(parent, wsToFit)
        wsToFit = sumSpectra(parent, wsToFit)
        fitWS = fitBankCurve(parent, vanWS)
        curves.append(fitWS)

    return curves

def fitBankCurve(parent, vanWS):
    """
    Fits a spline to a single-spectrum workspace (in d-spacing)

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param vanWS :: Vanadium workspace to fit (normally this contains spectra for a single bank)

    @returns fit workspace (MatrixWorkspace), with the same number of bins as the input
    workspace, and the Y values simulated from the fitted curve
    """
    expectedDim = 'dSpacing'
    dimType = vanWS.getXDimension().getName()
    if expectedDim != dimType:
        raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                         "the X dimension of the input workspace is: '%s'" % (expectedDim, dimType))

    if 1 != vanWS.getNumberHistograms():
        raise ValueError("The workspace does not have exactly one histogram. Inconsistency found.")

    functionName = 'BSpline'
    fitAlg = parent.createChildAlgorithm('Fit')
    fitAlg.setProperty('Function', 'name=' + functionName)
    fitAlg.setProperty('InputWorkspace', vanWS)
    # WorkspaceIndex is left to default '0' for 1D function fits
    # StartX, EndX left to default start/end of the spectrum
    fitWSName = 'vanadium_bank_fit'
    fitAlg.setProperty('Output', fitWSName)
    fitAlt.execute()
    fitWS = None
    fitWSName = fitWSName + '_Workspace'
    try:
        fitWS = mtd[fitWSName]
    except RuntimeError:
        raise RuntimeError("Could not find the workspace %s. It seems that this algorithm failed "
                           "to fit a function '%s' to the summed spectra of a bank: %s"
                           % (fitWSName, functionName, str(re)))
    return fitWS

def integrateSpectra(parent, ws):
    """
    Integrates all the spectra or a workspace, and return the result.
    Simply uses 'Integration' as a child algorithm.

    @param parent :: parent (Mantid) algorithm that wants to run this
    @param ws :: workspace (MatrixWorkspace)

    @returns integrated workspace, or result of integrating every spectra in the input workspace
    """
    intAlg = parent.createChildAlgorithm('Integration')
    intAlg.setPropertyValue('InputWorkspace', ws)
    intAlg.execute()
    ws = intAlg.getPropertyValue('OutputWorkspace')

    return ws
