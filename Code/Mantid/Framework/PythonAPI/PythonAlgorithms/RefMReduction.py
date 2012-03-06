from MantidFramework import *
from mantidsimple import *
import math
import os
import numpy

class RefMReduction(PythonAlgorithm):

    OFF_OFF = 'Off_Off'
    ON_ON   = 'On_On'
    OFF_ON  = 'Off_On'
    ON_OFF  = 'On_Off'
    PIXEL_SIZE = 0.0007 # m
    NX_PIXELS = 304
    NY_PIXELS = 256
    
    def category(self):
        return "Reflectometry"

    def name(self):
        return "RefMReduction"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareListProperty("RunNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("SignalPeakPixelRange", [216, 226], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractSignalBackground", False)
        self.declareListProperty("SignalBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("CropLowResDataAxis", False)
        self.declareListProperty("LowResDataAxisPixelRange", [0, 255], Validator=ArrayBoundedValidator(Lower=0))

        self.declareProperty("PerformNormalization", True, Description="If true, the signal will be normalized")
        self.declareProperty("NormalizationRunNumber", 0, Description="Run number for the direct beam normalization run")
        self.declareListProperty("NormPeakPixelRange", [90, 160], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", False)
        self.declareListProperty("NormBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("CropLowResNormAxis", False)
        self.declareListProperty("LowResNormAxisPixelRange", [0, 255], Validator=ArrayBoundedValidator(Lower=0))
        
        self.declareListProperty("TOFRange", [0., 0.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("Theta", 0.0, Description="Scattering angle (degrees)")
        self.declareProperty("ReflectivityPixel", 0.0, Description="Reflectivity pixel of the specular peak (REFPIX)")
        self.declareProperty("WavelengthMin", 2.5)
        self.declareProperty("WavelengthMax", 6.5)
        self.declareProperty("WavelengthStep", 0.1)
        self.declareProperty("NBins", 0)
        self.declareProperty("LogScale", True)
        
        self.declareProperty("SetDetectorAngle", False,
                             Description="If true, the DANGLE parameter will be replace by the given value")
        self.declareProperty("DetectorAngle", 0.0)
        self.declareProperty("SetDetectorAngle0", False,
                             Description="If true, the DANGLE0 parameter will be replace by the given value")
        self.declareProperty("DetectorAngle0", 0.0)
        self.declareProperty("SetDirectPixel", False,
                             Description="If true, the DIRPIX parameter will be replace by the given value")
        self.declareProperty("DirectPixel", 0.0)
        
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)
        self.declareProperty("OutputMessage", "", Direction=Direction.Output)

    def PyExec(self):
        self._output_message = ""
        # TOF range and binning
        self.TOFrange = self.getProperty("TOFRange")
        self.TOFsteps = 25.0

        # Process each polarization state, use the OFF_OFF state
        # for the output workspace
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        if mtd.workspaceExists(output_ws_name):
            mtd.deleteWorkspace(output_ws_name)
        output_ws = self._process_polarization(RefMReduction.OFF_OFF)
        self.setProperty("OutputWorkspace", mtd[output_ws])
        
        self._process_polarization(RefMReduction.ON_OFF)
        self._process_polarization(RefMReduction.OFF_ON)
        self._process_polarization(RefMReduction.ON_ON)

        self.setProperty("OutputMessage", self._output_message)

    def _calculate_angle(self, workspace):
        """
            Compute the scattering angle
        """
        sangle = 0
        if mtd[workspace].getRun().hasProperty("SANGLE"):
            sangle = mtd[workspace].getRun().getProperty("SANGLE").value[0]
            
        dangle = 0
        if self.getProperty("SetDetectorAngle"):
            dangle = self.getProperty("DetectorAngle")
            self._output_message += "   DANGLE set to %g" % dangle
        else:
            if mtd[workspace].getRun().hasProperty("DANGLE"):
                dangle = mtd[workspace].getRun().getProperty("DANGLE").value[0]
            
        dangle0 = 0
        if self.getProperty("SetDetectorAngle0"):
            dangle0 = self.getProperty("DetectorAngle0")
            self._output_message += "   DANGLE0 set to %g" % dangle0
        else:
            if mtd[workspace].getRun().hasProperty("DANGLE0"):
                dangle0 = mtd[workspace].getRun().getProperty("DANGLE0").value[0]
            
        det_distance = 2.562
        if mtd[workspace].getRun().hasProperty("SampleDetDis"):
            det_distance = mtd[workspace].getRun().getProperty("SampleDetDis").value[0]/1000.0

        direct_beam_pix = 0
        if self.getProperty("SetDirectPixel"):
            direct_beam_pix = self.getProperty("DirectPixel")
            self._output_message += "   DIRPIX set to %g" % direct_beam_pixel
        else:
            if mtd[workspace].getRun().hasProperty("DIRPIX"):
                direct_beam_pix = mtd[workspace].getRun().getProperty("DIRPIX").value[0]
        
        ref_pix = int(self.getProperty("ReflectivityPixel"))
        
        # If the reflectivity pixel is zero taken the position
        # in the middle of the data peak range
        if ref_pix==0:
            peak_range = self.getProperty("SignalPeakPixelRange")
            ref_pix = (peak_range[0]+peak_range[1])/2.0
            self._output_message += "   Supplied reflectivity pixel is zero\n"
            self._output_message += "     Taking peak mid-point: %g\n" % ref_pix
        
        theta = (dangle-dangle0)*math.pi/180.0/2.0\
            + ((direct_beam_pix-ref_pix)*RefMReduction.PIXEL_SIZE)/ (2.0*det_distance)
        
        return theta*180.0/math.pi
        
    def _process_polarization(self, polarization):
        """
            Process a polarization state
            
            @param polarization: polarization state
        """
        # Sanity check
        if polarization not in [RefMReduction.OFF_OFF, RefMReduction.OFF_ON,
                                RefMReduction.ON_OFF, RefMReduction.ON_ON]:
            raise RuntimeError("RefMReduction: invalid polarization %s" % polarization)
        
        # Convert to Q
        output_ws = self.getPropertyValue("OutputWorkspace")    
        
        # Rename for polarization
        if polarization != RefMReduction.OFF_OFF:
            if output_ws.find(RefMReduction.OFF_OFF)>0:
                output_ws = output_ws.replace(RefMReduction.OFF_OFF, polarization)
            else:
                output_ws += '_%s' % polarization

        # Load the data with the chosen polarization
        output_ws = self._load_data(polarization, output_ws)
        
        # Return immediately if we don't have any events
        if output_ws is None:
            return None
        
        return output_ws

    def _load_data(self, polarization, output_ws):
        """
            Load the signal data
            
            @param polarization: Off_Off, Off_On, On_Off, or On_On
        """
        run_numbers = self.getProperty("RunNumbers")
        mtd.sendLogMessage("RefMReduction: processing %s [%s]" % (run_numbers, polarization))
        self._output_message += "Processing %s [%s]\n" % (run_numbers, polarization)
        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("SignalPeakPixelRange")

        # Get the integration range in the low-res direction
        low_res_range = [0, RefMReduction.NY_PIXELS-1]
        if self.getProperty("CropLowResDataAxis"):
            low_res_range = self.getProperty("LowResDataAxisPixelRange")
                  
        
        # Subtract background
        bck_range = None
        if self.getProperty("SubtractSignalBackground"):
            bck_range = self.getProperty("SignalBackgroundPixelRange")
        
        ws_name = self._process_2D(run_numbers, peak_range, 
                                         low_res_range, bck_range, 
                                         polarization=polarization)
        
        # Check whether we have valid data, otherwise return
        if ws_name is None:
            return
                
        # Perform normalization according to wavelength distribution of
        # the direct beam
        if self.getProperty("PerformNormalization"):
            self._output_message += "   Normalization:\n"
            ws_wl_profile = self._process_normalization()
            RebinToWorkspace(WorkspaceToRebin=ws_wl_profile, 
                             WorkspaceToMatch=ws_name,
                             OutputWorkspace=ws_wl_profile)
            Divide(LHSWorkspace=ws_name,
                   RHSWorkspace=ws_wl_profile,
                   OutputWorkspace=ws_name)
            ReplaceSpecialValues(InputWorkspace=ws_name,
                                 OutputWorkspace=ws_name,
                                 NaNValue=0.0, NaNError=0.0,
                                 InfinityValue=0.0, InfinityError=0.0)
        
        # 2D reduction
        
        # Get theta angle in degrees
        theta = float(self.getProperty("Theta"))
        if theta==0:
            theta = self._calculate_angle(ws_name)
            
        self._output_message += "   Scattering angle: %g\n" % theta
            
        output_2D_ws = '2D_'+output_ws
        RefRoi(InputWorkspace=ws_name, OutputWorkspace=output_2D_ws,
               NXPixel=RefMReduction.NX_PIXELS, NYPixel=RefMReduction.NY_PIXELS,
               ConvertToQ=True, IntegrateY=True,
               YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
               ScatteringAngle=theta)
        
        # Crop the 2D to get the 1D reflectivity
        GroupDetectors(InputWorkspace=output_2D_ws, OutputWorkspace=output_ws,
                       SpectraList=range(peak_range[0],peak_range[1]+1))

        return output_ws
  
    def _process_normalization(self):
        """
            Process the direct beam normalization run
        """
        normalization_run = self.getProperty("NormalizationRunNumber")
        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("NormPeakPixelRange")

        # Get the integration range in the low-res direction
        low_res_range = [0, RefMReduction.NY_PIXELS-1]
        if self.getProperty("CropLowResNormAxis"):
            low_res_range = self.getProperty("LowResNormAxisPixelRange")
                  
        
        # Subtract background
        bck_range = None
        if self.getProperty("SubtractNormBackground"):
            bck_range = self.getProperty("NormBackgroundPixelRange")
        
        ws_wl_profile = self._process_2D([normalization_run], peak_range, 
                                         low_res_range, bck_range, 
                                         polarization=None)
                
        output_roi = "%s_%d_%d" % (ws_wl_profile, peak_range[0], peak_range[1])

        RefRoi(InputWorkspace=ws_wl_profile, OutputWorkspace=output_roi,
               NXPixel=RefMReduction.NX_PIXELS, NYPixel=RefMReduction.NY_PIXELS,
               ConvertToQ=False, IntegrateY=True, SumPixels=True,
               YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
               XPixelMin=peak_range[0], XPixelMax=peak_range[1],
               NormalizeSum=True)               
        
        return output_roi
        
    def _subtract_bakckground(self, raw_ws, peak_ws, peak_range, bck_range, low_res_range):
        """
            Subtract background around a peak
        """
        # Look for overlaps
        if bck_range[0]<peak_range[0] and bck_range[1]>peak_range[1]:
            bck_ws1 = "%s_%d_%d" % (raw_ws, bck_range[0], peak_range[0]-1)
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws1,
                   NXPixel=RefMReduction.NX_PIXELS, NYPixel=RefMReduction.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=True, SumPixels=True,
                   YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
                   XPixelMin=bck_range[0], XPixelMax=peak_range[0]-1,
                   NormalizeSum=True)
            
            bck_ws2 = "%s_%d_%d" % (raw_ws, peak_range[1]+1, bck_range[1])
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws2,
                   NXPixel=RefMReduction.NX_PIXELS, NYPixel=RefMReduction.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=True, SumPixels=True,
                   YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
                   XPixelMin=peak_range[1]+1, XPixelMax=bck_range[1],
                   NormalizeSum=True)
            
            Plus(RHSWorkspace=bck_ws1, LHSWorkspace=bck_ws2,
                OutputWorkspace=bck_ws1)
            Scale(InputWorkspace=bck_ws1, OutputWorkspace=bck_ws1,
                  Factor=0.5, Operation="Multiply")
            Minus(LHSWorkspace=peak_ws, RHSWorkspace=bck_ws1,
                  OutputWorkspace=peak_ws)
        else:
            if bck_range[1]>peak_range[0] and bck_range[1]<peak_range[1]:
                mtd.sendLogMessage("RefMReduction: background range overlaps with peak")
                bck_range[1]=peak_range[0]-1
                
            if bck_range[0]<peak_range[1] and bck_range[0]>peak_range[0]:
                mtd.sendLogMessage("RefMReduction: background range overlaps with peak")
                bck_range[0]=peak_range[1]+1
            
            bck_ws = "%s_%d_%d" % (raw_ws, bck_range[0], bck_range[1])
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws,
                   NXPixel=RefMReduction.NX_PIXELS, NYPixel=RefMReduction.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=True, SumPixels=True,
                   YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
                   XPixelMin=bck_range[0], XPixelMax=bck_range[1],
                   NormalizeSum=True)

            Minus(LHSWorkspace=peak_ws, RHSWorkspace=bck_ws,
                  OutputWorkspace=peak_ws)   
                               
    def _process_2D(self, run_numbers, peak_range, low_res_range, 
                    bck_range=None, polarization=None):
        """
            Load the signal data
            
            @param polarization: Off_Off, Off_On, On_Off, or On_On
        """
        allow_multiple = False
        if len(run_numbers)>1 and not allow_multiple:
            raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")
        
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_M%d" %run_numbers[0])
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefMReduction: could not find run %d\n" % run_numbers[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Pick a good workspace name
        ws_name = "__refm_%d_%s" % (run_numbers[0], polarization)
        ws_name_raw = "refm_%d_%s_raw" % (run_numbers[0], polarization)
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_name_raw):
            if polarization is not None:
                LoadEventNexus(Filename=data_file, NXentryName="entry-%s" % polarization, OutputWorkspace=ws_name_raw)
                # Check whether we have events
                if mtd[ws_name_raw].getNumberEvents()==0:
                    mtd.sendLogMessage("RefMReduction: no data in %s" % polarization)
                    self._output_message += "   No data for this polarization state\n"
                    mtd.deleteWorkspace(ws_name_raw)
                    mtd.sendLogMessage("RefMReduction: deleted empty workspace")
                    return None
            else:
                LoadEventNexus(Filename=data_file, OutputWorkspace=ws_name_raw)
        
        # Move the detector to its proper place
        sdd = 2.562
        det_distance = mtd[ws_name_raw].getInstrument().getDetector(0).getPos().getZ()
        if mtd[ws_name_raw].getRun().hasProperty("SampleDetDis"):
            sdd = mtd[ws_name_raw].getRun().getProperty("SampleDetDis").value[0]/1000.0
        MoveInstrumentComponent(Workspace=ws_name_raw,
                                ComponentName="detector1",
                                Z=sdd-det_distance, RelativePosition=True)        
        
        # Rebin and crop out both sides of the TOF distribution
        if self.TOFrange[0] == self.TOFrange[1]:
            self.TOFrange[0] = min(mtd[ws_name_raw].readX(0))
            self.TOFrange[1] = max(mtd[ws_name_raw].readX(0))
        Rebin(InputWorkspace=ws_name_raw, OutputWorkspace=ws_name, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]], PreserveEvents=True)
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_name, OutputWorkspace=ws_name)
        
        # Convert to wavelength and rebin to ensure we have common bins for all pixels
        ConvertUnits(InputWorkspace=ws_name, Target="Wavelength", OutputWorkspace=ws_name)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        nbins = self.getProperty("NBins")
        
        wave_x = mtd[ws_name].readX(0)
        wl_min = min(wave_x)
        wl_max = max(wave_x)
        if nbins>0:
            wl_step = (wl_max-wl_min)/nbins
        
        Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=[wl_min, wl_step, wl_max], PreserveEvents=True)
        self._output_message += "   Rebinned in wavelength from %g to %g in steps of %g\n" % (wl_min, wl_max, wl_step)

        # Subtract background
        if bck_range is not None:
            ConvertToMatrixWorkspace(InputWorkspace=ws_name,
                                     OutputWorkspace=ws_name)
            self._subtract_bakckground(ws_name, ws_name, peak_range, bck_range, low_res_range)
            self._output_message += "   Background: "
            self._output_message += " x=(%g,%g)\n" % (bck_range[0], bck_range[1])
        
        return ws_name
                               
mtd.registerPyAlgorithm(RefMReduction())
