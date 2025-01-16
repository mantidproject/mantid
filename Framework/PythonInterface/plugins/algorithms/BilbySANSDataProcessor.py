# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mantid.api import MatrixWorkspaceProperty, PropertyMode, WorkspaceUnitValidator
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory
from mantid.kernel import Direction, FloatArrayProperty, FloatBoundedValidator, FloatArrayMandatoryValidator, Logger
from mantid.api import IMaskWorkspace

SOURCE_APERTURE_RADIUS = 20.0
SOURCE_APERTURE_RADIUS_MAX = 40.0
SAMPLE_APERTURE_RADIUS_MAX = 40.0
SAMPLE_APERTURE_RADIUS = 6.25
NUMBER_OF_BINS = 10
NUMBER_OF_SPECTRA = 1
DELTA_WAVELENGTH = 0.1


class BilbySANSDataProcessor(DataProcessorAlgorithm):
    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

        self.sanslog = Logger("ANSTO SANS Data reduction")

    def category(self):
        return "Workflow\\SANS"

    def seeAlso(self):
        return ["Q1D", "TOFSANSResolutionByPixel", "SANSWideAngleCorrection"]

    def name(self):
        return "BilbySANSDataProcessor"

    def summary(self):
        return (
            "BILBY SANS data reduction. Converts a workspace in wavelength into a 1D or 2D workspace of"
            " momentum transfer, assuming elastic scattering."
        )

    def PyInit(self):
        # input
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="Particle counts as a function of wavelength",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("InputMaskingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Mask for the scattering data",
        )

        # blocked beam, beam shape and detector corrections
        self.declareProperty(
            MatrixWorkspaceProperty("BlockedBeamWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Blocked beam scattering",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                "EmptyBeamSpectrumShapeWorkspace",
                "",
                direction=Direction.Input,
                optional=PropertyMode.Mandatory,
                validator=WorkspaceUnitValidator("Wavelength"),
            ),
            doc="Empty beam transmission, where only a given wavelength slice is considered",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SensitivityCorrectionMatrix", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Detector sensitivity calibration data set",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionWorkspace", "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="Sample transmission workspace",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionEmptyBeamWorkspace", "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="Empty beam transmission workspace",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionMaskingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="Mask for the transmission data",
        )

        self.declareProperty(
            name="FitMethod",
            defaultValue="log",
            doc="Function to use to fit transmission; can be Linear, Log, Polynomial (first letter shall be capital)",
        )

        self.declareProperty(
            name="PolynomialOrder", defaultValue="3", doc="Used only for Polynomial function, but needed as an input parameter anyway"
        )

        self.declareProperty(name="ScalingFactor", defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0), doc="Attenuating factor")

        self.declareProperty(
            name="SampleThickness", defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0), doc="Thickness of sample"
        )

        self.declareProperty(
            FloatArrayProperty("BinningWavelength", direction=Direction.Input, validator=FloatArrayMandatoryValidator()),
            doc="Wavelength boundaries for reduction: a comma separated list of first bin boundary, width, last bin boundary",
        )

        self.declareProperty(
            FloatArrayProperty("BinningWavelengthTransm", direction=Direction.Input, validator=FloatArrayMandatoryValidator()),
            doc="Wavelengths boundaries for transmission binning: a comma separated list of first bin boundary, width, last bin",
        )

        self.declareProperty(
            FloatArrayProperty("BinningQ", direction=Direction.Input, validator=FloatArrayMandatoryValidator()),
            doc="Output Q-boundaries: a comma separated list of first bin boundary, width, last bin boundary",
        )

        self.declareProperty(name="Timemode", defaultValue=True, doc="If data collected in ToF or monochromatic mode")

        self.declareProperty(name="AccountForGravity", defaultValue=True, doc="Whether to correct for the effects of gravity")

        self.declareProperty(name="SolidAngleWeighting", defaultValue=True, doc="If True, pixels will be weighted by their solid angle")

        self.declareProperty(
            name="RadiusCut",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="To increase resolution some wavelengths are excluded within this distance from the"
            " beam center (mm). Note that RadiusCut and WaveCut both need to be larger than 0 to"
            " affect the effective cutoff. See the algorithm description for a detailed"
            " explanation of the cutoff.",
        )

        self.declareProperty(
            name="WaveCut",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="To increase resolution by starting to remove some wavelengths below this threshold"
            " (angstrom). Note that WaveCut and RadiusCut both need to be larger than 0 to affect"
            " on the effective cutoff. See the algorithm description for a detailed explanation"
            " of the cutoff.",
        )

        self.declareProperty(
            name="WideAngleCorrection", defaultValue=True, doc="If true, the wide angle correction for transmissions will be applied"
        )

        self.declareProperty(name="Reduce2D", defaultValue=False, doc="If true, 2D data reduction will be performed")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Name of the workspace that contains the result of the calculation. Created automatically.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceTransmissionFit", "", direction=Direction.Output),
            # This works only when transmission is True. Problems starts when it is not...
            doc="Counts vs wavelength, fit for the sample transmission",
        )

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

        # -- Validation --
        sam_histograms = ws_sam.getNumberHistograms()
        if sam_histograms <= 0:
            inputs["InputWorkspace"] = "has to contain at least one spectrum"
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
            inputs["EmptyBeamSpectrumShapeWorkspace"] = "has to contain only one spectrum"
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
            inputs["TransmissionWorkspace"] = "has to contain at least one spectrum"
        elif not ws_tranSam.isHistogramData():
            inputs["TransmissionWorkspace"] = "has to be a histogram"

        if ws_tranEmp.getNumberHistograms() != tran_histograms:
            inputs["TransmissionEmptyBeamWorkspace"] = "must have same number of spectra as the TransmissionWorkspace"
        elif not ws_tranEmp.isHistogramData():
            inputs["TransmissionEmptyBeamWorkspace"] = "has to be a histogram"

        if ws_tranMsk:
            isinstance(ws_tranMsk, IMaskWorkspace)

        inputs = self.check_geometry_and_cuts(inputs)

        return inputs

    def check_geometry_and_cuts(self, inputs):
        scale = self.getProperty("ScalingFactor").value
        thickness = self.getProperty("SampleThickness").value

        radiuscut = self.getProperty("RadiusCut").value
        wavecut = self.getProperty("WaveCut").value

        if scale <= 0.0:
            inputs["ScalingFactor"] = "has to be greater than zero"

        if thickness <= 0.0:
            inputs["SampleThickness"] = "has to be greater than zero"

        if radiuscut < 0.0:
            inputs["radiuscut"] = "has to be equal or greater than zero"

        if wavecut < 0.0:
            inputs["wavecut"] = "has to be equal or greater than zero"

        return inputs

    def PyExec(self):
        self.sanslog.warning("SANSDataProcessing is in the beta phase of development. Properties may change without notice.")
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

        time_mode = self.getProperty("TimeMode").value
        # True if External time frame (i.e. choppers), False if Internal time frames (Neutron Velocity Selector)
        account_for_gravity = self.getProperty("AccountForGravity").value
        solid_angle_weighting = self.getProperty("SolidAngleWeighting").value
        wide_angle_correction = self.getProperty("WideAngleCorrection").value
        reduce_2d = self.getProperty("Reduce2D").value

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
        # Intuitively one would think rebin for NVS data is not needed, but it is required;
        # not perfect match in binning leads to error like "not matching intervals for calculate_transmission"

        ws_sam = self._rebin(ws_sam, binning_wavelength, preserveevents=False)
        ws_tranSam = self._rebin(ws_tranSam, binning_wavelength_transm, preserveevents=False)

        ws_tranEmp = self._rebin(ws_tranEmp, binning_wavelength_transm, preserveevents=False)

        ws_tranroi = self._mask_to_roi(ws_tranMsk)

        self.sanslog.information("FitMethod " + fitmethod)
        self.sanslog.information("PolynomialOrder " + polynomialorder)

        ws_tran = self._calculate_transmission(ws_tranSam, ws_tranEmp, ws_tranroi, fitmethod, polynomialorder, binning_wavelength_transm)

        ws_tranemp_scale = self._get_frame_count(ws_tranEmp)
        ws_transam_scale = self._get_frame_count(ws_tranSam)

        f = self._single_valued_ws(ws_tranemp_scale / ws_transam_scale)
        ws_tran = self._multiply(ws_tran, f)

        transmission_fit = ws_tran
        self.setProperty("OutputWorkspaceTransmissionFit", transmission_fit)

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

        ws_tran = self._emp_shape_adjustment(ws_tran, ws_emp)  # swap arrays; ws_emp will always be shorter or equal to ws_tran
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
        # extra multiplier is needed because measured transmission is ~5% lower;
        # we need to divide result by lower number, hence need to lowering the final result, i.e. divide by 1.05
        f = self._single_valued_ws(scale_full / (thickness * 1.05))

        if reduce_2d:
            q_max = binning_q[2]
            q_delta = binning_q[1]
            qxy = self._qxy(ws_sam, q_max, q_delta, pixeladj, wavelengthadj, account_for_gravity, solid_angle_weighting, extralength)
            qxy = self._multiply(qxy, f)
            self.setProperty("OutputWorkspace", qxy)
        else:
            if ws_sam.run().getProperty("source_aperture").value:
                sourceapertureradius = float(ws_sam.run().getProperty("source_aperture").value) / 2.0
                if sourceapertureradius > SOURCE_APERTURE_RADIUS_MAX:
                    sourceapertureradius = SOURCE_APERTURE_RADIUS
                    print("sourceapertureradius value cannot be retrieved; generic value of 20mm taken")
            else:
                sourceapertureradius = SOURCE_APERTURE_RADIUS  # radius in mm
                print("sourceapertureradius value cannot be retrieved; generic value of 20mm taken")

            if ws_sam.run().getProperty("sample_aperture").value:
                sampleapertureradius = float(ws_sam.run().getProperty("source_aperture").value) / 2.0
                if sampleapertureradius > SAMPLE_APERTURE_RADIUS_MAX:
                    sampleapertureradius = SAMPLE_APERTURE_RADIUS
                    print("sampleapertureradius value cannot be retrieved; generic value of 6.25mm taken")
            else:
                sampleapertureradius = SAMPLE_APERTURE_RADIUS  # radius in mm
                print("sampleapertureradius value cannot be retrieved; generic value of 6.25mm taken")

                # creating empty array for SigmaModerator
            # SigmaModerator is a mandatory parameter for ISIS, but not needed for the reactor facility
            number_of_bins = NUMBER_OF_BINS
            number_of_spectra = NUMBER_OF_SPECTRA
            delta_wavelength = DELTA_WAVELENGTH

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
            qresolution = self._tofsansresolutionbypixel(
                ws_sam, sampleapertureradius, sourceapertureradius, sigmamoderator, real_l1, account_for_gravity, extralength
            )

            # Call Q1D, now with resolution
            q1d = self._q1d(
                ws_sam,
                binning_q,
                pixeladj,
                wavelengthadj,
                wavepixeladj,
                account_for_gravity,
                solid_angle_weighting,
                radiuscut,
                wavecut,
                extralength,
                qresolution,
            )

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
        alg = self.createChildAlgorithm("InvertMask")
        alg.setProperty("InputWorkspace", ws_mask)
        alg.execute()
        ws_tranmskinv = alg.getProperty("OutputWorkspace").value
        alg = self.createChildAlgorithm("ExtractMask")
        alg.setProperty("InputWorkspace", ws_tranmskinv)
        alg.execute()
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
        self.sanslog.warning("EmptyBeamSpectrumShapeWorkspace did not have expected wavelength binning and has to be rebinned")
        alg = self.createChildAlgorithm("RebinToWorkspace")
        alg.setProperty("WorkspaceToRebin", ws_emp)
        alg.setProperty("WorkspaceToMatch", ws_tran)
        alg.setProperty("PreserveEvents", False)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _tofsansresolutionbypixel(
        self,
        ws_sam,
        sampleapertureradius,
        sourceapertureradius,
        sigmamoderator,
        collimationlength,
        accountforgravity,
        extralength,
        deltar=5.0,
    ):
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

    def _q1d(
        self,
        ws_sam,
        binning_q,
        pixeladj,
        wavelengthadj,
        wavepixeladj,
        accountforgravity,
        solidangleweighting,
        radiuscut,
        wavecut,
        extralength,
        qresolution,
    ):
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

    def _qxy(self, ws_sam, q_max, q_delta, pixeladj, wavelengthadj, accountforgravity, solidangleweighting, extralength):
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
        alg.setProperty("DataX", data_x)
        alg.setProperty("DataY", data_y)
        alg.setProperty("NSpec", number_of_spectra)
        alg.setProperty("UnitX", unitx)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


# register algorithm
AlgorithmFactory.subscribe(BilbySANSDataProcessor)
