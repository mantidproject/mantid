# attenuators

import numpy as np

from mantid.api import MatrixWorkspaceProperty, IEventWorkspaceProperty, PropertyMode, WorkspaceUnitValidator
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory
from mantid.api import AnalysisDataService, AlgorithmManager
from mantid.kernel import Direction, FloatArrayProperty, FloatBoundedValidator, FloatArrayMandatoryValidator, Logger
from mantid.api import IMaskWorkspace


class SANSDataProcessor(DataProcessorAlgorithm):
    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

        self.sanslog = Logger("ANSTO SANS Data reduction")

    def category(self):
        return "Workflow\\SANS"

    def name(self):
        return "SANSDataProcessor"

    def summary(self):
        return ""

    def PyInit(self):
        # input
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Mandatory),
                             doc='workspace of sample or background data')

        self.declareProperty(MatrixWorkspaceProperty('InputMaskingWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='used to remove unwanted detectors regarding the data reduction')

        # blocked beam, beam shape and detector corrections
        self.declareProperty(MatrixWorkspaceProperty('BlockedBeamWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='workspace of blocked beam data')

        self.declareProperty(MatrixWorkspaceProperty('EmptyBeamSpectrumShapeWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Mandatory,
                                                     validator=WorkspaceUnitValidator("Wavelength")),
                             doc='also known as direct-beam spectrum shape')

        self.declareProperty(MatrixWorkspaceProperty('SensitivityCorrectionMatrix', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='workspace of scaling factors for each detector pixel')

        self.declareProperty(MatrixWorkspaceProperty('TransmissionWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Mandatory),
                             doc='workspace of sample or background transmission data')

        self.declareProperty(MatrixWorkspaceProperty('TransmissionEmptyBeamWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Mandatory),
                             doc='workspace of empty beam transmission data')

        self.declareProperty(MatrixWorkspaceProperty('TransmissionMaskingWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Mandatory),
                             doc='used to remove unwanted detectors regarding the transmission calculation')

        self.declareProperty(name='fitmethod',
                             defaultValue='log', doc='fit method for transmission')

        self.declareProperty(name='polynomialorder',
                             defaultValue='3',
                             doc='polynomial order for transmission; taken into account only for Polynomial fits,'
                                 'but ignored for all the rest')

        self.declareProperty(name='scalingfactor',
                             defaultValue=1.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc='final scaling factor (also includes attenuation scaling factors)')

        self.declareProperty(name='samplethickness',
                             defaultValue=1.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc='thickness of sample')

        self.declareProperty(FloatArrayProperty('binningwavelength',
                                                direction=Direction.Input,
                                                validator=FloatArrayMandatoryValidator()),
                             doc='used for the binning of the input workspace')

        self.declareProperty(FloatArrayProperty('binningwavelengthtransm',
                                                direction=Direction.Input,
                                                validator=FloatArrayMandatoryValidator()),
                             doc='used for the binning of the transmission input workspace')

        self.declareProperty(FloatArrayProperty('binningq',
                                                direction=Direction.Input,
                                                validator=FloatArrayMandatoryValidator()),
                             doc='used for the binning of the resulting Q workspace')

        self.declareProperty(name='timemode',
                             defaultValue=True,
                             doc='wether the mode is time-of-flight or monochromatic')

        self.declareProperty(name='accountforgravity',
                             defaultValue=True,
                             doc='whether to correct for the effects of gravity')

        self.declareProperty(name='solidangleweighting',
                             defaultValue=True,
                             doc='if true, pixels will be weighted by their solid angle')

        self.declareProperty(name='radiuscut',
                             defaultValue=1.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc='To increase resolution some wavelengths are excluded within this distance from '
                                 'the beam center (mm)')

        self.declareProperty(name='wavecut',
                             defaultValue=1.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc='To increase resolution by starting to remove some wavelengths below thisfreshold '
                                 '(angstrom)')

        self.declareProperty(name='wideanglecorrection',
                             defaultValue=True,
                             doc='if true, the wide angle correction for transmissions will be applied')

        self.declareProperty(name='reduce_2d',
                             defaultValue=False,
                             doc='if true, 2D data reduction will be performed')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='result from Q1D or Qxy algorithm')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceTransmission_Fit', '', direction=Direction.Output),
                             # This works only when transmission is True. Problems starts when it is not...
                             doc='transmission_fit')

    def validateInputs(self):
        inputs = dict()

        ws_sam = self.getProperty("InputWorkspace").value
        ws_samMsk = self.getProperty("InputMaskingWorkspace").value

        ws_blk = self.getProperty("BlockedBeamWorkspace").value
        ws_emp = self.getProperty("EmptyBeamSpectrumShapeWorkspace").value
        ws_sen = self.getProperty("SensitivityCorrectionMatrix").value

        ws_tranSam = self.getProperty("TransmissionWorkspace").value
        ws_tranEmp = self.getProperty("TransmissionEmptyBeamWorkspace").value
        ws_tranMsk = self.getProperty("TransmissionMaskingWorkspace").value

        scale = self.getProperty("ScalingFactor").value
        thickness = self.getProperty("SampleThickness").value

        radiuscut = self.getProperty("RadiusCut").value
        wavecut = self.getProperty("WaveCut").value

        # -- Validation --
        sam_histograms = ws_sam.getNumberHistograms()
        if sam_histograms <= 0:
            inputs["InputWorkspace"] = "hast to contain at least one spectrum"
        elif not ws_sam.isHistogramData():
            inputs["InputWorkspace"] = "has to be a histogram"

        if ws_samMsk:
            isinstance(ws_samMsk, IMaskWorkspace)
                       
        if ws_blk:
            if not ws_blk.isHistogramData():
                inputs["BlockedBeamWorkspace"] = "has to be a histogram"
            elif ws_blk.blocksize() != 1:
                inputs["BlockedBeamWorkspace"] = "each spectrum must contain only one y value"

        if ws_emp.getNumberHistograms() != 1:
            inputs["EmptyBeamSpectrumShapeWorkspace"] = "hast to contain only one spectrum"
        elif not ws_emp.isHistogramData():
            inputs["EmptyBeamSpectrumShapeWorkspace"] = "has to be a histogram"

        if ws_sen:
            if ws_sen.getNumberHistograms() != sam_histograms:
                inputs["SensitivityCorrectionMatrix"] = "must have same number of spectra as the InputWorkspace"
            elif not ws_sen.isHistogramData():
                inputs["SensitivityCorrectionMatrix"] = "has to be a histogram"
            elif ws_sen.getAxis(0).getUnit().symbol():
                inputs["SensitivityCorrectionMatrix"] = "has to be unitless"

        tran_histograms = ws_tranSam.getNumberHistograms()
        if tran_histograms <= 0:
            inputs["TransmissionWorkspace"] = "hast to contain at least one spectrum"
        elif not ws_tranSam.isHistogramData():
            inputs["TransmissionWorkspace"] = "has to be a histogram"

        if ws_tranEmp.getNumberHistograms() != tran_histograms:
            inputs["TransmissionEmptyBeamWorkspace"] = "must have same number of spectra as the TransmissionWorkspace"
        elif not ws_tranEmp.isHistogramData():
            inputs["TransmissionEmptyBeamWorkspace"] = "has to be a histogram"

        if ws_tranMsk:
            isinstance(ws_tranMsk, IMaskWorkspace)
              
        if scale <= 0.0:
            inputs["ScalingFactor"] = "has to be greater than zero"

        if thickness <= 0.0:
            inputs["SampleThickness"] = "has to be greater than zero"
        return inputs  # - CHECK -why need it? I would believe it is optional

        if radiuscut < 0.0:
            inputs["radiuscut"] = "has to be equal or greater than zero"

        if wavecut < 0.0:
            inputs["wavecut"] = "has to be equal or greater than zero"

    def PyExec(self):
        self.sanslog.warning(
            "SANSDataProcessing is in the beta phase of development. Properties may change without notice.")
        self.sanslog.warning("Log on the changes is recorded in the body of SANSDataProcessor.py file")

        # -- Get Arguments --
        ws_sam = self.getProperty("InputWorkspace").value
        ws_samMsk = self.getProperty("InputMaskingWorkspace").value

        ws_blk = self.getProperty("BlockedBeamWorkspace").value
        ws_emp = self.getProperty("EmptyBeamSpectrumShapeWorkspace").value
        ws_sen = self.getProperty("SensitivityCorrectionMatrix").value

        ws_tranSam = self.getProperty("TransmissionWorkspace").value
        ws_tranEmp = self.getProperty("TransmissionEmptyBeamWorkspace").value
        ws_tranMsk = self.getProperty("TransmissionMaskingWorkspace").value

        scale = self.getProperty("ScalingFactor").value
        thickness = self.getProperty("SampleThickness").value

        binning_wavelength = self.getProperty("BinningWavelength").value
        binning_q = self.getProperty("BinningQ").value

        wavecut = self.getProperty("WaveCut").value
        radiuscut = self.getProperty("RadiusCut").value

        binning_wavelength_transm = self.getProperty("BinningWavelengthTransm").value
        fitmethod = self.getProperty("FitMethod").value
        polynomialorder = self.getProperty("PolynomialOrder").value

        time_mode = self.getProperty(
            "TimeMode").value
        # True if External time frame (i.e. choppers), False if Internal time frames (Neutron Velocity Selector)
        account_for_gravity = self.getProperty("AccountForGravity").value
        solid_angle_weighting = self.getProperty("SolidAngleWeighting").value
        wide_angle_correction = self.getProperty("WideAngleCorrection").value
        reduce_2d = self.getProperty("reduce_2D").value

        # -- Masking --
        if ws_samMsk:
            self._apply_mask(ws_sam, ws_samMsk)

        if ws_tranMsk:
            self._apply_mask(ws_tranSam, ws_tranMsk)
            self._apply_mask(ws_tranEmp, ws_tranMsk)

        # -- Convert to Wavelength --  Only for the External time mode - choppers
        if time_mode:
            ws_sam = self._convert_units(ws_sam, "Wavelength")
            ws_tranSam = self._convert_units(ws_tranSam, "Wavelength")
            ws_tranEmp = self._convert_units(ws_tranEmp, "Wavelength")

        # -- Transmission -- 
        # Intuitively one would think rebin for NVS data is not needed, but it does;
        # not perfect match in binning leads to error like "not matching intervals for calculate_transmission"

        ws_sam = self._rebin(ws_sam, binning_wavelength, preserveevents=False)
        ws_tranSam = self._rebin(ws_tranSam, binning_wavelength_transm, preserveevents=False)
        
        ws_tranEmp = self._rebin(ws_tranEmp, binning_wavelength_transm, preserveevents=False)

        ws_tranroi = self._mask_to_roi(ws_tranMsk)

        self.sanslog.information("FitMethod " + fitmethod)
        self.sanslog.information("PolynomialOrder " + polynomialorder)

        ws_tran = self._calculate_transmission(ws_tranSam, ws_tranEmp, ws_tranroi, fitmethod, polynomialorder,
                                               binning_wavelength_transm)

        ws_tranemp_scale = self._get_frame_count(ws_tranEmp)
        ws_transam_scale = self._get_frame_count(ws_tranSam)

        f = self._single_valued_ws(ws_tranemp_scale / ws_transam_scale)
        ws_tran = self._multiply(ws_tran, f)

        transmission_fit = ws_tran
        self.setProperty("OutputWorkspaceTransmission_Fit", transmission_fit)

        # -- Blocked Beam Subtraction -- only if blk workspace has been provided (obviously)
        if ws_blk:
            ws_sam_time = self._get_frame_count(ws_sam)
            ws_blk_time = self._get_frame_count(ws_blk)

            ws_blk_scaling = self._single_valued_ws(ws_sam_time / ws_blk_time)

            # remove estimated blk counts from sample workspace        
            self._apply_mask(ws_blk, ws_samMsk)  # masking blocked beam the same way as sample data
            if time_mode:
                ws_blk = self._convert_units(ws_blk, "Wavelength")
            ws_blk = self._rebin(ws_blk, binning_wavelength, preserveevents=False)
            # estimated blk counts for given measurement time and bin width
            ws_blk_est = self._multiply(ws_blk, ws_blk_scaling)
            ws_sam = self._subtract(ws_sam, ws_blk_est)

        # sensitivity
        pixeladj = ws_sen

        ws_tran = self._emp_shape_adjustment(ws_tran, ws_emp)  # swap arrays; ws_emp is always be shorter or equal to ws_tran
        wavelengthadj = self._multiply(ws_emp, ws_tran)

        # calculate the wide angle correction for sample transmission
        if wide_angle_correction:
            wavepixeladj = self._wide_angle_correction(ws_sam, ws_tran)
        else:
            wavepixeladj = None

        # distance to maximum of parabolic motion of neutrons
        real_l1 = self._get_l1(ws_sam)  # distance from the end of the last guide to the sample
        extralength = 0.5 * real_l1  # neutrons following parabolic trajectory with maximum at the middle of the L1

        # normalize vector to counting time of sample & long empty beam run
        ws_emp_time = self._get_frame_count(ws_emp)
        ws_sam_time = self._get_frame_count(ws_sam)
        scale_full = scale * (ws_emp_time / ws_sam_time)
        # extra multiplier is needed because measured transmission is ~5% lower; we need to divide result by lower number, hence need to lowering the final result, i.e. divide by 1.05        
        f = self._single_valued_ws(scale_full / (thickness*1.05))
        
        if reduce_2d:
            q_max = binning_q[2]
            q_delta = binning_q[1]
            qxy = self._qxy(ws_sam, q_max, q_delta, pixeladj, wavelengthadj, account_for_gravity, solid_angle_weighting,
                            extralength)
            qxy = self._multiply(qxy, f)
            self.setProperty("OutputWorkspace", qxy)
        else:
            deltar = 5.0  # Virtual ring width on the detector (mm). Hardcoded. Not sure it is a good way to go.

            if (ws_sam.run().getProperty("source_aperture").value):
                sourceapertureradius = float(ws_sam.run().getProperty("source_aperture").value) / 2.0
                if sourceapertureradius > 40.0:
                    sourceapertureradius = 20.0
                    print "sourceapertureradius value cannot be retrieved; generic value of 20mm taken"
            else:
                sourceapertureradius = 20.0  # radius in mm
                print "sourceapertureradius value cannot be retrieved; generic value of 20mm taken"

            if (ws_sam.run().getProperty("sample_aperture").value):
                sampleapertureradius = float(ws_sam.run().getProperty("source_aperture").value) / 2.0
                if sampleapertureradius > 40.0:
                    sampleapertureradius = 6.25
                    print "sampleapertureradius value cannot be retrieved; generic value of 6.25mm taken"                    
            else:
                sampleapertureradius = 6.25  # radius in mm
                print "sampleapertureradius value cannot be retrieved; generic value of 6.25mm taken"                    

            # creating empty array for SigmaModerator
            # SigmaModerator is a mandatory parameter for ISIS, but not needed for the reactor facility
            number_of_bins = 10
            number_of_spectra = 1
            delta_wavelength = 0.1

            data_x = np.zeros(number_of_bins + 1)
            data_y = np.zeros(number_of_bins)
            x_value = 0.5
            y_value = 0.0
            for index in range(number_of_bins):
                data_x[index] = x_value
                data_y[index] = y_value
                x_value += delta_wavelength
            data_x[number_of_bins] = x_value
            units = "Wavelength"

            sigmamoderator = self._create_empty_ws(data_x, data_y, number_of_spectra, units)

            # Call TOFSANSResolutionByPixel
            ws_sam = self._multiply(ws_sam, f)
            qresolution = self._tofsansresolutionbypixel(ws_sam, deltar, sampleapertureradius, sourceapertureradius,
                                                         sigmamoderator, real_l1, account_for_gravity, extralength)

            # Call Q1D, now with resolution         
            q1d = self._q1d(ws_sam, binning_q, pixeladj, wavelengthadj, wavepixeladj, account_for_gravity,
                            solid_angle_weighting, radiuscut, wavecut, extralength, qresolution)

            self.setProperty("OutputWorkspace", q1d)  # set output, file 1D pattern

    def _get_time_span(self, ws):
        run = ws.getRun()
        duration = run.endTime() - run.starme()
        return float(duration.total_microseconds())

    def _get_bm_counts(self, ws):
        return float(ws.run().getProperty("bm_counts").value)

    def _get_frame_count(self, ws):
        return float(ws.run().getProperty("frame_count").value)

    def _get_period(self, ws):
        return float(ws.run().getProperty("period").value)

    def _get_l1(self, ws):
        return float(ws.run().getProperty("L1").value)

    def _apply_mask(self, ws, mask):
        alg = self.createChildAlgorithm("MaskDetectors")
        alg.setProperty("Workspace", ws)
        alg.setProperty("MaskedWorkspace", mask)
        alg.execute()

    def _convert_units(self, ws, unit):
        alg = self.createChildAlgorithm("ConvertUnits")
        alg.setProperty("InputWorkspace", ws)
        alg.setProperty("Target", unit)
        alg.execute()

        return alg.getProperty("OutputWorkspace").value

    def _rebin(self, ws, binning, preserveevents):
        alg = self.createChildAlgorithm("Rebin")
        alg.setProperty("InputWorkspace", ws)
        alg.setProperty("Params", binning)
        alg.setProperty("PreserveEvents", preserveevents)
        alg.execute()

        return alg.getProperty("OutputWorkspace").value

    def _multiply(self, a, b):
        alg = self.createChildAlgorithm("Multiply")
        alg.setProperty("LHSWorkspace", a)
        alg.setProperty("RHSWorkspace", b)
        alg.execute()

        return alg.getProperty("OutputWorkspace").value

    def _scale_mult(self, ws_input, factor, operation):
        alg = self.createChildAlgorithm("Scale")
        alg.setProperty("InputWorkspace", ws_input)
        alg.setProperty("Factor", factor)
        alg.setProperty("Operation", operation)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _subtract(self, a, b):
        alg = self.createChildAlgorithm("Minus")
        alg.setProperty("LHSWorkspace", a)
        alg.setProperty("RHSWorkspace", b)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _single_valued_ws(self, value):
        alg = self.createChildAlgorithm("CreateSingleValuedWorkspace")
        alg.setProperty("DataValue", value)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _mask_to_roi(self, ws_mask):
        # invert mask and then extract "masked" detectors in order to get ROI
        # BUG in Mantid forces us to use AnalysisDataService
        alg = AlgorithmManager.create("InvertMask")
        alg.initialize()
        alg.setProperty("InputWorkspace", ws_mask)
        alg.setPropertyValue("OutputWorkspace", "_ws")
        alg.execute()
        ws_tranmskinv = AnalysisDataService.retrieve("_ws")
        alg = self.createChildAlgorithm("ExtractMask")
        alg.setProperty("InputWorkspace", ws_tranmskinv)
        alg.execute()
        AnalysisDataService.remove("_ws")
        return alg.getProperty("DetectorList").value

    def _calculate_transmission(self, ws_tranSam, ws_tranEmp, ws_tranroi, fitmethod, polynomialorder, binning):
        alg = self.createChildAlgorithm("CalculateTransmission")
        alg.setProperty("SampleRunWorkspace", ws_tranSam)
        alg.setProperty("DirectRunWorkspace", ws_tranEmp)
        alg.setProperty("TransmissionROI", ws_tranroi)
        alg.setProperty("RebinParams", binning)
        alg.setProperty("FitMethod", fitmethod)  # new
        alg.setProperty("PolynomialOrder", polynomialorder)  # new
        # FitMethod = 'Polynomial', PolynomialOrder = '4'
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _wide_angle_correction(self, ws_sam, ws_tranSam):
        alg = self.createChildAlgorithm("SANSWideAngleCorrection")
        alg.setProperty("SampleData", ws_sam)
        alg.setProperty("TransmissionData", ws_tranSam)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _emp_shape_adjustment(self, ws_emp, ws_tran):
        if ws_emp.getNumberHistograms() != 1:
            raise ValueError
        if ws_tran.getNumberHistograms() != 1:
            raise ValueError
        ws_emp_bins = ws_emp.readX(0)
        ws_tran_bins = ws_tran.readX(0)
        if np.array_equal(ws_emp_bins, ws_tran_bins):  # check that bins match
            return ws_emp  # if they match keep them as they are
        self.sanslog.warning(
            "EmptyBeamSpectrumShapeWorkspace did not have expected wavelength binning and has to be rebinned")
        alg = self.createChildAlgorithm("RebinToWorkspace")
        alg.setProperty("WorkspaceToRebin", ws_emp)
        alg.setProperty("WorkspaceToMatch", ws_tran)
        alg.setProperty("PreserveEvents", False)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _tofsansresolutionbypixel(self, ws_sam, deltar, sampleapertureradius, sourceapertureradius, sigmamoderator,
                                  collimationlength, accountforgravity, extralength):
        alg = self.createChildAlgorithm("TOFSANSResolutionByPixel")
        alg.setProperty("InputWorkspace", ws_sam)
        alg.setProperty("DeltaR", deltar)
        alg.setProperty("SampleApertureRadius", sampleapertureradius)
        alg.setProperty("SourceApertureRadius", sourceapertureradius)
        alg.setProperty("SigmaModerator", sigmamoderator)
        alg.setProperty("CollimationLength", collimationlength)
        alg.setProperty("AccountForGravity", accountforgravity)
        alg.setProperty("ExtraLength", extralength)

        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _q1d(self, ws_sam, binning_q, pixeladj, wavelengthadj, wavepixeladj, accountforgravity, solidangleweighting,
             radiuscut, wavecut, extralength, qresolution):
        alg = self.createChildAlgorithm("Q1D")
        alg.setProperty("DetBankWorkspace", ws_sam)
        alg.setProperty("OutputBinning", binning_q)
        alg.setProperty("AccountForGravity", accountforgravity)
        alg.setProperty("SolidAngleWeighting", solidangleweighting)
        alg.setProperty("RadiusCut", radiuscut)
        alg.setProperty("WaveCut", wavecut)
        alg.setProperty("ExtraLength", extralength)
        alg.setProperty("QResolution", qresolution)
        # transmission and beam shape correction
        if wavelengthadj:
            alg.setProperty("WavelengthAdj", wavelengthadj)
        # wide angle correction
        if wavepixeladj:
            alg.setProperty("wavePixelAdj", wavepixeladj)
        # pixel sensitivity correction
        if pixeladj:
            alg.setProperty("PixelAdj", pixeladj)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _qxy(self, ws_sam, q_max, q_delta, pixeladj, wavelengthadj, accountforgravity, solidangleweighting,
             extralength):
        alg = self.createChildAlgorithm("Qxy")
        alg.setProperty("InputWorkspace", ws_sam)
        alg.setProperty("MaxQxy", q_max)
        alg.setProperty("DeltaQ", q_delta)
        alg.setProperty("AccountForGravity", accountforgravity)
        alg.setProperty("SolidAngleWeighting", solidangleweighting)
        alg.setProperty("ExtraLength", extralength)
        # pixel sensitivity correction
        if pixeladj:
            alg.setProperty("PixelAdj", pixeladj)
        # transmission and beam shape correction
        if wavelengthadj:
            alg.setProperty("WavelengthAdj", wavelengthadj)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _create_empty_ws(self, data_x, data_y, number_of_spectra, unitx):
        # empty output workspace in case 2D reduction is not happening
        alg = self.createChildAlgorithm("CreateWorkspace")
        alg.setProperty('DataX', data_x)
        alg.setProperty('DataY', data_y)
        alg.setProperty('NSpec', number_of_spectra)
        alg.setProperty('UnitX', unitx)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


# register algorithm
AlgorithmFactory.subscribe(SANSDataProcessor)
