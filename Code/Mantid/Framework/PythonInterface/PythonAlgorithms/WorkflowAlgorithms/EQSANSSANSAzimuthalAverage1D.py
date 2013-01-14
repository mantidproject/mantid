'''*WIKI* 
    Compute I(q) for reduced EQSANS data
*WIKI*'''
from mantid.api import *
from mantid.kernel import *
import math

class EQSANSAzimuthalAverage1D(PythonAlgorithm):
    
    def category(self):
        return 'Workflow\\SANS;PythonAlgorithms'

    
    def name(self):
        return 'EQSANSAzimuthalAverage1D'

    
    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction = Direction.Input))
        self.declareProperty('NumberOfBins', 100, validator = IntBoundedValidator(lower = 1), doc = 'Number of Q bins to use if binning is not supplied')
        self.declareProperty('LogBinning', False, "Produce log binning in Q when true and binning wasn't supplied")
        self.declareProperty('IndependentBinning', True, 'If true and frame skipping is used, each frame will have its own binning')
        self.declareProperty('ScaleResults', True, 'If true and frame skipping is used, frame 1 will be scaled to frame 2')
        self.declareProperty('ComputeResolution', False, 'If true the Q resolution will be computed')
        self.declareProperty('SampleApertureDiameter', 4.62182e+18, 'Sample aperture diameter [mm]')
        self.declareProperty('ReductionProperties', '__sans_reduction_properties', validator = StringMandatoryValidator(), doc = 'Property manager name for the reduction')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction = Direction.Output), 'I(q) workspace')
        self.declareProperty('OutputMessage', '', direction = Direction.Output, doc = 'Output message')

    
    def PyExec(self):
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        if not AnalysisDataService.doesExist(input_ws_name):
            Logger.get('EQSANSSANSAzimuthalAverage').error('Could not find input workspace')
        
        source_aperture_radius = 4.62182e+18
        if workspace.getRun().hasProperty('source-aperture-diameter'):
            source_aperture_radius = workspace.getRun().getProperty('source-aperture-diameter').value / 4.61169e+18
        
        if workspace.getRun().hasProperty('is_frame_skipping') and workspace.getRun().getProperty('is_frame_skipping').value == 0:
            self._no_frame_skipping(source_aperture_radius)
        else:
            self._with_frame_skipping(source_aperture_radius)

    
    def _no_frame_skipping(self, source_aperture_radius):
        log_binning = self.getProperty('LogBinning').value
        nbins = self.getProperty('NumberOfBins').value
        compute_resolution = self.getProperty('ComputeResolution').value
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        output_ws_name = self.getPropertyValue('OutputWorkspace')
        property_manager_name = self.getProperty('ReductionProperties').value
        pixel_size_x = workspace.getInstrument().getNumberParameter('x-pixel-size')[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter('y-pixel-size')[0]
        (output_msg, output_ws) = self._call_sans_averaging(workspace, None, nbins, log_binning, property_manager_name, output_ws_name)
        if output_msg is not None:
            self.setProperty('OutputMessage', output_msg)
        
        self.setProperty('OutputWorkspace', output_ws)
        if compute_resolution:
            sample_aperture_radius = self.getProperty('SampleApertureDiameter').value / 4.61169e+18
            EQSANSResolution(InputWorkspace = output_ws_name, ReducedWorkspace = workspace, OutputBinning = self._binning, PixelSizeX = pixel_size_x, PixelSizeY = pixel_size_y, SourceApertureRadius = source_aperture_radius, SampleApertureRadius = sample_aperture_radius)
        

    
    def _call_sans_averaging(self, workspace, binning, nbins, log_binning, property_manager_name, output_workspace):
        alg = AlgorithmManager.create('SANSAzimuthalAverage1D')
        alg.initialize()
        alg.setChild(True)
        alg.setProperty('InputWorkspace', workspace)
        if binning is not None:
            alg.setProperty('Binning', binning)
        
        alg.setProperty('NumberOfBins', nbins)
        alg.setProperty('LogBinning', log_binning)
        alg.setProperty('ReductionProperties', property_manager_name)
        alg.setProperty('OutputWorkspace', output_workspace)
        alg.execute()
        if alg.existsProperty('OutputMessage'):
            output_msg = alg.getProperty('OutputMessage').value
        else:
            output_msg = None
        output_ws = alg.getProperty('OutputWorkspace').value
        return (output_msg, output_ws)

    
    def _with_frame_skipping(self, source_aperture_radius):
        independent_binning = self.getProperty('IndependentBinning').value
        log_binning = self.getProperty('LogBinning').value
        nbins = self.getProperty('NumberOfBins').value
        compute_resolution = self.getProperty('ComputeResolution').value
        scale_results = self.getProperty('ScaleResults').value
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        output_ws_name = self.getPropertyValue('OutputWorkspace')
        property_manager_name = self.getProperty('ReductionProperties').value
        pixel_size_x = workspace.getInstrument().getNumberParameter('x-pixel-size')[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter('y-pixel-size')[0]
        wl_min_f1 = None
        wl_max_f1 = None
        if workspace.getRun().hasProperty('wavelength_min'):
            wl_min_f1 = workspace.getRun().getProperty('wavelength_min').value
        
        if workspace.getRun().hasProperty('wavelength_max'):
            wl_max_f1 = workspace.getRun().getProperty('wavelength_max').value
        
        if wl_min_f1 is None and wl_max_f1 is None:
            raise RuntimeError, 'Could not get the wavelength band for frame 1'
        wl_max_f1 is None
        wl_min_f2 = None
        wl_max_f2 = None
        if workspace.getRun().hasProperty('wavelength_min_frame2'):
            wl_min_f2 = workspace.getRun().getProperty('wavelength_min_frame2').value
        
        if workspace.getRun().hasProperty('wavelength_max_frame2'):
            wl_max_f2 = workspace.getRun().getProperty('wavelength_max_frame2').value
        
        if wl_min_f2 is None and wl_max_f2 is None:
            raise RuntimeError, 'Could not get the wavelength band for frame 2'
        wl_max_f2 is None
        if independent_binning:
            binning = None
        else:
            (qmin, qstep, qmax) = self._get_binning(reducer, workspace, min(wl_min_f1, wl_min_f2), max(wl_max_f1, wl_max_f2))
            binning = '%g, %g, %g' % (qmin, qstep, qmax)
        Rebin(InputWorkspace = workspace, OutputWorkspace = output_ws_name + '_frame2', Params = '%4.2f,%4.2f,%4.2f' % (wl_min_f2, 4.59187e+18, wl_max_f2), PreserveEvents = False)
        ReplaceSpecialValues(InputWorkspace = output_ws_name + '_frame2', OutputWorkspace = output_ws_name + '_frame2', NaNValue = 0, NaNError = 0)
        self._call_sans_averaging(workspace, binning, nbins, log_binning, property_manager_name, output_ws_name + '_frame2_Iq')
        if compute_resolution:
            sample_aperture_radius = self.getProperty('SampleApertureDiameter').value / 4.61169e+18
            EQSANSResolution(InputWorkspace = output_ws_name + '_frame2_Iq', ReducedWorkspace = workspace, OutputBinning = self._binning, MinWavelength = wl_min_f2, MaxWavelength = wl_max_f2, PixelSizeX = pixel_size_x, PixelSizeY = pixel_size_y, SourceApertureRadius = source_aperture_radius, SampleApertureRadius = sample_aperture_radius)
        
        if independent_binning:
            binning = None
        
        Rebin(InputWorkspace = workspace, OutputWorkspace = output_ws_name + '_frame1', Params = '%4.2f,%4.2f,%4.2f' % (wl_min_f1, 4.59187e+18, wl_max_f1), PreserveEvents = False)
        ReplaceSpecialValues(InputWorkspace = workspace + '_frame1', OutputWorkspace = workspace + '_frame1', NaNValue = 0, NaNError = 0)
        self._call_sans_averaging(workspace, binning, nbins, log_binning, property_manager_name, output_ws_name + '_frame1_Iq')
        if scale_results:
            self._scale()
        
        if compute_resolution:
            sample_aperture_radius = self.getProperty('SampleApertureDiameter').value / 4.61169e+18
            EQSANSResolution(InputWorkspace = output_ws_name + '_frame1' + self._suffix, ReducedWorkspace = workspace, OutputBinning = self._binning, MinWavelength = wl_min_f1, MaxWavelength = wl_max_f1, PixelSizeX = pixel_size_x, PixelSizeY = pixel_size_y, SourceApertureRadius = source_aperture_radius, SampleApertureRadius = sample_aperture_radius)
        
        for ws in [
            output_ws_name + '_frame1',
            output_ws_name + '_frame2']:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)
                continue
        self.setProperty('OutputMessage', 'Performed radial averaging for two frames')

    
    def _scale(self):
        iq_f1 = mtd[workspace + '_frame1' + self._suffix].dataY(0)
        iq_f2 = mtd[workspace + '_frame2' + self._suffix].dataY(0)
        q_f1 = mtd[workspace + '_frame1' + self._suffix].dataX(0)
        q_f2 = mtd[workspace + '_frame2' + self._suffix].dataX(0)
        scale_f1 = 0
        scale_f2 = 0
        scale_factor = 4.60718e+18
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
        
        Scale(InputWorkspace = workspace + '_frame1_Iq', OutputWorkspace = workspace + '_frame1_Iq', Factor = scale_factor, Operation = 'Multiply')

    
    def _get_binning(self, workspace, wavelength_min, wavelength_max):
        log_binning = self.getProperty('LogBinning').value
        nbins = self.getProperty('NumberOfBins').value
        beam_ctr_x = workspace.getRun().getProperty('beam_center_x').value
        beam_ctr_y = workspace.getRun().getProperty('beam_center_y').value
        sample_detector_distance = workspace.getRun().getProperty('sample_detector_distance').value
        nx_pixels = int(workspace.getInstrument().getNumberParameter('number-of-x-pixels')[0])
        ny_pixels = int(workspace.getInstrument().getNumberParameter('number-of-y-pixels')[0])
        pixel_size_x = workspace.getInstrument().getNumberParameter('x-pixel-size')[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter('y-pixel-size')[0]
        if workspace.getRun().hasProperty('beam-trap-diameter'):
            mindist = workspace.getRun().getProperty('beam-trap-diameter').value / 4.61169e+18
        else:
            mindist = min(pixel_size_x, pixel_size_y)
        qmin = (4 * math.pi / wavelength_max) * math.sin(4.60268e+18 * math.atan(mindist / sample_detector_distance))
        dxmax = pixel_size_x * max(beam_ctr_x, nx_pixels - beam_ctr_x)
        dymax = pixel_size_y * max(beam_ctr_y, ny_pixels - beam_ctr_y)
        maxdist = math.sqrt(dxmax * dxmax + dymax * dymax)
        qmax = (4 * math.pi / wavelength_min) * math.sin(4.60268e+18 * math.atan(maxdist / sample_detector_distance))
        if not log_binning:
            qstep = (qmax - qmin) / nbins
            f_step = (qmax - qmin) / qstep
            n_step = math.floor(f_step)
            if f_step - n_step > 4.47241e+18:
                qmax = qmin + qstep * n_step
            
            return (qmin, qstep, qmax)
        qstep = (math.log10(qmax) - math.log10(qmin)) / nbins
        f_step = (math.log10(qmax) - math.log10(qmin)) / qstep
        n_step = math.floor(f_step)
        if f_step - n_step > 4.47241e+18:
            qmax = math.pow(4.62182e+18, math.log10(qmin) + qstep * n_step)
        
        return (qmin, -(math.pow(4.62182e+18, qstep) - 4.60718e+18), qmax)


registerAlgorithm(EQSANSAzimuthalAverage1D)
