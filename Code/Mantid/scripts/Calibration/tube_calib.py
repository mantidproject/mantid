"""
This file is concerned with calibrating a specified set of tubes

The main function is :func:`getCalibration` which is at the end of this file.
It populates an empty Calibration Table Workspace with the new positions of the pixel detectors after calibration.
This Calibration Table Workspace can be used later to move the pixel detectors to the calibrated positions.

Users should not need to directly call any other function other than :func:`getCalibration` from this file.

"""
## Author: Karl palmen ISIS and for readPeakFile Gesner Passos ISIS


import numpy
from mantid.simpleapi import *
from mantid.kernel import *
from tube_spec import TubeSpec
from ideal_tube import IdealTube
import re
import os
import copy


def createTubeCalibtationWorkspaceByWorkspaceIndexList ( integratedWorkspace, outputWorkspace, workspaceIndexList, xUnit='Pixel', showPlot=False):
    """
       Creates workspace with integrated data for one tube against distance along tube
       The tube is specified by a list of workspace indices of its spectra

       @param IntegratedWorkspace: Workspace of integrated data
       @param workspaceIndexList:  list of workspace indices for the tube
       @param xUnit: unit of distance ( Pixel)
       @param showPlot: True = show plot of workspace created, False = just make the workspace.

       Return Value: Workspace created

    """

    nSpectra = len(workspaceIndexList)
    if( nSpectra < 1):
        return
    pixelNumbers = []
    integratedPixelCounts = []
    pixel = 1
    #integratedWorkspace.
    for i in workspaceIndexList:
     pixelNumbers.append(pixel)
     pixel = pixel + 1
     integratedPixelCounts.append( integratedWorkspace.dataY(i)[0] )

    CreateWorkspace(dataX=pixelNumbers,dataY=integratedPixelCounts, OutputWorkspace=outputWorkspace)
    #if (showPlot):
          #plotSpectrum(outputWorkspace,0)
          # For some reason plotSpectrum is not recognised, but instead we can plot this worspace afterwards.


# Return the udet number and [x,y,z] position of the detector (or virtual detector) corresponding to spectra spectra_number
# Thanks to Pascal Manuel for this function
def get_detector_pos(work_handle,spectra_number):
    udet=work_handle.getDetector(spectra_number)
    return udet.getID(), udet.getPos()

# Given the center of a slit in pixels return the interpolated y
#  Converts from pixel coords to Y.
#     If a pixel coord is not integer
#     it is effectively rounded to half integer before conversion, rather than interpolated.
#     It allows the pixel widths to vary (unlike correctTube).
# Thanks to Pascal Manuel for this function
def get_ypos(work_handle,pixel_float):
    center_low_pixel=int(math.floor(pixel_float))
    center_high_pixel=int(math.ceil(pixel_float))
    idlow, low=get_detector_pos(work_handle,center_low_pixel) #Get the detector position of the nearest lower pixel
    idhigh, high=get_detector_pos(work_handle,center_high_pixel) #Get the detector position of the nearest higher pixel
    center_y=(center_high_pixel-pixel_float)*low.getY()+(pixel_float-center_low_pixel)*high.getY()
    center_y/=(center_high_pixel-center_low_pixel)
    return center_y


def fitGaussianParams ( height, centre, sigma ): # Compose string argument for fit
    #print "name=Gaussian, Height="+str(height)+", PeakCentre="+str(centre)+", Sigma="+str(sigma)
    return "name=Gaussian, Height="+str(height)+", PeakCentre="+str(centre)+", Sigma="+str(sigma)

def fitEndErfcParams ( B, C ): # Compose string argument for fit
    #print "name=EndErfc, B="+str(B)+", C="+str(C)
    return "name=EndErfc, B="+str(B)+", C="+str(C)

#
# definition of the functions to fit
#

def fitEdges(fitPar, index, ws, outputWs):
    # find the edge position
    centre = fitPar.getPeaks()[index]
    outedge, inedge, endGrad = fitPar.getEdgeParameters()
    margin = fitPar.getMargin()
    #get values around the expected center
    all_values = ws.dataY(0)
    RIGHTLIMIT = len(all_values)
    values = all_values[max(centre-margin,0):min(centre+margin,len(all_values))]

    #identify if the edge is a sloping edge or descent edge
    descentMode =  values[0] > values[-1]
    if descentMode:
        start = max(centre - outedge,0)
        end = min(centre + inedge, RIGHTLIMIT)
        edgeMode = -1
    else:
        start = max(centre - inedge,0)
        end = min(centre + outedge, RIGHTLIMIT)
        edgeMode = 1
    Fit(InputWorkspace=ws,Function=fitEndErfcParams(centre,endGrad*edgeMode),StartX=str(start),EndX=str(end),Output=outputWs)
    return 1 # peakIndex (center) -> parameter B of EndERFC

def fitGaussian(fitPar, index, ws, outputWs):
    #find the peak position
    centre = fitPar.getPeaks()[index]
    margin = fitPar.getMargin()

    # get values around the expected center
    all_values = ws.dataY(0)

    RIGHTLIMIT = len(all_values)

    min_index = max(centre-margin,0)
    max_index = min(centre+margin, RIGHTLIMIT)
    values = all_values[min_index:max_index]

    # find the peak position
    if fitPar.getAutomatic():
        # find the parameters for fit dynamically
        max_value = numpy.max(values)
        min_value = numpy.min(values)
        half = (max_value - min_value) * 2 / 3 + min_value
        above_half_line = len(numpy.where(values > half)[0])
        beyond_half_line = len(values) - above_half_line
        if above_half_line < beyond_half_line:
            # means that there are few values above the midle, so it is a peak
            centre = numpy.argmax(values) + min_index
            background = min_value
            height = max_value - background
            width = len(numpy.where(values > height/2 + background))
        else:
            # means that there are many values above the midle, so it is a trough
            centre = numpy.argmin(values) + min_index
            background = max_value
            height = min_value - max_value # negative value
            width = len(numpy.where(values < min_value + height/2))

        start = max(centre - margin, 0)
        end = min(centre + margin, RIGHTLIMIT)

        fit_msg = 'name=LinearBackground,A0=%f;name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f'%(background, height, centre, width)

        Fit(InputWorkspace=ws, Function=fit_msg,
                StartX = str(start), EndX=str(end), Output=outputWs)

        peakIndex = 3

    else:
        # get the parameters from fitParams
        background = 1000
        height, width = fitPar.getHeightAndWidth()
        start = max(centre-margin, 0)
        end = min(centre+margin, RIGHTLIMIT)

        # fit the input data as a linear background + gaussian fit
        # it was seen that the best result for static general fitParamters,
        # is to divide the values in two fitting steps
        Fit(InputWorkspace=ws, Function='name=LinearBackground,A0=%f'%(background),
            StartX=str(start), EndX=str(end), Output='Z1')
        Fit(InputWorkspace='Z1_Workspace',Function='name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f' %(height, centre, width),
            WorkspaceIndex=2, StartX=str(start), EndX=str(end), Output=outputWs)
        CloneWorkspace(outputWs+'_Workspace',OutputWorkspace='gauss_'+str(index))
        peakIndex = 1

    return peakIndex



def getPoints ( IntegratedWorkspace, funcForms, fitParams, whichTube, showPlot=False ):
    """
    Get the centres of N slits or edges for calibration

    It does look for the peak position in pixels by fitting the peaks and
    edges. It is the method responsible for estimating the peak position in each tube.

    .. note::
      This N slit method is suited for WISH or the five sharp peaks of MERLIN .

    :param IntegratedWorkspace: Workspace of integrated data
    :param funcForms: array of function form 1=slit/bar, 2=edge
    :param fitParams: a TubeCalibFitParams object contain the fit parameters
    :param whichTube:  a list of workspace indices for one tube (define a single tube)
    :param showPlot: show plot for this tube

    :rtype: array of the slit/edge positions (-1.0 indicates failed to find position)

    """

    # Create input workspace for fitting
    ## get all the counts for the integrated workspace inside the tube
    countsY = numpy.array([IntegratedWorkspace.dataY(i)[0] for i in whichTube])
    if (len(countsY) == 0):
        return
    getPointsWs = CreateWorkspace(range(len(countsY)),countsY,OutputWorkspace='TubePlot')
    calibPointWs = 'CalibPoint'
    results = []
    fitt_y_values = []
    fitt_x_values = []


    # Loop over the points
    for i in range(len(funcForms)):
      if funcForms[i] == 2:
        # find the edge position
        peakIndex = fitEdges(fitParams, i, getPointsWs, calibPointWs)
      else:
        peakIndex = fitGaussian(fitParams, i, getPointsWs, calibPointWs)

      # get the peak centre
      peakCentre = mtd[calibPointWs + '_Parameters'].row(peakIndex).items()[1][1]
      results.append(peakCentre)

      if showPlot:
        ws = mtd[calibPointWs + '_Workspace']
        fitt_y_values.append(copy.copy(ws.dataY(1)))
        fitt_x_values.append(copy.copy(ws.dataX(1)))

    if showPlot:
        FittedData = CreateWorkspace(numpy.hstack(fitt_x_values),
                                         numpy.hstack(fitt_y_values))
    return results

def getIdealTubeFromNSlits ( IntegratedWorkspace, slits ):
    """
       Given N slits for calibration on an ideal tube
       convert to Y values to form a ideal tube for correctTubeToIdealTube()

       @param IntegratedWorkspace: Workspace of integrated data
       @param eP: positions of slits for ideal tube (in pixels)

       Return Value: Ideal tube in Y-coords for use by correctTubeToIdealTube()

    """
    ideal = []
    # print "slits for ideal tube", slits
    for i in range(len(slits)):
        # print slits[i]
        ideal.append( get_ypos( IntegratedWorkspace, slits[i] )) # Use Pascal Manuel's Y conversion.

    # print "Ideal Tube",ideal
    return ideal


def correctTube( AP, BP, CP, nDets ):
    """
       Corrects position errors in a tube in the same manner as is done for MERLIN
       according to an algorithm used by Rob Bewley in his MATLAB code.

       @param AP: Fit position of left (in pixels)
       @param BP: Fit position of right (in pixels)
       @param CP: Fit position of centre (in pixels)
       @param nDets: Number of pixel detectors in tube

       Return Value: Array of corrected Xs  (in pixels)
    """

    AO = AP/(nDets - AP)
    BO = (nDets - BP)/BP
    # First correct centre point for offsets
    CPN = CP - (AO*(nDets - CP)) + BO*CP
    x = []
    for i in range(nDets):
        xi = i+1.0
        x.append( xi -((nDets-xi)*AO) + (xi*BO)) #this is x corrected for offsets

    # Now calculate the gain error
    GainError = ( (nDets+1)/2.0 - CPN) / (CPN*(nDets-CPN))
    xBinNew = []
    for i in range(nDets):
        xo = x[i]
        xBinNew.append( xo + ( xo*(nDets-xo)*GainError )) #Final bin position values corrected for offsets and gain

    # print xBinNew
    return xBinNew

def correctTubeToIdealTube( tubePoints, idealTubePoints, nDets, TestMode=False, polinFit=2 ):
    """
       Corrects position errors in a tube given an array of points and their ideal positions.

       :param tubePoints: Array of Slit Points along tube to be fitted (in pixels)
       :param idealTubePoints: The corresponding points in an ideal tube (Y-coords advised)
       :param nDets: Number of pixel detectors in tube
       :param Testmode: If true, detectors at the position of a slit will be moved out of the way
                         to show the reckoned slit positions when the instrument is displayed.
       :param polinFit: Order of the polinomial to fit for the ideal positions

       Return Value: Array of corrected Xs  (in same units as ideal tube points)

       Note that any element of tubePoints not between 0.0 and nDets is considered a rogue point and so is ignored.
    """

    #print "correctTubeToIdealTube"

    # Check the arguments
    if ( len(tubePoints) != len(idealTubePoints) ):
        print "Number of points in tube", len(tubePoints),"must equal number of points in ideal tube", len(idealTubePoints)
        return xResult

    # Filter out rogue slit points
    usedTubePoints = []
    usedIdealTubePoints = []
    missedTubePoints = [] # Used for diagnostic print only
    for i in range(len(tubePoints)):
        if( tubePoints[i] > 0.0 and tubePoints[i] < nDets):
            usedTubePoints.append( tubePoints[i] )
            usedIdealTubePoints.append ( idealTubePoints[i] )
        else:
            missedTubePoints.append(i+1)

    # State number of rogue slit points, if any
    if( len(tubePoints) != len(usedTubePoints)):
        print "Only",len(usedTubePoints),"out of",len(tubePoints)," slit points used. Missed",missedTubePoints

    # Check number of usable points
    if( len(usedTubePoints) < 3):
        print "Too few usable points in tube",len(usedTubePoints)
        return []

    # Fit quadratic to ideal tube points
    CreateWorkspace(dataX=usedTubePoints,dataY=usedIdealTubePoints, OutputWorkspace="PolyFittingWorkspace")
    try:
       Fit(InputWorkspace="PolyFittingWorkspace",Function='name=Polynomial,n=%d'%(polinFit),StartX=str(0.0),EndX=str(nDets),Output="QF")
    except:
       print "Fit failed"
       return []

    paramQF = mtd['QF_Parameters']

    # get the coeficients, get the Value from every row, and exclude the last one because it is the error
    # rowErr is the last one, it could be used to check accuracy of fit
    c = [r['Value'] for r in paramQF][:-1]

    # Modify the output array by the fitted quadratic
    xResult = numpy.polynomial.polynomial.polyval(range(nDets),c)

    # In test mode, shove the pixels that are closest to the reckoned peaks
    # to the position of the first detector so that the resulting gaps can be seen.
    if( TestMode ):
        print "TestMode code"
        for i in range( len(usedTubePoints) ):
           #print "used point",i,"shoving pixel",int(usedTubePoints[i])
           xResult[ int(usedTubePoints[i]) ] = xResult[0]

    # print xResult
    return xResult


def getCalibratedPixelPositions( ws, tubePts, idealTubePts, whichTube, peakTestMode=False, polinFit=2 ):
    """
       Get the calibrated detector positions for one tube
       The tube is specified by a list of workspace indices of its spectra
       Calibration is assumed to be done parallel to the Y-axis

       :param ws: Workspace with tubes to be calibrated - may be integrated or raw
       :param tubePts: Array of calibration positions (in pixels)
       :param idealTubePts: Where these calibration positions should be (in Y coords)
       :param whichtube:  a list of workspace indices for the tube
       :param PeakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
       :param polinFit: Order of the polinominal to fit for the ideal positions

       Return  Array of pixel detector IDs and array of their calibrated positions
    """

    # Arrays to be returned
    detIDs = []
    detPositions = []
    #Get position of first and last pixel of tube
    nDets = len(whichTube)
    if( nDets < 1):
        return  detIDs, detPositions

    # Correct positions of detectors in tube by quadratic fit
    pixels = correctTubeToIdealTube ( tubePts, idealTubePts, nDets, TestMode=peakTestMode, polinFit=polinFit )
    #print pixels
    if( len(pixels) != nDets):
       print "Tube correction failed."
       return detIDs, detPositions
    baseInstrument = ws.getInstrument().getBaseInstrument()
    # Get tube unit vector
    # get the detector from the baseInstrument, in order to get the positions
    # before any calibration being loaded.
    det0 = baseInstrument.getDetector(ws.getDetector( whichTube[0]).getID())
    detN = baseInstrument.getDetector(ws.getDetector (whichTube[-1]).getID())
    d0pos,dNpos = det0.getPos(),detN.getPos()
    ## identical to norm of vector: |dNpos - d0pos|
    tubeLength = det0.getDistance(detN)
    if( tubeLength <= 0.0):
        print "Zero length tube cannot be calibrated, calibration failed."
        return detIDs, detPositions
    #unfortunatelly, the operation '/' is not defined in V3D object, so
    #I have to use the multiplication.
    # unit_vectors are defined as u = (v2-v1)/|v2-v1| = (dn-d0)/length
    unit_vector = (dNpos-d0pos) * (1.0/tubeLength)

    # Get Centre (really want to get if from IDF to allow calibration a multiple number of times)
    center = (dNpos+d0pos)*0.5 #(1.0/2)

    # Move the pixel detectors (might not work for sloping tubes)
    for i in range(nDets):
        deti = ws.getDetector( whichTube[i])
        det_pos = deti.getPos()
        pNew = pixels[i]
        # again, the opeartion float * v3d is not defined, but v3d * float is,
        # so, I wrote the new pos as center + unit_vector * (float)
        newPos = center + unit_vector * pNew

        detIDs.append( deti.getID() )
        detPositions.append( newPos )
        # print i, detIDs[i], detPositions[i]

    return detIDs, detPositions


def readPeakFile(file_name):
    """Load the file calibration

    It returns a list of tuples, where the first value is the detector identification
    and the second value is its calibration values.

    Example of usage:
        for (det_code, cal_values) in readPeakFile('pathname/TubeDemo'):
            print det_code
            print cal_values

    """
    loaded_file = []
    #split the entries to the main values:
    # For example:
    # MERLIN/door1/tube_1_1 [34.199347724575574, 525.5864438725401, 1001.7456248836971]
    # Will be splited as:
    # ['MERLIN/door1/tube_1_1', '', '34.199347724575574', '', '525.5864438725401', '', '1001.7456248836971', '', '', '']
    pattern = re.compile('[\[\],\s\r]')
    saveDirectory = config['defaultsave.directory']
    pfile = os.path.join(saveDirectory, file_name)
    for line in open(pfile,'r'):
        #check if the entry is a comment line
        if line.startswith('#'):
            continue
        #split all values
        line_vals = re.split(pattern,line)
        id_ = line_vals[0]
        if id_ == '':
            continue
        try:
            f_values = [float(v) for v in line_vals[1:] if v!='']
        except ValueError:
            #print 'Wrong format: we expected only numbers, but receive this line ',str(line_vals[1:])
            continue

        loaded_file.append((id_,f_values))
    return loaded_file



### THESE FUNCTIONS NEXT SHOULD BE THE ONLY FUNCTIONS THE USER CALLS FROM THIS FILE

def getCalibration( ws, tubeSet, calibTable, fitPar, iTube, peaksTable,
                    overridePeaks=dict(), excludeShortTubes=0.0, plotTube=[],
                    rangeList = None, polinFit=2, peaksTestMode=False):
    """
    Get the results the calibration and put them in the calibration table provided.

    :param ws: Integrated Workspace with tubes to be calibrated
    :param tubeSet: Specification of Set of tubes to be calibrated ( :class:`~tube_spec.TubeSpec` object)
    :param calibTable: Empty calibration table into which the calibration results are placed. It is composed by 'Detector ID' and a V3D column 'Detector Position'. It will be filled with the IDs and calibrated positions of the detectors.
    :param fitPar: A :class:`~tube_calib_fit_params.TubeCalibFitParams` object for fitting the peaks
    :param iTube: The :class:`~ideal_tube.IdealTube` which contains the positions in metres of the shadows of the slits, bars or edges used for calibration.
    :param peaksTable: Peaks table into wich the peaks positions will be put
    :param overridePeak: dictionary with tube indexes keys and an array of peaks in pixels to override those that would be fitted for one tube
    :param exludeShortTubes: Exlude tubes shorter than specified length from calibration
    :param plotTube: List of tube indexes that will be ploted
    :param rangelist: list of the tube indexes that will be calibrated. Default None, means all the tubes in tubeSet
    :param polinFit: Order of the polinomial to fit against the known positions. Acceptable: 2, 3
    :param peakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)


    This is the main method called from :func:`~tube.calibrate` to perform the calibration.
    """
    nTubes = tubeSet.getNumTubes()
    print "Number of tubes =",nTubes

    if rangeList is None:
        rangeList = range(nTubes)

    all_skipped = set()

    for i in rangeList:

        # Deal with (i+1)st tube specified
        wht, skipped = tubeSet.getTube(i)
        all_skipped.update(skipped)

        print "Calibrating tube", i+1,"of",nTubes, tubeSet.getTubeName(i)
        if ( len(wht) < 1 ):
           print "Unable to get any workspace indices (spectra) for this tube. Tube",tubeSet.getTubeName(i),"not calibrated."
           #skip this tube
           continue

        # Calibribate the tube, if possible
        if (tubeSet.getTubeLength(i) <= excludeShortTubes):
            #skip this tube
            continue

        ##############################
        # Define Peak Position session
        ##############################

        # if this tube is to be override, get the peaks positions for this tube.
        if overridePeaks.has_key(i):
            actualTube = overridePeaks[i]
        else:
            #find the peaks positions
            plotThisTube = i in plotTube
            actualTube = getPoints(ws, iTube.getFunctionalForms(), fitPar, wht, showPlot = plotThisTube)
            if plotThisTube:
                RenameWorkspace('FittedData',OutputWorkspace='FittedTube%d'%(i))
                RenameWorkspace('TubePlot', OutputWorkspace='TubePlot%d'%(i))


        # Set the peak positions at the peakTable
        peaksTable.addRow([tubeSet.getTubeName(i)] + list(actualTube))

        ##########################################
        # Define the correct position of detectors
        ##########################################

        detIDList, detPosList = getCalibratedPixelPositions( ws, actualTube, iTube.getArray(), wht, peaksTestMode, polinFit)
        #save the detector positions to calibTable
        if( len(detIDList) == len(wht)): # We have corrected positions
            for j in range(len(wht)):
                nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
                calibTable.addRow ( nextRow )

    if len(all_skipped) > 0:
        print "%i histogram(s) were excluded from the calibration since they did not have an assigned detector." % len(all_skipped)

    # Delete temporary workspaces used in the calibration
    for ws_name in ('TubePlot','CalibPoint_NormalisedCovarianceMatrix',
                    'CalibPoint_NormalisedCovarianceMatrix','CalibPoint_NormalisedCovarianceMatrix',
                    'CalibPoint_Parameters', 'CalibPoint_Workspace', 'PolyFittingWorkspace',
                    'QF_NormalisedCovarianceMatrix', 'QF_Parameters', 'QF_Workspace',
                    'Z1_Workspace', 'Z1_Parameters', 'Z1_NormalisedCovarianceMatrix'):
        try:
            DeleteWorkspace(ws_name)
        except:
            pass



def getCalibrationFromPeakFile ( ws, calibTable, iTube,  PeakFile ):
    """
       Get the results the calibration and put them in the calibration table provided.

       @param ws: Integrated Workspace with tubes to be calibrated
       @param calibTable: Calibration table into which the calibration results are placed
       @param  iTube: The ideal tube
       @param PeakFile: File of peaks for calibtation

    """

    # Get Ideal Tube
    idealTube = iTube.getArray()

    # Read Peak File
    PeakArray = readPeakFile( PeakFile )
    nTubes = len(PeakArray)
    print "Number of tubes read from file =",nTubes

    for i in range(nTubes):

        # Deal with (i+1)st tube got from file
        TubeName = PeakArray[i][0] # e.g. 'MERLIN/door3/tube_3_1'
        tube = TubeSpec(ws)
        tube.setTubeSpecByString(TubeName)
        actualTube = PeakArray[i][1] # e.g.  [2.0, 512.5, 1022.0]

        wht, _ = tube.getTube(0)
        print "Calibrating tube", i+1 ,"of", nTubes, TubeName #, " length", tubeSet.getTubeLength(i)
        if ( len(wht) < 1 ):
            print "Unable to get any workspace indices for this tube. Calibration abandoned."
            return

        detIDList, detPosList = getCalibratedPixelPositions( ws, actualTube, idealTube, wht)

        #print len(wht)
        if( len(detIDList) == len(wht)): # We have corrected positions
            for j in range(len(wht)):
                nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
                calibTable.addRow ( nextRow )

    if(nTubes == 0):
       return

    # Delete temporary workspaces for getting new detector positions
    DeleteWorkspace('PolyFittingWorkspace')
    DeleteWorkspace('QF_NormalisedCovarianceMatrix')
    DeleteWorkspace('QF_Parameters')
    DeleteWorkspace('QF_Workspace')



## implement this function
def constructIdealTubeFromRealTube( ws, tube, fitPar, funcForm ):
   """
   Construct an ideal tube from an actual tube (assumed ideal)

   :param ws: integrated workspace
   :param tube: specification of one tube (if several tubes, only first tube is used)
   :param fitPar: initial fit parameters for peak of the tube
   :param funcForm: listing the type of known positions 1=Gaussian; 2=edge
   :rtype: IdealTube

   """
   # Get workspace indices
   idealTube = IdealTube()

   nTubes = tube.getNumTubes()
   if(nTubes < 1):
       raise RuntimeError("Invalid tube specification received by constructIdealTubeFromRealTube")
   elif(nTubes > 1):
      print "Specification has several tubes. The ideal tube will be based on the first tube",tube.getTubeName(0)

   wht, _ = tube.getTube(0)
   # print wht

   # Check tube
   if ( len(wht) < 1 ):
        raise RuntimeError("Unable to get any workspace indices for this tube. Cannot use as ideal tube.")

   # Get actual tube on which ideal tube is based
   actualTube = getPoints ( ws, funcForm, fitPar, wht)
   print "Actual tube that ideal tube is to be based upon",actualTube

   # Get ideal tube based on this actual tube
   try:
       idealTube.setArray(actualTube)
   except:
       msg = "Attempted to create ideal tube based on actual tube" + str(actualTube)
       msg += "Unable to create ideal tube."
       msg += "Please choose another tube for constructIdealTubeFromRealTube()."
       raise RuntimeError(msg)
   return idealTube

