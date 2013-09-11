"""*WIKI* 

Liquids Reflectometer (REFL) reduction

*WIKI*"""

#from MantidFramework import *
#from mantidsimple import *
from mantid.api import *
from mantid.simpleapi import *
from numpy import zeros, shape, arange
import math

# import sfCalculator
import sys
import os
sys.path.insert(0,os.path.dirname(__file__))
import sfCalculator
sys.path.pop(0)


from mantid.kernel import *

class RefLReduction(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "RefLReduction"
    
    def version(self):
        return 1

    def PyInit(self):
#         self.declareListProperty("RunNumbers", [0], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="List of run numbers to process")
#         self.declareProperty("RunNumbers", "", "List of run numbers to process")
        self.declareProperty(IntArrayProperty("RunNumbers"), "List of run numbers to process")
        
#         self.declareProperty("NormalizationRunNumber", 0, 
#                              Description="Run number of the normalization run to use")
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")

#         self.declareListProperty("SignalPeakPixelRange", [126, 134], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range defining the data peak")

#         default_data_peak = [126, 134]
#         arrVal = IntArrayLengthValidator(2)
#         self.declareProperty(IntArrayProperty("SignalPeakPixelRange", default_data_peak, 
#                                               arrVal, direction=Direction.Input),
#                              "Pixelrange defining the data peak. Default:(126,134)")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange"), "Pixel range defining the data peak")



#         self.declareProperty("SubtractSignalBackground", True,
#                              Description="If true, the background will be subtracted from the data peak")
        self.declareProperty("SubtractSignalBackground", True,
                             doc='If true, the background will be subtracted from the data peak')

#         self.declareListProperty("SignalBackgroundPixelRange", [123, 137], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range defining the background")
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixelrange defining the background. Default:(123,137)")

#         self.declareProperty("NormFlag", True, Description="If true, the data will be normalized")
        self.declareProperty("NormFlag", True, doc="If true, the data will be normalized")

#         self.declareListProperty("NormPeakPixelRange", [127, 133], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range defining the normalization peak")
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127,133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the normalization peak")

#         self.declareProperty("SubtractNormBackground", True,
#                              Description="If true, the background will be subtracted from the normalization peak")
        self.declareProperty("SubtractNormBackground", True, 
                             doc="If true, the background will be subtracted from the normalization peak")
        
#         self.declareListProperty("NormBackgroundPixelRange", [123, 137], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range defining the background for the normalization")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127,137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background for the normalization")            

#         self.declareProperty("LowResDataAxisPixelRangeFlag", True,
#                                  Description="If true, the low-resolution direction of the data will be cropped according to the LowResDataAxisPixelRange property")e
        self.declareProperty("LowResDataAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the data will be cropped according to the lowResDataAxisPixelRange property")

#         self.declareListProperty("LowResDataAxisPixelRange", [115, 210], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range to use in the low-resolution direction of the data")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115,210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the data")        
        
#         self.declareProperty("LowResNormAxisPixelRangeFlag", True,
#                              Description="If true, the low-resolution direction of the normalization run will be cropped according to the LowResNormAxisPixelRange property")
        self.declareProperty("LowResNormAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped according to the LowResNormAxisPixelRange property")

#         self.declareListProperty("LowResNormAxisPixelRange", [115, 210], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="Pixel range to use in the low-resolution direction of the normalization run")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115,210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the normalizaion run")

#         self.declareListProperty("TOFRange", [9000., 23600.], 
#                                  Validator=ArrayBoundedValidator(Lower=0),
#                                  Description="TOF range to use")
        self.declareProperty(FloatArrayProperty("TOFRange", [9000., 23600.],
                                                FloatArrayLengthValidator(2), direction=Direction.Input),
                             "TOF range to use")

#         self.declareProperty("TofRangeFlag", True,
#                              Description="If true, the TOF will be cropped according to the TOFRange property")
        self.declareProperty("TofRangeFlag", True,
                             doc="If true, the TOF will be cropped according to the TOF range property")        

#         self.declareProperty("QMin", 0.05, 
#                              Description="Minimum Q-value")
        self.declareProperty("QMin", 0.05, doc="Mnimum Q-value")
        
#         self.declareProperty("QStep", 0.02, 
#                              Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")

#         self.declareProperty("AngleOffset", 0.0, 
#                              Description="Angle offset (degrees)")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")

#         self.declareProperty("AngleOffsetError", 0.0, 
#                              Description="Angle offset error (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, doc="Angle offset error (degrees)")

#         # Output workspace to put the transmission histo into
#         self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")

#         #scaling factor file
#         self.declareProperty("ScalingFactorFile", "", 
#                              Description="Scaling Factor configuration file")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")

#         self.declareProperty("SlitsWidthFlag", True,
#                              Description="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("SlitsWidthFlag", True, 
                             doc="Looking for perfect match of slits width when using Scaling Factor file")

#         #incident medium
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
#         self.declareProperty("IncidentMediumSelected", "",
#                              Description="Incident medium used for those runs")

    def PyExec(self):   
        
        print '-- > starting reduction ...'
        
        import os
        import numpy
        import math
        from reduction.instruments.reflectometer import wks_utility
        
        from mantid import mtd
        #remove all previous workspaces
        list_mt = mtd.getObjectNames()
        for _mt in list_mt:
            if _mt.find('_scaled') != -1:
                DeleteWorkspace(_mt)
#                 mtd.remove(_mt)
            if _mt.find('_reflectivity') != -1:
                DeleteWorkspace(_mt)
#                 mtd.remove(_mt)
            
#        from mantidsimple import mtd    

        bDebug = True
        if bDebug:
            print '====== Running in mode DEBUGGING ======='

        run_numbers = self.getProperty("RunNumbers").value

        backSubMethod = 1   #1 uses RefRoi, 2 used own method

#        mtd.sendLogMessage("RefLReduction: processing %s" % run_numbers)

        #run with normalization or not    
        NormFlag = self.getProperty("NormFlag")
        
        normalization_run = self.getProperty("NormalizationRunNumber").value

        data_peak = self.getProperty("SignalPeakPixelRange").value
        data_back = self.getProperty("SignalBackgroundPixelRange").value

        # TOF range to consider
        TOFrangeFlag = self.getProperty("TofRangeFlag")
        if (TOFrangeFlag):
            TOFrange = self.getProperty("TOFRange").value #microS
        else:
            TOFrange = [0, 200000]
            
        # Steps for TOF rebin
        TOFsteps = 100.0

        #use now a global q binning (user does not have control over it)
        #q_min = 0.005
        #q_step = -0.01

        # Q binning for output distribution
        q_min = self.getProperty("QMin").value
        q_step = self.getProperty("QStep").value
        if (q_step > 0):
            q_step = -q_step
        
        #dimension of the detector (256 by 304 pixels)
        maxX = 304
        maxY = 256
                
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        data_low_res_flag = self.getProperty("LowResDataAxisPixelRangeFlag")
        if data_low_res_flag:
            data_low_res = self.getProperty("LowResDataAxisPixelRange").value
        else:
            data_low_res = [0,maxX-1]

        norm_low_res_flag = self.getProperty("LowResNormAxisPixelRangeFlag")
        if norm_low_res_flag:
            norm_low_res = self.getProperty("LowResNormAxisPixelRange").value
        else:
            norm_low_res = [0,maxX-1]
                
        h = 6.626e-34  #m^2 kg s^-1
        m = 1.675e-27     #kg

        norm_back = self.getProperty("NormBackgroundPixelRange").value
        norm_peak = self.getProperty("NormPeakPixelRange").value

        subtract_data_bck = self.getProperty("SubtractSignalBackground").value
        subtract_norm_bck = self.getProperty("SubtractNormBackground").value

        #name of the sfCalculator txt file
#        slitsValuePrecision = 0.01       #precision of slits = 10% 
        slitsValuePrecision = sfCalculator.PRECISION
        sfFile = self.getProperty("ScalingFactorFile").value
        incidentMedium = self.getProperty("IncidentMediumSelected").value
        slitsWidthFlag = self.getProperty("SlitsWidthFlag")
                
        # Pick a good workspace name
        ws_name = "refl%d" % run_numbers[0]
        ws_event_data = ws_name+"_evt"  
        
        # Load the data into its workspace
        allow_multiple = True        
        
        if len(run_numbers)>1 and allow_multiple:

            _list = []
            for _run in run_numbers:
                _list.append(str(_run))
            list_run = ','.join(_list)
            print '** Working with data runs: ' + str(list_run)

            _index = 0
            
            for _run in run_numbers:

                ##############################################################
                # Find full path to event NeXus data file
                try:
                    data_file = FileFinder.findRuns("REF_L%d" %_run)[0]
                except RuntimeError:
                    msg = "RefLReduction: could not find run %d\n" % _run
                    msg += "Add your data folder to your User Data Directories in the File menu"
                    raise RuntimeError(msg)
                
#                 if not mtd.doesExist(ws_event_data.name()):
                if _index == 0:
                    ws_event_data = LoadEventNexus(Filename=data_file)
                    _index += 1
                else:
                    tmp = LoadEventNexus(Filename=data_file)
#                    mt1 = mtd[ws_event_data]
#                    mt2 = mtd['tmp']
                    Plus(LHSWorkspace=ws_event_data,
                         RHSWorkspace=tmp, 
                         OutputWorkspace=ws_event_data)
                    DeleteWorkspace(tmp)
        else:

            print '** Working with data run: ' + str(run_numbers[0])
            
            try:
                data_file = FileFinder.findRuns("REF_L%d" %run_numbers[0])[0]
            except RuntimeError:
                msg = "RefLReduction: could not find run %d\n" %run_numbers[0]
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)

#             if not mtd.doesExist(ws_event_data):
            ws_event_data = LoadEventNexus(Filename=data_file)
        
        # Get metadata
        #mt_run = mtd[ws_event_data].getRun()
        mt_run = ws_event_data.getRun()
        
        ##get angles value
        thi_value = mt_run.getProperty('thi').value[0]
        thi_units = mt_run.getProperty('thi').units
        tthd_value = mt_run.getProperty('tthd').value[0]
        tthd_units = mt_run.getProperty('tthd').units
        thi_rad = wks_utility.angleUnitConversion(value=thi_value,
                                                  from_units=thi_units,
                                                  to_units='rad')
        tthd_rad = wks_utility.angleUnitConversion(value=tthd_value,
                                                   from_units=tthd_units,
                                                   to_units='rad')

        # Rebin data (x-axis is in TOF)
        print '-> Rebin'
        ws_histo_data = Rebin(InputWorkspace=ws_event_data, 
                              Params=[TOFrange[0], 
                                      TOFsteps, 
                                      TOFrange[1]],
                              PreserveEvents=True)
                
        # Keep only range of TOF of interest
        print '-> Crop TOF range'
        ws_histo_data = CropWorkspace(InputWorkspace = ws_histo_data,
                                      XMin=TOFrange[0], 
                                      XMax=TOFrange[1])

        # Normalized by Current (proton charge)
        print '-> Normalize by proton charge'
        ws_histo_data = NormaliseByCurrent(InputWorkspace=ws_histo_data)
    
        # Calculation of the central pixel (using weighted average)
        pixelXtof_data = wks_utility.getPixelXTOF(ws_histo_data, 
                                                  maxX=maxX, maxY=maxY)
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
        sample = ws_event_data.getInstrument().getSample()
        source = ws_event_data.getInstrument().getSource()
        dSM = sample.getDistance(source)
        # Create array of distances pixel->sample
        dPS_array = numpy.zeros((maxY, maxX))
        for x in range(maxX):
            for y in range(maxY):
                _index = maxY * x + y
                detector = ws_event_data.getDetector(_index)
                dPS_array[y, x] = sample.getDistance(detector)
        
        DeleteWorkspace(ws_event_data)
        
        # Array of distances pixel->source
        dMP_array = dPS_array + dSM
        # Distance sample->center of detector
        dSD = dPS_array[maxY / 2, maxX / 2]
        # Distance source->center of detector        
        dMD = dSD + dSM

#         ws_data = '_' + ws_name + '_DataWks'

        #Even if user select Background subtraction
        #make sure there is a background selection (peak != back selection)        

        _LfromPx = data_back[0]
        _LtoPx = data_peak[0]
        _RfromPx = data_peak[1]
        _RtoPx = data_back[1]

        if ((_LfromPx == _LtoPx) and (_RfromPx == _RtoPx)):
            subtract_data_bck = False
        
        if (subtract_data_bck and (backSubMethod == 1)):

            print '-> substract background'
            ws_histo_data = ConvertToMatrixWorkspace(InputWorkspace=ws_histo_data)
            
#             ws_data_bck = '_' + ws_name + '_DataBckWks'
            
            bBackLeft = False
            if (data_back[0] < (data_peak[0]-1)):

                bBackLeft = True
                                
                ws_data_bck_1 = RefRoi(InputWorkspace=ws_histo_data,
                       NXPixel=maxX,
                       NYPixel=maxY,
                       ConvertToQ=False,
                       IntegrateY=False,
                       SumPixels=True,
                       XPixelMin=int(data_low_res[0]),
                       XPixelMax=int(data_low_res[1]),
                       YPixelMin=int(data_back[0]),
                       YPixelMax=int(data_peak[0]-1),
                       NormalizeSum=True)

                ws_data_bck_1_rebin = RebinToWorkspace(WorkspaceToRebin=ws_data_bck_1, 
                                 WorkspaceToMatch=ws_histo_data)

            bBackRight = False
            if ((data_peak[1]+1) < data_back[1]):

                bBackRight = True
                ws_data_bck_2 = RefRoi(InputWorkspace=ws_histo_data,
                       NXPixel=maxX,
                       NYPixel=maxY,
                       ConvertToQ=False,
                       IntegrateY=False,
                       SumPixels=True,
                       XPixelMin=int(data_low_res[0]),
                       XPixelMax=int(data_low_res[1]),
                       YPixelMin=int(data_peak[1]+1),
                       YPixelMax=int(data_back[1]),
                       NormalizeSum=True)
            
                ws_data_bck_2_rebin = RebinToWorkspace(WorkspaceToRebin=ws_data_bck_2, 
                                 WorkspaceToMatch=ws_histo_data)
                
            if (bBackLeft and bBackRight):
            
                ws_data_bck = Plus(RHSWorkspace=ws_data_bck_1_rebin,
                     LHSWorkspace=ws_data_bck_2_rebin)
                
                ws_data_bck_scale = Scale(InputWorkspace=ws_data_bck,
                      Factor=0.5,
                      Operation="Multiply")
                
                ws_data = Minus(LHSWorkspace=ws_histo_data, 
                      RHSWorkspace=ws_data_bck_scale) 

#                 if mtd.workspaceExists(ws_data_bck+'_scale'):
#                     mtd.deleteWorkspace(ws_data_bck+'_scale')
#                 
#                 if mtd.workspaceExists(ws_data_bck):
#                     mtd.deleteWorkspace(ws_data_bck)
#                 
#                 if mtd.workspaceExists(ws_data_bck_1_rebin):
#                     mtd.deleteWorkspace(ws_data_bck_1_rebin)
#                 
#                 if mtd.workspaceExists(ws_data_bck_2_rebin):
#                     mtd.deleteWorkspace(ws_data_bck_2_rebin)
#                 
#                 if mtd.workspaceExists(ws_data_bck_1):
#                     mtd.deleteWorkspace(ws_data_bck_1)
#                 
#                 if mtd.workspaceExists(ws_data_bck_2):
#                     mtd.deleteWorkspace(ws_data_bck_2)
#                 
#                 if mtd.workspaceExists(ws_histo_data):
#                     mtd.deleteWorkspace(ws_histo_data)

            elif (bBackLeft):
                
                ws_data = Minus(LHSWorkspace=ws_histo_data,
                      RHSWorkspace=ws_data_bck_1_rebin)
                
#                 if mtd.workspaceExists(ws_data_bck_1_rebin):
#                     mtd.deleteWorkspace(ws_data_bck_1_rebin)
#                 
#                 if mtd.workspaceExists(ws_data_bck_1):
#                     mtd.deleteWorkspace(ws_data_bck_1)
                
            elif (bBackRight):
                
                ws_data = Minus(LHSWorkspace=ws_histo_data,
                      RHSWorkspace=ws_data_bck_2_rebin)

#                 if mtd.workspaceExists(ws_data_bck_2_rebin):
#                     mtd.deleteWorkspace(ws_data_bck_2_rebin)
#                 
#                 if mtd.workspaceExists(ws_data_bck_2):
#                     mtd.deleteWorkspace(ws_data_bck_2)

            #cleanup (remove all negatives values
            ws_data = ResetNegatives(InputWorkspace=ws_data,
                           AddMinimum=0)
            
#             if mtd.workspaceExists(ws_histo_data):
#                 mtd.deleteWorkspace(ws_histo_data)

        if (subtract_data_bck and (backSubMethod == 2)):
                            
            #integrate over the x axis in the low axis range specified
            ws_histo_data_1D = wks_utility.createIntegratedWorkspace(ws_histo_data, 
                                                                     fromXpixel=data_low_res[0],
                                                                     toXpixel=data_low_res[1],
                                                                     fromYpixel=0,
                                                                     toYpixel=255,
                                                                     maxX=maxX,
                                                                     maxY=maxY)

            #for each TOF, get the average counts over the two
            #background regions (top and bottom)
            _mt = ws_histo_data_1D
            _x_axis = _mt.readX(0)[:]
            _nbr_tof = len(_x_axis)
            _tof_range = range(_nbr_tof-1)
            _back_array = zeros(_nbr_tof-1)
            _back_array_error = zeros(_nbr_tof-1) 
            
            #work on left side
            _LfromPx = data_back[0]
            _LtoPx = data_peak[0]
            #work on right side
            _RfromPx = data_peak[1]
            _RtoPx = data_back[1]

            bLeftBack = False            
            if (_LfromPx < _LtoPx):
                _Larray = arange(_LtoPx - _LfromPx) + _LfromPx
                bLeftBack = True
            
            bRightBack = False
            if (_RfromPx < _RtoPx):
                _Rarray = arange(_RtoPx - _RfromPx) + _RfromPx
                bRightBack = True
            
            if (bLeftBack and bRightBack):
                _y_px_range = numpy.append(_Larray,_Rarray)
#                _y_px_range = _y_px_range.flatten()
            else:
                if (bLeftBack):
                    _y_px_range = _Larray
                else: 
                    _y_px_range = _Rarray

            for i in _tof_range:
                _sum = 0.
                _sum_error = 0.
                _pts_summed = 0.
                
                _val = 0.
                _err = 0.
                for j in _y_px_range:
                    _val = float(_mt.readY(int(j))[int(i)])
                    _err = float(_mt.readE(int(j))[int(i)])
                    if (_val != 0 and _err !=0):
                        _new_val = float(_val / _err)
                        _new_err = 1./_err
                        _sum += _new_val
                        _sum_error += _new_err

                if (_val !=0. and _err !=0.):                                        
                    _back_array[i] = float(_sum / _sum_error)
                    _back_array_error[i] = float(1./ _sum_error)

            #substract this number from the rest
            background = CreateWorkspace(DataX=_x_axis,
                                         DataY=_back_array,
                                         DataE=_back_array_error,
                                         UnitX="TOF",
                                         ParentWorkspace=ws_histo_data.name(),
                                         NSpec=1)

            #recreate workspace at the end                
#             mt1 = mtd[ws_histo_data+'_1D']
#             mt2 = mtd['background']
                                    
            ws_data = Minus(LHSWorkspace=ws_histo_data_1D,
                            RHSWorkspace=background)

            ws_data = ResetNegatives(InputWorkspace=ws_data,
                                     AddMinimum=0)
            
#             if mtd.doesExist(ws_histo_data_1D):
            DeleteWorkspace(ws_histo_data_1D)
            DeleteWorkspace(background)

#            SumSpectra(InputWorkspace=ws_data, 
#                       OutputWorkspace='wks_after_back_subtraction_1d')
        
        if (not(subtract_data_bck)):

            ws_data = wks_utility.createIntegratedWorkspace(ws_histo_data, 
                                                            fromXpixel=data_low_res[0],
                                                            toXpixel=data_low_res[1],
                                                            fromYpixel=data_peak[0],
                                                            toYpixel=data_peak[1],
                                                            maxX=maxX,
                                                            maxY=maxY)     
            DeleteWorkspace(ws_histo_data)
            ws_data = ConvertToMatrixWorkspace(InputWorkspace=ws_data)
            
#            ConvertToMatrixWorkspace(InputWorkspace=ws_data,
#                                     OutputWorkspace=ws_data)

# work with normalization file

        if (NormFlag):

            normalization_run = str(normalization_run)

            print '-> normalization file is ' + normalization_run
            # Find full path to event NeXus data file
            try:
                norm_file = FileFinder.findRuns("REF_L%d" %int(normalization_run))[0]
            except RuntimeError:
                msg = "RefLReduction: could not find run %d\n" %int(normalization_run)
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)
            
            #load normalization file
            ws_name = "_normalization_refl%d" % int(normalization_run)
            ws_norm_event_data = ws_name+"_evt"  
            ws_norm_histo_data = ws_name+"_histo"  

            if not mtd.doesExist(ws_norm_event_data):
                ws_norm_event_data = LoadEventNexus(Filename=norm_file)
 
            # Rebin data
            print '-> rebin normalization'
            ws_norm_histo_data = Rebin(InputWorkspace=ws_norm_event_data, 
                                       Params=[TOFrange[0], 
                                               TOFsteps, 
                                               TOFrange[1]])
 
            # Keep only range of TOF of interest
            print '-> Crop TOF range'
            ws_norm_histo_data = CropWorkspace(InputWorkspace=ws_norm_histo_data,
                                               XMin=TOFrange[0], 
                                               XMax=TOFrange[1])    
            
            # Normalized by Current (proton charge)
            print '-> normalized by current direct beam'
            ws_norm_histo_data = NormaliseByCurrent(InputWorkspace=ws_norm_histo_data) 

#            ws_data_bck = '_' + ws_name + '_NormBckWks'
#            ws_norm_rebinned = '_' + ws_name + '_NormRebinnedWks'
#            ws_norm_rebinned = ws_name + '_NormRebinnedWks'
            if (subtract_norm_bck and (backSubMethod == 1)):
                
                print '-> substract background to direct beam'
                ws_norm_histo_data = ConvertToMatrixWorkspace(InputWorkspace=ws_norm_histo_data)
                
                bBackLeft = False
                if (norm_back[0] < (norm_peak[0]-1)):
            
                    bBackLeft = True
                    ws_norm_bck_1 = RefRoi(InputWorkspace=ws_norm_histo_data,
                           NXPixel=maxX,
                           NYPixel=maxY,
                           ConvertToQ=False,
                           IntegrateY=False,
                           SumPixels=True,
                           XPixelMin=int(norm_low_res[0]),
                           XPixelMax=int(norm_low_res[1]),
                           YPixelMin=int(norm_back[0]),
                           YPixelMax=int(norm_peak[0]-1),
                           NormalizeSum=True)
                           
                    ws_norm_bck_1_rebin = RebinToWorkspace(WorkspaceToRebin=ws_norm_bck_1, 
                                     WorkspaceToMatch=ws_norm_histo_data)


                bBackRight = False
                if ((norm_peak[1]+1) < norm_back[1]):

                    bBackRight = True
                    ws_norm_bck_2 = RefRoi(InputWorkspace=ws_norm_histo_data,
                           NXPixel=maxX,
                           NYPixel=maxY,
                           ConvertToQ=False,
                           IntegrateY=False,
                           SumPixels=True,
                           XPixelMin=int(norm_low_res[0]),
                           XPixelMax=int(norm_low_res[1]),
                           YPixelMin=int(norm_peak[1]+1),
                           YPixelMax=int(norm_back[1]),
                           NormalizeSum=True)
            
                    ws_norm_bck_2_rebin = RebinToWorkspace(WorkspaceToRebin=ws_norm_bck_2, 
                                     WorkspaceToMatch=ws_norm_histo_data)

                if (bBackLeft and bBackRight):

                    ws_norm_bck = Plus(RHSWorkspace=ws_norm_bck_1_rebin,
                         LHSWorkspace=ws_norm_bck_2_rebin)
                    ws_norm_bck_scale = Scale(InputWorkspace=ws_norm_bck,
                          Factor=0.5,
                          Operation="Multiply")

                    ws_norm_rebinned = Minus(LHSWorkspace=ws_norm_histo_data, 
                          RHSWorkspace=ws_norm_bck_scale)
                    
#                     if mtd.workspaceExists(ws_norm_bck_1_rebin):
#                         mtd.deleteWorkspace(ws_norm_bck_1_rebin)
#                     
#                     if mtd.workspaceExists(ws_norm_bck_2_rebin):
#                         mtd.deleteWorkspace(ws_norm_bck_2_rebin)
#                     
#                     if mtd.workspaceExists(ws_norm_bck_1):
#                         mtd.deleteWorkspace(ws_norm_bck_1)
#                     
#                     if mtd.workspaceExists(ws_norm_bck_2):
#                         mtd.deleteWorkspace(ws_norm_bck_2)
#                     
#                     if mtd.workspaceExists(ws_norm_histo_data):
#                         mtd.deleteWorkspace(ws_norm_histo_data)
#                     
#                     if mtd.workspaceExists(ws_norm_bck+'_scale'):
#                         mtd.deleteWorkspace(ws_norm_bck+'_scale')

                elif (bBackLeft):
                    
                    ws_norm_rebinned = Minus(LHSWorkspace=ws_norm_histo_data,
                          RHSWorkspace=ws_norm_bck_1_rebin)
                    
#                     if mtd.workspaceExists(ws_norm_bck_1_rebin):
#                         mtd.deleteWorkspace(ws_norm_bck_1_rebin)
#                     
#                     if mtd.workspaceExists(ws_norm_bck_1):
#                         mtd.deleteWorkspace(ws_norm_bck_1)
#                         
#                     if mtd.workspaceExists(ws_norm_histo_data):
#                         mtd.deleteWorkspace(ws_norm_histo_data)

                elif (bBackRight):
                    
                    ws_norm_rebinned = Minus(LHSWorkspace=ws_norm_histo_data,
                          RHSWorkspace=ws_norm_bck_2_rebin)
                    
#                     if mtd.workspaceExists(ws_norm_bck_2_rebin):
#                         mtd.deleteWorkspace(ws_norm_bck_2_rebin)
#                         
#                     if mtd.workspaceExists(ws_norm_bck_2):
#                         mtd.deleteWorkspace(ws_norm_bck_2)
#                     
#                     if mtd.workspaceExists(ws_norm_histo_data):
#                         mtd.deleteWorkspace(ws_norm_histo_data)

                
                #Here I need to set to zeros all the negative entries
                ws_norm_rebinned = ResetNegatives(InputWorkspace=ws_norm_rebinned,
                               AddMinimum=0)

                ws_norm_rebinned = wks_utility.createIntegratedWorkspace(ws_norm_rebinned, 
                                                      fromXpixel=norm_low_res[0],
                                                      toXpixel=norm_low_res[1],
                                                      fromYpixel=norm_peak[0],
                                                      toYpixel=norm_peak[1],
                                                      maxX=maxX,
                                                      maxY=maxY,
                                                      bCleaning=True)

            if (subtract_norm_bck and (backSubMethod == 2)):
                     
                #integrate over the x axis in the low axis range specified
                ws_norm_histo_data_1D = wks_utility.createIntegratedWorkspace(ws_norm_histo_data, 
                                                                              fromXpixel=norm_low_res[0],
                                                                              toXpixel=norm_low_res[1],
                                                                              fromYpixel=0,
                                                                              toYpixel=255,
                                                                              maxX=maxX,
                                                                              maxY=maxY)

                #for each TOF, get the average counts over the two
                #background regions (top and bottom)
                _mt = ws_norm_histo_data_1D
                _x_axis = _mt.readX(0)[:]
                _nbr_tof = len(_x_axis)
                _tof_range = range(_nbr_tof-1)
                _back_array = zeros(_nbr_tof-1)
                _back_array_error = zeros(_nbr_tof-1) 
            
                #work on left side
                _LfromPx = norm_back[0]
                _LtoPx = norm_peak[0]
                #work on right side
                _RfromPx = norm_peak[1]
                _RtoPx = norm_back[1]

                bLeftBack = False            
                if (_LfromPx < _LtoPx):
                    _Larray = arange(_LtoPx - _LfromPx) + _LfromPx
                    bLeftBack = True
            
                bRightBack = False
                if (_RfromPx < _RtoPx):
                    _Rarray = arange(_RtoPx - _RfromPx) + _RfromPx
                    bRightBack = True
            
                if (bLeftBack and bRightBack):
                    _y_px_range = numpy.append(_Larray,_Rarray)
                else:
                    if (bLeftBack):
                        _y_px_range = _Larray
                    else:
                        _y_px_range = _Rarray

                for i in _tof_range:
                    _sum = 0.
                    _sum_error = 0.
                    _pts_summed = 0.
                
                    _val = 0.
                    _err = 0.
                    for j in _y_px_range:
                        _val = float(_mt.readY(int(j))[int(i)])
                        _err = float(_mt.readE(int(j))[int(i)])
                        if (_val != 0 and _err !=0):
                            _new_val = float(_val / _err)
                            _new_err = 1./_err
                            _sum += _new_val
                            _sum_error += _new_err

                    if (_val !=0. and _err !=0.):                                        
                        _back_array[i] = float(_sum / _sum_error)
                        _back_array_error[i] = float(1./ _sum_error)
              
                #substract this number from the rest
                background = CreateWorkspace(DataX=_x_axis,
                                             DataY=_back_array,
                                             DataE=_back_array_error,
                                             UnitX="TOF",
                                             ParentWorkspace=ws_norm_histo_data.name(),
                                             NSpec=1)
            
                DeleteWorkspace(ws_norm_histo_data)
            
#                #recreate workspace at the end                
#                mt1 = mtd[ws_norm_histo_data+'_1D']
#                mt2 = mtd['background']
                                    
                ws_norm_rebinned = Minus(LHSWorkspace=ws_norm_histo_data_1D,
                                         RHSWorkspace=background)

#                 if mtd.doesExist(ws_norm_histo_data_1D.name()):
                DeleteWorkspace(ws_norm_histo_data_1D)
                    
#                 if mtd.doesExist(background.name()):                    
                DeleteWorkspace(background)

                ws_norm_rebinned = ResetNegatives(InputWorkspace=ws_norm_rebinned,
                                                  AddMinimum=0)

            else:
            
                #Create a new event workspace of only the range of pixel of interest 
                #background range (along the y-axis) and of only the pixel
                #of interest along the x-axis (to avoid the frame effect)
                ws_integrated_data = wks_utility.createIntegratedWorkspace(ws_norm_histo_data, 
                                                                           fromXpixel=norm_low_res[0],
                                                                           toXpixel=norm_low_res[1],
                                                                           fromYpixel=norm_peak[0],
                                                                           toYpixel=norm_peak[1],
                                                                           maxX=maxX,
                                                                           maxY=maxY)
            
                ws_norm_rebinned = RebinToWorkspace(WorkspaceToRebin=ws_integrated_data,
                                                    WorkspaceToMatch=ws_data)

#                 if mtd.workspaceExists(ws_integrated_data):
                DeleteWorkspace(ws_integrated_data)

            #Normalization    
            print '-> Sum spectra'       
            ws_norm_rebinned = SumSpectra(InputWorkspace=ws_norm_rebinned)
             
            #### divide data by normalize histo workspace
            print '-> Divide data by direct beam'
            ws_data = Divide(LHSWorkspace=ws_data,
                   RHSWorkspace=ws_norm_rebinned)

        #now we can convert to Q
        
        theta = math.fabs(tthd_rad - thi_rad)/2.
        AngleOffset_deg = float(self.getProperty("AngleOffset").value)
        AngleOffset_rad = (AngleOffset_deg * math.pi) / 180.
        theta += AngleOffset_rad

        #this is where we need to apply the scaling factor
        sfFile = self.getProperty("ScalingFactorFile").value
        incidentMedium = self.getProperty("IncidentMediumSelected")
        if os.path.isfile(sfFile):
            print '-> Apply automatic SF!'        
            print '--> using SF config file: ' + sfFile
            ws_data_scaled = wks_utility.applySF(ws_data,
                                                 incidentMedium,
                                                 sfFile,
                                                 slitsValuePrecision,
                                                 slitsWidthFlag)
            
        else:
            print '-> Automatic SF not applied!'
            print '--> unknown or no SF config file defined !'
            ws_data_scaled = ws_data
            
        if dMD is not None and theta is not None:

            if bDebug:
                print 'DEBUG: theta= {0:4f}'.format(theta) 
                    
            _tof_axis = ws_data.readX(0)
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
            if (q_min >= q_max):
                q_min = min(_q_axis)

            if bDebug:
                print 'DEBUG: [q_min:q_bin:q_max]=[{0:4f},{1:4f},{2:4f}]'.format(q_min, q_step, q_max) 

        if (backSubMethod == 1):    
            print 'in backSubMethod == 1'    
            ws_integrated_data = ws_name + '_IntegratedDataWks'
            print '-> keep only range of pixel of interest' 
            
            ws_integrated_data = wks_utility.createIntegratedWorkspace(ws_data_scaled, 
                                                                       fromXpixel=data_low_res[0],
                                                                       toXpixel=data_low_res[1],
                                                                       fromYpixel=data_peak[0],
                                                                       toYpixel=data_peak[1],
                                                                       maxX=maxX,
                                                                       maxY=maxY)     
            
#             ws_data_cleaned = ws_name + '_cleaned'
#            wks_utility.cleanup_data(InputWorkspace=ws_integrated_data,
#                                     OutputWorkspace=ws_data_cleaned,
#                                     maxY=maxY)

            #        mtd.deleteWorkspace(ws_data_scaled)
            #        mtd.deleteWorkspace(ws_data)
#             ws_data_Q = ws_data + '_Q'
            print '-> convert to Q'
            #        wks_utility.convertWorkspaceToQ(ws_data_scaled,
            ws_data_Q = wks_utility.convertWorkspaceToQ(ws_integrated_data,
#            wks_utility.convertWorkspaceToQ(ws_data_cleaned,
                                            fromYpixel=data_peak[0],
                                            toYpixel=data_peak[1],
                                            cpix=data_cpix,
                                            source_to_detector=dMD,
                                            sample_to_detector=dSD,
                                            theta=theta,
                                            geo_correction=False,
                                            q_binning=[q_min,q_step,q_max])

#             if mtd.doesExist(ws_integrated_data):
#             DeleteWorkspace(ws_integrated_data)

        else:
            
            withCorrection = True               #REMOVEME should be True
            if withCorrection:
                print 'Convert to Q with geo-correction'
            else:
                print 'Convert to Q without geo-correction'
            ws_data_Q = wks_utility.convertWorkspaceToQ(ws_data_scaled,
                                                        fromYpixel=data_peak[0],
                                                        toYpixel=data_peak[1],
                                                        cpix=data_cpix,
                                                        source_to_detector=dMD,
                                                        sample_to_detector=dSD,
                                                        theta=theta,
                                                        geo_correction=withCorrection, 
                                                        q_binning=[q_min,q_step,q_max])

            DeleteWorkspace(ws_data_scaled)

        print '-> replace special values'
#         mt = ws_data_Q
        ws_data_Q = ReplaceSpecialValues(InputWorkspace=ws_data_Q, 
                                         NaNValue=0, 
                                         NaNError=0, 
                                         InfinityValue=0, 
                                         InfinityError=0)
        
#         output_ws = self.getPropertyValue("OutputWorkspace")        
        
        #add a unique time stamp to the data to sort them for the 
        #stitching process
        import time
        _time = int(time.time())
#         output_ws = output_ws + '_#' + str(_time) + 'ts'        
        name_output_ws = self.getPropertyValue("OutputWorkspace")
         
        import time
        _time = int(time.time())
        name_output_ws = name_output_ws + '_#' + str(_time) + 'ts'
         
#         if mtd.doesExist(output_ws.name()):
#             DeleteWorkspace(output_ws)
#             
        print '-> sum spectra'    
        output_ws = SumSpectra(InputWorkspace=ws_data_Q, 
                               OutputWorkspace=name_output_ws)
        
#         DeleteWorkspace(ws_data_Q)

        #keep only none zero values
        try:
            print '-> keep only non-zeros values'
#             mt = output_ws
            sz = shape(output_ws.readY(0)[:])[0]
            data_x = []
            data_y = []
            data_y_error = []
            for i in range(sz):
                _y = output_ws.readY(0)[i]
                #print '_y={0:3f} at i={1:2d}'.format(_y, i)
                if _y != 0.:
                    data_x.append(output_ws.readX(0)[i])
                    data_y.append(_y)
                    data_y_error.append(output_ws.readE(0)[i])
          
            #if at least one non zero value found
            if data_x != []:
                print '-> cleanup data (remove zeros)'
#                 DeleteWorkspace(output_ws)
  
                output_ws = CreateWorkspace(OutputWorkspace=name_output_ws,
                                            DataX=data_x,
                                            DataY=data_y,
                                            DataE=data_y_error,
                                            Nspec=1,
                                            UnitX="MomentumTransfer")
 
#                 factory = WorkspaceFactoryImpl.Instance()
#                 output_ws = factory.create("Workspace2D",1,len(data_x), len(data_y))
#                 output_ws.setX(0,data_x)
#                 output_ws.setY(0,data_y)
#                 output_ws.setE(0,data_y_error)
#                 output_ws.getAxis(0).setUnit("MomentumTransfer")
         
        except:
            print 'in except of keep only non-zeros values'
  
        #removing first and last Q points (edge effect) 
        x_axis = output_ws.readX(0)[:]
        if (len(x_axis) > 2):
            print '-> remove first and last point (edge effet)'
            qmin = x_axis[1]
            qmax = x_axis[-2]
            output_ws = CropWorkspace(InputWorkspace = output_ws,
                                      OutputWorkspace = name_output_ws,
                                      XMin=qmin, XMax=qmax)
                
        self.setProperty('OutputWorkspace', mtd[name_output_ws])
        

        #cleanup all workspace used
#         print '-> Cleaning useless workspaces'
#         if mtd.doesExist(ws_event_data.name()):
#         DeleteWorkspace(ws_event_data)
#         print 'a'
#         if mtd.doesExist(ws_data_Q.name()):
#         DeleteWorkspace(ws_data_Q)
#         print 'b'
                 
#         if (NormFlag):
#             print 'c'
# #             if mtd.doesExist(ws_norm_event_data.name()):
#             print 'd'
#             DeleteWorkspace(ws_norm_event_data)
        
AlgorithmFactory.subscribe(RefLReduction)
# registerAlgorithm(RefLReduction())


