from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import Scale
import math

class EQSANSAzimuthalAverage1D(PythonAlgorithm):

    def category(self):
        return 'Workflow\\SANS\\UsesPropertyManager'

    def name(self):
        return 'EQSANSAzimuthalAverage1D'

    def summary(self):
        return "Compute I(q) for reduced EQSANS data"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction = Direction.Input))
        self.declareProperty('NumberOfBins', 100,
                             validator = IntBoundedValidator(lower = 1),
                             doc = 'Number of Q bins to use if binning is not supplied')
        self.declareProperty('LogBinning', False,
                             doc = "Produce log binning in Q when true and binning wasn't supplied")
        self.declareProperty('IndependentBinning', True,
                             doc = 'If true and frame skipping is used, each frame will have its own binning')
        self.declareProperty('ScaleResults', True,
                             doc = 'If true and frame skipping is used, frame 1 will be scaled to frame 2')
        self.declareProperty('ComputeResolution', True,
                             doc = 'If true the Q resolution will be computed')
        self.declareProperty('SampleApertureDiameter', 10.0,
                             doc = 'Sample aperture diameter [mm]')
        self.declareProperty('ReductionProperties', '__sans_reduction_properties',
                             validator = StringMandatoryValidator(),
                             doc = 'Property manager name for the reduction')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction = Direction.Output),
                             doc = 'I(q) workspace')
        self.declareProperty('OutputMessage', '', direction = Direction.Output,
                             doc = 'Output message')

    def PyExec(self):
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        if not AnalysisDataService.doesExist(input_ws_name):
            Logger('EQSANSSANSAzimuthalAverage').error('Could not find input workspace')

        # Get the source aperture from the run logs
        source_aperture_radius = 10.0
        if workspace.getRun().hasProperty('source-aperture-diameter'):
            source_aperture_radius = workspace.getRun().getProperty('source-aperture-diameter').value / 2.0

        # Perform azimuthal averaging according to whether or not
        # we are in frame-skipping mode
        if workspace.getRun().hasProperty('is_frame_skipping') \
            and workspace.getRun().getProperty('is_frame_skipping').value == 0:
            self._no_frame_skipping(source_aperture_radius)
        else:
            self._with_frame_skipping(source_aperture_radius)

    def _no_frame_skipping(self, source_aperture_radius):
        """
            Perform azimuthal averaging assuming no frame-skipping
            @param source_aperture_radius: source aperture radius [mm]
        """
        log_binning = self.getProperty('LogBinning').value
        nbins = self.getProperty('NumberOfBins').value
        compute_resolution = self.getProperty('ComputeResolution').value
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        output_ws_name = self.getPropertyValue('OutputWorkspace')
        property_manager_name = self.getProperty('ReductionProperties').value
        pixel_size_x = workspace.getInstrument().getNumberParameter('x-pixel-size')[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter('y-pixel-size')[0]
        (output_msg, output_ws, output_binning) = \
            self._call_sans_averaging(workspace, None,
                                      nbins, log_binning,
                                      property_manager_name,
                                      output_ws_name)
        if compute_resolution:
            sample_aperture_radius = self.getProperty('SampleApertureDiameter').value / 2.0
            alg = AlgorithmManager.create("EQSANSResolution")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("ReducedWorkspace", workspace)
            alg.setPropertyValue("OutputBinning", output_binning)
            alg.setProperty("PixelSizeX", pixel_size_x)
            alg.setProperty("PixelSizeY", pixel_size_y)
            alg.setProperty("SourceApertureRadius", source_aperture_radius)
            alg.setProperty("SampleApertureRadius", sample_aperture_radius)
            alg.execute()
            output_msg += "Resolution computed\n"

        if output_msg is not None:
            self.setProperty('OutputMessage', output_msg)

        self.setProperty('OutputWorkspace', output_ws)

    def _call_sans_averaging(self, workspace, binning, nbins, log_binning,
                             property_manager_name, output_workspace):
        """
            Call the generic azimuthal averaging for SANS
            @param workspace: workspace to average
            @param binning: I(Q) binning (optional)
            @param nbins: number of Q bins
            @param log_binning: if True, the output binning will be logarithmic
            @param property_manager_name: name of the property manager object
            @param output_workspace: name of the output workspace
        """
        alg = AlgorithmManager.create('SANSAzimuthalAverage1D')
        alg.initialize()
        alg.setChild(True)
        alg.setProperty('InputWorkspace', workspace)
        if binning is not None:
            alg.setProperty('Binning', binning)

        alg.setProperty('NumberOfBins', nbins)
        alg.setProperty('LogBinning', log_binning)
        alg.setProperty('ComputeResolution', False)
        alg.setProperty('ReductionProperties', property_manager_name)
        alg.setProperty('OutputWorkspace', output_workspace)
        alg.execute()
        if alg.existsProperty('OutputMessage'):
            output_msg = alg.getProperty('OutputMessage').value
        else:
            output_msg = None
        output_ws = alg.getProperty('OutputWorkspace').value

        # Get output binning
        output_binning = alg.getPropertyValue("Binning")
        return (output_msg, output_ws, output_binning)


    def _with_frame_skipping(self, source_aperture_radius):
        """
            Perform azimuthal averaging assuming frame-skipping
            @param source_aperture_radius: source aperture radius [mm]
        """
        independent_binning = self.getProperty('IndependentBinning').value
        scale_results = self.getProperty('ScaleResults').value
        workspace = self.getProperty('InputWorkspace').value
        output_ws_name = self.getPropertyValue('OutputWorkspace')
        ws_frame1 = output_ws_name.replace('_Iq', '_frame1_Iq')
        ws_frame2 = output_ws_name.replace('_Iq', '_frame2_Iq')

        # Get wavelength bands
        # First frame
        wl_min_f1 = None
        wl_max_f1 = None
        if workspace.getRun().hasProperty("wavelength_min"):
            wl_min_f1 = workspace.getRun().getProperty("wavelength_min").value
        if workspace.getRun().hasProperty("wavelength_max"):
            wl_max_f1 = workspace.getRun().getProperty("wavelength_max").value
        if wl_min_f1 is None and wl_max_f1 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 1"

        # Second frame
        wl_min_f2 = None
        wl_max_f2 = None
        if workspace.getRun().hasProperty("wavelength_min_frame2"):
            wl_min_f2 = workspace.getRun().getProperty("wavelength_min_frame2").value
        if workspace.getRun().hasProperty("wavelength_max_frame2"):
            wl_max_f2 = workspace.getRun().getProperty("wavelength_max_frame2").value
        if wl_min_f2 is None and wl_max_f2 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 2"

        # Compute binning
        if independent_binning:
            binning = None
        else:
            (qmin, qstep, qmax) = self._get_binning(workspace,
                                                    min(wl_min_f1, wl_min_f2),
                                                    max(wl_max_f1, wl_max_f2))
            binning = '%g, %g, %g' % (qmin, qstep, qmax)

        # Average second frame
        output_frame2 = self._process_frame(workspace, wl_min_f2, wl_max_f2,
                                            source_aperture_radius, '2', binning)

        # Average first frame
        if independent_binning:
            binning = None

        output_frame1 = self._process_frame(workspace, wl_min_f1, wl_max_f1,
                                            source_aperture_radius, '1', binning)

        if scale_results:
            output_frame1 = self._scale(output_frame1, output_frame2)

        self.setPropertyValue('OutputWorkspace', ws_frame1)
        self.setProperty('OutputWorkspace', output_frame1)

        self.declareProperty(MatrixWorkspaceProperty('OutputFrame2', ws_frame2,
                                                     direction = Direction.Output))
        self.setProperty('OutputFrame2', output_frame2)

        self.setProperty('OutputMessage', 'Performed radial averaging for two frames')

    def _process_frame(self, workspace, wl_min, wl_max, source_aperture_radius,
                       frame_ID='1', binning=None):
        """
            Perform azimuthal averaging for a single frame
            @param workspace: reduced workspace object
            @param wl_min: minimum wavelength
            @param wl_max: maximum wavelength
            @param source_aperture_radius: radius of the source aperture [mm]
            @param frame_ID: frame ID string, '1' or '2'
            @param binning: binning parameters, or None for automated determination
        """
        log_binning = self.getProperty('LogBinning').value
        nbins = self.getProperty('NumberOfBins').value
        property_manager_name = self.getProperty('ReductionProperties').value
        pixel_size_x = workspace.getInstrument().getNumberParameter('x-pixel-size')[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter('y-pixel-size')[0]
        output_ws_name = self.getPropertyValue('OutputWorkspace')
        compute_resolution = self.getProperty('ComputeResolution').value

        ws_frame = output_ws_name.replace('_Iq', '_frame'+frame_ID+'_Iq')

        # Rebin the data to cover the frame we are interested in
        alg = AlgorithmManager.create("Rebin")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", workspace)
        alg.setPropertyValue("OutputWorkspace", output_ws_name + '_frame' + frame_ID)
        alg.setPropertyValue("Params", '%4.2f,%4.2f,%4.2f' % (wl_min, 0.1, wl_max))
        alg.setProperty("PreserveEvents", False)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        # Replace bad values for safety
        alg = AlgorithmManager.create("ReplaceSpecialValues")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", output_ws)
        alg.setPropertyValue("OutputWorkspace", output_ws_name + '_frame' + frame_ID)
        alg.setProperty("NaNValue", 0.0)
        alg.setProperty("NaNError", 0.0)
        alg.setProperty("InfinityValue", 0.0)
        alg.setProperty("InfinityError", 0.0)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        (output_msg, output_iq, output_binning) = \
            self._call_sans_averaging(output_ws, binning,
                                      nbins, log_binning,
                                      property_manager_name,
                                      ws_frame)
        if compute_resolution:
            sample_aperture_radius = self.getProperty('SampleApertureDiameter').value / 2.0
            alg = AlgorithmManager.create("EQSANSResolution")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", output_iq)
            alg.setProperty("ReducedWorkspace", workspace)
            alg.setPropertyValue("OutputBinning", output_binning)
            alg.setProperty("MinWavelength", wl_min)
            alg.setProperty("MaxWavelength", wl_max)
            alg.setProperty("PixelSizeX", pixel_size_x)
            alg.setProperty("PixelSizeY", pixel_size_y)
            alg.setProperty("SourceApertureRadius", source_aperture_radius)
            alg.setProperty("SampleApertureRadius", sample_aperture_radius)
            alg.execute()

        return output_iq

    def _scale(self, ws_frame1, ws_frame2):
        """
            Scale frame 1 to overlap frame 2
            @param ws_frame1: frame 1 workspace object
            @param ws_frame2: frame 2 workspace object
        """
        iq_f1 = ws_frame1.readY(0)
        iq_f2 = ws_frame2.readY(0)
        q_f1 = ws_frame1.readX(0)
        q_f2 = ws_frame2.readX(0)
        scale_f1 = 0.0
        scale_f2 = 0.0
        scale_factor = 1.0
        qmin = None
        qmax = None
        for i in range(len(iq_f1)):
            if iq_f1[i] <= 0:
                break
                continue
            if qmin is None or q_f1[i] < qmin:
                qmin = q_f1[i]

            if qmax is None or q_f1[i] > qmax:
                qmax = q_f1[i]
                continue
        qmin2 = q_f2[len(q_f2) - 1]
        qmax2 = q_f2[0]
        for i in range(len(iq_f2)):
            if iq_f2[i] <= 0:
                break
                continue
            if qmin2 is None or q_f2[i] < qmin2:
                qmin2 = q_f2[i]

            if qmax2 is None or q_f2[i] > qmax2:
                qmax2 = q_f2[i]
                continue
        qmin = max(qmin, qmin2)
        qmax = min(qmax, qmax2)
        for i in range(len(iq_f1)):
            if q_f1[i] >= qmin and q_f1[i] <= qmax:
                scale_f1 += iq_f1[i] * (q_f1[i + 1] - q_f1[i])
                continue
        for i in range(len(iq_f2)):
            if q_f2[i] >= qmin and q_f2[i] <= qmax:
                scale_f2 += iq_f2[i] * (q_f2[i + 1] - q_f2[i])
                continue
        if scale_f1 > 0 and scale_f2 > 0:
            scale_factor = scale_f2 / scale_f1

        output_ws_name = self.getPropertyValue('OutputWorkspace')
        ws_frame1_name = output_ws_name.replace('_Iq', '_frame1_Iq')

        # Dq is not propagated by scale, so do it by hand
        # First, store Dq
        dq = ws_frame1.readDx(0)

        alg = AlgorithmManager.create("Scale")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", ws_frame1)
        alg.setPropertyValue("OutputWorkspace", ws_frame1_name)
        alg.setProperty("Factor", scale_factor)
        alg.setProperty("Operation", 'Multiply')
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        # ... then put Dq back
        dq_output = output_ws.dataDx(0)
        for i in range(len(dq_output)):
            dq_output[i]=dq[i]

        return output_ws

    def _get_binning(self, workspace, wavelength_min, wavelength_max):
        """
            Determine the I(Q) binning
            @param workspace: reduced workspace object
            @param wavelength_min: lower wavelength cut
            @param wavelength_max: upper wavelength cut
        """
        log_binning = self.getProperty("LogBinning").value
        nbins = self.getProperty("NumberOfBins").value
        sample_detector_distance = workspace.getRun().getProperty("sample_detector_distance").value
        nx_pixels = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        ny_pixels = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])
        pixel_size_x = workspace.getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter("y-pixel-size")[0]

        if workspace.getRun().hasProperty("beam_center_x") and \
             workspace.getRun().hasProperty("beam_center_y"):
            beam_ctr_x = workspace.getRun().getProperty("beam_center_x").value
            beam_ctr_y = workspace.getRun().getProperty("beam_center_y").value
        else:
            property_manager_name = self.getProperty("ReductionProperties").value
            property_manager = PropertyManagerDataService.retrieve(property_manager_name)
            if property_manager.existsProperty("LatestBeamCenterX") and \
                property_manager.existsProperty("LatestBeamCenterY"):
                beam_ctr_x = property_manager.getProperty("LatestBeamCenterX").value
                beam_ctr_y = property_manager.getProperty("LatestBeamCenterY").value
            else:
                raise RuntimeError, "No beam center information can be found on the data set"

        # Q min is one pixel from the center, unless we have the beam trap size
        if workspace.getRun().hasProperty("beam-trap-diameter"):
            mindist = workspace.getRun().getProperty("beam-trap-diameter").value/2.0
        else:
            mindist = min(pixel_size_x, pixel_size_y)
        qmin = 4*math.pi/wavelength_max*math.sin(0.5*math.atan(mindist/sample_detector_distance))

        dxmax = pixel_size_x*max(beam_ctr_x,nx_pixels-beam_ctr_x)
        dymax = pixel_size_y*max(beam_ctr_y,ny_pixels-beam_ctr_y)
        maxdist = math.sqrt(dxmax*dxmax+dymax*dymax)
        qmax = 4*math.pi/wavelength_min*math.sin(0.5*math.atan(maxdist/sample_detector_distance))

        if not log_binning:
            qstep = (qmax-qmin)/nbins
            f_step = (qmax-qmin)/qstep
            n_step = math.floor(f_step)
            if f_step-n_step>10e-10:
                qmax = qmin+qstep*n_step
            return qmin, qstep, qmax
        else:
            # Note: the log binning in Mantid is x_i+1 = x_i * ( 1 + dx )
            qstep = (math.log10(qmax)-math.log10(qmin))/nbins
            f_step = (math.log10(qmax)-math.log10(qmin))/qstep
            n_step = math.floor(f_step)
            if f_step-n_step>10e-10:
                qmax = math.pow(10.0, math.log10(qmin)+qstep*n_step)
            return qmin, -(math.pow(10.0,qstep)-1.0), qmax

AlgorithmFactory.subscribe(EQSANSAzimuthalAverage1D)
