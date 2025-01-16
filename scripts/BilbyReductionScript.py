# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid_api
import os
from mantid.kernel import Logger
import BilbyCustomFunctions_Reduction

try:
    import mantidplot
except (Exception, Warning):
    mantidplot = None

ansto_logger = Logger("AnstoDataReduction")


class RunBilbyReduction:
    def __init__(
        self,
        reduction_settings_file,
        reduction_settings_index,
        file_index,
        tube_shift_correction_file,
        save_files=True,
        use_default_save_directory=False,
    ):
        self.reduction_settings_file = reduction_settings_file
        self.reduction_settings_index = reduction_settings_index
        self.file_index = file_index
        self.tube_shift_correction_file = tube_shift_correction_file
        self.save_files = save_files
        self.use_default_save_directory = use_default_save_directory

        # INPUT - index of a line with reduction parameters
        index_reduction_settings = [self.reduction_settings_index]  # INDEX OF THE LINE WITH REDUCTION SETTINGS

        if len(index_reduction_settings) > 1:  # must be single choice
            raise ValueError("Please check your choice of reduction settigns; only single value is allowed")

        # ID to evaluate - INPUT, in any combination of 'a-b' or ',c', or empty line; empty line means evaluate all files listed in csv
        self.index_files_to_reduce = self.file_index

        red_settings = mantid_api.FileFinder.getFullPath(self.reduction_settings_file)
        # Reading parameters from the reduction settings file
        reduction_settings_list = BilbyCustomFunctions_Reduction.files_list_reduce(red_settings)
        # take only one line, # index_reduction_settings
        self.current_reduction_settings = BilbyCustomFunctions_Reduction.files_to_reduce(
            reduction_settings_list, index_reduction_settings[0]
        )

        self.path_tube_shift_correction = mantid_api.FileFinder.getFullPath(self.tube_shift_correction_file)

        # settings - what to do - applied for all loaded files - not to be changed by a user
        self.correct_tubes_shift = True
        self.data_before_2016 = False  # curtains moved/ aligned /extra shift applied
        self.data_before_May_2016 = False  # Attenuators changed
        self.account_for_gravity = True  # False
        self.solid_angle_weighting = True  # False
        self.wide_angle_correction = False
        self.blocked_beam = True  # False

        # If needed to reduce 2D - this option is a defining one for the overall reduction
        reduce_2D_input = self.current_reduction_settings[0]["reduce_2D"].lower()
        self.reduce_2D = BilbyCustomFunctions_Reduction.string_boolean(reduce_2D_input)

    def setup_save_out_path(self, csv_files_to_reduce_list):
        if self.use_default_save_directory:
            reduced_files_path_folder = mantid_api.ConfigService["defaultsave.directory"]
        else:
            reduced_files_path_folder = os.path.dirname(csv_files_to_reduce_list)

        reduced_files_path = (
            reduced_files_path_folder + "\\" + self.current_reduction_settings[0]["reduced_files_folder"]
        )  # construct a path to a folder for reduced files
        # If folder does not exist, make it
        if not os.path.exists(reduced_files_path):
            os.makedirs(reduced_files_path)

        return reduced_files_path

    def save_out_files(self, reduced_files_path, base_output_name, output_workspace):
        if self.save_files:
            if self.reduce_2D:
                SaveNxs = os.path.join(os.path.expanduser(reduced_files_path), output_workspace.name() + ".nxs")
                mantid_api.SaveNISTDAT(output_workspace.name(), SaveNxs)
            else:
                n_1D = base_output_name + ".dat"  # 1D output file; name based on "base_output_name" construct
                savefile = os.path.join(os.path.expanduser(reduced_files_path), n_1D)  # setting up full path
                mantid_api.SaveAscii(
                    InputWorkspace=base_output_name, Filename=savefile, WriteXError=True, WriteSpectrumID=False, Separator="CSV"
                )  # saving file
                print("1D File Exists:{}".format(os.path.exists(savefile)))

    def plot_graphs(self, i, reduced_files_path, base_output_name, output_workspace, plot_2D, plot1Dgraph):
        if mantidplot:
            if self.reduce_2D:
                plot2Dgraph = mantidplot.plot2D(base_output_name)
                n_2D = output_workspace.name() + ".png"
                SavePlot = os.path.join(os.path.expanduser(reduced_files_path), n_2D)
                plot2Dgraph.export(SavePlot)
                print("2D File Exists:{}".format(os.path.exists(SavePlot)))

                # is there more elegant way to do it?
                # Problem is that plot2Dgraph creates and plot the graph file at the same time...
                if not plot_2D:
                    plot2Dgraph.close()
            else:
                if i == 0:
                    # to create first graph to stick all the rest to it; perhaps there is more elegant way
                    #  of creating initial empty handler, but I am not aware ... yet
                    plot1Dgraph = mantidplot.plotSpectrum(
                        base_output_name, 0, distribution=mantidplot.DistrFlag.DistrFalse, clearWindow=False
                    )
                else:
                    mantidplot.mergePlots(
                        plot1Dgraph,
                        mantidplot.plotSpectrum(base_output_name, 0, distribution=mantidplot.DistrFlag.DistrFalse, clearWindow=False),
                    )

    def get_base_output_name(self, i, sam_file, binning_wavelength, time_range, suffix):
        if self.reduce_2D:
            base_output_name = (
                sam_file[0:10] + "_2D_" + str(binning_wavelength[i][0]) + "_" + str(binning_wavelength[i][2]) + time_range + suffix
            )  # A core of output name; made from the name of the input sample
        else:
            base_output_name = (
                sam_file[0:10]
                + "_1D_"
                + str(round(binning_wavelength[i][0], 3))
                + "_"
                + str(round(binning_wavelength[i][2], 3))
                + time_range
                + suffix
            )  # A core of output name; made from the name of the input sample

        return base_output_name

    def retrieve_reduction_settings(self, name, default=None, message="", raise_exception=False):
        try:
            return self.current_reduction_settings[0][name]
        except:
            if raise_exception:
                raise ValueError(message)
            else:
                return default

    def setup_time_range(self, current_file, sam_file):
        StartTime = current_file["StartTime"]
        EndTime = current_file["EndTime"]
        # Errors: if trying to reduce time slice larger than the total time of the measurement:
        # Error message & Stop - to add

        if ((not StartTime) and (EndTime)) or ((StartTime) and (not EndTime)):
            raise ValueError("Check StartTime and EndTime values; either both or none shall be intered.")

        if (not StartTime) and (not EndTime):
            ws_sam = mantid_api.LoadBBY(sam_file)
            time_range = ""
        elif (float(StartTime)) > (float(EndTime)):
            raise ValueError("Check StartTime and EndTime values; EndTime cannot be smaller than StartTime.")
        else:
            ws_sam = mantid_api.LoadBBY(sam_file)  # test loader: to check if given StartTime and EndTime are reasonable
            Real_EndTime_max = float(ws_sam.run().getProperty("bm_counts").value)
            if float(EndTime) > Real_EndTime_max * 1.1:
                raise ValueError(
                    "EndTime value is wrong, it is more than 10%% larger than the data collection time: %7.2f" % Real_EndTime_max
                )
            ws_sam = mantid_api.LoadBBY(
                sam_file, FilterByTimeStart=StartTime, FilterByTimeStop=EndTime
            )  # now to load file within requested time slice if values are feasible
            time_range = "_" + StartTime + "_" + EndTime

        return ws_sam, time_range

    def run_bilby_reduction(self):
        # Read input csv file and define / create a folder for the output data
        csv_files_to_reduce_list = mantid_api.FileFinder.getFullPath(self.current_reduction_settings[0]["csv_file_name"])
        reduced_files_path = self.setup_save_out_path(csv_files_to_reduce_list)

        # Wavelength binning
        binning_wavelength_ini_str = self.retrieve_reduction_settings(
            "binning_wavelength_ini", raise_exception=True, message="binning_wavelength_ini cannot be empty"
        )

        binning_wavelength_ini = BilbyCustomFunctions_Reduction.read_convert_to_float(binning_wavelength_ini_str)
        binning_wavelength_ini_original = binning_wavelength_ini

        # WAVELENGTH RANGE FOR TRANSMISSION: the aim is to fit transmission on the whole range, and take only part for the data reduction
        # must  be equal or longer than binning_wavelength_ini
        binning_wavelength_transmission_str = self.current_reduction_settings[0]["binning_wavelength_transmission"]
        binning_wavelength_transmission = BilbyCustomFunctions_Reduction.read_convert_to_float(binning_wavelength_transmission_str)
        binning_wavelength_transmission_original = binning_wavelength_transmission

        # Check of wavelength range: transmission range must be equal or longer than the wavelength binning range for data reduction
        if (binning_wavelength_ini[0] < binning_wavelength_transmission[0]) or (
            binning_wavelength_ini[2] > binning_wavelength_transmission[2]
        ):
            raise ValueError(
                "Range for transmission binning shall be equal or wider than the range for the sample wavelength binning (refer to line 94)"
            )

        # Binning for Q
        binning_q_str = self.current_reduction_settings[0]["binning_q"]
        binning_q = BilbyCustomFunctions_Reduction.read_convert_to_float(binning_q_str)

        RadiusCut = self.retrieve_reduction_settings("RadiusCut", default=0.0)
        WaveCut = self.retrieve_reduction_settings("WaveCut", default=0.0)

        # Transmission fit parameters
        transmission_fit_ini = self.current_reduction_settings[0]["transmission_fit"]
        if (transmission_fit_ini != "Linear") and (transmission_fit_ini != "Log") and (transmission_fit_ini != "Polynomial"):
            raise ValueError(
                'Check value of transmission_fit; it can be only "Linear", "Log" or "Polynomial", first letter is mandatory capital'
            )

        PolynomialOrder = self.current_reduction_settings[0]["PolynomialOrder"]

        # Wavelength interval: if reduction on wavelength intervals is needed
        wavelength_interval_input = self.current_reduction_settings[0]["wavelength_intervals"].lower()
        wavelength_intervals = BilbyCustomFunctions_Reduction.string_boolean(wavelength_interval_input)
        wavelength_intervals_original = wavelength_intervals
        wav_delta = 0.0  # set the value, needed for the "wavelengh_slices" function

        if self.reduce_2D:
            print("2D reduction is performing. Q interval and number of points are taking into account; Q-binning intervals are ignored.")
            number_data_points_2D = float(
                self.retrieve_reduction_settings("2D_number_data_points", raise_exception=True, message="Number of points shall be given")
            )

            plot_2D = self.current_reduction_settings[0]["plot_2D"].lower()
            plot_2D = BilbyCustomFunctions_Reduction.string_boolean(plot_2D)
            binning_q[1] = (binning_q[0] + binning_q[2]) / number_data_points_2D  # To replace deltaQ from the input file
        else:
            plot_2D = None

        ######################################
        # Calling function to read given csv file
        parameters = BilbyCustomFunctions_Reduction.files_list_reduce(csv_files_to_reduce_list)
        files_to_reduce = BilbyCustomFunctions_Reduction.files_to_reduce(parameters, self.index_files_to_reduce)
        if len(files_to_reduce) == 0:
            raise ValueError("Please check index_files_to_reduce; chosen one does not exist")

        # reduce requested files one by one
        for current_file in files_to_reduce:
            sam_file = current_file["Sample"] + ".tar"

            ws_sam, time_range = self.setup_time_range(current_file, sam_file)

            # To read the mode value: True - ToF; False - NVS; this will define some steps inside SANSDataProcessor
            try:
                external_mode = ws_sam.run().getProperty("is_tof").value
            except:
                external_mode = True  # This is needed for old files, where the ToF/mono mode value has not been recorded

            # Internal frame source has been used during data collection; it is not always NVS only,
            # one can have both, NVS and choppers running for this mode
            if not external_mode:
                print("Internal frame source. Binning range is taken from the sample scattering data.")
                binning_wavelength_ini = (
                    ws_sam.readX(0)[0],
                    ws_sam.readX(0)[ws_sam.blocksize()] - ws_sam.readX(0)[0],
                    ws_sam.readX(0)[ws_sam.blocksize()],
                )
                binning_wavelength_transmission = binning_wavelength_ini
                if wavelength_intervals:
                    wavelength_intervals = False
                    print("NVS: monochromatic mode")
                    print(
                        "There is no sense to reduce monochromatic data on multiple wavelength;"
                        ' "wavelength_intervals" value changed to False.'
                    )
            else:
                # important for the case when NVS data is being analysed first,
                # ie to be able to come back to the whole range & wavelength slices, if needed
                binning_wavelength_ini = binning_wavelength_ini_original
                binning_wavelength_transmission = binning_wavelength_transmission_original
                wavelength_intervals = wavelength_intervals_original
                if wavelength_intervals:
                    wav_delta = float(self.current_reduction_settings[0]["wav_delta"])  # no need to read if the previous is false

            # empty beam scattering in transmission mode
            ws_emp_file = current_file["T_EmptyBeam"] + ".tar"
            mantid_api.LoadBBY(
                ws_emp_file, OutputWorkspace="ws_emp"
            )  # Note that this is of course a transmission measurement - shall be long

            # transmission workspaces and masks
            transm_file = current_file["T_Sample"] + ".tar"
            ws_tranSam = mantid_api.LoadBBY(transm_file)
            ws_tranEmp = mantid_api.LoadBBY(ws_emp_file)  # empty beam for transmission
            transm_mask = current_file["mask_transmission"] + ".xml"
            ws_tranMsk = mantid_api.LoadMask("BILBY", transm_mask)

            sam_mask_file = current_file["mask"] + ".xml"
            ws_samMsk = mantid_api.LoadMask("BILBY", sam_mask_file)

            # scaling: attenuation
            att_pos = float(ws_tranSam.run().getProperty("att_pos").value)

            scale = BilbyCustomFunctions_Reduction.attenuation_correction(att_pos, self.data_before_May_2016)
            print("scale, aka attenuation factor {}".format(scale))

            thickness = current_file["thickness [cm]"]

            # Cd / Al masks shift
            if self.correct_tubes_shift:
                BilbyCustomFunctions_Reduction.correction_tubes_shift(ws_sam, self.path_tube_shift_correction)

            if self.data_before_2016:
                BilbyCustomFunctions_Reduction.det_shift_before_2016(ws_sam)

                # Blocked beam
            if self.blocked_beam:
                ws_blocked_beam = current_file["BlockedBeam"] + ".tar"
                ws_blk = mantid_api.LoadBBY(ws_blocked_beam)
                if self.correct_tubes_shift:
                    BilbyCustomFunctions_Reduction.correction_tubes_shift(ws_blk, self.path_tube_shift_correction)
            else:
                ws_blk = None

            # Detector sensitivity
            ws_sen = None

            # empty beam normalisation
            mantid_api.MaskDetectors("ws_emp", MaskedWorkspace=ws_tranMsk)  # does not have to be ws_tranMsk, can be a specific mask
            mantid_api.ConvertUnits("ws_emp", Target="Wavelength", OutputWorkspace="ws_emp")

            # wavelenth intervals: building  binning_wavelength list
            binning_wavelength, n = BilbyCustomFunctions_Reduction.wavelengh_slices(wavelength_intervals, binning_wavelength_ini, wav_delta)

            # By now we know how many wavelengths bins we have, so shall run Q1D n times
            # -- Processing --
            suffix = "_" + current_file["suffix"]  # is the same for all wavelength intervals
            suffix_2 = current_file["additional_description"]
            if suffix_2 != "":
                suffix += "_" + suffix_2

            plot1Dgraph = None

            for i in range(n):
                ws_emp_partial = mantid_api.Rebin("ws_emp", Params=binning_wavelength[i])
                ws_emp_partial = mantid_api.SumSpectra(ws_emp_partial, IncludeMonitors=False)

                base_output_name = self.get_base_output_name(i, sam_file, binning_wavelength, time_range, suffix)

                # needed here, otherwise SANSDataProcessor replaced it with "transmission_fit" string
                transmission_fit = transmission_fit_ini

                output_workspace, transmission_fit = mantid_api.BilbySANSDataProcessor(
                    InputWorkspace=ws_sam,
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
                    TimeMode=external_mode,
                    AccountForGravity=self.account_for_gravity,
                    SolidAngleWeighting=self.solid_angle_weighting,
                    RadiusCut=RadiusCut,
                    WaveCut=WaveCut,
                    WideAngleCorrection=self.wide_angle_correction,
                    Reduce2D=self.reduce_2D,
                    OutputWorkspace=base_output_name,
                )

                if not self.reduce_2D:
                    BilbyCustomFunctions_Reduction.strip_NaNs(output_workspace, base_output_name)

                self.plot_graphs(i, reduced_files_path, base_output_name, output_workspace, plot_2D, plot1Dgraph)
                self.save_out_files(reduced_files_path, base_output_name, output_workspace)

                return output_workspace, transmission_fit
