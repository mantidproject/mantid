#pylint: disable=invalid-name
import os

from mantid.api import *
import mantid.simpleapi as sapi

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
