from MantidFramework import *
from mantidsimple import *
import math
import os
import numpy

class ReflectivityReduction(PythonAlgorithm):

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
        return "ReflectivityReduction"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty("DataRun", "")
        self.declareListProperty("SignalPeakPixelRange", [216, 226], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractSignalBackground", False)
        self.declareListProperty("SignalBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("CropLowResDataAxis", False)
        self.declareListProperty("LowResDataAxisPixelRange", [0, 255], Validator=ArrayBoundedValidator(Lower=0))

        self.declareProperty("PerformNormalization", True, Description="If true, the signal will be normalized")
        self.declareProperty("NormalizationRun", "", Description="Run number for the direct beam normalization run")
        self.declareListProperty("NormPeakPixelRange", [90, 160], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", False)
        self.declareListProperty("NormBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("CropLowResNormAxis", False)
        self.declareListProperty("LowResNormAxisPixelRange", [0, 255], Validator=ArrayBoundedValidator(Lower=0))
        
        self.declareListProperty("TOFRange", [0., 0.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("Theta", 0.0, Description="Scattering angle (degrees)")
        self.declareProperty("ReflectivityPixel", 0.0, Description="Reflectivity pixel of the specular peak (REFPIX)")
        self.declareProperty("NBins", 40, Validator=BoundedValidator(Lower=1))
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

        self.declareProperty("PolarizedData", True)
        self.declareProperty("AngleOffset", 0.0)
        instruments = ["REF_M", "REF_L"]
        self.declareProperty("Instrument", "REF_M",
                             Validator=ListValidator(instruments))
        
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
        
        if self.getProperty("PolarizedData"):
            output_ws = self._process_polarization(ReflectivityReduction.OFF_OFF)
            self.setProperty("OutputWorkspace", mtd[output_ws])
            
            self._process_polarization(ReflectivityReduction.ON_OFF)
            self._process_polarization(ReflectivityReduction.OFF_ON)
            self._process_polarization(ReflectivityReduction.ON_ON)
        else:
            output_ws = self._reduce_data(None, output_ws_name)
            self.setProperty("OutputWorkspace", mtd[output_ws])

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
            + ((direct_beam_pix-ref_pix)*self.PIXEL_SIZE)/ (2.0*det_distance)
        
        return theta*180.0/math.pi
        
    def _calculate_angle_REFL(self, workspace):
        """
            Returns the scattering angle computed for REFL in degrees
        """
        ths = mtd[workspace].getRun().getProperty('ths').value[0]
        tthd = mtd[workspace].getRun().getProperty('tthd').value[0]
        offset = self.getProperty("AngleOffset")
        return tthd - ths + offset

    def _process_polarization(self, polarization):
        """
            Process a polarization state
            
            @param polarization: polarization state
        """
        # Sanity check
        if polarization not in [ReflectivityReduction.OFF_OFF, ReflectivityReduction.OFF_ON,
                                ReflectivityReduction.ON_OFF, ReflectivityReduction.ON_ON]:
            raise RuntimeError("ReflectivityReduction: invalid polarization %s" % polarization)
        
        # Convert to Q
        output_ws = self.getPropertyValue("OutputWorkspace")    
        
        # Rename for polarization
        if polarization != ReflectivityReduction.OFF_OFF:
            if output_ws.find(ReflectivityReduction.OFF_OFF)>0:
                output_ws = output_ws.replace(ReflectivityReduction.OFF_OFF, polarization)
            else:
                output_ws += '_%s' % polarization

        # Load the data with the chosen polarization
        return self._reduce_data(polarization, output_ws)

    def _reduce_data(self, polarization, output_ws):
        """
            Load the signal data
            
            @param polarization: Off_Off, Off_On, On_Off, or On_On
        """
        run_numbers = self.getProperty("DataRun")
        mtd.sendLogMessage("ReflectivityReduction: processing %s [%s]" % (run_numbers, polarization))
        self._output_message += "Processing %s [%s]\n" % (run_numbers, polarization)
        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("SignalPeakPixelRange")

        # Get the integration range in the low-res direction
        low_res_range = None
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
            CloneWorkspace(ws_wl_profile, ws_wl_profile+'_nomode')
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
        mtd.sendLogMessage("corrected angle: %g" % (theta))        
            
        output_2D_ws = '2D_'+output_ws
        if self.getProperty("Instrument")=="REF_M":
            if theta==0:
                theta = self._calculate_angle(ws_name)
            if low_res_range is None:
                low_res_range = [0, self.NY_PIXELS-1]
            RefRoi(InputWorkspace=ws_name, OutputWorkspace=output_2D_ws,
                   NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
                   ConvertToQ=True, IntegrateY=True,
                   YPixelMin=low_res_range[0], YPixelMax=low_res_range[1],
                   ScatteringAngle=theta)
        else:
            if theta==0:
                theta = self._calculate_angle_REFL(ws_name)
            if low_res_range is None:
                low_res_range = [0, self.NX_PIXELS-1]            
            RefRoi(InputWorkspace=ws_name, OutputWorkspace=output_2D_ws,
                   NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
                   ConvertToQ=True, IntegrateY=False,
                   XPixelMin=low_res_range[0], XPixelMax=low_res_range[1],
                   ScatteringAngle=theta)

        self._output_message += "   Scattering angle: %g\n" % theta    
        
        # Crop the 2D to get the 1D reflectivity
        GroupDetectors(InputWorkspace=output_2D_ws, OutputWorkspace=output_ws,
                       SpectraList=range(peak_range[0],peak_range[1]+1))

        return output_ws
  
    def _process_normalization(self):
        """
            Process the direct beam normalization run
        """
        normalization_run = self.getProperty("NormalizationRun")
        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("NormPeakPixelRange")

        # Get the integration range in the low-res direction
        low_res_range = None
        if self.getProperty("CropLowResNormAxis"):
            low_res_range = self.getProperty("LowResNormAxisPixelRange")
        
        # Subtract background
        bck_range = None
        if self.getProperty("SubtractNormBackground"):
            bck_range = self.getProperty("NormBackgroundPixelRange")
        
        ws_wl_profile = self._process_2D(normalization_run, peak_range, 
                                         low_res_range, bck_range, 
                                         polarization=None)
                
        output_roi = "%s_%d_%d" % (ws_wl_profile, peak_range[0], peak_range[1])

        integrate_y = self.getProperty("Instrument")=="REF_M"
        if integrate_y:
            if low_res_range is None:
                low_res_range = [0, self.NY_PIXELS-1]
            xmin = peak_range[0]
            xmax = peak_range[1]
            ymin = low_res_range[0]
            ymax = low_res_range[1]
        else:
            if low_res_range is None:
                low_res_range = [0, self.NX_PIXELS-1]
            ymin = peak_range[0]
            ymax = peak_range[1]
            xmin = low_res_range[0]
            xmax = low_res_range[1]
            
        RefRoi(InputWorkspace=ws_wl_profile, OutputWorkspace=output_roi,
               NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
               ConvertToQ=False, IntegrateY=integrate_y, SumPixels=True,
               YPixelMin=ymin, YPixelMax=ymax,
               XPixelMin=xmin, XPixelMax=xmax,
               NormalizeSum=True)    
        
        return output_roi
        
    def _subtract_bakckground(self, raw_ws, peak_ws, peak_range, bck_range, low_res_range):
        """
            Subtract background around a peak
        """
        
        integrate_y = self.getProperty("Instrument")=="REF_M"
        if integrate_y:
            if low_res_range is None:
                low_res_range = [0, self.NY_PIXELS-1]
            ymin = low_res_range[0]
            ymax = low_res_range[1]
        else:
            if low_res_range is None:
                low_res_range = [0, self.NX_PIXELS-1]
            xmin = low_res_range[0]
            xmax = low_res_range[1]
        
        # Look for overlaps
        if bck_range[0]<peak_range[0] and bck_range[1]>peak_range[1]:
            bck_ws1 = "%s_%d_%d" % (raw_ws, bck_range[0], peak_range[0]-1)
            if integrate_y:
                xmin = bck_range[0]
                xmax = peak_range[0]-1
            else:
                ymin = bck_range[0]
                ymax = peak_range[0]-1
            
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws1,
                   NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=integrate_y, SumPixels=True,
                   YPixelMin=ymin, YPixelMax=ymax,
                   XPixelMin=xmin, XPixelMax=xmax,
                   NormalizeSum=True)
            
            bck_ws2 = "%s_%d_%d" % (raw_ws, peak_range[1]+1, bck_range[1])
            if integrate_y:
                xmin = peak_range[1]+1
                xmax = bck_range[1]
            else:
                ymin = peak_range[1]+1
                ymax = bck_range[1]
            
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws2,
                   NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=integrate_y, SumPixels=True,
                   YPixelMin=ymin, YPixelMax=ymax,
                   XPixelMin=xmin, XPixelMax=xmax,
                   NormalizeSum=True)
            
            Plus(RHSWorkspace=bck_ws1, LHSWorkspace=bck_ws2,
                OutputWorkspace=bck_ws1)
            Scale(InputWorkspace=bck_ws1, OutputWorkspace=bck_ws1,
                  Factor=0.5, Operation="Multiply")
            Minus(LHSWorkspace=peak_ws, RHSWorkspace=bck_ws1,
                  OutputWorkspace=peak_ws)
        else:
            if bck_range[1]>peak_range[0] and bck_range[1]<peak_range[1]:
                mtd.sendLogMessage("ReflectivityReduction: background range overlaps with peak")
                bck_range[1]=peak_range[0]-1
                
            if bck_range[0]<peak_range[1] and bck_range[0]>peak_range[0]:
                mtd.sendLogMessage("ReflectivityReduction: background range overlaps with peak")
                bck_range[0]=peak_range[1]+1
            
            bck_ws = "%s_%d_%d" % (raw_ws, bck_range[0], bck_range[1])
            if integrate_y:
                xmin = bck_range[0]
                xmax = bck_range[1]
            else:
                ymin = bck_range[0]
                ymax = bck_range[1]
            
            RefRoi(InputWorkspace=raw_ws, OutputWorkspace=bck_ws,
                   NXPixel=self.NX_PIXELS, NYPixel=self.NY_PIXELS,
                   ConvertToQ=False, IntegrateY=integrate_y, SumPixels=True,
                   YPixelMin=ymin, YPixelMax=ymax,
                   XPixelMin=xmin, XPixelMax=xmax,
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
        
        # Check for workspace
        if mtd.workspaceExists(run_numbers):
           ws_name = "__"+run_numbers
           ws_name_raw = run_numbers
        else: 
            # Find full path to event NeXus data file
            instrument = self.getProperty("Instrument")
            f = FileFinder.findRuns("%s%s" % (instrument, run_numbers))
            if len(f)==0:
                f = FileFinder.findRuns(run_numbers)
            if len(f)>0 and os.path.isfile(f[0]):
                if len(f)>1 and not allow_multiple:
                    raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")
    
                data_file = f[0]
            else:
                msg = "ReflectivityReduction: could not find run %d\n" % run_numbers
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)
            
            # Pick a good workspace name
            # 1- Get rid of the extension
            base_name = os.path.basename(run_numbers)
            root_name,_ = os.path.splitext(base_name)
            if len(root_name)==0:
                root_name = base_name
            # Add the polarization state only if needed
            pol_state = "_%s" % polarization
            if polarization is None:
                pol_state = ''
            # Add the instrument name only if it's not already there
            instrument_str = instrument.lower()+'_'
            if base_name.lower().find(instrument.lower())>=0:
                instrument_str = ''
                
            ws_name = "__%s%s%s" % (instrument_str, root_name, pol_state)
            ws_name_raw = "%s%s%s_raw" % (instrument_str, root_name, pol_state)
        
            # Load the data into its workspace
            if not mtd.workspaceExists(ws_name_raw):
                if polarization is not None:
                    LoadEventNexus(Filename=data_file, NXentryName="entry-%s" % polarization, OutputWorkspace=ws_name_raw)
                    # Check whether we have events
                    if mtd[ws_name_raw].getNumberEvents()==0:
                        mtd.sendLogMessage("ReflectivityReduction: no data in %s" % polarization)
                        self._output_message += "   No data for this polarization state\n"
                        mtd.deleteWorkspace(ws_name_raw)
                        mtd.sendLogMessage("ReflectivityReduction: deleted empty workspace")
                        return None
                else:
                    LoadEventNexus(Filename=data_file, OutputWorkspace=ws_name_raw)
        
        # Move the detector to its proper place
        if self.getProperty("Instrument")=="REF_M":
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
        nbins = self.getProperty("NBins")
        
        wave_x = mtd[ws_name].readX(0)
        wl_min = min(wave_x)
        wl_max = max(wave_x)
        wl_step = (wl_max-wl_min)/nbins        
        Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=[wl_min, wl_step, wl_max], PreserveEvents=True)

        # Subtract background
        if bck_range is not None:
            ConvertToMatrixWorkspace(InputWorkspace=ws_name,
                                     OutputWorkspace=ws_name)
            self._subtract_bakckground(ws_name, ws_name, peak_range, bck_range, low_res_range)
            self._output_message += "   Background: "
            self._output_message += " x=(%g,%g)\n" % (bck_range[0], bck_range[1])
        
        return ws_name
                               
#mtd.registerPyAlgorithm(ReflectivityReduction())
