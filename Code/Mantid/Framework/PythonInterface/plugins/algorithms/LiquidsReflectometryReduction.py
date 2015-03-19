#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.simpleapi import *
import time
import math
import os
import numpy

# import sfCalculator
#TODO: Not sure what this is for. Remove.
import sys
sys.path.insert(0, os.path.dirname(__file__))
import sfCalculator
sys.path.pop(0)

from mantid.kernel import *

class LiquidsReflectometryReduction(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LiquidsReflectometryReduction"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer (REFL) reduction"

    def PyInit(self):
        self.declareProperty(IntArrayProperty("RunNumbers"), "List of run numbers to process")
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange"), "Pixel range defining the data peak")
        self.declareProperty("SubtractSignalBackground", True,
                             doc='If true, the background will be subtracted from the data peak')
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                                              "Pixelrange defining the background. Default:(123,137)")
        self.declareProperty("NormFlag", True, doc="If true, the data will be normalized")
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127, 133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                                              "Pixel range defining the normalization peak")
        self.declareProperty("SubtractNormBackground", True,
                             doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                                              "Pixel range defining the background for the normalization")
        self.declareProperty("LowResDataAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the data will be cropped according to the lowResDataAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                                              "Pixel range to use in the low resolution direction of the data")
        self.declareProperty("LowResNormAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped according to the LowResNormAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                                              "Pixel range to use in the low resolution direction of the normalizaion run")
        self.declareProperty(FloatArrayProperty("TOFRange", [9000., 23600.],
                                                FloatArrayLengthValidator(2), direction=Direction.Input),
                                                "TOF range to use")
        self.declareProperty("TofRangeFlag", True,
                             doc="If true, the TOF will be cropped according to the TOF range property")
        self.declareProperty("QMin", 0.05, doc="Mnimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, doc="Angle offset error (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")
        self.declareProperty("SlitsWidthFlag", True,
                             doc="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
        self.declareProperty("GeometryCorrectionFlag", False, doc="Use or not the geometry correction")
        self.declareProperty("FrontSlitName", "Si", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "S2", doc="Name of the back slit")

    def PyExec(self):
        from reduction.instruments.reflectometer import wks_utility
        # DATA
        dataRunNumbers = self.getProperty("RunNumbers").value
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # NORM
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value
        normBackFlag = self.getProperty("SubtractNormBackground").value

        #GENERAL
        #TODO: Why are there two versions of this?
        TOFrangeFlag = self.getProperty("TofRangeFlag")
        if TOFrangeFlag:
            TOFrange = self.getProperty("TOFRange").value  #microS
        else:
            TOFrange = [0, 200000]
        # TOF binning parameters
        binTOFrange = [0, 200000]
        binTOFsteps = 40
        # Steps of 400 are easier to develop with
        #binTOFsteps = 400

        # geometry correction
        geometryCorrectionFlag = self.getProperty("GeometryCorrectionFlag").value

        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value
        if qStep > 0:  #force logarithmic binning
            qStep = -qStep

        # sfCalculator settings
        slitsValuePrecision = sfCalculator.PRECISION
        sfFile = self.getProperty("ScalingFactorFile").value

        incidentMedium = self.getProperty("IncidentMediumSelected").value
        slitsWidthFlag = self.getProperty("SlitsWidthFlag").value

        # If we have multiple files, add them
        file_list = []
        for item in dataRunNumbers:
            data_file = FileFinder.findRuns("REF_L%d" % item)[0]
            file_list.append(data_file)
        runs = reduce((lambda x, y: '%s+%s' % (x, y)), file_list)
        ws_event_data = Load(Filename=runs)

        # Rebin data
        ws_histo_data = Rebin(InputWorkspace=ws_event_data,
                              Params=[binTOFrange[0], binTOFsteps, binTOFrange[1]],
                              PreserveEvents=False)

        # Calculate the central pixel (using weighted average)
        ws_integrated = Integration(InputWorkspace=ws_histo_data)

        # Number of pixels in each direction
        #TODO: revisit this when we update the IDF
        self.number_of_pixels_x = int(ws_event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        self.number_of_pixels_y = int(ws_event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])
        ws_roi = RefRoi(InputWorkspace=ws_integrated, IntegrateY=False,
                        NXPixel=self.number_of_pixels_x, NYPixel=self.number_of_pixels_y,
                        ConvertToQ=False)
        ws_center_peak = Transpose(InputWorkspace=ws_roi)
        counts = ws_center_peak.readY(0)
        bins = numpy.arange(len(counts))
        summed = bins * counts
        data_central_pixel = sum(summed) / sum(counts)

        # get the distance moderator-detector and sample-detector
        #TODO: get rid of this
        [dMD, dSD] = wks_utility.getDistances(ws_event_data)

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(ws_event_data)

        # Slit size
        front_slit = self.getProperty("FrontSlitName").value
        back_slit = self.getProperty("BackSlitName").value

        first_slit_size = ws_event_data.getRun().getProperty("%sVHeight" % front_slit).value
        last_slit_size = ws_event_data.getRun().getProperty("%sVHeight" % back_slit).value

        # Crop TOF range
        ws_histo_data = CropWorkspace(InputWorkspace=ws_histo_data,
                                      XMin=TOFrange[0], XMax=TOFrange[1])

        # Q range
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * dMD / h * math.sin(theta)
        q_range = [qMin, qStep, constant / TOFrange[0] * 1.2]

        # Normalize by current proton charge
        ws_histo_data = NormaliseByCurrent(InputWorkspace=ws_histo_data)

        # Integrate over low resolution range
        #TODO: revisit this once we convert everything to working with workspaces
        #TODO: RefRoi excludes the max pixel value. Fix that.
        dataLowResFlag = self.getProperty("LowResDataAxisPixelRangeFlag")
        if dataLowResFlag:
            dataLowResRange = self.getProperty("LowResDataAxisPixelRange").value
            roi = RefRoi(InputWorkspace=ws_histo_data, IntegrateY=False,
                         NXPixel=self.number_of_pixels_x, NYPixel=self.number_of_pixels_y,
                         ConvertToQ=False, XPixelMin=int(dataLowResRange[0]),
                         XPixelMax=int(dataLowResRange[1]) + 1)
            data_y_axis = roi.extractY()
            tof_axis_full = roi.readX(0)
            data_y_error_axis = roi.extractE()
        else:
            data_y_axis = ws_histo_data.extractY()
            tof_axis_full = ws_histo_data.readX(0)
            data_y_error_axis = ws_histo_data.extractE()
        

        # Subtract background
        #TODO: we should remove the background before normalizing by the proton charge
        # so that we don't have to replace the error on zeros by 1/C but can simply use 1.
        bck_request = self.getProperty("SubtractSignalBackground").value
        if bck_request:

            ws_histo_data2 = self.subtract_background(ws_histo_data, dataPeakRange, dataBackRange)

            # Extract the data array so that we can proceed with the old code.
            #TODO: remove this
            data_y_axis2 = ws_histo_data2.extractY()
            data_y_error_axis2 = ws_histo_data2.extractE()

            # Need to extract the peak pixels to be compatible with old method
            # Offset to match old code range
            offset = -1
            data_y_axis = data_y_axis2[int(dataPeakRange[0]) + offset:int(dataPeakRange[1]) + 1 + offset]
            data_y_error_axis = data_y_error_axis2[int(dataPeakRange[0]) + offset:int(dataPeakRange[1]) + 1 + offset]

#            # Get proton charge
#            proton_charge = ws_event_data.getRun().getProperty('gd_prtn_chrg').value
#            error_0 = 1. / proton_charge
#            tof_axis = tof_axis_full[0:-1]
#            [data_y_axis, data_y_error_axis] = wks_utility.substractBackground(tof_axis ,
#                                                                               data_y_axis,
#                                                                               data_y_error_axis,
#                                                                               dataPeakRange,
#                                                                               bck_request,
#                                                                               dataBackRange,
#                                                                               error_0,
#                                                                               'data')
        # ----- Normalization ----------------------------------------------
        # Load normalization
        ws_event_norm = LoadEventNexus("REF_L_%s" % normalizationRunNumber)

        # Rebin normalization
        ws_histo_norm = Rebin(InputWorkspace=ws_event_norm,
                              Params=[binTOFrange[0], binTOFsteps, binTOFrange[1]],
                              PreserveEvents=False)

        # Crop TOF range
        ws_histo_norm = CropWorkspace(InputWorkspace=ws_histo_norm,
                                      XMin=TOFrange[0], XMax=TOFrange[1])

        # Normalize by current proton charge
        ws_histo_norm = NormaliseByCurrent(InputWorkspace=ws_histo_norm)

        # Integrate over low resolution range
        normLowResFlag = self.getProperty("LowResNormAxisPixelRangeFlag")
        if normLowResFlag:
            normLowResRange = self.getProperty("LowResNormAxisPixelRange").value
        [norm_tof_axis, norm_y_axis, norm_y_error_axis] = wks_utility.integrateOverLowResRange(ws_histo_norm, \
                                                            normLowResRange, \
                                                            'normalization', \
                                                            True)

        # Subtract background
        # get proton charge
        proton_charge = ws_event_norm.getRun().getProperty('gd_prtn_chrg').value
        error_0 = 1. / proton_charge

        [norm_y_axis, norm_y_error_axis] = wks_utility.substractBackground(norm_tof_axis[0:-1], \
                                                        norm_y_axis, \
                                                        norm_y_error_axis, \
                                                        normPeakRange, \
                                                        normBackFlag, \
                                                        normBackRange, \
                                                        error_0, \
                                                        'normalization')

        [av_norm, av_norm_error] = wks_utility.fullSumWithError(norm_y_axis, \
                                                           norm_y_error_axis)


        [final_data_y_axis, final_data_y_error_axis] = wks_utility.divideDataByNormalization(data_y_axis,
                                                                                             data_y_error_axis,
                                                                                             av_norm,
                                                                                             av_norm_error)

        # apply Scaling factor
        [tof_axis_full, y_axis, y_error_axis, isSFfound] = wks_utility.applyScalingFactor(tof_axis_full,
                                                                               final_data_y_axis,
                                                                               final_data_y_error_axis,
                                                                               incidentMedium,
                                                                               sfFile,
                                                                               slitsValuePrecision,
                                                                               slitsWidthFlag)


        if geometryCorrectionFlag:  # convert To Q with correction
            [q_axis, y_axis, y_error_axis] = wks_utility.convertToQ(tof_axis_full,
                                                                    y_axis,
                                                                    y_error_axis,
                                                                    peak_range=dataPeakRange,
                                                                    central_pixel=data_central_pixel,
                                                                    source_to_detector_distance=dMD,
                                                                    sample_to_detector_distance=dSD,
                                                                    theta=theta,
                                                                    first_slit_size=first_slit_size,
                                                                    last_slit_size=last_slit_size)

        else:  # convert to Q without correction

            [q_axis, y_axis, y_error_axis] = wks_utility.convertToQWithoutCorrection(tof_axis_full,
                                                                                     y_axis,
                                                                                     y_error_axis,
                                                                                     peak_range=dataPeakRange,
                                                                                     source_to_detector_distance=dMD,
                                                                                     sample_to_detector_distance=dSD,
                                                                                     theta=theta,
                                                                                     first_slit_size=first_slit_size,
                                                                                     last_slit_size=last_slit_size)

        sz = q_axis.shape

        # create workspace
        q_workspace = wks_utility.createQworkspace(q_axis, y_axis, y_error_axis)

        q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range, PreserveEvents=True)

        # keep only the q values that have non zero counts
        nonzero_q_rebin_wks = wks_utility.cropAxisToOnlyNonzeroElements(q_rebin,
                                                                        dataPeakRange)

        # integrate spectra (normal mean) and remove first and last Q value
        [final_x_axis, final_y_axis, final_error_axis] = wks_utility.integrateOverPeakRange(nonzero_q_rebin_wks, dataPeakRange)


        # cleanup data
        [final_y_axis, final_y_error_axis] = wks_utility.cleanupData1D(final_y_axis, final_error_axis)


        # create final workspace
        _time = int(time.time())
        name_output_ws = self.getPropertyValue("OutputWorkspace")
        name_output_ws = name_output_ws + '_#' + str(_time) + 'ts'
        final_workspace = wks_utility.createFinalWorkspace(final_x_axis,
                                                           final_y_axis,
                                                           final_y_error_axis,
                                                           name_output_ws,
                                                           ws_event_data)
        AddSampleLog(Workspace=name_output_ws,
                     LogName='isSFfound',
                     LogText=str(isSFfound))

        self.setProperty('OutputWorkspace', mtd[name_output_ws])


    def calculate_scattering_angle(self, ws_event_data):
        """
            Compute the scattering angle
            @param ws_event_data: data workspace
        """
        run_object = ws_event_data.getRun()
        thi_value = run_object.getProperty('thi').value[0]
        thi_units = run_object.getProperty('thi').units
        tthd_value = run_object.getProperty('tthd').value[0]
        tthd_units = run_object.getProperty('tthd').units

        # Make sure we have radians
        if thi_units == 'degree':
            thi_value *= math.pi / 180.0
        if tthd_units == 'degree':
            tthd_value *= math.pi / 180.0

        theta = math.fabs(tthd_value - thi_value) / 2.

        # Add the offset
        angle_offset_deg = self.getProperty("AngleOffset").value
        return theta + angle_offset_deg * math.pi / 180.0

    def clocking_correction(self, workspace, pixel_range, range_width=3):
        """
            Applies the "clocking correction". The pixel range is
            the range that contains the reflectivity data. Compute the
            average noise per pixel over two small bands on each side.
            The subtract that noise pixel-wise from the data
        """
        pass

    def subtract_background(self, workspace, peak_range, background_range):
        """
            Subtract background in place
            #TODO: RefRoi needs to do error weighting and deal with zeros
        """
        offset = -1
        peak_min = int(peak_range[0]) + offset
        peak_max = int(peak_range[1]) + offset
        bck_min = int(background_range[0]) + offset
        bck_max = int(background_range[1]) + offset

        if peak_range[0] > background_range[0] and peak_range[1] < background_range[1]:
            left_bck = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                              NXPixel=self.number_of_pixels_x,
                              NYPixel=self.number_of_pixels_y,
                              ConvertToQ=False,
                              YPixelMin=bck_min,
                              YPixelMax=peak_min - 1,
                              SumPixels=True, NormalizeSum=True)

            right_bck = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                               NXPixel=self.number_of_pixels_x,
                               NYPixel=self.number_of_pixels_y,
                               ConvertToQ=False,
                               YPixelMin=peak_max + 1,
                               YPixelMax=bck_max,
                               SumPixels=True, NormalizeSum=True)
            average = (left_bck + right_bck) / 2.0
        else:
            average = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                             NXPixel=self.number_of_pixels_x,
                             NYPixel=self.number_of_pixels_y,
                             ConvertToQ=False,
                             YPixelMin=bck_min,
                             YPixelMax=bck_max,
                             SumPixels=True, NormalizeSum=True)
        # Integrate over the low-res direction
        workspace = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                           NXPixel=self.number_of_pixels_x,
                           NYPixel=self.number_of_pixels_y,
                           ConvertToQ=False)
        workspace = Minus(LHSWorkspace=workspace, RHSWorkspace=average)
        return workspace

AlgorithmFactory.subscribe(LiquidsReflectometryReduction)
