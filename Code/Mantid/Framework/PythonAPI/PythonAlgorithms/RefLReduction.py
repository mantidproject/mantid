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

        #run with normalization or not    
        NormFlag = self.getProperty("NormFlag")
        
        normalization_run = self.getProperty("NormalizationRunNumber")

        print '** Working with data runs: ' + str(run_numbers)
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
        from_norm_peak = norm_peak[0]
        to_norm_peak = norm_peak[1]
        
        subtract_data_bck = self.getProperty("SubtractSignalBackground")
        subtract_norm_bck = self.getProperty("SubtractNormBackground")

        #name of the sfCalculator txt file
        slitsValuePrecision = 0.1       #precision of slits = 10% 

        # Pick a good workspace n    ame
        ws_name = "refl%d" % run_numbers[0]
        ws_event_data = ws_name+"_evt"  
        
        # Load the data into its workspace
        allow_multiple = True        
        if len(run_numbers)>1 and allow_multiple:
            
            for _run in run_numbers:

                ########################################################################
                # Find full path to event NeXus data file
                _File = FileFinder.findRuns("REF_L%d" %_run)
                if len(_File)>0 and os.path.isfile(_File[0]): 
                    data_file = _File[0]
                else:
                    msg = "RefLReduction: could not find run %d\n" % _run
                    msg += "Add your data folder to your User Data Directories in the File menu"
                    raise RuntimeError(msg)
                
                if not mtd.workspaceExists(ws_event_data):
                    LoadEventNexus(Filename=data_file, OutputWorkspace=ws_event_data)
                else:
                    LoadEventNexus(Filename=data_file, OutputWorkspace='tmp')
                    mt1 = mtd[ws_event_data]
                    mt2 = mtd['tmp']
                    Plus(LHSWorkspace=ws_event_data,
                         RHSWorkspace='tmp',
                         OutputWorkspace=ws_event_data)
        else:
            
            _File = FileFinder.findRuns("REF_L%d" %run_numbers[0])
            if len(_File)>0 and os.path.isfile(_File[0]): 
                data_file = _File[0]    
            else:
                msg = "RefLReduction: could not find run %d\n" % _run
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)

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
        print '-> Rebin'
        ws_histo_data = "_"+ws_name+"_histo"
        Rebin(InputWorkspace=ws_event_data, 
              OutputWorkspace=ws_histo_data, 
              Params=[TOFrange[0], 
                      TOFsteps, 
                      TOFrange[1]],
              PreserveEvents=True)
        
#        # Keep only range of TOF of interest
#        print '-> Crop TOF range'
#        CropWorkspace(ws_histo_data,ws_histo_data,XMin=TOFrange[0], XMax=TOFrange[1])

        # Normalized by Current (proton charge)
        print '-> Normalize by proton charge'
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
        print '-> Central pixel is {0:.1f}'.format(data_cpix)
        
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
        
        ws_data = '_' + ws_name + '_DataWks'
        if subtract_data_bck:

            print '-> substract background'
            ConvertToMatrixWorkspace(InputWorkspace=ws_histo_data,
                                     OutputWorkspace=ws_histo_data)
            
            ws_data_bck = '_' + ws_name + '_DataBckWks'
            
            bBackLeft = False
            if (BackfromYpixel < (data_peak[0]-1)):

                bBackLeft = True
                ws_data_bck_1 = ws_data_bck + "_1"
                RefRoi(InputWorkspace=ws_histo_data,
                       OutputWorkspace=ws_data_bck_1,
                       NXPixel=maxX,
                       NYPixel=maxY,
                       ConvertToQ=False,
                       IntegrateY=False,
                       SumPixels=True,
                       XPixelMin=Xrange[0],
                       XPixelMax=Xrange[1],
                       YPixelMin=BackfromYpixel,
                       YPixelMax=data_peak[0]-1,
                       NormalizeSum=True)
                           
                ws_data_bck_1_rebin = ws_data_bck_1 + '_rebin'
                RebinToWorkspace(WorkspaceToRebin=ws_data_bck_1, 
                                 WorkspaceToMatch=ws_histo_data, 
                                 OutputWorkspace=ws_data_bck_1_rebin)

            bBackRight = False
            if ((data_peak[1]+1) < BacktoYpixel):

                bBackRight = True
                ws_data_bck_2 = ws_data_bck + "_2"
                RefRoi(InputWorkspace=ws_histo_data,
                       OutputWorkspace=ws_data_bck_2,
                       NXPixel=maxX,
                       NYPixel=maxY,
                       ConvertToQ=False,
                       IntegrateY=False,
                       SumPixels=True,
                       XPixelMin=Xrange[0],
                       XPixelMax=Xrange[1],
                       YPixelMin=data_peak[1]+1,
                       YPixelMax=BacktoYpixel,
                       NormalizeSum=True)
            
                ws_data_bck_2_rebin = ws_data_bck_2 + '_rebin'
                RebinToWorkspace(WorkspaceToRebin=ws_data_bck_2, 
                                 WorkspaceToMatch=ws_histo_data, 
                                 OutputWorkspace=ws_data_bck_2_rebin)

            if (bBackLeft and bBackRight):
            
                Plus(RHSWorkspace=ws_data_bck_1_rebin,
                     LHSWorkspace=ws_data_bck_2_rebin,
                     OutputWorkspace=ws_data_bck)
                Scale(InputWorkspace=ws_data_bck,
                      OutputWorkspace=ws_data_bck+'_scale',
                      Factor=0.5,
                      Operation="Multiply")
                
                Minus(LHSWorkspace=ws_histo_data, 
                      RHSWorkspace=ws_data_bck+'_scale', 
                      OutputWorkspace=ws_data)

                mtd.deleteWorkspace(ws_data_bck+'_scale')
                mtd.deleteWorkspace(ws_data_bck)
                mtd.deleteWorkspace(ws_data_bck_1_rebin)
                mtd.deleteWorkspace(ws_data_bck_2_rebin)
                mtd.deleteWorkspace(ws_data_bck_1)
                mtd.deleteWorkspace(ws_data_bck_2)
                mtd.deleteWorkspace(ws_histo_data)

            elif (bBackLeft):
                
                Minus(LHSWorkspace=ws_histo_data,
                      RHSWorkspace=ws_data_bck_1_rebin,
                      OutputWorkspace=ws_data)
                mtd.deleteWorkspace(ws_data_bck_1_rebin)
                mtd.deleteWorkspace(ws_data_bck_1)
                
            elif (bBackRight):
                
                Minus(LHSWorkspace=ws_histo_data,
                      RHSWorkspace=ws_data_bck_2_rebin,
                      OutputWorkspace=ws_data)
                mtd.deleteWorkspace(ws_data_bck_2_rebin)
                mtd.deleteWorkspace(ws_data_bck_2)

            mtd.deleteWorkspace(ws_histo_data)

        else:        
        
            ConvertToMatrixWorkspace(InputWorkspace=ws_histo_data,
                                     OutputWorkspace=ws_data)

            mtd.deleteWorkspace(ws_histo_data)

                
        if (NormFlag):

            print '-> normalization file is ' + str(normalization_run)
            # Find full path to event NeXus data file
            f = FileFinder.findRuns("REF_L%d" %normalization_run)
            if len(f)>0 and os.path.isfile(f[0]): 
                norm_file = f[0]
            else:
                msg = "RefLReduction: could not find run %d\n" % normalization_run
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)
            
            #load normalization file
            ws_name = "_normalization_refl%d" % normalization_run
            ws_norm_event_data = ws_name+"_evt"  
            ws_norm_histo_data = ws_name+"_histo"  

            if not mtd.workspaceExists(ws_norm_event_data):
                LoadEventNexus(Filename=norm_file, 
                               OutputWorkspace=ws_norm_event_data)
 
            # Rebin data
            print '-> rebin normalization'
            Rebin(InputWorkspace=ws_norm_event_data, 
                  OutputWorkspace=ws_norm_histo_data, 
                  Params=[TOFrange[0], 
                          TOFsteps, 
                          TOFrange[1]])
 
            # Normalized by Current (proton charge)
            print '-> normalized by current direct beam'
            NormaliseByCurrent(InputWorkspace=ws_norm_histo_data, 
                               OutputWorkspace=ws_norm_histo_data)

            ws_data_bck = '_' + ws_name + '_NormBckWks'
            ws_norm_rebinned = '_' + ws_name + '_NormRebinnedWks'
            if subtract_norm_bck:
                
                print '-> substract background to direct beam'
                ConvertToMatrixWorkspace(InputWorkspace=ws_norm_histo_data,
                                         OutputWorkspace=ws_norm_histo_data)
                            
                ws_norm_bck = '_' + ws_name + '_NormBckWks'
                
                bBackLeft = False
                if (norm_back[0] < (norm_peak[0]-1)):
            
                    bBackLeft = True
                    ws_norm_bck_1 = ws_norm_bck + "_1"
                    RefRoi(InputWorkspace=ws_norm_histo_data,
                           OutputWorkspace=ws_norm_bck_1,
                           NXPixel=maxX,
                           NYPixel=maxY,
                           ConvertToQ=False,
                           IntegrateY=False,
                           SumPixels=True,
                           XPixelMin=normXrange[0],
                           XPixelMax=normXrange[1],
                           YPixelMin=norm_back[0],
                           YPixelMax=norm_peak[0]-1,
                           NormalizeSum=True)
                           
                    ws_norm_bck_1_rebin = ws_norm_bck_1 + '_rebin'
                    RebinToWorkspace(WorkspaceToRebin=ws_norm_bck_1, 
                                     WorkspaceToMatch=ws_norm_histo_data, 
                                     OutputWorkspace=ws_norm_bck_1_rebin)


                bBackRight = False
                if ((norm_peak[1]+1) < norm_back[1]):

                    bBackRight = True
                    ws_norm_bck_2 = ws_norm_bck + "_2"
                    RefRoi(InputWorkspace=ws_norm_histo_data,
                           OutputWorkspace=ws_norm_bck_2,
                           NXPixel=maxX,
                           NYPixel=maxY,
                           ConvertToQ=False,
                           IntegrateY=False,
                           SumPixels=True,
                           XPixelMin=normXrange[0],
                           XPixelMax=normXrange[1],
                           YPixelMin=norm_peak[1]+1,
                           YPixelMax=norm_back[1],
                           NormalizeSum=True)
            
                    ws_norm_bck_2_rebin = ws_norm_bck_2 + '_rebin'
                    RebinToWorkspace(WorkspaceToRebin=ws_norm_bck_2, 
                                     WorkspaceToMatch=ws_norm_histo_data, 
                                     OutputWorkspace=ws_norm_bck_2_rebin)

                if (bBackLeft and bBackRight):

                    Plus(RHSWorkspace=ws_norm_bck_1_rebin,
                         LHSWorkspace=ws_norm_bck_2_rebin,
                         OutputWorkspace=ws_norm_bck)
                    Scale(InputWorkspace=ws_norm_bck,
                          OutputWorkspace=ws_norm_bck+'_scale',
                          Factor=0.5,
                          Operation="Multiply")

                    Minus(LHSWorkspace=ws_norm_histo_data, 
                          RHSWorkspace=ws_norm_bck+'_scale', 
                          OutputWorkspace=ws_norm_rebinned)
                    mtd.deleteWorkspace(ws_norm_bck_1_rebin)
                    mtd.deleteWorkspace(ws_norm_bck_2_rebin)
                    mtd.deleteWorkspace(ws_norm_bck_1)
                    mtd.deleteWorkspace(ws_norm_bck_2)
                    mtd.deleteWorkspace(ws_norm_histo_data)
                    mtd.deleteWorkspace(ws_norm_bck+'_scale')

                elif (bBackLeft):
                    
                    Minus(LHSWorkspace=ws_norm_histo_data,
                          RHSWorkspace=ws_norm_bck_1_rebin,
                          OutputWorkspace=ws_norm_rebinned)
                    mtd.deleteWorkspace(ws_norm_bck_1_rebin)
                    mtd.deleteWorkspace(ws_norm_bck_1)
                    mtd.deleteWorkspace(ws_norm_histo_data)

                elif (bBackRight):
                    
                    Minus(LHSWorkspace=ws_norm_histo_data,
                          RHSWorkspace=ws_norm_bck_2_rebin,
                          OutputWorkspace=ws_norm_rebinned)
                    mtd.deleteWorkspace(ws_norm_bck_2_rebin)
                    mtd.deleteWorkspace(ws_norm_bck_2)
                    mtd.deleteWorkspace(ws_norm_histo_data)

                wks_utility.createIntegratedWorkspace(mtd[ws_norm_rebinned], 
                                                      ws_norm_rebinned,
                                                      fromXpixel=normXrange[0],
                                                      toXpixel=normXrange[1],
                                                      fromYpixel=BackfromYpixel,
                                                      toYpixel=BacktoYpixel,
                                                      maxX=maxX,
                                                      maxY=maxY)

            else:
            
                #Create a new event workspace of only the range of pixel of interest 
                #background range (along the y-axis) and of only the pixel
                #of interest along the x-axis (to avoid the frame effect)
                ws_integrated_data = '_' + ws_name + '_IntegratedNormWks'
                wks_utility.createIntegratedWorkspace(mtd[ws_norm_histo_data], 
                                                      ws_integrated_data,
                                                      fromXpixel=normXrange[0],
                                                      toXpixel=normXrange[1],
                                                      fromYpixel=BackfromYpixel,
                                                      toYpixel=BacktoYpixel,
                                                      maxX=maxX,
                                                      maxY=maxY)
            
                RebinToWorkspace(WorkspaceToRebin=ws_integrated_data,
                                 WorkspaceToMatch=ws_data,
                                 OutputWorkspace=ws_norm_rebinned)

                mtd.deleteWorkspace(ws_integrated_data)
                
            #Normalization    
            print '-> Sum spectra'       
            SumSpectra(InputWorkspace=ws_norm_rebinned, 
                       OutputWorkspace=ws_norm_rebinned)

            #### divide data by normalize histo workspace
            print '-> Divide data by direct beam'
            Divide(LHSWorkspace=ws_data,
                   RHSWorkspace=ws_norm_rebinned,
                   OutputWorkspace=ws_data)

        #This is where I need to move from TOF to Q (not before that)
        #now we can convert to Q
        
        theta = tthd_rad - ths_rad
        AngleOffset_deg = float(self.getProperty("AngleOffset"))
        AngleOffset_rad = (AngleOffset_deg * math.pi) / 180.
        theta += AngleOffset_rad

#        this is where we need to apply the scaling factor
#        print '-> Apply SF'
#        ws_data_scaled = wks_utility.applySF(ws_data,
#                                             slitsValuePrecision)
        ws_data_scaled = ws_data   #REMOVE_ME

        if dMD is not None and theta is not None:
                    
            _tof_axis = mtd[ws_data].readX(0)
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
        
        ws_integrated_data = '_' + ws_name + '_IntegratedDataWks'
        print '-> keep only range of pixel of interest'
        wks_utility.createIntegratedWorkspace(mtd[ws_data_scaled], 
                                              ws_integrated_data,
                                              fromXpixel=Xrange[0],
                                              toXpixel=Xrange[1],
                                              fromYpixel=BackfromYpixel,
                                              toYpixel=BacktoYpixel,
                                              maxX=maxX,
                                              maxY=maxY)     

        
#        mtd.deleteWorkspace(ws_data_scaled)
#        mtd.deleteWorkspace(ws_data)
        ws_data_Q = ws_data + '_Q'
        print '-> convert to Q'
#        wks_utility.convertWorkspaceToQ(ws_data_scaled,
        wks_utility.convertWorkspaceToQ(ws_integrated_data,
                                        ws_data_Q,
                                        fromYpixel=data_peak[0],
                                        toYpixel=data_peak[1],
                                        cpix=data_cpix,
                                        source_to_detector=dMD,
                                        sample_to_detector=dSD,
                                        theta=theta,
                                        geo_correction=False,
                                        q_binning=[q_min,q_step,q_max])

        print '-> replace special values'
        mt = mtd[ws_data_Q]
        ReplaceSpecialValues(InputWorkspace=ws_data_Q, 
                             NaNValue=0, 
                             NaNError=0, 
                             InfinityValue=0, 
                             InfinityError=0, 
                             OutputWorkspace=ws_data_Q)

        output_ws = self.getPropertyValue("OutputWorkspace")        
        
        if mtd.workspaceExists(output_ws):
            mtd.deleteWorkspace(output_ws)
            
        print '-> sum spectra'    
        SumSpectra(InputWorkspace=ws_data_Q, OutputWorkspace=output_ws)

        #keep only none zero values
        try:
            print '-> keep only non-zeros values'
            mt = mtd[output_ws]
            sz = shape(mt.readY(0)[:])[0]
            data_x = []
            data_y = []
            data_y_error = []
            for i in range(sz):
                _y = mt.readY(0)[i]
                #print '_y={0:3f} at i={1:2d}'.format(_y, i)
                if _y != 0.:
                    data_x.append(mt.readX(0)[i])
                    data_y.append(_y)
                    data_y_error.append(mt.readE(0)[i])
        
            #if at least one non zero value found
            if data_x != []:
                print '-> cleanup data (remove 0s)'
                CreateWorkspace(OutputWorkspace=output_ws,
                                DataX=data_x,
                                DataY=data_y,
                                DataE=data_y_error,
                                Nspec=1,
                                UnitX="MomentumTransfer")
        except:
            pass

        #removing first and last Q points (edge effect) 
        mt=mtd[output_ws]
        x_axis = mt.readX(0)[:]
        if (len(x_axis) > 2):
            print '-> remove first and last point (edge effet)'
            qmin = x_axis[1]
            qmax = x_axis[-2]
            CropWorkspace(InputWorkspace=output_ws,
                          OutputWorkspace=output_ws,
                          XMin=qmin, XMax=qmax)

         #space
        self.setProperty("OutputWorkspace", mtd[output_ws])
        
        #cleanup all workspace used
        print '-> Cleaning useless workspaces'
        mtd.deleteWorkspace(ws_event_data)
        mtd.deleteWorkspace(ws_data_Q)
        mtd.deleteWorkspace(ws_data)
        mtd.deleteWorkspace(ws_norm_event_data)
        
        print
        
        
mtd.registerPyAlgorithm(RefLReduction())
