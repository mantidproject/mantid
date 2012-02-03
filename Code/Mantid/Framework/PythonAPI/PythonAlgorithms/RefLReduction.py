from MantidFramework import *
from mantidsimple import *
from numpy import zeros, shape
import math

class RefLReduction(PythonAlgorithm):

    def category(self):
        return "Reflectometry"

    def name(self):
        return "RefLReduction"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareListProperty("RunNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("NormalizationRunNumber", 0, Description="")
        self.declareListProperty("SignalPeakPixelRange", [126, 134], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractSignalBackground", True)
        self.declareListProperty("SignalBackgroundPixelRange", [123, 137], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("NormPeakPixelRange", [127, 133], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", True)
        self.declareListProperty("NormBackgroundPixelRange", [123, 137], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("LowResAxisPixelRange", [115, 210], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("TOFRange", [9000., 23600.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("TOFBinning", [0.,200.,200000.],
                                 Description="Positive is linear bins, negative is logarithmic")
        self.declareProperty("QMin", 0.001, Description="Minimum Q-value")
        self.declareProperty("QStep", 0.001, Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("AngleOffset", "", Description="Angle offset (rad)")
        self.declareProperty("AngleOffsetError", "", Description="Angle offset error (rad)")
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        import os
        import numpy
        import math
        from reduction.instruments.reflectometer import wks_utility
        tof_binning = self.getProperty("TOFBinning")
        if len(tof_binning) != 1 and len(tof_binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(tof_binning))
        if len(tof_binning) == 3:
            if tof_binning[0] == 0. and tof_binning[1] == 0. and tof_binning[2] == 0.:
                raise RuntimeError("Failed to specify the TOF binning")
        
        run_numbers = self.getProperty("RunNumbers")
        allow_multiple = False
        if len(run_numbers)>1 and not allow_multiple:
            raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")
    
        normalization_run = self.getProperty("NormalizationRunNumber")
    
        data_peak = self.getProperty("SignalPeakPixelRange")
        data_back = self.getProperty("SignalBackgroundPixelRange")

        TOFrange = self.getProperty("TOFRange") #microS

        q_min = self.getProperty("QMin")
        q_step = self.getProperty("QStep")
                
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        Xrange = self.getProperty("LowResAxisPixelRange")
        
        h = 6.626e-34  #m^2 kg s^-1
        m = 1.675e-27     #kg
        
        #dimension of the detector (256 by 304 pixels)
        maxX = 304
        maxY = 256
        
        norm_back = self.getProperty("NormBackgroundPixelRange")
        BackfromYpixel = norm_back[0]
        BacktoYpixel = norm_back[1]

        norm_peak = self.getProperty("NormPeakPixelRange")
        from_peak = norm_peak[0]
        to_peak = norm_peak[1]
        
        subtract_data_bck = self.getProperty("SubtractSignalBackground")
        subtract_norm_bck = self.getProperty("SubtractNormBackground")
        
        ########################################################################
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_L%d" %run_numbers[0])
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefLReduction: could not find run %d\n" % run_numbers[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Pick a good workspace name
        ws_name = "refl%d" % run_numbers[0]
        ws_event_data = ws_name+"_evt"  
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_event_data):
            LoadEventNexus(Filename=data_file, OutputWorkspace=ws_event_data)
        
        # Get metadata
        mt_run = mtd[ws_event_data].getRun()
        ##get angles value
        ths_value = mt_run.getProperty('ths').value[0]
        ths_units = mt_run.getProperty('ths').units
        tthd_value = mt_run.getProperty('tthd').value[0]
        tthd_units = mt_run.getProperty('tthd').units
        ths_rad = wks_utility.angleUnitConversion(value=ths_value,
                                                  from_units=ths_units,
                                                  to_units='rad')
        tthd_rad = wks_utility.angleUnitConversion(value=tthd_value,
                                                   from_units=tthd_units,
                                                   to_units='rad')

        # Rebin data (x-axis is in TOF)
        ws_histo_data = ws_name+"_histo"
        Rebin(InputWorkspace=ws_event_data, OutputWorkspace=ws_histo_data, Params=tof_binning)
        
        # Keep only range of TOF of interest
        CropWorkspace(ws_histo_data,ws_histo_data,XMin=TOFrange[0], XMax=TOFrange[1])

        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_histo_data, OutputWorkspace=ws_histo_data)
        
        # Calculation of the central pixel (using weighted average)
        pixelXtof_data = wks_utility.getPixelXTOF(mtd[ws_histo_data], maxX=maxX, maxY=maxY)
        pixelXtof_1d = pixelXtof_data.sum(axis=1)
        
        # Keep only range of pixels
        pixelXtof_roi = pixelXtof_1d[data_peak[0]:data_peak[1]]
        
        sz = pixelXtof_roi.size
        _num = 0
        _den = 0
        start_pixel = data_peak[0]
        for i in range(sz):
            _num += (start_pixel * pixelXtof_roi[i])
            start_pixel = start_pixel + 1
            _den += pixelXtof_roi[i]
        data_cpix = _num / _den    
        
        # Retrieve geometry of instrument
        # Sample-to-detector distance
        sample = mtd[ws_event_data].getInstrument().getSample()
        source = mtd[ws_event_data].getInstrument().getSource()
        dSM = sample.getDistance(source)
        # Create array of distances pixel->sample
        dPS_array = numpy.zeros((maxY, maxX))
        for x in range(maxX):
            for y in range(maxY):
                _index = maxY * x + y
                detector = mtd[ws_event_data].getDetector(_index)
                dPS_array[y, x] = sample.getDistance(detector)
        # Array of distances pixel->source
        dMP_array = dPS_array + dSM
        # Distance sample->center of detector
        dSD = dPS_array[maxY / 2, maxX / 2]
        # Distance source->center of detector        
        dMD = dSD + dSM

        # Background subtraction
        BackfromYpixel = data_back[0]
        BacktoYpixel = data_back[1]
        
        # Create a new event workspace of only the range of pixel of interest 
        # background range (along the y-axis) and of only the pixel
        # of interest along the x-axis (to avoid the frame effect)
        theta = tthd_rad - ths_rad
        AngleOffset = self.getProperty("AngleOffset")
        if (AngleOffset != ""):
            theta += float(AngleOffset)
        
        if dMD is not None and theta is not None:
#            _tof_axis = mtd[ws_histo_data].readX(0)
#            print _tof_axis
#            _const = float(4) * math.pi * m * dMD / h
#            _q_axis = 1e-10 * _const * math.sin(theta) / (_tof_axis*1e-6)
#            q_max = max(_q_axis)
                    
            _tof_axis = mtd[ws_histo_data].readX(0)
            _const = float(4) * math.pi * m * dMD / h
            sz_tof = numpy.shape(_tof_axis)[0]
            _q_axis = zeros(sz_tof-1)
            for t in range(sz_tof-1):
                tof1 = _tof_axis[t]
                tof2 = _tof_axis[t+1]
                tofm = (tof1+tof2)/2.
                _Q = _const * math.sin(theta) / (tofm*1e-6)
                _q_axis[t] = _Q*1e-10
            q_max = max(_q_axis)

        wks_utility.createIntegratedWorkspace(mtd[ws_histo_data], 
                                              "IntegratedDataWks1",
                                              fromXpixel=Xrange[0],
                                              toXpixel=Xrange[1],
                                              fromYpixel=BackfromYpixel,
                                              toYpixel=BacktoYpixel,
                                              maxX=maxX,
                                              maxY=maxY,
                                              cpix=data_cpix,
                                              source_to_detector=dMD,
                                              sample_to_detector=dSD,
                                              theta=theta,
                                              geo_correction=True,
                                              q_binning=[q_min,q_step,q_max])

        ConvertToHistogram(InputWorkspace='IntegratedDataWks1',
                           OutputWorkspace='IntegratedDataWks')

        Transpose(InputWorkspace='IntegratedDataWks',
                  OutputWorkspace='TransposedID')
        
        ConvertToHistogram(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID')
         
        if subtract_data_bck:
            FlatBackground(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID',
                           StartX=BackfromYpixel,
                           Mode='Mean',
                           EndX=data_peak[0],
                           OutputMode="Return Background")

        Transpose(InputWorkspace='TransposedID',
                  OutputWorkspace='DataBckWks')

        ConvertToHistogram("DataBckWks", OutputWorkspace="DataBckWks")
        RebinToWorkspace(WorkspaceToRebin="DataBckWks", WorkspaceToMatch="IntegratedDataWks", OutputWorkspace="DataBckWks")
                
        Minus("IntegratedDataWks", "DataBckWks", OutputWorkspace="DataWks")
             
        # Work on Normalization file #########################################
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_L%d" %normalization_run)
        if len(f)>0 and os.path.isfile(f[0]): 
            norm_file = f[0]
        else:
            msg = "RefLReduction: could not find run %d\n" % run_numbers[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
            
        #load normalization file
        ws_norm = "__normalization_refl%d" % run_numbers[0]
        ws_norm_event_data = ws_norm+"_evt"  
        ws_norm_histo_data = ws_norm+"_histo"  

        if not mtd.workspaceExists(ws_norm_event_data):
            LoadEventNexus(Filename=norm_file, OutputWorkspace=ws_norm_event_data)
    
        # Rebin data
        Rebin(InputWorkspace=ws_norm_event_data, OutputWorkspace=ws_norm_histo_data, Params=tof_binning)
    
        # Keep only range of TOF of interest
        CropWorkspace(ws_norm_histo_data, ws_norm_histo_data, XMin=TOFrange[0], XMax=TOFrange[1])
    
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_norm_histo_data, OutputWorkspace=ws_norm_histo_data)
    
        ##Background subtraction

        #Create a new event workspace of only the range of pixel of interest 
        #background range (along the y-axis) and of only the pixel
        #of interest along the x-axis (to avoid the frame effect)
        wks_utility.createIntegratedWorkspace(mtd[ws_norm_histo_data], 
                                              "IntegratedNormWks",
                                              fromXpixel=Xrange[0],
                                              toXpixel=Xrange[1],
                                              fromYpixel=BackfromYpixel,
                                              toYpixel=BacktoYpixel,
                                              maxX=maxX,
                                              maxY=maxY,
                                              cpix=data_cpix,
                                              source_to_detector=dMD,
                                              sample_to_detector=dSD,
                                              theta=theta,
                                              geo_correction=False)

        Transpose(InputWorkspace='IntegratedNormWks',
                  OutputWorkspace='TransposedID')
        
        ConvertToHistogram(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID')
        
        if subtract_norm_bck:
            FlatBackground(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID',
                           StartX=BackfromYpixel,
                           Mode='Mean',
                           EndX=norm_peak[0],
                           OutputMode="Return Background")
    
        Transpose(InputWorkspace='TransposedID',
                  OutputWorkspace='NormBckWks')
        
        ConvertToHistogram("NormBckWks", OutputWorkspace="NormBckWks")
        RebinToWorkspace(WorkspaceToRebin="NormBckWks", WorkspaceToMatch="IntegratedNormWks", OutputWorkspace="NormBckWks")
       
        Minus("IntegratedNormWks", "NormBckWks", OutputWorkspace="NormWks")

        RebinToWorkspace(WorkspaceToRebin="NormWks", 
                         WorkspaceToMatch='DataWks',
                         OutputWorkspace='NormWks')
   
        #perform the integration myself
        mt_temp = mtd['NormWks']
        x_axis = mt_temp.readX(0)[:]   #[9100,9300,.... 23500] (73,1)
        NormPeakRange = numpy.arange(to_peak-from_peak+1) + from_peak
        counts_vs_tof = numpy.zeros(len(x_axis))

        # Normalization           
        SumSpectra(InputWorkspace="NormWks", OutputWorkspace="NormWks")
        
        #### divide data by normalize histo workspace
        Divide(LHSWorkspace='DataWks',
               RHSWorkspace='NormWks',
               OutputWorkspace='NormalizedWks')
        ReplaceSpecialValues(InputWorkspace="NormalizedWks", NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0, OutputWorkspace="NormalizedWks")
        
        output_ws = self.getPropertyValue("OutputWorkspace")        
        
        SumSpectra(InputWorkspace="NormalizedWks", OutputWorkspace=output_ws)
        
        self.setProperty("OutputWorkspace", mtd[output_ws])
            
mtd.registerPyAlgorithm(RefLReduction())
