"""*WIKI* 

Liquids Reflectometer (REFL) reduction

*WIKI*"""

from mantid.api import *
from mantid.simpleapi import *
from numpy import zeros, shape, arange
import math

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

        bDebug = True
        if bDebug:
            print '====== Running in mode DEBUGGING ======='

        # retrieve settings from GUI
        print '-> Retrieving settings from GUI'
        
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
        if (TOFrangeFlag):
            TOFrange = self.getProperty("TOFRange").value #microS
        else:
            TOFrange = [0, 200000]
#         TOFsteps = 200.0 # TOF binnng
                
        # TOF binning parameters
        binTOFrange = [0, 200000]
        binTOFsteps = 50

        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value
        if (qStep > 0): #force logarithmic binning
            qStep = -qStep;

        # angle offset
        angleOffsetDeg = self.getProperty("AngleOffset").value

        #dimension of the detector (256 by 304 pixels)
        maxX = 304
        maxY = 256
                
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
        
        ## retrieve general informations
        # calculate the central pixel (using weighted average)
        print '-> retrieving general informations'
        data_central_pixel = wks_utility.getCentralPixel(ws_event_data, 
                                                         dataPeakRange) 
        # get the distance moderator-detector and sample-detector
        [dMD, dSD] = wks_utility.getDistances(ws_event_data)
        # get theta
        theta = wks_utility.getTheta(ws_event_data, angleOffsetDeg)
                
        # rebin data
        ws_histo_data = wks_utility.rebinNeXus(ws_event_data,
                              [binTOFrange[0], binTOFsteps, binTOFrange[1]],
                              'data')
        # get q range
        q_range = wks_utility.getQrange(ws_histo_data, theta, dMD, qMin, qStep)

        # slit size
        [first_slit_size, last_slit_size] = wks_utility.getSlitsSize(ws_histo_data)

        # keep only TOF range
        ws_histo_data = wks_utility.cropTOF(ws_histo_data, 
                                      TOFrange[0],
                                      TOFrange[1],
                                      'data')
        # normalize by current proton charge
        ws_histo_data = wks_utility.normalizeNeXus(ws_histo_data, 'data')
        # integrate over low resolution range
        [data_tof_axis, data_y_axis, data_y_error_axis] = wks_utility.integrateOverLowResRange(ws_histo_data,
                                                            dataLowResRange,
                                                            'data')

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
                                                                           'data')
        
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Desktop/DebugMantid/DataAndNormBeingSameFile/data_file_back_sub_not_integrated.txt',
#                                      data_tof_axis,
#                                      data_y_axis,
#                                      data_y_error_axis)
         
         
# #         ## DEBUGGING ONLY
#         [inte_data_y_axis, inte_data_y_error_axis] = wks_utility.integratedOverPixelDim(data_y_axis, data_y_error_axis)          
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/' + name_output_ws + '.txt'
#         wks_utility.ouput_ascii_file(fileName,
#                                      data_tof_axis,
#                                      inte_data_y_axis, 
#                                      inte_data_y_error_axis)

        # work with normalization        
        # load normalization
        ws_event_norm = wks_utility.loadNeXus(int(normalizationRunNumber), 'normalization')        
        # rebin normalization
        ws_histo_norm = wks_utility.rebinNeXus(ws_event_norm,
                              [binTOFrange[0], binTOFsteps, binTOFrange[1]],
                              'normalization')
        # keep only TOF range
        ws_histo_norm = wks_utility.cropTOF(ws_histo_norm, 
                                      TOFrange[0],
                                      TOFrange[1],
                                      'normalization')
        # normalize by current proton charge
        ws_histo_norm = wks_utility.normalizeNeXus(ws_histo_norm, 'normalization')
        # integrate over low resolution range
        [norm_tof_axis, norm_y_axis, norm_y_error_axis] = wks_utility.integrateOverLowResRange(ws_histo_norm,
                                                            normLowResRange,
                                                            'normalization')

        # substract background
        [norm_y_axis, norm_y_error_axis] = wks_utility.substractBackground(norm_tof_axis[0:-1], 
                                                        norm_y_axis, 
                                                        norm_y_error_axis,
                                                        normPeakRange,
                                                        normBackFlag,
                                                        normBackRange,
                                                        'normalization') 

#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Desktop/DebugMantid/DataAndNormBeingSameFile/norm_file_back_sub_not_integrated.txt',
#                                      norm_tof_axis,
#                                      norm_y_axis,
#                                      norm_y_error_axis)

        # get average mean of peak region for each TOF
#         [av_norm, av_norm_error] = wks_utility.meanOfRange(norm_y_axis, 
#                                                            norm_y_error_axis)

        [av_norm, av_norm_error] = wks_utility.fullSumWithError(norm_y_axis, 
                                                           norm_y_error_axis)


        


#         ## DEBUGGING ONLY
#        wks_utility.ouput_ascii_file('/mnt/hgfs/j35/Desktop/DebugMantid/DataAndNormBeingSameFile/norm_file_back_sub_integrated.txt',
#                                     norm_tof_axis, 
#                                     av_norm, 
#                                     av_norm_error)

#         ## DEBUGGING ONLY
#         [inte_data_y_axis, inte_data_y_error_axis] = wks_utility.integratedOverPixelDim(data_y_axis, data_y_error_axis)          
#         [final_data, final_data_error] = wks_utility.divideData1DbyNormalization(inte_data_y_axis,
#                                                                                    inte_data_y_error_axis,
#                                                                                    av_norm,
#                                                                                    av_norm_error)
#         ## DEBUGGING ONLY
#         wks_utility.ouput_ascii_file('/mnt/hgfs/j35/Dropbox/temporary/data_over_norm_file.txt',
#                                      norm_tof_axis,
#                                      final_data, 
#                                      final_data_error)
        
        ## up to here, works perfectly !!!

        # divide data by normalization
        #print data_y_axis.shape #(7,62) = (nbr_pixels, nbr_tof)
        
        
        
        [final_data_y_axis, final_data_y_error_axis] = wks_utility.divideDataByNormalization(data_y_axis,
                                                                                             data_y_error_axis,
                                                                                             av_norm,
                                                                                             av_norm_error)


#         ## DEBUGGING ONLY
#         my_x_axis = zeros((17,240))
#         for i in range(17):
#             my_x_axis[i,:] = data_tof_axis[0:-1]
# 
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/justAfterNormalization_' + name_output_ws + '.txt'
#         wks_utility.ouput_big_Q_ascii_file(fileName,
#                                            my_x_axis, 
#                                            final_data_y_axis, 
#                                            final_data_y_error_axis)
        

            
        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Desktop/DebugMantid/DataAndNormBeingSameFile/data_over_norm_file_not_integrated.txt',
                                      tof_axis,
                                      final_data_y_axis,
                                      final_data_y_error_axis)

#         ## debugging only
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/justAfterNormalization_' + name_output_ws + '.txt'
#         wks_utility.ouput_big_Q_ascii_file(fileName,
#                                            my_x_axis, 
#                                            final_data_y_axis, 
#                                            final_data_y_error_axis)


#         ## for debugging only, create ascii file of data
#         # integrate data_y_axis over range of peak
#          
#         ## for degbugging only
#         size = final_data_y_axis.shape
#         nbr_tof = size[1]
#         final_data = zeros(nbr_tof)
#         final_data_error = zeros(nbr_tof)
#         for t in range(nbr_tof):
#             [data, error] = wks_utility.sumWithError(final_data_y_axis[:,t], 
#                                                      final_data_y_error_axis[:,t])
#             final_data[t] = data
#             final_data_error[t] = error
#   
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/' + name_output_ws + '.txt'
#         wks_utility.ouput_ascii_file(fileName,
#                                          norm_tof_axis,
#                                          final_data, 
#                                          final_data_error)



#        
#         ## DEBUGGING ONLY
#         wks_utility.ouput_ascii_file('/mnt/hgfs/j35/Dropbox/temporary/data_over_norm_file.txt',
#                                      norm_tof_axis,
#                                      final_data, 
#                                      final_data_error)

# 
#         ## DEBUGGING ONLY
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/' + name_output_ws + '.txt'
#         wks_utility.ouput_ascii_file(fileName,
#                                      final_x_axis,
#                                      final_y_axis,
#                                      final_error_axis)

        # apply Scaling factor    
        [tof_axis_full, y_axis, y_error_axis] = wks_utility.applyScalingFactor(tof_axis_full, 
                                                                               final_data_y_axis, 
                                                                               final_data_y_error_axis, 
                                                                               incidentMedium,
                                                                               sfFile,
                                                                               slitsValuePrecision,
                                                                               slitsWidthFlag)
        
#         ## DEBUGGING ONLY
#        wks_utility.ouput_big_ascii_file('/mnt/hgfs/j35/Desktop/data_before_convertToQ_not_integrated.txt',
#                                         tof_axis,
#                                         y_axis, 
#                                         y_error_axis)
#        
#        sys.exit('stop here')
        
        
#         # convert to Q without correction
#        [q_axis, y_axis, y_error_axis] = wks_utility.convertToQWithoutCorrection(tof_axis_full,
#                                                                                 y_axis, 
#                                                                                 y_error_axis,
#                                                                                 peak_range = dataPeakRange,  
#                                                                                 central_pixel = data_central_pixel,
#                                                                                 source_to_detector_distance = dMD,
#                                                                                 sample_to_detector_distance = dSD,
#                                                                                 theta = theta,
#                                                                                 first_slit_size = first_slit_size,
#                                                                                 last_slit_size = last_slit_size)

        
        
        
        
        # convert To Q with correction
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

#         ## debugging only
#        name_output_ws = self.getPropertyValue("OutputWorkspace")
#        fileName = '/mnt/hgfs/j35/Desktop/beforeRebin_' + name_output_ws + '.txt'
#        wks_utility.ouput_big_Q_ascii_file(fileName,
#                                           q_axis,
#                                           y_axis, 
#                                           y_error_axis)
        
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/beforeRebin_' + name_output_ws + '.txt'
#         wks_utility.ouput_big_Q_ascii_file(fileName,
#                                            q_axis,
#                                            y_axis, 
#                                            y_error_axis)
         
        sz = q_axis.shape
        nbr_pixel = sz[0]
        
        # create workspace
        q_workspace = wks_utility.createQworkspace(q_axis, y_axis, y_error_axis)

#         q_workspace = SumSpectra(q_workspace)

#         import time
#         _time = int(time.time())
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         name_output_ws = name_output_ws + '_#' + str(_time) + 'ts'
        q_rebin = Rebin(InputWorkspace=q_workspace,
                        Params=q_range,         
                        PreserveEvents=True)
        
        ## debugging only
#         q_axis = q_rebin.readX(0)[:]
#         x_axis = zeros((nbr_pixel,len(q_axis)))
#         y_axis = zeros((nbr_pixel,len(q_axis)))
#         y_error_axis = zeros((nbr_pixel,len(q_axis)))
#         
#         print 'nbr_pixel: ' , nbr_pixel
#         
#         for i in range(nbr_pixel):
#             x_axis[i,:] = q_axis[:]
#             y_axis[i,:] = q_rebin.readY(i)[:]
#             y_error_axis[i,:] = q_rebin.readE(i)[:]
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/afterRebin_' + name_output_ws + '.txt'
# 
#         print x_axis.shape
#         print y_axis.shape
# 
#         wks_utility.ouput_big_Q_ascii_file(fileName,
#                                            x_axis,
#                                            y_axis, 
#                                            y_error_axis)


# #         
        # keep only the q values that have non zero counts
        nonzero_q_rebin_wks = wks_utility.cropAxisToOnlyNonzeroElements(q_rebin, 
                                                                        dataPeakRange)
        new_q_axis = nonzero_q_rebin_wks.readX(0)[:]
                  
#         integrate spectra (normal mean) and remove first and last Q value
        [final_x_axis, final_y_axis, final_error_axis] = wks_utility.integrateOverPeakRange(nonzero_q_rebin_wks, dataPeakRange)
         
                  
        # for debugging only, only work with TOF 
        
#         [final_y_axis, final_error_axis] = wks_utility.weightedMeanOfRange(y_axis, y_error_axis)
#         final_x_axis = tof_axis.copy()    
# 
#         ## DEBUGGING ONLY
#         name_output_ws = self.getPropertyValue("OutputWorkspace")
#         fileName = '/mnt/hgfs/j35/Dropbox/temporary/' + name_output_ws + '.txt'
#         wks_utility.ouput_ascii_file(fileName,
#                                      final_x_axis,
#                                      final_y_axis,
#                                      final_error_axis)
                     
        
        
#                # cleanup data
        [final_y_axis, final_y_error_axis] = wks_utility.cleanupData1D(final_y_axis,
                                                                        final_error_axis)

        
        # create final workspace
        import time
        _time = int(time.time())
        name_output_ws = self.getPropertyValue("OutputWorkspace")
        name_output_ws = name_output_ws + '_#' + str(_time) + 'ts'
        final_workspace = wks_utility.createFinalWorkspace(final_x_axis, 
                                                           final_y_axis, 
                                                           final_y_error_axis,
                                                           name_output_ws)
         
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
                
                
                
                
#         dataX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
#         dataY = [1,2,3,4,5,6,7,8,9,10,11,12]
#         dataE = dataY
#         tmpOutputWks = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4, UnitX="Wavelength")
                       
        self.setProperty('OutputWorkspace', mtd[name_output_ws])
           
        
AlgorithmFactory.subscribe(RefLReduction)
# registerAlgorithm(RefLReduction())

