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

        self.declareProperty("PerformNormalization", True, Description="If true, the signal will be normalized")
        self.declareProperty("NormalizationRunNumber", 0, Description="Run number for the direct beam normalization run")
        self.declareListProperty("NormPeakPixelRange", [90, 160], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", False)
        self.declareListProperty("NormBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        
        self.declareListProperty("LowResDataAxisPixelRange", [100, 165], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("LowResNormAxisPixelRange", [100, 165], Validator=ArrayBoundedValidator(Lower=0))
        
        self.declareListProperty("TOFRange", [10700., 24500.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("QMin", 0.0025, Description="Minimum Q-value")
        self.declareProperty("QStep", -0.01, Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("Theta", 0.0, Description="Scattering angle (degrees)")
        self.declareProperty("CenterPixel", 0.0, Description="Center pixel of the specular peak")
        self.declareProperty("WavelengthMin", 2.5)
        self.declareProperty("WavelengthMax", 6.5)
        self.declareProperty("WavelengthStep", 0.1)
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        # TOF range and binning
        self.TOFrange = self.getProperty("TOFRange")
        self.TOFsteps = 25.0

        # Process each polarization state, use the OFF_OFF state
        # for the outputworkspace
        output_ws = self._process_polarization(RefMReduction.OFF_OFF)
        self.setProperty("OutputWorkspace", mtd[output_ws])
        
        self._process_polarization(RefMReduction.ON_OFF)


    def _calculate_angle(self, workspace):
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
        
        # Perform normalization according to wavelength distribution of
        # the direct beam
        if self.getProperty("PerformNormalization"):
            ws_wl_profile = self._process_normalization()
            
            RebinToWorkspace(WorkspaceToRebin=ws_wl_profile, 
                             WorkspaceToMatch=output_roi,
                             OutputWorkspace=ws_wl_profile)
            Divide(LHSWorkspace=output_roi,
                   RHSWorkspace=ws_wl_profile,
                   OutputWorkspace=output_roi)
            ReplaceSpecialValues(InputWorkspace=output_roi,
                                 OutputWorkspace=output_roi,
                                 NaNValue=0.0, NaNError=0.0,
                                 InfinityValue=0.0, InfinityError=0.0)
            
            #if mtd.workspaceExists(ws_wl_profile):
            #    mtd.deleteWorkspace(ws_wl_profile)

        # Convert to Q
        output_ws = self.getPropertyValue("OutputWorkspace")    
        
        # Rename for polarization
        if polarization != RefMReduction.OFF_OFF:
            if output_ws.find(RefMReduction.OFF_OFF)>0:
                output_ws = output_ws.replace(RefMReduction.OFF_OFF, polarization)
            else:
                output_ws += '_%s' % polarization
           
        self._convert_to_q(output_roi, output_ws)
        
        #mtd.deleteWorkspace(output_roi)
        
        return output_ws

    def _load_data(self, polarization):
        """
            Load the signal data
            
            @param polarization: Off_Off, Off_On, On_Off, or On_On
        """
        run_numbers = self.getProperty("RunNumbers")
        mtd.sendLogMessage("RefMReduction: processing %s" % run_numbers)
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
        ws_name = "refm_%d_%s" % (run_numbers[0], polarization)
        ws_name_raw = ws_name+'_raw'
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_name_raw):
            LoadEventNexus(Filename=data_file, NXentryName="entry-%s" % polarization, OutputWorkspace=ws_name_raw)
        
        # Check whether we have events
        if mtd[ws_name_raw].getNumberEvents()==0:
            mtd.sendLogMessage("RefMReduction: no data in %s" % polarization)
        
        # Rebin and crop out both sides of the TOF distribution
        Rebin(InputWorkspace=ws_name_raw, OutputWorkspace=ws_name, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]], PreserveEvents=True)
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_name, OutputWorkspace=ws_name)
        
        # Convert to wavelength and rebin to ensure we have common bins for all pixels
        ConvertUnits(InputWorkspace=ws_name, Target="Wavelength", OutputWorkspace=ws_name)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=[wl_min, wl_step, wl_max], PreserveEvents=False)

        # Get the integration range in the low-res direction
        low_res_range = self.getProperty("LowResDataAxisPixelRange")
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("SignalPeakPixelRange")
        output_roi = self._crop_roi(ws_name, peak_range, low_res_range)

        # Subtract background
        if self.getProperty("SubtractSignalBackground"):
            bck_range = self.getProperty("SignalBackgroundPixelRange")
            bck_ws = self._crop_roi(ws_name, bck_range, low_res_range)
            scaling_factor = math.fabs(peak_range[1]-peak_range[0])/\
                            math.fabs(bck_range[1]-bck_range[0])
            Scale(InputWorkspace=bck_ws, OutputWorkspace=bck_ws,
                  Factor=scaling_factor, Operation="Multiply")
            Minus(LHSWorkspace=output_roi, RHSWorkspace=bck_ws,
                  OutputWorkspace=output_roi)
        
        return output_roi
 
    def _crop_roi(self, input_ws, peak_range, low_res_range):
        """
            Crop the region of interest and produce the sum of counts as
            a function of wavelength
        """
        # Extract the pixels that we count as signal
        # Detector ID = NY*x + y
        pixel_size = RefMReduction.PIXEL_SIZE # m
        nx_pixels = 304
        ny_pixels = 256
        xmin = (peak_range[0]-nx_pixels/2.0)*pixel_size
        xmax = (peak_range[1]-nx_pixels/2.0)*pixel_size
        ymin = (low_res_range[0]-ny_pixels/2.0)*pixel_size
        ymax = (low_res_range[1]-ny_pixels/2.0)*pixel_size
        
        xml_shape =  "<cuboid id=\"shape\">\n"
        xml_shape += "  <left-front-bottom-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmin,ymin)
        xml_shape += "  <left-front-top-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmin, ymax)
        xml_shape += "  <left-back-bottom-point x=\"%g\" y=\"%g\" z=\"5.0\"  />\n" % (xmin, ymin)
        xml_shape += "  <right-front-bottom-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmax, ymin)
        xml_shape += "</cuboid>\n<algebra val=\"shape\" />\n"

        alg = FindDetectorsInShape(Workspace=input_ws, ShapeXML=xml_shape)
        det_list = alg.getPropertyValue("DetectorList")

        output_roi = "__%s_%d_%d" % (input_ws, peak_range[0], peak_range[1])
        GroupDetectors(InputWorkspace=input_ws, DetectorList=det_list, OutputWorkspace=output_roi)
        return output_roi
 
    def _process_normalization(self):
        """
            Process the direct beam normalization run
        """
        normalization_run = self.getProperty("NormalizationRunNumber")
        ws_normalization = "normalization_%d_raw" % normalization_run
        ws_wl_profile = "tof_profile_%d" % normalization_run
        
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_M%d" % normalization_run)
        
        # FindRuns() is finding event workspaces, but we need histo
        if len(f)>0:
            f[0] = f[0].replace("event", "histo")
            
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefMReduction: could not find run %d\n" % normalization_run
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_normalization):
            LoadTOFRawNexus(Filename=data_file, OutputWorkspace=ws_normalization)
            
        # Trim TOF
        Rebin(InputWorkspace=ws_normalization, OutputWorkspace=ws_wl_profile, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]])
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile)

        # Convert to wavelength
        ConvertUnits(InputWorkspace=ws_wl_profile, Target="Wavelength", OutputWorkspace=ws_wl_profile)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        Rebin(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile, Params=[wl_min, wl_step, wl_max], PreserveEvents=True)
        
        # Get the integration range in the low-res direction
        low_res_range = self.getProperty("LowResNormAxisPixelRange")        
        # Sum the normalization counts as a function of wavelength in ROI
        peak_range = self.getProperty("NormPeakPixelRange")

        if True:
            SumSpectra(InputWorkspace=ws_wl_profile, OutputWorkspace=ws_wl_profile+'_roi')
            ws_wl_profile = ws_wl_profile+'_roi'
        else:
            ws_wl_profile = self._crop_roi(ws_wl_profile, peak_range, low_res_range)
            # Subtract background
            if self.getProperty("SubtractNormBackground"):
                bck_range = self.getProperty("NormBackgroundPixelRange")
                bck_ws = self._crop_roi(ws_wl_profile, bck_range, low_res_range)
                Minus(LHSWorkspace=ws_wl_profile, RHSWorkspace=bck_ws,
                      OutputWorkspace=ws_wl_profile)       
        
        return ws_wl_profile
        
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
    
mtd.registerPyAlgorithm(RefMReduction())
