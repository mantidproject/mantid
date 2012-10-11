
from tube_calib_fit_params import * # To handle fit parameters
from tube_calib import *  # For tube calibration functions
from mantid.simpleapi import *
from mantid.kernel import *
import math

# This class is the ideal tube, which specifies where the peaks formed by slits or edges should occur

class IdealTube:

   def __init__( self ):
        """
        Create empty instance
        """
        self.idealTubeArray = []
        
   def setArray ( self, array ):
       """     
       Construct and ideal tube directly from an array
                   
       @param array: Array of points where the peaks should be in Metres
  
       """
       self.idealTubeArray = array	

        

   def constructTubeFor3PointsMethod( self, idealAP, idealBP, idealCP, activeTubeLen ):
       """     
       Construct and ideal tube for Merlin 3-point calibration 
                   
       @param idealAP: Ideal left (AP) in pixels
       @param idealBP: ideal right (BP) in pixels
       @param idealCP: ideal centre (CP) in pixels
       @param activeTubeLen: Active tube length in metres
   
       """	
       #Contruct Ideal tube for 3 point calibration of MERLIN standard tube (code could be put into a function)
       pixelLen = activeTubeLen/1024  # Pixel length

       # we then convert idealAP, idealCP and idealBP to Y coordinates and put into ideal tube array
       self.idealTubeArray = [ idealAP*pixelLen - activeTubeLen/2,  idealCP*pixelLen - activeTubeLen/2, idealBP*pixelLen - activeTubeLen/2]
     
     
   def constructIdealTubeFromRealTube( self, ws, tube, fitPar ):
       """     
          Construct and ideal tube from an actual tube (assumed ideal) 
        
          @param ws: integrated workspace          
          @param tube: specification of one tube (if several tubes, only first tube is used)
          @param fitPar: initial fit parameters for peak of the tube
   
       """   
       # Get workspace indices
       nTubes = tube.getNumTubes()
       if(nTubes < 1):
           print "Invalid tube specification received by constructIdealTubeFromRealTube"
           return 
       elif(nTubes > 1):
          print "Specification has several tubes. The ideal tube will be based on the first tube",tube.getTubeName(0)
    
       wht = tube.getTube(0)
       print wht
    
       # Check tube
       if ( len(wht) < 1 ):
            print "Unable to get any workspace indices for this tube. Cannot use as ideal tube."
            return 
         
       # Get actual tube on which ideal tube is based           
       eP = fitPar.getPeaks()
       ht, wd = fitPar.getHeightAndWidth()   
       actualTube = getPeaksForNSlitsMethod ( ws, eP, ht, wd, wht)
       print "Actual tube that ideal tube is to be based upon",actualTube
   
       # Get ideal tube based on this actual tube
       try:
           self.idealTubeArray = getIdealTubeFromNSlits ( ws, actualTube )
       except:
           print "Attempted to create ideal tube based on actual tube",actualTube
           print "Unable to create ideal tube."
           print "Please choose another tube for constructIdealTubeFromRealTube()."
           
   def getArray( self ):
       """
       Reurn the array of of points where the peaks should be in Metres
       """
       return self.idealTubeArray
        


    
    
