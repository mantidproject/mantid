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
        self.declareProperty("NormFlag", True)
        self.declareListProperty("NormPeakPixelRange", [127, 133], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", True)
        self.declareListProperty("NormBackgroundPixelRange", [123, 137], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("LowResDataAxisPixelRangeFlag", True)
        self.declareListProperty("LowResDataAxisPixelRange", [115, 210], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("LowResNormAxisPixelRangeFlag", True)
        self.declareListProperty("LowResNormAxisPixelRange", [115, 210], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("TOFRange", [9000., 23600.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("QMin", 0.001, Description="Minimum Q-value")
        self.declareProperty("QStep", 0.001, Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("AngleOffset", 0.0, Description="Angle offset (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, Description="Angle offset error (degrees)")
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        import os
        import numpy
        import math
        from reduction.instruments.reflectometer import wks_utility
        
        run_numbers = self.getProperty("RunNumbers")

        mtd.sendLogMessage("RefLReduction: processing %s" % run_numbers)
        allow_multiple = False
        if len(run_numbers)>1 and not allow_multiple:
            raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")

        #run with normalization or not    
        NormFlag = self.getProperty("NormFlag")
        
        normalization_run = self.getProperty("NormalizationRunNumber")
    
        data_peak = self.getProperty("SignalPeakPixelRange")
        data_back = self.getProperty("SignalBackgroundPixelRange")

        # TOF range to consider
        TOFrange = self.getProperty("TOFRange") #microS
        # Steps for TOF rebin
        TOFsteps = 50.0

        # Q binning for output distribution
        q_min = self.getProperty("QMin")
        q_step = self.getProperty("QStep")
                
        #dimension of the detector (256 by 304 pixels)
        maxX = 304
        maxY = 256
                
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        DataXrangeFlag = self.getProperty("LowResDataAxisPixelRangeFlag")
        if DataXrangeFlag:
            Xrange = self.getProperty("LowResDataAxisPixelRange")
        else:
            Xrange = [0,maxX-1]

        NormXrangeFlag = self.getProperty("LowResNormAxisPixelRangeFlag")
        if NormXrangeFlag:
            normXrange = self.getProperty("LowResNormAxisPixelRange")
        else:
            normXrange = [0,maxX-1]
                
        h = 6.626e-34  #m^2 kg s^-1
        m = 1.675e-27     #kg
                
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
        ws_histo_data = "__"+ws_name+"_histo"
        Rebin(InputWorkspace=ws_event_data, OutputWorkspace=ws_histo_data, Params=[TOFrange[0], TOFsteps, TOFrange[1]])
        
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
        AngleOffset_deg = float(self.getProperty("AngleOffset"))
        AngleOffset_rad = (AngleOffset_deg * math.pi) / 180.
        theta += AngleOffset_rad
        
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

        ws_integrated_data = "__IntegratedDataWks"
        wks_utility.createIntegratedWorkspace(mtd[ws_histo_data], 
                                              ws_integrated_data,
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

        ws_data = "__DataWks"
        ws_transposed = '__TransposedID'
        if subtract_data_bck:

            print "with data background"
            ConvertToHistogram(InputWorkspace=ws_integrated_data,
                               OutputWorkspace=ws_integrated_data)

            Transpose(InputWorkspace=ws_integrated_data,
                      OutputWorkspace=ws_transposed)
        
            ConvertToHistogram(InputWorkspace=ws_transposed,
                               OutputWorkspace=ws_transposed)

            ws_transposed_1 = ws_transposed + '_1'
            ws_transposed_2 = ws_transposed + '_2'
            FlatBackground(InputWorkspace=ws_transposed,
                           OutputWorkspace=ws_transposed_1,
                           StartX=BackfromYpixel,
                           Mode='Mean',
                           EndX=data_peak[0],
                           OutputMode="Return Background")

            FlatBackground(InputWorkspace=ws_transposed,
                           OutputWorkspace=ws_transposed_2,
                           StartX=data_peak[1],
                           Mode='Mean',
                           EndX=BacktoYpixel,
                           OutputMode="Return Background")

            ws_data_bck = "__DataBckWks"
            ws_data_bck_1 = ws_data_bck + "_1"
            Transpose(InputWorkspace=ws_transposed,
                      OutputWorkspace=ws_data_bck_1)
            ws_data_bck_2 = ws_data_bck + "_2"
            Transpose(InputWorkspace=ws_transposed,
                      OutputWorkspace=ws_data_bck_2)

            ConvertToHistogram(ws_data_bck_1, OutputWorkspace=ws_data_bck_1)
            ConvertToHistogram(ws_data_bck_2, OutputWorkspace=ws_data_bck_2)

            RebinToWorkspace(WorkspaceToRebin=ws_data_bck_1, 
                             WorkspaceToMatch=ws_integrated_data, 
                             OutputWorkspace=ws_data_bck_1)
                
            RebinToWorkspace(WorkspaceToRebin=ws_data_bck_2, 
                             WorkspaceToMatch=ws_integrated_data, 
                             OutputWorkspace=ws_data_bck_2)

            WeightedMean(ws_data_bck_1, ws_data_bck_2, ws_data_bck)

            Minus(ws_integrated_data, ws_data_bck, OutputWorkspace=ws_data)
            
            # Clean up intermediary workspaces
            mtd.deleteWorkspace(ws_integrated_data)
            mtd.deleteWorkspace(ws_histo_data)
            mtd.deleteWorkspace(ws_data_bck)
            mtd.deleteWorkspace(ws_transposed)

        else:

            print "without data background"
            ConvertToHistogram(InputWorkspace=ws_integrated_data,
                               OutputWorkspace=ws_data)
        
        # Work on Normalization file #########################################
        if (NormFlag):
        
        
            print "with normalization"
            # Find full path to event NeXus data file
            f = FileFinder.findRuns("REF_L%d" %normalization_run)
            if len(f)>0 and os.path.isfile(f[0]): 
                norm_file = f[0]
            else:
                msg = "RefLReduction: could not find run %d\n" % run_numbers[0]
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)
            
            #load normalization file
            ws_name = "__normalization_refl%d" % run_numbers[0]
            ws_norm_event_data = ws_name+"_evt"  
            ws_norm_histo_data = ws_name+"_histo"  

            if not mtd.workspaceExists(ws_norm_event_data):
                LoadEventNexus(Filename=norm_file, OutputWorkspace=ws_norm_event_data)
    
            # Rebin data
            Rebin(InputWorkspace=ws_norm_event_data, OutputWorkspace=ws_norm_histo_data, Params=[TOFrange[0], TOFsteps, TOFrange[1]])
    
            # Keep only range of TOF of interest
            CropWorkspace(ws_norm_histo_data, ws_norm_histo_data, XMin=TOFrange[0], XMax=TOFrange[1])
    
            # Normalized by Current (proton charge)
            NormaliseByCurrent(InputWorkspace=ws_norm_histo_data, OutputWorkspace=ws_norm_histo_data)

            #Create a new event workspace of only the range of pixel of interest 
            #background range (along the y-axis) and of only the pixel
            #of interest along the x-axis (to avoid the frame effect)
            ws_integrated_data = "__IntegratedNormWks"
            wks_utility.createIntegratedWorkspace(mtd[ws_norm_histo_data], 
                                                  ws_integrated_data,
                                                  fromXpixel=normXrange[0],
                                                  toXpixel=normXrange[1],
                                                  fromYpixel=BackfromYpixel,
                                                  toYpixel=BacktoYpixel,
                                                  maxX=maxX,
                                                  maxY=maxY,
                                                  cpix=data_cpix,
                                                  source_to_detector=dMD,
                                                  sample_to_detector=dSD,
                                                  theta=theta,
                                                  geo_correction=False)

            ws_data_bck = "__NormBckWks"
            if subtract_norm_bck:
                Transpose(InputWorkspace=ws_integrated_data,
                          OutputWorkspace=ws_transposed)
        
                ConvertToHistogram(InputWorkspace=ws_transposed,
                                   OutputWorkspace=ws_transposed)
    
                BackfromYpixel = norm_back[0]
                BacktoYpixel = norm_back[1]

                FlatBackground(InputWorkspace=ws_transposed,
                               OutputWorkspace=ws_transposed_1,
                               StartX=BackfromYpixel,
                               Mode='Mean',
                               EndX=norm_peak[0],
                               OutputMode="Return Background")
    
                Transpose(InputWorkspace=ws_transposed,
                          OutputWorkspace=ws_data_bck_1)

                FlatBackground(InputWorkspace=ws_transposed,
                               OutputWorkspace=ws_transposed_2,
                               StartX=norm_peak[1],
                               Mode='Mean',
                               EndX=BacktoYpixel,
                               OutputMode="Return Background")

                Transpose(InputWorkspace=ws_transposed,
                          OutputWorkspace=ws_data_bck_2)
        
                ConvertToHistogram(ws_data_bck_1, OutputWorkspace=ws_data_bck_1)
                ConvertToHistogram(ws_data_bck_2, OutputWorkspace=ws_data_bck_2)

                RebinToWorkspace(WorkspaceToRebin=ws_data_bck_1, 
                                 WorkspaceToMatch=ws_integrated_data, 
                                 OutputWorkspace=ws_data_bck_1)
       
                RebinToWorkspace(WorkspaceToRebin=ws_data_bck_2, 
                                 WorkspaceToMatch=ws_integrated_data, 
                                 OutputWorkspace=ws_data_bck_2)

                WeightedMean(ws_data_bck_1, ws_data_bck_2, ws_data_bck)

                ws_norm = "__NormWks"
                Minus(ws_integrated_data, ws_data_bck, OutputWorkspace=ws_norm)

                # Clean up intermediary workspaces
                mtd.deleteWorkspace(ws_data_bck)
                mtd.deleteWorkspace(ws_integrated_data)
                mtd.deleteWorkspace(ws_transposed)

                ws_norm_rebinned = "__NormRebinnedWks"
                RebinToWorkspace(WorkspaceToRebin=ws_norm, 
                                 WorkspaceToMatch=ws_data,
                                 OutputWorkspace=ws_norm_rebinned)

#            else:
#            
#                ws_norm_rebinned = "__NormRebinnedWks"
#                RebinToWorkspace(WorkspaceToRebin=ws_norm, 
#                                 WorkspaceToMatch=ws_data,
#                                 OutputWorkspace=ws_norm_rebinned)
            
            #perform the integration myself
            mt_temp = mtd[ws_norm_rebinned]
            x_axis = mt_temp.readX(0)[:]   #[9100,9300,.... 23500] (73,1)
            NormPeakRange = numpy.arange(to_peak-from_peak+1) + from_peak
            counts_vs_tof = numpy.zeros(len(x_axis))

            #Normalization           
            SumSpectra(InputWorkspace=ws_norm_rebinned, OutputWorkspace=ws_norm_rebinned)
        
            #### divide data by normalize histo workspace
            Divide(LHSWorkspace=ws_data,
                   RHSWorkspace=ws_norm_rebinned,
                   OutputWorkspace=ws_data)

        mt =mtd[ws_data]
        
        ReplaceSpecialValues(InputWorkspace=ws_data, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0, OutputWorkspace=ws_data)

        output_ws = self.getPropertyValue("OutputWorkspace")        
        
        if mtd.workspaceExists(output_ws):
            mtd.deleteWorkspace(output_ws)
            
        SumSpectra(InputWorkspace=ws_data, OutputWorkspace=output_ws)
        
        self.setProperty("OutputWorkspace", mtd[output_ws])
        
        # Clean up intermediary workspaces
        mtd.deleteWorkspace(ws_data)

        if (NormFlag):
            mtd.deleteWorkspace(ws_norm)
            mtd.deleteWorkspace(ws_norm_rebinned)
            mtd.deleteWorkspace(ws_norm_histo_data)
        
            
mtd.registerPyAlgorithm(RefLReduction())
