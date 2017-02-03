#pylint: disable=no-init,invalid-name
"""
    Magnetism reflectometry reduction
"""
from __future__ import (absolute_import, division, print_function)
import math
import functools
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *

INSTRUMENT_NAME = "REF_M"

class MagnetismReflectometryReduction(PythonAlgorithm):
    """
        Workflow algorithm to reduce REF_M data
    """
    number_of_pixels_x=0
    number_of_pixels_y=0

    def category(self):
        """ Algorithm category """
        return "Reflectometry\\SNS"

    def name(self):
        """ Algorithm name """
        return "MagnetismReflectometryReduction"

    def version(self):
        """ Version number """
        return 1

    def summary(self):
        """ Friendly description """
        return "Magnetism Reflectometer (REFM) reduction"

    def PyInit(self):
        """ Initialization """
        self.declareProperty(StringArrayProperty("RunNumbers"), "List of run numbers to process")
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the data peak")
        self.declareProperty("SubtractSignalBackground", True,
                             doc='If true, the background will be subtracted from the data peak')
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background. Default:(123,137)")
        self.declareProperty("ApplyNormalization", True, doc="If true, the data will be normalized")
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127, 133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the normalization peak")
        self.declareProperty("SubtractNormBackground", True,
                             doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background for the normalization")
        self.declareProperty("CutLowResDataAxis", True,
                             doc="If true, the low resolution direction of the data will be cropped according "+
                             "to the lowResDataAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the data")
        self.declareProperty("CutLowResNormAxis", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped "+
                             "according to the LowResNormAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the normalizaion run")
        self.declareProperty(FloatArrayProperty("TOFRange", [0., 340000.],
                                                FloatArrayLengthValidator(2), direction=Direction.Input),
                             "TOF range to use")
        self.declareProperty("CutTOFRange", True,
                             doc="If true, the TOF will be cropped according to the TOF range property")
        self.declareProperty("SpecularPixel", 180.0, doc="Pixel position of the specular reflectivity")
        self.declareProperty("QMin", 0.005, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("TOFSteps", 40.0, doc="TOF step size")
        self.declareProperty("EntryName", "entry-Off_Off", doc="Name of the entry to load")
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")

    #pylint: disable=too-many-locals,too-many-branches
    def PyExec(self):
        """ Main execution """
        # DATA
        dataRunNumbers = self.getProperty("RunNumbers").value
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # NORMALIZATION
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value

        # Get Q range
        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value
        if qStep > 0:  #force logarithmic binning
            qStep = -qStep

        # If we have multiple files, add them
        file_list = []
        for item in dataRunNumbers:
            # The standard mode of operation is to give a run number as input
            try:
                data_file = FileFinder.findRuns("%s%s" % (INSTRUMENT_NAME, item))[0]
            except RuntimeError:
                # Allow for a file name or file path as input
                data_file = FileFinder.findRuns(item)[0]
            file_list.append(data_file)
        runs = functools.reduce((lambda x, y: '%s+%s' % (x, y)), file_list)

        entry_name = self.getProperty("EntryName").value
        ws_event_data = LoadEventNexus(Filename=runs, NXentryName=entry_name,
                                       OutputWorkspace="%s_%s" % (INSTRUMENT_NAME, dataRunNumbers[0]))

        # Get the TOF range
        crop_TOF = self.getProperty("CutTOFRange").value
        tof_step = self.getProperty("TOFSteps").value
        if crop_TOF:
            TOFrange = self.getProperty("TOFRange").value  #microS
            if TOFrange[0] <= 0:
                TOFrange[0] = tof_step
                logger.error("Lower bound of TOF range cannot be zero: using %s" % tof_step)
        else:
            # If the TOF range option is turned off, use the full range
            # Protect against TOF=0, which will crash when going to Q.
            tof_min = ws_event_data.getTofMin()
            if tof_min <= 0:
                tof_min = tof_step
            tof_max = ws_event_data.getTofMax()
            TOFrange = [tof_min, tof_max]
            logger.information("Using TOF range: %g %g" % (tof_min, tof_max))

        # Number of pixels in each direction
        self.number_of_pixels_x = int(ws_event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        self.number_of_pixels_y = int(ws_event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(ws_event_data)
        two_theta_degrees = 2.0*theta*180.0/math.pi
        AddSampleLog(Workspace=ws_event_data, LogName='two_theta', LogText=str(two_theta_degrees),
                     LogType='Number', LogUnit='degree')

        # ----- Process Sample Data -------------------------------------------
        crop_request = self.getProperty("CutLowResDataAxis").value
        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        bck_request = self.getProperty("SubtractSignalBackground").value
        data_cropped = self.process_data(ws_event_data, TOFrange,
                                         crop_request, low_res_range,
                                         dataPeakRange, bck_request, dataBackRange)

        # ----- Normalization -------------------------------------------------
        perform_normalization = self.getProperty("ApplyNormalization").value
        if perform_normalization:
            # Load normalization
            ws_event_norm = LoadEventNexus("%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber),
                                           OutputWorkspace="%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber))
            crop_request = self.getProperty("CutLowResNormAxis").value
            low_res_range = self.getProperty("LowResNormAxisPixelRange").value
            bck_request = self.getProperty("SubtractNormBackground").value
            norm_cropped = self.process_data(ws_event_norm, TOFrange,
                                             crop_request, low_res_range,
                                             normPeakRange, bck_request, normBackRange)
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(ws_event_norm))

            # Sum up the normalization peak
            norm_summed = SumSpectra(InputWorkspace = norm_cropped)
            norm_summed = RebinToWorkspace(WorkspaceToRebin=norm_summed,
                                           WorkspaceToMatch=data_cropped,
                                           OutputWorkspace=str(norm_summed))

            # Sum up the normalization peak
            #norm_summed = SumSpectra(InputWorkspace = norm_cropped)

            # Normalize the data
            normalized_data = data_cropped / norm_summed
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(data_cropped))
            AnalysisDataService.remove(str(norm_cropped))
            AnalysisDataService.remove(str(norm_summed))
            AddSampleLog(Workspace=normalized_data, LogName='normalization_run', LogText=str(normalizationRunNumber))
        else:
            normalized_data = data_cropped
            AddSampleLog(Workspace=normalized_data, LogName='normalization_run', LogText="None")

        # At this point, the workspace should be considered a distribution of points
        normalized_data = ConvertToPointData(InputWorkspace=normalized_data,
                                             OutputWorkspace=str(normalized_data))
        normalized_data.setDistribution(True)

        q_workspace = SumSpectra(InputWorkspace = normalized_data)
        q_workspace.getAxis(0).setUnit("MomentumTransfer")

        # Get the distance fromthe moderator to the detector
        run_object = ws_event_data.getRun()
        sample_detector_distance = run_object['SampleDetDis'].getStatistics().mean / 1000.0
        source_sample_distance = run_object['ModeratorSamDis'].getStatistics().mean / 1000.0
        source_detector_distance = source_sample_distance + sample_detector_distance

        # Convert to Q
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * source_detector_distance / h * math.sin(theta)
        q_range = [qMin, qStep, constant / TOFrange[0] * 1.2]

        q_min_from_data = constant / TOFrange[1]
        q_max_from_data = constant / TOFrange[0]
        AddSampleLog(Workspace=q_workspace, LogName='q_min', LogText=str(q_min_from_data),
                     LogType='Number', LogUnit='1/Angstrom')
        AddSampleLog(Workspace=q_workspace, LogName='q_max', LogText=str(q_max_from_data),
                     LogType='Number', LogUnit='1/Angstrom')

        tof_to_lambda = 1.0e4 * h / (m * source_detector_distance)
        lambda_min = tof_to_lambda * TOFrange[0]
        lambda_max = tof_to_lambda * TOFrange[1]
        AddSampleLog(Workspace=q_workspace, LogName='lambda_min', LogText=str(lambda_min),
                     LogType='Number', LogUnit='Angstrom')
        AddSampleLog(Workspace=q_workspace, LogName='lambda_max', LogText=str(lambda_max),
                     LogType='Number', LogUnit='Angstrom')

        data_x = q_workspace.dataX(0)
        for i in range(len(data_x)):
            data_x[i] = constant / data_x[i]
        q_workspace = SortXAxis(InputWorkspace=q_workspace, OutputWorkspace=str(q_workspace))

        name_output_ws = self.getPropertyValue("OutputWorkspace")

        if q_range[2] < q_range[0]:
            logger.error("Bad Q range: %s ; determining range from data" % str(q_range))
            q_range = [q_min_from_data, qStep, q_max_from_data]
        q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range,
                        OutputWorkspace=name_output_ws)

        # Replace NaNs by zeros
        q_rebin = ReplaceSpecialValues(InputWorkspace=q_rebin,
                                       OutputWorkspace=name_output_ws,
                                       NaNValue=0.0, NaNError=0.0)
        # Crop to non-zero values
        data_y = q_rebin.readY(0)
        low_q = None
        high_q = None
        for i in range(len(data_y)):
            if low_q is None and abs(data_y[i])>0:
                low_q = i
            if high_q is None and abs(data_y[len(data_y)-1-i])>0:
                high_q = len(data_y)-1-i
            if low_q is not None and high_q is not None:
                break

        crop = self.getProperty("CropFirstAndLastPoints").value
        if low_q is not None and high_q is not None:
            # Get rid of first and last Q points to avoid edge effects
            if crop:
                low_q += 1
                high_q -= 1
            data_x = q_rebin.readX(0)
            q_rebin = CropWorkspace(InputWorkspace=q_rebin,
                                    OutputWorkspace=str(q_rebin),
                                    XMin=data_x[low_q], XMax=data_x[high_q])
        else:
            logger.error("Data is all zeros. Check your TOF ranges.")

        # Clean up the workspace for backward compatibility
        data_y = q_rebin.dataY(0)
        data_e = q_rebin.dataE(0)
        # Again for backward compatibility, the first and last points of the
        # raw output when not cropping was simply set to 0 += 1.
        if crop is False:
            data_y[0] = 0
            data_e[0] = 1
            data_y[len(data_y)-1] = 0
            data_e[len(data_y)-1] = 1
        # Values < 1e-12 and values where the error is greater than the value are replaced by 0+-1
        for i in range(len(data_y)):
            if data_y[i] < 1e-12 or data_e[i]>data_y[i]:
                data_y[i]=0.0
                data_e[i]=1.0

        # Sanity check
        if sum(data_y) == 0:
            raise RuntimeError("The reflectivity is all zeros: check your peak selection")

        # Avoid leaving trash behind
        for ws in ['ws_event_data', 'normalized_data', 'q_workspace']:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)

        self.setProperty('OutputWorkspace', mtd[name_output_ws])

    def calculate_scattering_angle(self, ws_event_data):
        """
            Compute the scattering angle
            @param ws_event_data: data workspace
        """
        run_object = ws_event_data.getRun()

        dangle = run_object['DANGLE'].getStatistics().mean
        dangle0 = run_object['DANGLE0'].getStatistics().mean
        det_distance = run_object['SampleDetDis'].getStatistics().mean / 1000.0
        direct_beam_pix = run_object['DIRPIX'].getStatistics().mean
        ref_pix = self.getProperty("SpecularPixel").value

        # Get pixel size from instrument properties
        if ws_event_data.getInstrument().hasParameter("pixel_width"):
            pixel_width = float(ws_event_data.getInstrument().getNumberParameter("pixel_width")[0])
        else:
            pixel_width = 0.0007

        #theta = (dangle - dangle0) * math.pi / 360.0
        theta = (dangle - dangle0) * math.pi / 180.0 / 2.0 + ((direct_beam_pix - ref_pix) * pixel_width) / (2.0 * det_distance)

        angle_offset = self.getProperty("AngleOffset").value
        return theta + angle_offset

    #pylint: disable=too-many-arguments
    def process_data(self, workspace, tof_range, crop_low_res, low_res_range,
                     peak_range, subtract_background, background_range):
        """
            Common processing for both sample data and normalization.
        """
        # Rebin TOF axis
        tof_max = workspace.getTofMax()
        tof_min = workspace.getTofMin()
        if tof_min > tof_range[1] or tof_max < tof_range[0]:
            error_msg = "Requested TOF range does not match data for %s: " % str(workspace)
            error_msg += "[%g, %g] found [%g, %g]" % (tof_range[0], tof_range[1],
                                                      tof_min, tof_max)
            raise RuntimeError(error_msg)

        tof_step = self.getProperty("TOFSteps").value
        workspace = Rebin(InputWorkspace=workspace, Params=[0, tof_step, tof_max],
                          PreserveEvents=True, OutputWorkspace="%s_histo" % str(workspace))

        # Crop TOF range
        workspace = CropWorkspace(InputWorkspace=workspace,
                                  XMin=tof_range[0], XMax=tof_range[1],
                                  OutputWorkspace=str(workspace))

        # Integrate over low resolution range
        low_res_min = 0
        low_res_max = self.number_of_pixels_y
        if crop_low_res:
            low_res_min = int(low_res_range[0])
            low_res_max = int(low_res_range[1])

        # Subtract background
        if subtract_background:
            average = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                             NXPixel=self.number_of_pixels_x,
                             NYPixel=self.number_of_pixels_y,
                             ConvertToQ=False,
                             XPixelMin=int(background_range[0]),
                             XPixelMax=int(background_range[1]),
                             YPixelMin=low_res_min,
                             YPixelMax=low_res_max,
                             ErrorWeighting = True,
                             SumPixels=True, NormalizeSum=True)

            signal = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                            NXPixel=self.number_of_pixels_x,
                            NYPixel=self.number_of_pixels_y,
                            ConvertToQ=False, YPixelMin=low_res_min, YPixelMax=low_res_max,
                            OutputWorkspace="signal_%s" % str(workspace))
            subtracted = Minus(LHSWorkspace=signal, RHSWorkspace=average,
                               OutputWorkspace="subtracted_%s" % str(workspace))
            AnalysisDataService.remove(str(average))
            AnalysisDataService.remove(str(signal))
        else:
            # If we don't subtract the background, we still have to integrate
            # over the low resolution axis
            subtracted = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                                NXPixel=self.number_of_pixels_x,
                                NYPixel=self.number_of_pixels_y,
                                ConvertToQ=False, YPixelMin=low_res_min, YPixelMax=low_res_max,
                                OutputWorkspace="subtracted_%s" % str(workspace))

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        subtracted = NormaliseByCurrent(InputWorkspace=subtracted, OutputWorkspace=str(subtracted))

        # Crop to only the selected peak region
        cropped = CropWorkspace(InputWorkspace=subtracted,
                                StartWorkspaceIndex=int(peak_range[0]),
                                EndWorkspaceIndex=int(peak_range[1]),
                                OutputWorkspace="%s_cropped" % str(subtracted))

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(workspace))
        AnalysisDataService.remove(str(subtracted))

        return cropped

AlgorithmFactory.subscribe(MagnetismReflectometryReduction)
