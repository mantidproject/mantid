from MantidFramework import *
from mantidsimple import *
import math
import os
import numpy

class RefMReduction(PythonAlgorithm):

    def category(self):
        return "Reflectometry"

    def name(self):
        return "RefMReduction"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareListProperty("RunNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("SignalPeakPixelRange", [216, 226], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractSignalBackground", True)
        self.declareListProperty("SignalBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))

        self.declareProperty("PerformNormalization", True, Description="If true, the signal will be normalized")
        self.declareProperty("NormalizationRunNumber", 0, Description="")
        self.declareListProperty("NormPeakPixelRange", [90, 160], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("SubtractNormBackground", True)
        self.declareListProperty("NormBackgroundPixelRange", [80, 170], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("LowResDataAxisPixelRange", [100, 165], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("LowResNormAxisPixelRange", [100, 165], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("TOFRange", [10700., 24500.], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("QMin", 0.0025, Description="Minimum Q-value")
        self.declareProperty("QStep", -0.01, Description="Step-size in Q. Enter a negative value to get a log scale.")
        self.declareProperty("Theta", 0.0, Description="Scattering angle (degrees)")
        self.declareProperty("WavelengthMin", 2.5)
        self.declareProperty("WavelengthMax", 6.5)
        self.declareProperty("WavelengthStep", 0.1)
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        run_numbers = self.getProperty("RunNumbers")
        mtd.sendLogMessage("RefMReduction: processing %s" % run_numbers)
        allow_multiple = False
        if len(run_numbers)>1 and not allow_multiple:
            raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")
    
        normalization_run = self.getProperty("NormalizationRunNumber")
    
        data_peak = self.getProperty("SignalPeakPixelRange")
        data_back = self.getProperty("SignalBackgroundPixelRange")

        # TOF range to consider
        self.TOFrange = self.getProperty("TOFRange") #microS
        # Steps for TOF rebin
        self.TOFsteps = 25.0

        # Q binning for output distribution
        q_min = self.getProperty("QMin")
        q_step = self.getProperty("QStep")
                
        low_res_range = self.getProperty("LowResDataAxisPixelRange")
        
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        pixel_size = 0.0007 # m
        
        #dimension of the detector (256 by 304 pixels)
        nx_pixels = 304
        ny_pixels = 256
        
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
        f = FileFinder.findRuns("REF_M%d" %run_numbers[0])
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefMReduction: could not find run %d\n" % run_numbers[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Pick a good workspace name
        ws_name_raw = "refm_%d" % run_numbers[0]
        
        # Load the data into its workspace
        if not mtd.workspaceExists(ws_name_raw):
            LoadEventNexus(Filename=data_file, NXentryName="entry-Off_Off", OutputWorkspace=ws_name_raw)
        
        # Rebin and crop out both sides of the TOF distribution
        ws_name = ws_name_raw+'_cropped'
        Rebin(InputWorkspace=ws_name_raw, OutputWorkspace=ws_name, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]], PreserveEvents=False)
        #CropWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_cropped, XMin=self.TOFrange[0], XMax=self.TOFrange[1])        
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_name, OutputWorkspace=ws_name)
        
        # Convert to wavelength
        ConvertUnits(InputWorkspace=ws_name, Target="Wavelength", OutputWorkspace=ws_name)
        wl_min = self.getProperty("WavelengthMin")
        wl_max = self.getProperty("WavelengthMax")
        wl_step = self.getProperty("WavelengthStep")
        Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=[wl_min, wl_step, wl_max])
        
        # Extract geometry
        sample_z = mtd[ws_name].getInstrument().getSample().getPos().getZ()
        source_z = mtd[ws_name].getInstrument().getSource().getPos().getZ()
        detector_z = mtd[ws_name].getInstrument().getDetector(0).getPos().getZ()
        
        # We haven't moved the detector, so it's aligned along the beam
        source_to_detector = detector_z - source_z
        
        # Get theta angle in degrees
        #run = mtd[ws_name].getRun()
        #theta = run.getProperty('SANGLE').value[0]
        theta = float(self.getProperty("Theta"))
        theta_radian = theta * math.pi / 180.

        # ID = NY*x + y
        xmin = (data_peak[0]-nx_pixels/2.0)*pixel_size
        xmax = (data_peak[1]-nx_pixels/2.0)*pixel_size
        ymin = (low_res_range[0]-ny_pixels/2.0)*pixel_size
        ymax = (low_res_range[1]-ny_pixels/2.0)*pixel_size
        
        xml_shape =  "<cuboid id=\"shape\">\n"
        xml_shape += "  <left-front-bottom-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmin,ymin)
        xml_shape += "  <left-front-top-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmin, ymax)
        xml_shape += "  <left-back-bottom-point x=\"%g\" y=\"%g\" z=\"5.0\"  />\n" % (xmin, ymin)
        xml_shape += "  <right-front-bottom-point x=\"%g\" y=\"%g\" z=\"-5.0\"  />\n" % (xmax, ymin)
        xml_shape += "</cuboid>\n<algebra val=\"shape\" />\n"

        alg = FindDetectorsInShape(Workspace=ws_name, ShapeXML=xml_shape)
        det_list = alg.getPropertyValue("DetectorList")

        output_ws = self.getPropertyValue("OutputWorkspace")        
        
        if mtd.workspaceExists(output_ws):
            mtd.deleteWorkspace(output_ws)
            
        output_unsorted = '__'+output_ws+'_unsorted'
        GroupDetectors(InputWorkspace=ws_name, DetectorList=det_list, OutputWorkspace=output_unsorted)
        #SumSpectra(InputWorkspace=ws_name, OutputWorkspace=output_unsorted)
        
        # Perform normalization according to wavelength distribution of
        # the direct beam
        if self.getProperty("PerformNormalization"):
            ws_wl_profile = self._process_normalization()
            RebinToWorkspace(WorkspaceToRebin=ws_wl_profile, 
                             WorkspaceToMatch=output_unsorted,
                             OutputWorkspace=ws_wl_profile)
            Divide(LHSWorkspace=output_unsorted,
                   RHSWorkspace=ws_wl_profile,
                   OutputWorkspace=output_unsorted)
            ReplaceSpecialValues(InputWorkspace=output_unsorted,
                                 OutputWorkspace=output_unsorted,
                                 NaNValue=0.0, NaNError=0.0,
                                 InfinityValue=0.0, InfinityError=0.0)
            
        # Convert to Q
        x = mtd[output_unsorted].readX(0)
        y = mtd[output_unsorted].readY(0)
        e = mtd[output_unsorted].readE(0)

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
                        ParentWorkspace=ws_name)
        mtd[output_ws].setDistribution(True)
        Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws,
              Params=[q_min, q_step, max(q)])
        
        self.setProperty("OutputWorkspace", mtd[output_ws])
        
        mtd.deleteWorkspace(output_unsorted)
        mtd.deleteWorkspace(ws_name)

    def _process_normalization(self):
        """
            Process the direct beam normalization run
        """
        normalization_run = self.getProperty("NormalizationRunNumber")
        ws_normalization = "normalization_%d" % normalization_run
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
            Rebin(InputWorkspace=ws_normalization, OutputWorkspace=ws_normalization, Params=[self.TOFrange[0], self.TOFsteps, self.TOFrange[1]])
            
            # Normalized by Current (proton charge)
            NormaliseByCurrent(InputWorkspace=ws_normalization, OutputWorkspace=ws_normalization)

            # Convert to wavelength
            ConvertUnits(InputWorkspace=ws_normalization, Target="Wavelength", OutputWorkspace=ws_normalization)
        
            
        SumSpectra(InputWorkspace=ws_normalization, OutputWorkspace=ws_wl_profile)
        return ws_wl_profile
    
mtd.registerPyAlgorithm(RefMReduction())
