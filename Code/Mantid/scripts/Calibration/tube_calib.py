#
#  This file is concerned with calibrating a specified set of tubes
#
#  The main function is getCaibration(....) which is at the end of this file. 
#  It populates an empty Calibration Table Workspace with the new positions of the pixel detectors after calibration.
#  This Calibration Table Workspace can be used later to move the pixel detectors to the calibrated positions.
#
#  Users should not need to directly call any other function other than getCaibration(....) from this file.
#
#  Author: Karl palmen ISIS and for readPeakFile Gesner Passos ISIS
#
from mantid.simpleapi import *
from mantid.kernel import *
from tube_spec import * # For tube specification class
import math
import re
import os


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
    
def getPoints ( IntegratedWorkspace, funcForms, fitParams, whichTube, showPlot=False ):
    """     
       Get the centres of N slits or edges for calibration 
       This N slit method is suited for WISH or the five sharp peaks of MERLIN .
       
       @param IntegratedWorkspace: Workspace of integrated data
       @param funcForms: array of function form 1=slit/bar, 2=edge
       @param fitParams: a TubeCalibFitParams object contain the fit parameters
       @param whichTube:  a list of workspace indices for one tube
       @param showPlot: show plot for this tube
   
       Return Value: array of the slit/edge positions (-1.0 indicates failed to find position)
       
    """	
    nDets = len(whichTube)
    eP = fitParams.getPeaks()
    nPts = len(eP)
    nFf = len(funcForms)
    results = []
    
    eHeight, eWidth = fitParams.getHeightAndWidth()
    outedge, inedge, endGrad = fitParams.getEdgeParameters()
    
    margin = fitParams.getMargin() 
    
    # Set workspace names, a different workspace if plotting, so plot survives.
    calibPointWs = "CalibPoint"
    getPointsWs = "getPoints"
    if(showPlot):
       getPointsWs = "TubePlot"
    
    # Check functional form
    if ( nFf != 0 and nFf < nPts):
       print "tube_calib.getPoints Error: Number of functional forms",nFf,"not compatible with number of points",nPts,"."
       return []
     
    # Create input workspace for fitting  
    createTubeCalibtationWorkspaceByWorkspaceIndexList( IntegratedWorkspace, getPointsWs, whichTube, showPlot=showPlot )
    
    # Prepare to loop over points
    tallPeak = False
    edgeMode = 1  # We assume first edge is approached from outside
    # Loop over the points
    for i in range(nPts):
        # Use different workpace if plotting fit, so plot survives
        if(showPlot):
           calibPointWs = "TubeCalibPlot"+str(i)
           
        if( nFf != 0 and funcForms[i] == 2 ):
           # We have an edge
           centre = eP[i]
           if( edgeMode > 0):
              start = eP[i] - outedge
              end = eP[i] + inedge
           else:
              start = eP[i] - inedge
              end = eP[i] + outedge
           Fit(InputWorkspace=getPointsWs,Function=fitEndErfcParams(centre,endGrad*edgeMode),StartX=str(start),EndX=str(end),Output=calibPointWs) 
           edgeMode = -edgeMode # Next edge would be reverse of this edge
           tallPeak = True
        else:
           # We have a slit or bar
           centre = eP[i]
           if( i == 0):
              start = (1-margin)*eP[i]
           else:
              start = margin*eP[i-1] + (1-margin)*eP[i]
           if( i == nPts-1):
              end = (1-margin)*eP[i] + (margin)*nDets
           else:
              end = (1-margin)*eP[i] + (margin)*eP[i+1]   
           Fit(InputWorkspace=getPointsWs,Function='name=LinearBackground, A0=1000',StartX=str(start),EndX=str(end),Output="Z1")
           Fit(InputWorkspace="Z1_Workspace",Function=fitGaussianParams(eHeight,centre,eWidth), WorkspaceIndex='2',StartX=str(start),EndX=str(end),Output=calibPointWs)
           tallPeak = False # Force peak height to be checked
           
        paramCalibPeak = mtd[calibPointWs+'_Parameters']
        if( not tallPeak ):
            pkHeightRow = paramCalibPeak.row(0).items()
            thisHeight = pkHeightRow[1][1]
            tallPeak = ( thisHeight/eHeight > 0.2)
        pkRow = paramCalibPeak.row(1).items()
        thisResult = pkRow[1][1]
        if( tallPeak and start < thisResult and thisResult < end ):
            results.append( thisResult )
        else:
            results.append(-1.0) #ensure result is ignored
    
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
           #print "used point",i,"shoving pixel",int(usedTubePoints[i])
           xResult[ int(usedTubePoints[i]) ] = xResult[0]
         
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
    pixels = correctTubeToIdealTube ( tubePts, idealTubePts, nDets, TestMode=peakTestMode )
    #print pixels
    if( len(pixels) != nDets):
       print "Tube correction failed."
       return detIDs, detPositions 
    
    # Get tube unit vector 
    det0 = ws.getDetector( whichTube[0])
    detN = ws.getDetector (whichTube[nDets-1])
    d0x = det0.getPos().X()
    d0y = det0.getPos().Y()
    d0z = det0.getPos().Z()
    dNx = detN.getPos().X()
    dNy = detN.getPos().Y()
    dNz = detN.getPos().Z()
    tubeLength = math.sqrt( (dNx - d0x)*(dNx - d0x) + (dNy - d0y)*(dNy - d0y) + (dNz - d0z)*(dNz - d0z) )
    if( tubeLength <= 0.0):
        print "Zero length tube cannot be calibrated, calibration failed."
        return detIDs, detPositions
    uX = (dNx - d0x)/tubeLength
    uY = (dNy - d0y)/tubeLength
    uZ = (dNz - d0z)/tubeLength
    
    # Get Centre (really want to get if from IDF to allow calibration a multiple number of times)
    cX = (d0x + dNx)/2
    cY = (d0y + dNy)/2
    cZ = (d0z + dNz)/2
    
    # Move the pixel detectors (might not work for sloping tubes)
    for i in range(nDets):
        deti = ws.getDetector( whichTube[i])
	detiPositionX = deti.getPos().X()
	detiPositionY = deti.getPos().Y()
	detiPositionZ = deti.getPos().Z()
	pNew = pixels[i]
	detIDs.append( deti.getID() )
	xNew = (1.0 - uX*uX)*detiPositionX + uX*uX*(pNew + cX)
	yNew = (1.0 - uY*uY)*detiPositionY + uY*uY*(pNew + cY)
	zNew = (1.0 - uZ*uZ)*detiPositionZ + uZ*uZ*(pNew + cZ)

	detPositions.append( V3D( xNew, yNew, zNew ) )
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
    
    
    
""" THESE FUNCTIONS NEXT SHOULD BE THE ONLY FUNCTIONS THE USER CALLS FROM THIS FILE
"""
    
    
def getCalibration ( ws, tubeSet, calibTable, fitPar, iTube, PeakTestMode=False, OverridePeaks=[], PeakFile="", ExcludeShortTubes=0.0, PlotTube=-1):
    """     
       Get the results the calibration and put them in the calibration table provided.
             
       @param ws: Integrated Workspace with tubes to be calibrated 
       @param tubeSet: Specification of Set of tubes to be calibrated
       @param calibTable: Empty calibration table into which the calibration results are placed
       @param fitPar: A TubeCalibFitParam object for fitting the peaks
       @param  iTube: The ideal tube
       @param PeakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
       @param OverridePeaks: if non-zero length an array of peaks in pixels to override those that would be fitted for one tube
       @param PeakFile: Optional output file for peaks 
       @param ExludeShortTubes: Exlude tubes shorter than specified length from calibration
       @param PlotTube: If in range plot graph of tube with this index 
       
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
       saveDirectory = config['defaultsave.directory']
       pFile = open(os.path.join(saveDirectory, PeakFile),'w')

    for i in range(nTubes):

        # Deal with (i+1)st tube specified
        wht = tubeSet.getTube(i)
        print "Calibrating tube", i+1,"of",nTubes, tubeSet.getTubeName(i)  #, " length", tubeSet.getTubeLength(i) 
        if ( len(wht) < 1 ):
           print "Unable to get any workspace indices (spectra) for this tube. Tube",tubeSet.getTubeName(i),"not calibrated."
        else:
           # Calibribate the tube, if possible
           if( tubeSet.getTubeLength(i) >= ExcludeShortTubes ):  # Only calibrate tubes not excluded by ExcludeShortTubes
               if( overrideFit ):
                  actualTube = OverridePeaks
               else:
                  ff = iTube.getFunctionalForms()
                  actualTube = getPoints ( ws, ff, fitPar, wht, showPlot=(i==PlotTube) )
               
               # print actualTube
               if( len(actualTube) == 0):
                   print "getPoints failed"
                   return
                
               # Print peak positions fitted into PeaksFile, if it exists
               if( PeakFile != ""):
                   print >> pFile, tubeSet.getTubeName(i), actualTube
                
               detIDList, detPosList = getCalibratedPixelPositions( ws, actualTube, idealTube, wht, PeakTestMode )
        
               #print len(wht)
               if( len(detIDList) == len(wht)): # We have corrected positions
                   for j in range(len(wht)):
	               nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
	               calibTable.addRow ( nextRow )

    if(PeakFile != ""):
       pFile.close()
       
    if(nTubes == 0):
       return

    # Delete temporary workspaces for obtaining points
    if( OverridePeaks == []):
       DeleteWorkspace('getPoints')
       DeleteWorkspace('CalibPoint_NormalisedCovarianceMatrix')
       DeleteWorkspace('CalibPoint_Parameters')
       DeleteWorkspace('CalibPoint_Workspace')
       # Assume at least one slit or peak (else we'd need to check here)
       DeleteWorkspace('Z1_NormalisedCovarianceMatrix')
       DeleteWorkspace('Z1_Parameters')
       DeleteWorkspace('Z1_Workspace')
       
    
    # Delete temporary workspaces for getting new detector positions
    DeleteWorkspace('QuadraticFittingWorkspace')
    DeleteWorkspace('QF_NormalisedCovarianceMatrix')
    DeleteWorkspace('QF_Parameters')
    DeleteWorkspace('QF_Workspace')
    
    
    
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

        wht = tube.getTube(0)
        print "Calibrating tube", i+1 ,"of", nTubes, TubeName #, " length", tubeSet.getTubeLength(i) 
        if ( len(wht) < 1 ):
            print "Unable to get any workspace indices for this tube. Calibration abandoned."
            return                         
                
        detIDList, detPosList = getCalibratedPixelPositions( ws, actualTube, idealTube, wht, False )
        
        #print len(wht)
        if( len(detIDList) == len(wht)): # We have corrected positions
            for j in range(len(wht)):
	        nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
	        calibTable.addRow ( nextRow )
     
    if(nTubes == 0):
       return                
    
    # Delete temporary workspaces for getting new detector positions
    DeleteWorkspace('QuadraticFittingWorkspace')
    DeleteWorkspace('QF_NormalisedCovarianceMatrix')
    DeleteWorkspace('QF_Parameters')
    DeleteWorkspace('QF_Workspace')
