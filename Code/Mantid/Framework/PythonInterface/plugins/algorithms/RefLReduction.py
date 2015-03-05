#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.simpleapi import *

# import sfCalculator
import sys
import os
sys.path.insert(0,os.path.dirname(__file__))
import sfCalculator
sys.path.pop(0)

from mantid.kernel import *

class RefLReduction(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "RefLReduction"

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
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127,133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the normalization peak")
        self.declareProperty("SubtractNormBackground", True,
                             doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127,137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background for the normalization")
        self.declareProperty("LowResDataAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the data will be cropped according to the lowResDataAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115,210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the data")
        self.declareProperty("LowResNormAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped according to the LowResNormAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115,210],
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
#         self.declareProperty("", True,
#                              doc="Use Scaling Factor configuration file")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")
        self.declareProperty("SlitsWidthFlag", True,
                             doc="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
        self.declareProperty("GeometryCorrectionFlag", False, doc="Use or not the geometry correction")

    def PyExec(self):

        print '-- > starting new Reflectometer Reduction ...'

        import os
        import numpy
        import math
        from reduction.instruments.reflectometer import wks_utility

        from mantid import mtd
        #remove all previous workspaces
        list_mt = mtd.getObjectNames()
        for _mt in list_mt:
            if _mt.find('_scaled') != -1:
                DeleteWorkspace(_mt)
            if _mt.find('_reflectivity') != -1:
                DeleteWorkspace(_mt)

        # retrieve settings from GUI
        print '-> Retrieving settings from GUI'

        #print 'RunNumbers: '  + str(self.getProperty("RunNumbers").value)
        #print 'NormalizationRunNumber: ' + str(self.getProperty("NormalizationRunNumber").value)
        #print 'SignalPeakPixelRange: ' + str(self.getProperty("SignalPeakPixelRange").value)
        #print 'SubtractSignalBackground: ' + str(self.getProperty("SubtractSignalBackground").value)
        #print 'SignalBackgroundPixelRange: ' + str(self.getProperty("SignalBackgroundPixelRange").value)
        #print "NormFlag: " + str(self.getProperty("NormFlag").value)
        #print "NormPeakPixelRange: " + str(self.getProperty("NormPeakPixelRange").value)
        #print "NormBackgroundPixelRange: " + str(self.getProperty("NormBackgroundPixelRange").value)
        #print "SubtractNormBackground: " + str(self.getProperty("SubtractNormBackground").value)
        #print "LowResDataAxisPixelRangeFlag: " + str(self.getProperty("LowResDataAxisPixelRangeFlag").value)
        #print "LowResDataAxisPixelRange: " + str(self.getProperty("LowResDataAxisPixelRange").value)
        #print "LowResNormAxisPixelRangeFlag: " + str(self.getProperty("LowResNormAxisPixelRangeFlag").value)
        #print "LowResNormAxisPixelRange: " + str(self.getProperty("LowResNormAxisPixelRange").value)
        #print "TOFRange: " + str(self.getProperty("TOFRange").value)
        #print "IncidentMediumSelected: " + str(self.getProperty("incidentMediumSelected").value)
        #print "GeometryCorrectionFlag: " + str(self.getProperty("GeometryCorrectionFlag").value)
        #print "QMin: " + str(self.getProperty("QMin").value)
        #print "QStep: " + str(self.getProperty("QStep").value)
        #print "ScalingFactorFile: " + str(self.getProperty("ScalingFactorFile").value)
        #print "SlitsWidthFlag: " + str(self.getProperty("SlitsWidthFlag").value)
        #print "OutputWorkspace: " + str(self.getProperty("OutputWorkspace").value)

        # DATA
        dataRunNumbers = self.getProperty("RunNumbers").value
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value
        dataBackFlag = self.getProperty("SubtractSignalBackground").value
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        dataLowResFlag = self.getProperty("LowResDataAxisPixelRangeFlag")
        if dataLowResFlag:
            dataLowResRange = self.getProperty("LowResDataAxisPixelRange").value
        else:
            dataLowResRange = [0,maxX-1]

        # NORM
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        normFlag = self.getProperty("NormFlag")
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value
        normBackFlag = self.getProperty("SubtractNormBackground").value
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        normLowResFlag = self.getProperty("LowResNormAxisPixelRangeFlag")
        if normLowResFlag:
            normLowResRange = self.getProperty("LowResNormAxisPixelRange").value
        else:
            normLowResRange = [0,maxX-1]

        #GENERAL
        TOFrangeFlag = self.getProperty("TofRangeFlag")
        if TOFrangeFlag:
            TOFrange = self.getProperty("TOFRange").value #microS
        else:
            TOFrange = [0, 200000]
        # TOF binning parameters
        binTOFrange = [0, 200000]
        binTOFsteps = 40

        # geometry correction
        geometryCorrectionFlag = self.getProperty("GeometryCorrectionFlag").value

        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value
        if qStep > 0: #force logarithmic binning
            qStep = -qStep

        # angle offset
        angleOffsetDeg = self.getProperty("AngleOffset").value

        h = 6.626e-34  #m^2 kg s^-1
        m = 1.675e-27     #kg

        # sfCalculator settings
        slitsValuePrecision = sfCalculator.PRECISION
        sfFile = self.getProperty("ScalingFactorFile").value

        incidentMedium = self.getProperty("IncidentMediumSelected").value
        slitsWidthFlag = self.getProperty("SlitsWidthFlag").value
        # ==== done retrievin the settings =====

        # ==== start reduction ====

        # work with data
        # load data
        ws_event_data = wks_utility.loadNeXus(dataRunNumbers, 'data')

        is_nexus_detector_rotated_flag = wks_utility.isNexusTakeAfterRefDate(ws_event_data.getRun().getProperty('run_start').value)
        print '-> is NeXus taken with new detector geometry: ' + str(is_nexus_detector_rotated_flag)

        #dimension of the detector (256 by 304 pixels)
        if is_nexus_detector_rotated_flag:
            maxX = 256
            maxY = 304
        else:
            maxX = 304
            maxY = 256

        ## retrieve general informations
        # calculate the central pixel (using weighted average)
        print '-> retrieving general informations'
        data_central_pixel = wks_utility.getCentralPixel(ws_event_data,
                                                         dataPeakRange,
                                                         is_nexus_detector_rotated_flag)
        # get the distance moderator-detector and sample-detector
        [dMD, dSD] = wks_utility.getDistances(ws_event_data)
        # get theta
        theta = wks_utility.getTheta(ws_event_data, angleOffsetDeg)
        # get proton charge
        pc = wks_utility.getProtonCharge(ws_event_data)
        error_0 = 1. / pc

        # rebin data
        ws_histo_data = wks_utility.rebinNeXus(ws_event_data,\
                              [binTOFrange[0], binTOFsteps, binTOFrange[1]],\
                              'data')

        # get q range
        q_range = wks_utility.getQrange(ws_histo_data, theta, dMD, qMin, qStep)

        # slit size
        [first_slit_size, last_slit_size] = wks_utility.getSlitsSize(ws_histo_data)

        # keep only TOF range
        ws_histo_data = wks_utility.cropTOF(ws_histo_data,\
                                      TOFrange[0],\
                                      TOFrange[1],\
                                      'data')

        # normalize by current proton charge
        ws_histo_data = wks_utility.normalizeNeXus(ws_histo_data, 'data')

        # integrate over low resolution range
        [data_tof_axis, data_y_axis, data_y_error_axis] = wks_utility.integrateOverLowResRange(ws_histo_data,\
                                                            dataLowResRange,\
                                                            'data',\
                                                            is_nexus_detector_rotated_flag)

#        #DEBUG ONLY
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/data_file_after_low_resolution_integration.txt',
#                                         data_tof_axis,
#                                         data_y_axis,
#                                         data_y_error_axis)

        tof_axis = data_tof_axis[0:-1].copy()
        tof_axis_full = data_tof_axis.copy()

        # data_tof_axis.shape -> (62,)
        # data_y_axis.shape -> (256,61)

        #substract background
        [data_y_axis, data_y_error_axis] = wks_utility.substractBackground(tof_axis ,
                                                                           data_y_axis,
                                                                           data_y_error_axis,
                                                                           dataPeakRange,
                                                                           dataBackFlag,
                                                                           dataBackRange,
                                                                           error_0,
                                                                           'data')
#        #DEBUG ONLY
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/data_file_back_sub_not_integrated.txt',
#                                         data_tof_axis,
#                                         data_y_axis,
#                                         data_y_error_axis)

        # work with normalization

        # load normalization
        ws_event_norm = wks_utility.loadNeXus(int(normalizationRunNumber), 'normalization')

        # get proton charge
        pc = wks_utility.getProtonCharge(ws_event_norm)
        error_0 = 1. / pc

        # rebin normalization
        ws_histo_norm = wks_utility.rebinNeXus(ws_event_norm,\
                              [binTOFrange[0], binTOFsteps, binTOFrange[1]],\
                              'normalization')

        # keep only TOF range
        ws_histo_norm = wks_utility.cropTOF(ws_histo_norm,\
                                      TOFrange[0],\
                                      TOFrange[1],\
                                      'normalization')

        # normalize by current proton charge
        ws_histo_norm = wks_utility.normalizeNeXus(ws_histo_norm, 'normalization')

        # integrate over low resolution range
        [norm_tof_axis, norm_y_axis, norm_y_error_axis] = wks_utility.integrateOverLowResRange(ws_histo_norm,\
                                                            normLowResRange,\
                                                            'normalization',\
                                                            is_nexus_detector_rotated_flag)

        # substract background
        [norm_y_axis, norm_y_error_axis] = wks_utility.substractBackground(norm_tof_axis[0:-1],\
                                                        norm_y_axis,\
                                                        norm_y_error_axis,\
                                                        normPeakRange,\
                                                        normBackFlag,\
                                                        normBackRange,\
                                                        error_0,\
                                                        'normalization')

        [av_norm, av_norm_error] = wks_utility.fullSumWithError(norm_y_axis,\
                                                           norm_y_error_axis)

#        ## DEBUGGING ONLY
#        wks_utility.ouput_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/norm_file_back_sub_not_integrated.txt',
#                                     norm_tof_axis,
#                                     av_norm,
#                                     av_norm_error)

        [final_data_y_axis, final_data_y_error_axis] = wks_utility.divideDataByNormalization(data_y_axis,
                                                                                             data_y_error_axis,
                                                                                             av_norm,
                                                                                             av_norm_error)

#        #DEBUG ONLY
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/data_divided_by_norm_not_integrated.txt',
#                                         data_tof_axis,
#                                         final_data_y_axis,
#                                         final_data_y_error_axis)

        # apply Scaling factor
        [tof_axis_full, y_axis, y_error_axis, isSFfound] = wks_utility.applyScalingFactor(tof_axis_full,
                                                                               final_data_y_axis,
                                                                               final_data_y_error_axis,
                                                                               incidentMedium,
                                                                               sfFile,
                                                                               slitsValuePrecision,
                                                                               slitsWidthFlag)

#        #DEBUG ONLY
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/after_applying_scaling_factor.txt',
#                                         data_tof_axis,
#                                         y_axis,
#                                         y_error_axis)

        if geometryCorrectionFlag: # convert To Q with correction
            [q_axis, y_axis, y_error_axis] = wks_utility.convertToQ(tof_axis_full,
                                                                    y_axis,
                                                                    y_error_axis,
                                                                    peak_range = dataPeakRange,
                                                                    central_pixel = data_central_pixel,
                                                                    source_to_detector_distance = dMD,
                                                                    sample_to_detector_distance = dSD,
                                                                    theta = theta,
                                                                    first_slit_size = first_slit_size,
                                                                    last_slit_size = last_slit_size)

        else: # convert to Q without correction

            [q_axis, y_axis, y_error_axis] = wks_utility.convertToQWithoutCorrection(tof_axis_full,
                                                                                     y_axis,
                                                                                     y_error_axis,
                                                                                     peak_range = dataPeakRange,
                                                                                     source_to_detector_distance = dMD,
                                                                                     sample_to_detector_distance = dSD,
                                                                                     theta = theta,
                                                                                     first_slit_size = first_slit_size,
                                                                                     last_slit_size = last_slit_size)


#            wks_utility.ouput_big_Q_ascii_file('/mnt/hgfs/j35/Matlab/DebugMantid/Strange0ValuesToData/after_conversion_to_q.txt',
#                                         q_axis,
#                                         y_axis,
#                                         y_error_axis)

        sz = q_axis.shape
        nbr_pixel = sz[0]

        # create workspace
        q_workspace = wks_utility.createQworkspace(q_axis, y_axis, y_error_axis)

        q_rebin = Rebin(InputWorkspace=q_workspace,
                        Params=q_range,
                        PreserveEvents=True)

        # keep only the q values that have non zero counts
        nonzero_q_rebin_wks = wks_utility.cropAxisToOnlyNonzeroElements(q_rebin,
                                                                        dataPeakRange)
        new_q_axis = nonzero_q_rebin_wks.readX(0)[:]

        # integrate spectra (normal mean) and remove first and last Q value
        [final_x_axis, final_y_axis, final_error_axis] = wks_utility.integrateOverPeakRange(nonzero_q_rebin_wks, dataPeakRange)


        # cleanup data
        [final_y_axis, final_y_error_axis] = wks_utility.cleanupData1D(final_y_axis,\
                                                                        final_error_axis)


        # create final workspace
        import time
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
                     LOgText=str(isSFfound))

        self.setProperty('OutputWorkspace', mtd[name_output_ws])

AlgorithmFactory.subscribe(RefLReduction)
