#
#  This file is concerned with calibrating a specified set of tubes
#
#  The main function is getCaibration(....) which is at the end of this file. 
#  It populates an empty Calibration Table Workspace with the new positions of the pixel detectors after calibration.
#  This Calibration Table Workspace can be used later to move the pixel detectors to the calibrated positions.
#
#  Users should not need to directly call any other function other than getCaibration(....) from this file.
#
from mantid.simpleapi import *
from mantid.kernel import *
import math


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
    if (showPlot):
          plotSpectrum(outputWorkspace,0)
          
          
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
    
def get3pointsFor3pointMethod ( IntegratedWorkspace, whichTube, fitParams):
    """     
       Get the three points for the three point calibration method
       from a centre peak, the left rise and right fall of the function.
       This 3 point method is suited for MERLIN.
       
       @param IntegratedWorkspace: Workspace of integrated data
       @param whichTube:  a list of workspace indices for one tube
       @param fitParams: a TubeCalibFitParams object contain the fit parameters
   
       Return Value: the three points (left, right, centre)
       
    """	
    nDets = len(whichTube)
    
    #Values that may need adjusting for instrument and tube type (must not be integer)
    yVal, sigma = fitParams.getHeightAndWidth()

    # We don't currently use the peaks of fitParams, but use hardcoded values
    centre = nDets/2 # Centre peak position (pixels)
    left = (5.0, 35.0, 85.0) # expected left in pixels
    right =  (nDets-left[2], nDets-left[1], nDets-left[0]) # expected right in pixels
    endGrad = 6.0 #expected end gradient

    createTubeCalibtationWorkspaceByWorkspaceIndexList( IntegratedWorkspace, "get3pointsFor3pointMethod", whichTube, showPlot=False )

    #Get three points for calibration
    Fit(InputWorkspace="get3pointsFor3pointMethod",Function='name=LinearBackground, A0=1000',StartX=str(centre-2*sigma),EndX=str(centre+2*sigma),Output="Z1")

    #Fit(InputWorkspace="Z1_Workspace",Function='name=Gaussian, Height=4500.0, PeakCentre=512.0, Sigma=34.0', WorkspaceIndex='2',StartX='444.0',EndX='580.0',Output="CentrePoint")
    Fit(InputWorkspace="Z1_Workspace",Function=fitGaussianParams(yVal,centre,sigma), WorkspaceIndex='2',StartX=str(centre-5*sigma),EndX=str(centre+5*sigma),Output="CentrePoint")


    paramG1 = mtd['CentrePoint_Parameters']


    #Fit(InputWorkspace="get3pointsFor3pointMethod",Function='name=EndErfc, B=24.0, C=6.0 ',StartX='6.0',EndX='90.0',Output="LeftPoint")
    Fit(InputWorkspace="get3pointsFor3pointMethod",Function=fitEndErfcParams(left[1],endGrad),StartX=str(left[0]),EndX=str(left[2]),Output="LeftPoint")  
  

    paramL1 = mtd['LeftPoint_Parameters']


    #Fit(InputWorkspace="get3pointsFor3pointMethod",Function='name=EndErfc, B=980.0, C=-6.0 ',StartX='940.0',EndX='1023.0',Output="RightPoint")
    Fit(InputWorkspace="get3pointsFor3pointMethod",Function=fitEndErfcParams(right[1],-endGrad),StartX=str(right[0]),EndX=str(right[2]),Output="RightPoint")
        
    paramR1 = mtd['RightPoint_Parameters']
    
    #Check fits
    #  we just check that there is a positive peak in centre
    htRow = paramG1.row(0).items()
    ht = htRow[1][1]
    htOK = (ht > 0.0)

    l = paramL1.row(1).items()
    g = paramG1.row(1).items()
    r = paramR1.row(1).items()
    A = l[1][1]
    B = r[1][1]
    if( htOK ):
        C = g[1][1]
    else:
        C = (A+B)/2.0  # If we haven't got a centre peak, we put one half way between the end points.
    return A, B, C 
    
def getPeaksForNSlitsMethod ( IntegratedWorkspace, eP, eHeight, eWidth, whichTube):
    """     
       Get the centres of N slits for calibration 
       This N slit method is suited for WISH or the five sharp peaks of MERLIN .
       
       @param IntegratedWorkspace: Workspace of integrated data
       @param eP: array of expected positions of the slits (in pixels)
       @param eHeight: expected height of slits
       @param eWidth: expected width of slits
       @param whichTube:  a list of workspace indices for one tube
   
       Return Value: array of the slit positions (-1.0 indicates failed to find slit position)
       
    """	
    nDets = len(whichTube)
    nPeaks = len(eP)
    results = []
    
    createTubeCalibtationWorkspaceByWorkspaceIndexList( IntegratedWorkspace, "getPeaksForNSlitsMethod", whichTube, showPlot=False )
    
    for i in range(nPeaks):
        margin = 0.3
        if( i == 0):
           start = (1-margin)*eP[i]
        else:
           start = margin*eP[i-1] + (1-margin)*eP[i]
        centre = eP[i]
        if( i == nPeaks-1):
           end = (1-margin)*eP[i] + (margin)*nDets
        else:
           end = (1-margin)*eP[i] + (margin)*eP[i+1]
	   
        Fit(InputWorkspace="getPeaksForNSlitsMethod",Function=fitGaussianParams(eHeight,centre,eWidth), StartX=str(start),EndX=str(end),Output="CalibPeak")
        paramCalibPeak = mtd['CalibPeak_Parameters']
        pkRow = paramCalibPeak.row(1).items()
        thisResult = pkRow[1][1]
        if( start < thisResult and thisResult < end):
            results.append( thisResult )
        else:
            results.append(-1.0) #ensure result is ignored
    
    #print whichTube[0], results    
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
    print "slits for ideal tube", slits
    for i in range(len(slits)):
        print slits[i]
        ideal.append( get_ypos( IntegratedWorkspace, slits[i] )) # Use Pascal Manuel's Y conversion.
    
    print "Ideal Tube",ideal   
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
    
def correctTubeToIdealTube( tubePoints, idealTubePoints, nDets, TestMode=False ): 
    """     
       Corrects position errors in a tube given an array of points and their ideal positions.
       
       @param tubePoints: Array of Slit Points along tube to be fitted (in pixels) 
       @param idealTubePoints: The corresponding points in an ideal tube (Y-coords advised)
       @param nDets: Number of pixel detectors in tube
       @parame Testmode: If true, detectors at the position of a slit will be moved out of the way
                         to show the reckoned slit positions when the instrument is displayed.

       Return Value: Array of corrected Xs  (in same units as ideal tube points)
       
       Note that any element of tubePoints not between 0.0 and nDets is considered a rogue point and so is ignored. 
    """           
    
    xResult = []
    #print "correctTubeToIdealTube"

    # Check the arguments
    if ( len(tubePoints) != len(idealTubePoints) ):
        print "Number of points in tube", len(tubePoints),"must equal number of points in ideal tube", len(idealTubePoints)
        return xResult
        
    # Filter out rogue slit points
    usedTubePoints = []
    usedIdealTubePoints = []
    for i in range(len(tubePoints)):
        if( tubePoints[i] > 0.0 and tubePoints[i] < nDets): 
            usedTubePoints.append( tubePoints[i] )
            usedIdealTubePoints.append ( idealTubePoints[i] )
            
    # State number of rogue slit points, if any
    if( len(tubePoints) != len(usedTubePoints)):
        print "Only",len(usedTubePoints),"out of",len(tubePoints)," slit points used."
    
    # Check number of usable points
    if( len(usedTubePoints) < 3):
        print "Too few usable points in tube",len(usedTubePoints)
        return xResult
                
    # Fit quadratic to ideal tube points 
    CreateWorkspace(dataX=usedTubePoints,dataY=usedIdealTubePoints, OutputWorkspace="QuadraticFittingWorkspace")
    try:
       Fit(InputWorkspace="QuadraticFittingWorkspace",Function='name=Quadratic',StartX=str(0.0),EndX=str(nDets),Output="QF")
    except:
       print "Fit failed"
       return xResult
    
    paramQF = mtd['QF_Parameters']
    rowP0 = paramQF.row(0).items()
    rowP1 = paramQF.row(1).items()
    rowP2 = paramQF.row(2).items()
    rowErr = paramQF.row(3).items() # may use to check accuracy of fit
    
    p0 = rowP0[1][1]
    p1 = rowP1[1][1]
    p2 = rowP2[1][1]
    
    # Modify the output array by the fitted quadratic
    for i in range(nDets):
        xResult.append( p0 + p1*i + p2*i*i)
        
    # In test mode, shove the pixels that are closest to the reckoned peaks 
    # to the position of the first detector so that the resulting gaps can be seen.
    if( TestMode ):
        print "TestMode code"
        for i in range( len(usedTubePoints) ):
           #print "used point",i,"shoving pixel",int(usedTubePoints[i]+0.5)
           xResult[ int(usedTubePoints[i]+0.5) ] = xResult[0]
         
    # print xResult	 
    return xResult
   

def getCalibratedPixelPositions( ws, tubePts, idealTubePts, whichTube, peakTestMode ):
    """     
       Get the calibrated detector positions for one tube
       The tube is specified by a list of workspace indices of its spectra
       Calibration is assumed to be done parallel to the Y-axis
             
       @param ws: Workspace with tubes to be calibrated - may be integrated or raw
       @param tubePts: Array of calibration positions (in pixels)
       @param idealTubePts: Where these calibration positions should be (in Y coords)
       @param whichtube:  a list of workspace indices for the tube
       @param PeakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
       
       Return  Array of pixel detector IDs and array of their calibrated positions (in Y coords)
    """	
    
    # Arrays to be returned
    detIDs = []
    detPositions = []
    #Get position of first and last pixel of tube
    nDets = len(whichTube)
    if( nDets < 1):
        return  detIDs, detPositions 

    # Correct positions of detectors in tube by quadratic fit
    pixels = correctTubeToIdealTube ( tubePts, idealTubePts, nDets, TestMode=peakTestMode )
    #print pixels
    if( len(pixels) != nDets):
       print "Tube correction failed."
       return detIDs, detPositions 
       
    for i in range(nDets):
        deti = ws.getDetector( whichTube[i])
	detiPositionX = deti.getPos().X()
	detiPositionZ = deti.getPos().Z()
	yNew = pixels[i]
	detIDs.append( deti.getID() )
	detPositions.append(  V3D( detiPositionX, yNew, detiPositionZ ) )
	# print i, detIDs[i], detPositions[i]

    return detIDs, detPositions
    
def getCalibration ( ws, tubeSet, calibTable, fitPar, iTube, PeakTestMode=False, OverridePeaks=[], PeakFile="", ExcludeShortTubes=0.0):
    """     
       Get the results the calibration and put them in the calibration table provided.
       
       THIS SHOULD BE THE ONLY FUNCTION THE USER CALLS FROM THIS FILE
             
       @param ws: Integrated Workspace with tubes to be calibrated 
       @param tubeSet: Specification of Set of tubes to be calibrated
       @param calibTable: Empty calibration table into which the calibration results are placed
       @param fitPar: A TubeCalibFitParam object for fitting the peaks
       @param  iTube: The ideal tube
       @param PeakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
       @param OverridePeaks: if non-zero length an array of peaks in pixels to override those that would be fitted for one tube
       @param PeakFile: Optional output file for peaks 
       @param ExludeShortTubes: Exlude tubes shorter than specified length from calibration
       
    """	
    
    nTubes = tubeSet.getNumTubes()
    print "Number of tubes =",nTubes
    
    # Get Ideal Tube 
    idealTube = iTube.getArray()
    
    # Get expected positions of peaks if required
    if( not fitPar.isThreePointMethod() ):
        eP = fitPar.getPeaks()
        
    # Check override of peakfitting
    overrideFit = ( len(OverridePeaks) > 0 )
    
    # Open Peaks File if specificed
    if( PeakFile != ""):
       pFile = open(PeakFile,'w')

    for i in range(nTubes):

        # Deal with (i+1)st tube specified
        wht = tubeSet.getTube(i)
        print "tube listed", i+1, tubeSet.getTubeName(i), " length", tubeSet.getTubeLength(i) 
        if ( len(wht) < 1 ):
            print "Unable to get any workspace indices for this tube. Calibration abandoned."
            return
            
        # Calibribate the tube, if possible
        if( tubeSet.getTubeLength(i) >= ExcludeShortTubes ):  # Only calibrate tubes not excluded by ExcludeShortTubes
            if( overrideFit ):
                actualTube = OverridePeaks
            elif( fitPar.isThreePointMethod() ):
                # Find the three peaks in the tube
                AP, BP, CP =  get3pointsFor3pointMethod( ws, wht, fitPar ) 
                #print i+1, AP, BP, CP
                actualTube = [AP, CP, BP] 
            else:
                ht, wd = fitPar.getHeightAndWidth()   
                actualTube = getPeaksForNSlitsMethod ( ws, eP, ht, wd, wht)
                print actualTube
                
            if( len(actualTube) == 0):
                print "getPeaksForNSlitMethod failed"
                return
                
            # Print peak positions fitted into PeaksFile, if it exists
            if( PeakFile != ""):
                print >> pFile, tubeSet.getTubeName(i), actualTube
                
            #detIDList, detPosList = get3pointsMethodResults( ws, AP, BP, CP, wht, 2900 ) 
            detIDList, detPosList = getCalibratedPixelPositions( ws, actualTube, idealTube, wht, PeakTestMode )
        
            print len(wht)
            if( len(detIDList) == len(wht)): # We have corrected positions
                for j in range(len(wht)):
	            nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
	            calibTable.addRow ( nextRow )
                        
    if(PeakFile != ""):
       pFile.close()

