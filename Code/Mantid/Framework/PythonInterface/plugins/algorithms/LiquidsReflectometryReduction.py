#pylint: disable=no-init,invalid-name
import time
import math
import os
from mantid.api import *
from mantid.simpleapi import *
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
        self.declareProperty("QMin", 0.05, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, doc="Angle offset error (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")
        self.declareProperty("SlitsWidthFlag", True,
                             doc="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
        self.declareProperty("GeometryCorrectionFlag", False, doc="Use or not the geometry correction")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "Si", doc="Name of the back slit")
        self.declareProperty("TOFSteps", 40.0, doc="TOF step size")
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")

    def PyExec(self):
        # The old reflectivity reduction has an offset between the input
        # pixel numbers and what it actually uses. Set the offset to zero
        # to turn it off.
        self.LEGACY_OFFSET = -1

        # The old reduction code had a tolerance value for matching the
        # slit parameters to get the scaling factors
        self.TOLERANCE = 0.020

        # DATA
        dataRunNumbers = self.getProperty("RunNumbers").value
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # NORM
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
            data_file = FileFinder.findRuns("REF_L%d" % item)[0]
            file_list.append(data_file)
        runs = reduce((lambda x, y: '%s+%s' % (x, y)), file_list)
        ws_event_data = Load(Filename=runs)

        # Get the TOF range
        TOFrangeFlag = self.getProperty("TofRangeFlag")
        if TOFrangeFlag:
            TOFrange = self.getProperty("TOFRange").value  #microS
        else:
            tof_max = ws_event_data.getTofMax()
            TOFrange = [0, tof_max]

        # Number of pixels in each direction
        #TODO: revisit this when we update the IDF
        self.number_of_pixels_x = int(ws_event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        self.number_of_pixels_y = int(ws_event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(ws_event_data)

        # ----- Process Sample Data -------------------------------------------
        crop_request = self.getProperty("LowResDataAxisPixelRangeFlag")
        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        bck_request = self.getProperty("SubtractSignalBackground").value
        data_cropped = self.process_data(ws_event_data, TOFrange,
                                         crop_request, low_res_range,
                                         dataPeakRange, bck_request, dataBackRange)

        # ----- Normalization -------------------------------------------------
        # Load normalization
        ws_event_norm = LoadEventNexus("REF_L_%s" % normalizationRunNumber,
                                       OutputWorkspace="REF_L_%s" % normalizationRunNumber)
        crop_request = self.getProperty("LowResNormAxisPixelRangeFlag")
        low_res_range = self.getProperty("LowResNormAxisPixelRange").value
        bck_request = self.getProperty("SubtractNormBackground").value
        norm_cropped = self.process_data(ws_event_norm, TOFrange,
                                         crop_request, low_res_range,
                                         normPeakRange, bck_request, normBackRange)
        # Avoid leaving trash behind
        AnalysisDataService.remove(str(ws_event_norm))

        # Sum up the normalization peak
        norm_summed = SumSpectra(InputWorkspace = norm_cropped)

        # Normalize the data
        normalized_data = data_cropped / norm_summed
        # Avoid leaving trash behind
        AnalysisDataService.remove(str(data_cropped))
        AnalysisDataService.remove(str(norm_summed))

        normalized_data = ConvertToPointData(InputWorkspace=normalized_data,
                                             OutputWorkspace=str(normalized_data))

        # Apply scaling factors
        normalized_data = self.apply_scaling_factor(normalized_data)

        q_workspace = SumSpectra(InputWorkspace = normalized_data)
        q_workspace.getAxis(0).setUnit("MomentumTransfer")

        # Geometry correction to convert To Q with correction
        geometryCorrectionFlag = self.getProperty("GeometryCorrectionFlag").value
        if geometryCorrectionFlag:
            logger.error("The geometry correction for the Q conversion has not been implemented.")

        # Get the distance fromthe moderator to the detector
        sample = ws_event_data.getInstrument().getSample()
        source = ws_event_data.getInstrument().getSource()
        source_sample_distance = sample.getDistance(source)
        detector = ws_event_data.getDetector(0)
        sample_detector_distance = detector.getPos().getZ()
        source_detector_distance = source_sample_distance + sample_detector_distance

        # Convert to Q
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * source_detector_distance / h * math.sin(theta)
        q_range = [qMin, qStep, constant / TOFrange[0] * 1.2]

        data_x = q_workspace.dataX(0)
        for i in range(len(data_x)):
            data_x[i] = constant / data_x[i]
        q_workspace = SortXAxis(InputWorkspace=q_workspace, OutputWorkspace=str(q_workspace))

        # Cook up a name compatible with the UI for backward compatibility
        _time = int(time.time())
        name_output_ws = self.getPropertyValue("OutputWorkspace")
        name_output_ws = name_output_ws + '_#' + str(_time) + 'ts'

        q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range,
                        OutputWorkspace=name_output_ws)

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

        #TODO: remove this, which we use during development to make sure we don't leave trash
        logger.information(str(AnalysisDataService.getObjectNames()))

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

    def subtract_background(self, workspace, peak_range, background_range,
                            low_res_range, sum_peak=False, offset=None):
        """
            Subtract background in place
            #TODO: RefRoi needs to do error weighting and deal with zeros
            @param workspace: Mantid workspace
            @param peak_range: range of pixels defining the peak [min, max]
            @param background_range: range of pixels defining the background [min, max]
            @param low_res_range: low resolution range to integrate over
            @param sum_peak: if True, the resulting peak will be summed
        """
        if offset is None:
            offset = self.LEGACY_OFFSET
        peak_min = int(peak_range[0]) + offset
        peak_max = int(peak_range[1]) + offset
        bck_min = int(background_range[0]) + offset
        bck_max = int(background_range[1]) + offset

        # Get low-resolution range
        x_min = int(low_res_range[0]) + offset
        x_max = int(low_res_range[1]) + offset

        left_bck = None
        if peak_min > bck_min:
            left_bck = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                              NXPixel=self.number_of_pixels_x,
                              NYPixel=self.number_of_pixels_y,
                              ConvertToQ=False,
                              XPixelMin=x_min,
                              XPixelMax=x_max,
                              YPixelMin=bck_min,
                              YPixelMax=peak_min - 1,
                              ErrorWeighting = True,
                              SumPixels=True, NormalizeSum=True)

        right_bck = None
        if peak_max < bck_max:
            right_bck = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                               NXPixel=self.number_of_pixels_x,
                               NYPixel=self.number_of_pixels_y,
                               ConvertToQ=False,
                               XPixelMin=x_min,
                               XPixelMax=x_max,
                               YPixelMin=peak_max + 1,
                               YPixelMax=bck_max,
                               ErrorWeighting = True,
                               SumPixels=True, NormalizeSum=True)
            
        if right_bck is not None and left_bck is not None:
            average = (left_bck + right_bck) / 2.0
        elif right_bck is not None:
            average = right_bck
        elif left_bck is not None:
            average = left_bck
        else:
            average = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                             NXPixel=self.number_of_pixels_x,
                             NYPixel=self.number_of_pixels_y,
                             ConvertToQ=False,
                             XPixelMin=x_min,
                             XPixelMax=x_max,
                             YPixelMin=bck_min,
                             YPixelMax=bck_max,
                             ErrorWeighting = True,
                             SumPixels=True, NormalizeSum=True)
        # Integrate over the low-res direction
        workspace = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                           NXPixel=self.number_of_pixels_x,
                           NYPixel=self.number_of_pixels_y,
                           XPixelMin=x_min,
                           XPixelMax=x_max,
                           ConvertToQ=False,
                           SumPixels=sum_peak,
                           OutputWorkspace=str(workspace))
        #TODO Check whether we should multiply by the number of pixels
        # in the low-res direction
        workspace = Minus(LHSWorkspace=workspace, RHSWorkspace=average,
                          OutputWorkspace=str(workspace))
        # Avoid leaving trash behind
        average_name = str(average)
        if AnalysisDataService.doesExist(str(left_bck)):
            AnalysisDataService.remove(str(left_bck))
        if AnalysisDataService.doesExist(str(right_bck)):
            AnalysisDataService.remove(str(right_bck))
        if AnalysisDataService.doesExist(average_name):
            AnalysisDataService.remove(average_name)
        return workspace

    def process_data(self, workspace, tof_range, crop_low_res, low_res_range,
                     peak_range, subtract_background, background_range):
        """
            Common processing for both sample data and normalization.
        """
        # Rebin TOF axis
        tof_max = workspace.getTofMax()
        tof_step = self.getProperty("TOFSteps").value
        workspace = Rebin(InputWorkspace=workspace, Params=[0, tof_step, tof_max], 
                          PreserveEvents=False, OutputWorkspace="%s_histo" % str(workspace))

        # Crop TOF range
        workspace = CropWorkspace(InputWorkspace=workspace,
                                  XMin=tof_range[0], XMax=tof_range[1],
                                  OutputWorkspace=str(workspace))

        # Integrate over low resolution range
        x_min = 0
        x_max = self.number_of_pixels_x
        if crop_low_res:
            x_min = int(low_res_range[0])
            x_max = int(low_res_range[1])

        # Subtract background
        if subtract_background:
            workspace = self.subtract_background(workspace,
                                                 peak_range, background_range,
                                                 [x_min, x_max])
        else:
            # If we don't subtract the background, we still have to integrate
            # over the low resolution axis
            workspace = RefRoi(InputWorkspace=workspace, IntegrateY=False,
                               NXPixel=self.number_of_pixels_x,
                               NYPixel=self.number_of_pixels_y,
                               ConvertToQ=False, XPixelMin=x_min, XPixelMax=x_max,
                               OutputWorkspace=str(workspace))

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        workspace = NormaliseByCurrent(InputWorkspace=workspace, OutputWorkspace=str(workspace))

        # Crop to only the selected peak region
        cropped = CropWorkspace(InputWorkspace = workspace,
                                StartWorkspaceIndex=int(peak_range[0]) + self.LEGACY_OFFSET,
                                EndWorkspaceIndex=int(peak_range[1]) + self.LEGACY_OFFSET,
                                OutputWorkspace="%s_cropped" % str(workspace))

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(workspace))

        return cropped

    def apply_scaling_factor(self, workspace):
        """
            Apply scaling factor from reference scaling data
            @param workspace: Mantid workspace
        """
        scaling_factor_file = self.getProperty("ScalingFactorFile").value
        if not os.path.isfile(scaling_factor_file):
            scaling_factor_files = FileFinder.findRuns(scaling_factor_file)
            if len(scaling_factor_files)>0:
                scaling_factor_file = scaling_factor_files[0]
                if not os.path.isfile(scaling_factor_file):
                    logger.error("Could not find scaling factor file %s" % scaling_factor_file)
                    return
            else:
                logger.error("Could not find scaling factor file %s" % scaling_factor_file)
                return

        # Get the incident medium
        incident_medium = self.getProperty("IncidentMediumSelected").value
        
        # Get the wavelength
        lr = workspace.getRun().getProperty('LambdaRequest').value[0]
        lr_value = float("{0:.2f}".format(lr))

        # Get the slit information
        front_slit = self.getProperty("FrontSlitName").value
        back_slit = self.getProperty("BackSlitName").value

        # Option to match slit widths or not
        match_slit_width = self.getProperty("SlitsWidthFlag").value

        s1h = abs(workspace.getRun().getProperty("%sVHeight" % front_slit).value[0])
        s1w = abs(workspace.getRun().getProperty("%sHWidth" % front_slit).value[0])
        try:
            s2h = abs(workspace.getRun().getProperty("%sVHeight" % back_slit).value[0])
            s2w = abs(workspace.getRun().getProperty("%sHWidth" % back_slit).value[0])
        except:
            # For backward compatibility with old code
            logger.error("Specified slit could not be found: %s  Trying S2" % back_slit)
            s2h = abs(workspace.getRun().getProperty("S2VHeight").value[0])
            s2w = abs(workspace.getRun().getProperty("S2HWidth").value[0])

        scaling_info = "Scaling settings: %s wl=%s S1H=%s S2H=%s" % (incident_medium,
                                                                     lr_value, s1h, s2h)
        if match_slit_width:
            scaling_info += " S1W=%s S2W=%s" % (s1w, s2w)
        logger.information(scaling_info)

        def _reduce(accumulation, item):
            """
                Reduce function that accumulates values in a dictionary
            """
            toks_item = item.split('=')
            if len(toks_item)!=2:
                return accumulation
            if type(accumulation)==dict:
                accumulation[toks_item[0].strip()] = toks_item[1].strip()
            else:
                toks_accum = accumulation.split('=')
                accumulation = {toks_item[0].strip(): toks_item[1].strip(),
                                toks_accum[0].strip(): toks_accum[1].strip()}
            return accumulation

        def _value_check(key, data, reference):
            """
                Check an entry against a reference value
                #TODO: get rid of sfCalculator reference
            """
            if key in data:
                return abs(abs(float(data[key])) - abs(float(reference))) <= self.TOLERANCE
            return False

        scaling_data = open(scaling_factor_file, 'r')
        file_content = scaling_data.read()
        scaling_data.close()
        
        data_found = None
        for line in file_content.split('\n'):
            if line.startswith('#'):
                continue

            # Parse the line of data and produce a dict
            toks = line.split()
            data_dict = reduce(_reduce, toks, {})

            if 'IncidentMedium' in data_dict \
                and data_dict['IncidentMedium'] == incident_medium.strip() \
                and _value_check('LambdaRequested', data_dict, lr_value) \
                and _value_check('S1H', data_dict, s1h) \
                and _value_check('S2H', data_dict, s2h):

                if not match_slit_width or (_value_check('S1W', data_dict, s1w) \
                        and _value_check('S2W', data_dict, s2w)):
                    data_found = data_dict
                    break

        AddSampleLog(Workspace=workspace, LogName='isSFfound', LogText=str(data_found is not None))
        if data_found is not None:
            a = float(data_found['a'])
            b = float(data_found['b'])
            a_error = float(data_found['error_a'])
            b_error = float(data_found['error_b'])

            # Extract a single spectrum, just so we have the TOF axis
            # to create a normalization workspace
            normalization = ExtractSingleSpectrum(InputWorkspace=workspace,
                                                  OutputWorkspace="normalization",
                                                  WorkspaceIndex=0)
            norm_tof = normalization.dataX(0)
            norm_value = normalization.dataY(0)
            norm_error = normalization.dataE(0)
            #TODO: The following is done on the bin edges.
            # Should it not be done for the center of the bin?
            for i in range(len(norm_value)):
                norm_value[i] = norm_tof[i] * b + a
                norm_error[i] = math.sqrt(a_error*a_error + norm_tof[i] * norm_tof[i] * b_error * b_error)

            workspace = Divide(LHSWorkspace=workspace,
                               RHSWorkspace=normalization,
                               OutputWorkspace=str(workspace))
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(normalization))
        else:
            logger.error("Could not find scaling factor")
        return workspace


AlgorithmFactory.subscribe(LiquidsReflectometryReduction)
