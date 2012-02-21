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
        self.declareProperty("QMin", 0.0025, Description="Minimum Q-value")
        self.declareProperty("QStep", -0.01, Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("Theta", 0.0, Description="Scattering angle (degrees)")
        self.declareProperty("CenterPixel", 0.0, Description="Center pixel of the specular peak")
        self.declareProperty("WavelengthMin", 2.5)
        self.declareProperty("WavelengthMax", 6.5)
        self.declareProperty("WavelengthStep", 0.1)
        self.declareProperty("RemoveIntermediateWorkspaces", True)
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

        # Clean up
        if self.getProperty("RemoveIntermediateWorkspaces"):
            for ws in mtd.keys():
                if ws.startswith('__'):
                    mtd.deleteWorkspace(ws)
                    
        self.setProperty("OutputMessage", self._output_message)

    def _calculate_angle(self, workspace):
        """
            Compute the scattering angle
        """
        sangle = 0
        if mtd[workspace].getRun().hasProperty("SANGLE"):
            sangle = mtd[workspace].getRun().getProperty("SANGLE").value[0]
            
        dangle = 0
        if mtd[workspace].getRun().hasProperty("DANGLE"):
            dangle = mtd[workspace].getRun().getProperty("DANGLE").value[0]
            
        dangle0 = 0
        if mtd[workspace].getRun().hasProperty("DANGLE0"):
            dangle0 = mtd[workspace].getRun().getProperty("DANGLE0").value[0]
            
        det_distance = mtd[workspace].getInstrument().getDetector(0).getPos().getZ()

        direct_beam_pix = 0
        if mtd[workspace].getRun().hasProperty("DIRPIX"):
            direct_beam_pix = mtd[workspace].getRun().getProperty("DIRPIX").value[0]
        
        center_pix = int(self.getProperty("CenterPixel"))
        
        delta = (dangle-dangle0)/2.0\
            + ((direct_beam_pix-center_pix)*RefMReduction.PIXEL_SIZE)/ (2.0*det_distance)
        
        return sangle-delta
        
    def _process_polarization(self, polarization):
        """
            Process a polarization state
            
            @param polarization: polarization state
        """
        # Sanity check
        if polarization not in [RefMReduction.OFF_OFF, RefMReduction.OFF_ON,
                                RefMReduction.ON_OFF, RefMReduction.ON_ON]:
            raise RuntimeError("RefMReduction: invalid polarization %s" % polarization)
        
        # Load the data with the chosen polarization
        output_roi = self._load_data(polarization)
        
        # Return immediately if we don't have any events
        if output_roi is None:
            return None
        
        # Convert to Q
        output_ws = self.getPropertyValue("OutputWorkspace")    
        
        # Rename for polarization
        if polarization != RefMReduction.OFF_OFF:
            if output_ws.find(RefMReduction.OFF_OFF)>0:
                output_ws = output_ws.replace(RefMReduction.OFF_OFF, polarization)
            else:
                output_ws += '_%s' % polarization
           
        self._convert_to_q(output_roi, output_ws)
        
        return output_ws

    def _load_data(self, polarization):
        """
            Load the signal data
            
            @param polarization: Off_Off, Off_On, On_Off, or On_On
        """
        run_numbers = self.getProperty("RunNumbers")
        mtd.sendLogMessage("RefMReduction: processing %s [%s]" % (run_numbers, polarization))
        self._output_message += "Processing %s [%s]\n" % (run_numbers, polarization)
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
            LoadEventNexus(Filename=data_file, NXentryName="entry-%s" % polarization, OutputWorkspace=ws_name_raw)
        
        # Check whether we have events
        if mtd[ws_name_raw].getNumberEvents()==0:
            mtd.sendLogMessage("RefMReduction: no data in %s" % polarization)
            self._output_message += "   No data for this polarization state\n"
            mtd.deleteWorkspace(ws_name_raw)
            return None
        
        # Rebin and crop out both sides of the TOF distribution
        if self.TOFrange[0] == self.TOFrange[1]:
            self.TOFrange[0] = min(mtd[ws_name_raw].readX(0))
            self.TOFrange[1] = max(mtd[ws_name_raw].readX(0))
            
        Rebin(InputWorkspace=ws_name_raw, OutputWorkspace=ws_name, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]], PreserveEvents=True)
        #self._output_message += "   Rebinned from %g to %g\n" % (self.TOFrange[0], self.TOFrange[1])
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_name, OutputWorkspace=ws_name)
        
        # Convert to wavelength and rebin to ensure we have common bins for all pixels
        ConvertUnits(InputWorkspace=ws_name, Target="Wavelength", OutputWorkspace=ws_name)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=[wl_min, wl_step, wl_max], PreserveEvents=True)
        self._output_message += "   Rebinned in wavelength from %g to %g in steps of %g\n" % (wl_min, wl_max, wl_step)

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

        # Get the integration range in the low-res direction
        low_res_range = [0, RefMReduction.NY_PIXELS-1]
        if self.getProperty("CropLowResDataAxis"):
            low_res_range = self.getProperty("LowResDataAxisPixelRange")
            
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("SignalPeakPixelRange")
        output_roi, npix = self._crop_roi(ws_name, peak_range, low_res_range)
        self._output_message += "   ROI x=(%g,%g) y=(%g,%g)\n" % (peak_range[0],peak_range[1],
                                                                low_res_range[0], low_res_range[1])

        # Subtract background
        if self.getProperty("SubtractSignalBackground"):
            bck_range = self.getProperty("SignalBackgroundPixelRange")
            self._subtract_bakcground(ws_name, output_roi, peak_range, npix, bck_range, low_res_range)
            self._output_message += "   Background: "
            self._output_message += " x=(%g,%g)\n" % (bck_range[0], bck_range[1])
        
        return output_roi
 
    def _crop_roi(self, input_ws, peak_range, low_res_range):
        """
            Crop the region of interest and produce the sum of counts as
            a function of wavelength
        """
        # Extract the pixels that we count as signal
        # Detector ID = NY*x + y
        output_roi = "%s_%d_%d" % (input_ws, peak_range[0], peak_range[1])

        det_list = []
        for ix in range(peak_range[0], peak_range[1]+1):
            det_list.extend(range(RefMReduction.NY_PIXELS*ix+low_res_range[0],
                                  RefMReduction.NY_PIXELS*ix+low_res_range[1]+1))
        
        y = numpy.zeros(len(mtd[input_ws].readY(0)))
        e = numpy.zeros(len(y))
        for i in det_list:
            y_pix = mtd[input_ws].readY(i)
            e_pix = mtd[input_ws].readE(i)
            for itof in range(len(y)):
                y[itof] += y_pix[itof]
                e[itof] += e_pix[itof]*e_pix[itof]
                
        x = mtd[input_ws].readX(0)
        e = numpy.sqrt(e)
        CreateWorkspace(OutputWorkspace=output_roi, DataX=x, DataY=y, DataE=e,
                        UnitX="Wavelength", ParentWorkspace=input_ws)
        
        return output_roi, len(det_list)
 
    def _process_normalization(self):
        """
            Process the direct beam normalization run
        """
        normalization_run = self.getProperty("NormalizationRunNumber")
        self._output_message += "      Processing %d\n" % normalization_run
        ws_normalization = "normalization_%d_raw" % normalization_run
        ws_wl_profile = "__normalization_%d" % normalization_run
        
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_M%d" % normalization_run)
        
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefMReduction: could not find run %d\n" % normalization_run
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_normalization):
            LoadEventNexus(Filename=data_file, OutputWorkspace=ws_normalization)
            
        # Trim TOF
        Rebin(InputWorkspace=ws_normalization, OutputWorkspace=ws_wl_profile, 
              Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]],
              PreserveEvents=True)
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile)

        # Convert to wavelength
        ConvertUnits(InputWorkspace=ws_wl_profile, Target="Wavelength", OutputWorkspace=ws_wl_profile)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        Rebin(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile, Params=[wl_min, wl_step, wl_max], PreserveEvents=True)
        self._output_message += "         Rebinned in wavelength from %g to %g in steps of %g\n" % (wl_min, wl_max, wl_step)
        
        # Get the integration range in the low-res direction
        low_res_range = [0, RefMReduction.NY_PIXELS-1]
        if self.getProperty("CropLowResNormAxis"):
            low_res_range = self.getProperty("LowResNormAxisPixelRange")        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("NormPeakPixelRange")

        if False:
            ws_wl_profile_roi = ws_wl_profile+'_roi'
            SumSpectra(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile_roi)
        else:
            ws_wl_profile_roi, npix = self._crop_roi(ws_wl_profile, peak_range, low_res_range)
            self._output_message += "         ROI x=(%g,%g) y=(%g,%g)\n" % (peak_range[0],peak_range[1],
                                                                    low_res_range[0], low_res_range[1])
            # Subtract background
            if self.getProperty("SubtractNormBackground"):
                bck_range = self.getProperty("NormBackgroundPixelRange")
                self._subtract_bakcground(ws_wl_profile, ws_wl_profile_roi, peak_range, npix, bck_range, low_res_range)
                self._output_message += "         Background: "
                self._output_message += " x=(%g,%g)\n" % (bck_range[0], bck_range[1])

            Scale(InputWorkspace=ws_wl_profile_roi, OutputWorkspace=ws_wl_profile_roi,
                  Factor=1.0/(peak_range[1]-peak_range[0]), Operation="Multiply")     
        
        return ws_wl_profile_roi
        
    def _subtract_bakcground(self, raw_ws, peak_ws, peak_range, npix_peak, bck_range, low_res_range):
        """
            Subtract background around a peak
        """
        # Look for overlaps
        if bck_range[0]<peak_range[0] and bck_range[1]>peak_range[1]:
            # Background on both sides
            bck_ws1, npix_bck1 = self._crop_roi(raw_ws, [bck_range[0],peak_range[0]-1], low_res_range)
            bck_ws2, npix_bck2 = self._crop_roi(raw_ws, [peak_range[1]+1,bck_range[1]], low_res_range)
            
            scaling_factor = npix_peak/(2.0*npix_bck1)
            Scale(InputWorkspace=bck_ws1, OutputWorkspace=bck_ws1,
                  Factor=scaling_factor, Operation="Multiply")
            scaling_factor = npix_peak/(2.0*npix_bck2)
            Scale(InputWorkspace=bck_ws2, OutputWorkspace=bck_ws2,
                  Factor=scaling_factor, Operation="Multiply")
            Plus(RHSWorkspace=bck_ws1, LHSWorkspace=bck_ws2,
                OutputWorkspace=bck_ws1)
            Minus(LHSWorkspace=peak_ws, RHSWorkspace=bck_ws1,
                  OutputWorkspace=peak_ws)
        else:
            if bck_range[1]>peak_range[0] and bck_range[1]<peak_range[1]:
                mtd.sendLogMessage("RefMReduction: background range overlaps with peak")
                bck_range[1]=peak_range[0]-1
                
            if bck_range[0]<peak_range[1] and bck_range[0]>peak_range[0]:
                mtd.sendLogMessage("RefMReduction: background range overlaps with peak")
                bck_range[0]=peak_range[1]+1
            
            bck_ws, npix_bck = self._crop_roi(raw_ws, bck_range, low_res_range)
            scaling_factor = npix_peak/npix_bck
            Scale(InputWorkspace=bck_ws, OutputWorkspace=bck_ws,
                  Factor=scaling_factor, Operation="Multiply")
            Minus(LHSWorkspace=peak_ws, RHSWorkspace=bck_ws,
                  OutputWorkspace=peak_ws)  
            
        
    def _convert_to_q(self, input_ws, output_ws):
        """
            Convert to Q
            
            @param input_ws: workspace of wavelength data
            @param output_ws: output workspace of q data
        """
        # Get theta angle in degrees
        theta = float(self.getProperty("Theta"))
        if theta>0:
            theta_radian = theta * math.pi / 180.
        else:
            theta_radian = self._calculate_angle(input_ws)

        x = mtd[input_ws].readX(0)
        y = mtd[input_ws].readY(0)
        e = mtd[input_ws].readE(0)

        q = 4.0*math.pi*math.sin(theta_radian) / x

        zipped = zip(q,y,e)
        def cmp(p1,p2):
            if p2[0]==p1[0]:
                return 0
            return -1 if p2[0]>p1[0] else 1
        combined = sorted(zipped, cmp)
        q_,y_,e_ = zip(*combined)
        
        if len(q)>len(q_):
            q_ = list(q_)
            q_.insert(0,q[len(q)-1])

        CreateWorkspace(DataX=q_, DataY=y_, DataE=e_,
                        OutputWorkspace=output_ws,
                        UnitX="MomentumTransfer",
                        YUnitLabel="Reflectivity",
                        ParentWorkspace=input_ws)
        
        # Make sure the system knows we are creating a distribution
        mtd[output_ws].setDistribution(True)
        
        # Rebin in the requested Q binning
        q_min = self.getProperty("QMin")
        q_step = self.getProperty("QStep")
                
        Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws,
              Params=[q_min, q_step, max(q)])
        self._output_message += "   Converted to Q\n"
    
mtd.registerPyAlgorithm(RefMReduction())
