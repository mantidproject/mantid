# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import stresstesting
import BilbyCustomFunctions_Reduction
from mantid.simpleapi import *


class BilbySANSDataProcessorTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
        # INPUT - mandatory from a USER - START
        ###########################################################################################
        red_settings = FileFinder.getFullPath('mantid_reduction_settings_example.csv')

        # INPUT - index of a line with reduction parameters
        index_reduction_settings = ["0"]  # INDEX OF THE LINE WITH REDUCTION SETTINGS

        if len(index_reduction_settings) > 1:  # must be single choice
            raise ValueError('Please check your choice of reduction settigns; only single value is allowed')

        # ID to evaluate - INPUT, in any combination of 'a-b' or ',c', or empty line; empty line means evaluate all files listed in csv
        index_files_to_reduce = "0"  # as per csv_files_to_reduce_list file - LINES' INDEXES FOR FILES TO BE REDUCED

        # Data file with numbers
        path_tube_shift_correction = FileFinder.getFullPath('shift_assembled.csv')

        ###########################################################################################
        # INPUT - mandatory from a USER - END
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
        ###########################################################################################
        # settings - what to do - applied for all loaded files - not to be changed by a user

        correct_tubes_shift = True
        data_before_2016 = False  # curtains moved/ aligned /extra shift applied
        data_before_May_2016 = False  # Attenuators changed
        account_for_gravity = True  # False
        solid_angle_weighting = True  # False
        wide_angle_correction = False
        blocked_beam = True  # False

        ######################################
        ######################################
        # Reading parameters from the reduction settings file
        reduction_settings_list = BilbyCustomFunctions_Reduction.FilesListReduce(red_settings)  # read entire file
        current_reduction_settings = BilbyCustomFunctions_Reduction.FilesToReduce(reduction_settings_list,
                                                                                  index_reduction_settings[
                                                                                      0])  # take only one line, # index_reduction_settings

        # Read input csv file and define / create a folder for the output data
        csv_files_to_reduce_list = FileFinder.getFullPath(current_reduction_settings[0]["csv_file_name"])
        reduced_files_path_folder = os.path.dirname(csv_files_to_reduce_list)

        reduced_files_path = reduced_files_path_folder + '\\' + current_reduction_settings[0][
            "reduced_files_folder"]  # construct a path to a folder for reduced files
        # If folder does not exist, make it
        if not os.path.exists(reduced_files_path):
            os.makedirs(reduced_files_path)

        # Wavelength binning
        try:
            binning_wavelength_ini_str = current_reduction_settings[0]["binning_wavelength_ini"]
        except:
            raise ValueError("binning_wavelength_ini cannot be empty")

        binning_wavelength_ini = BilbyCustomFunctions_Reduction.read_convert_to_float(binning_wavelength_ini_str)
        binning_wavelength_ini_original = binning_wavelength_ini

        # WAVELENGTH RANGE FOR TRANSMISSION: the aim is to fit transmission on the whole range, and take only part for the data reduction
        # must  be equal or longer than binning_wavelength_ini
        binning_wavelength_transmission_str = current_reduction_settings[0]["binning_wavelength_transmission"]
        binning_wavelength_transmission = BilbyCustomFunctions_Reduction.read_convert_to_float(
            binning_wavelength_transmission_str)
        binning_wavelength_transmission_original = binning_wavelength_transmission

        # Check of wavelength range: transmission range must be equal or longer than the wavelength binning range for data reduction
        if (binning_wavelength_ini[0] < binning_wavelength_transmission[0]) or (
                    binning_wavelength_ini[2] > binning_wavelength_transmission[2]):
            raise ValueError("Range for transmission binning shall be equal or wider than the range for the"
                             " sample wavelength binning (refer to line 94)")

        # Binning for Q
        binning_q_str = current_reduction_settings[0]["binning_q"]
        binning_q = BilbyCustomFunctions_Reduction.read_convert_to_float(binning_q_str)

        # Put more explanation here what it is and what is going on
        try:
            RadiusCut = current_reduction_settings[0]["RadiusCut"]
        except:
            RadiusCut = 0.0  # in case of data before February 2017 or in case the value is forgotten in the input file

        try:
            WaveCut = current_reduction_settings[0]["WaveCut"]
        except:
            WaveCut = 0.0  # in case of data before February 2017 or in case the value is forgotten in the input file

        # Transmission fit parameters
        transmission_fit_ini = current_reduction_settings[0]["transmission_fit"]
        if (transmission_fit_ini != "Linear") and (transmission_fit_ini != "Log") and (
                    transmission_fit_ini != "Polynomial"):
            raise ValueError(
                "Check value of transmission_fit; it can be only \"Linear\", \"Log\" or \"Polynomial\", first letter is mandatory capital")

        PolynomialOrder = current_reduction_settings[0]["PolynomialOrder"]

        # Wavelength interval: if reduction on wavelength intervals is needed
        wavelength_interval_input = current_reduction_settings[0]["wavelength_intervals"].lower()
        wavelength_intervals = BilbyCustomFunctions_Reduction.string_boolean(wavelength_interval_input)
        wavelength_intervals_original = wavelength_intervals
        wav_delta = 0.0  # set the value, needed for the "wavelengh_slices" function

        # If needed to reduce 2D - this option is a defining one for the overall reduction

        parameters = BilbyCustomFunctions_Reduction.FilesListReduce(csv_files_to_reduce_list)
        files_to_reduce = BilbyCustomFunctions_Reduction.FilesToReduce(parameters, index_files_to_reduce)
        if len(files_to_reduce) == 0:
            raise ValueError('Please check index_files_to_reduce; chosen one does not exist')

            # reduce requested files one by one
        for current_file in files_to_reduce:
            sam_file = current_file["Sample"] + '.tar'

            ws_sam = LoadBBY(sam_file)
            time_range = ''

            # important for the case when NVS data is being analysed first, ie to be able to come back to the whole
            #  range & wavelength slices, if needed
            binning_wavelength_ini = binning_wavelength_ini_original
            binning_wavelength_transmission = binning_wavelength_transmission_original
            wavelength_intervals = wavelength_intervals_original
            if wavelength_intervals:
                wav_delta = float(
                    current_reduction_settings[0]["wav_delta"])  # no need to read if the previous is false

            # empty beam scattering in transmission mode
            ws_emp_file = current_file["T_EmptyBeam"] + '.tar'
            LoadBBY(ws_emp_file, OutputWorkspace='ws_emp')  # Note that this is of course a transmission measurement - shall be long

            # transmission workspaces and masks
            transm_file = current_file["T_Sample"] + '.tar'
            ws_tranSam = LoadBBY(transm_file)
            ws_tranEmp = LoadBBY(ws_emp_file)  # empty beam for transmission
            transm_mask = current_file["mask_transmission"] + '.xml'
            ws_tranMsk = LoadMask('Bilby', transm_mask)

            sam_mask_file = current_file["mask"] + '.xml'
            ws_samMsk = LoadMask('Bilby', sam_mask_file)

            # scaling: attenuation
            att_pos = float(ws_tranSam.run().getProperty("att_pos").value)

            scale = BilbyCustomFunctions_Reduction.AttenuationCorrection(att_pos, data_before_May_2016)

            thickness = current_file["thickness [cm]"]

            # Cd / Al masks shift
            if correct_tubes_shift:
                BilbyCustomFunctions_Reduction.CorrectionTubesShift(ws_sam, path_tube_shift_correction)

            if data_before_2016:
                BilbyCustomFunctions_Reduction.DetShift_before2016(ws_sam)

                # Blocked beam
            if blocked_beam:
                ws_blocked_beam = current_file["BlockedBeam"] + '.tar'
                ws_blk = LoadBBY(ws_blocked_beam)
                if correct_tubes_shift:
                    BilbyCustomFunctions_Reduction.CorrectionTubesShift(ws_blk, path_tube_shift_correction)
            else:
                ws_blk = None

            # Detector sensitivity
            ws_sen = None

            # empty beam normalisation
            MaskDetectors("ws_emp", MaskedWorkspace=ws_tranMsk)
            ConvertUnits("ws_emp", Target="Wavelength", OutputWorkspace='ws_emp')

            # wavelenth intervals: building  binning_wavelength list
            binning_wavelength, n = BilbyCustomFunctions_Reduction.wavelengh_slices(wavelength_intervals,
                                                                                    binning_wavelength_ini, wav_delta)

            ###############################################################
            # By now we know how many wavelengths bins we have, so shall run Q1D n times
            # -- Processing --
            suffix = '_' + current_file["suffix"]  # is the same for all wavelength intervals
            suffix_2 = current_file["additional_description"]
            if suffix_2 != '':
                suffix += '_' + suffix_2

            for i in range(n):
                ws_emp_partial = Rebin("ws_emp", Params=binning_wavelength[i])
                ws_emp_partial = SumSpectra(ws_emp_partial, IncludeMonitors=False)

                base_output_name = sam_file[0:10] + '_1D_' + str(round(binning_wavelength[i][0], 3)) + '_' + str(
                    round(binning_wavelength[i][2],
                          3)) + time_range + suffix  # A core of output name; made from the name of the input sample

                # needed here, otherwise BilbySANSDataProcessor replaced it with "transmission_fit" string
                transmission_fit = transmission_fit_ini

                output_workspace, transmission_fit = BilbySANSDataProcessor(InputWorkspace=ws_sam,
                                                                            InputMaskingWorkspace=ws_samMsk,
                                                                            BlockedBeamWorkspace=ws_blk,
                                                                            EmptyBeamSpectrumShapeWorkspace=ws_emp_partial,
                                                                            SensitivityCorrectionMatrix=ws_sen,
                                                                            TransmissionWorkspace=ws_tranSam,
                                                                            TransmissionEmptyBeamWorkspace=ws_tranEmp,
                                                                            TransmissionMaskingWorkspace=ws_tranMsk,
                                                                            ScalingFactor=scale,
                                                                            SampleThickness=thickness,
                                                                            FitMethod=transmission_fit,
                                                                            PolynomialOrder=PolynomialOrder,
                                                                            BinningWavelength=binning_wavelength[i],
                                                                            BinningWavelengthTransm=binning_wavelength_transmission,
                                                                            BinningQ=binning_q,
                                                                            TimeMode=True,
                                                                            AccountForGravity=account_for_gravity,
                                                                            SolidAngleWeighting=solid_angle_weighting,
                                                                            RadiusCut=RadiusCut, WaveCut=WaveCut,
                                                                            WideAngleCorrection=wide_angle_correction,
                                                                            Reduce_2D=False,
                                                                            OutputWorkspace=base_output_name)

                BilbyCustomFunctions_Reduction.strip_NaNs(output_workspace, base_output_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'BBY0019749_1D_2.0_18.0_AgBeh', 'BilbyReductionExampleOutput.nxs'
