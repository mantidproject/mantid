# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MultipleFileProperty,
    FileAction,
    MatrixWorkspaceProperty,
    FileProperty,
)
from mantid.kernel import Direction, StringListValidator
from mantid.simpleapi import (
    CloneWorkspace,
    ConvertUnits,
    CreateWorkspace,
    DeleteWorkspace,
    Divide,
    ExtractMonitors,
    GroupWorkspaces,
    LoadAndMerge,
    Multiply,
    SortXAxis,
    SplineInterpolation,
    Subtract,
)

import numpy as np
from typing import List, Tuple


class LagrangeILLReduction(DataProcessorAlgorithm):
    progress = None
    output_ws_name = None
    empty_cell_ws = None
    water_correction = None
    water_corr_ws = None
    intermediate_workspaces = None

    INCIDENT_ENERGY_OFFSET = 4.5

    use_incident_energy = False
    convert_to_wavenumber = False
    _empty_cell_nexus = None
    _sample_nexus = None

    # max difference between two identical points, two points closer than that will be merged in the merge algorithm
    EPSILON = 1e-2

    # default header size in data files in case it couldn't be determined
    HEADER_SIZE_DEFAULT = 36

    # default column indices in data files in case they couldn't be determined
    ENERGY_COL_DEFAULT = 1
    MONITOR_COUNT_COL_DEFAULT = 2
    DETECTOR_COUNT_COL_DEFAULT = 5
    TIME_COL_DEFAULT = 4
    TEMPERATURE_COL_DEFAULT = 9

    # normalisation approach
    normalise_by = None

    # workspace title
    _title = None

    @staticmethod
    def category():
        return "ILL\\Indirect"

    @staticmethod
    def summary():
        return "Reduces Lagrange data."

    @staticmethod
    def seeAlso():
        return []

    @staticmethod
    def name():
        return "LagrangeILLReduction"

    def setup(self):
        self.use_incident_energy = self.getProperty("UseIncidentEnergy").value
        self.convert_to_wavenumber = self.getProperty("ConvertToWaveNumber").value
        self.normalise_by = self.getPropertyValue("NormaliseBy")

        self._empty_cell_nexus = ".nxs" in self.getPropertyValue("ContainerRuns")
        self._sample_nexus = ".nxs" in self.getPropertyValue("SampleRuns")

        # the list of all the intermediate workspaces to group at the end
        self.intermediate_workspaces = []

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("SampleRuns", action=FileAction.Load, extensions=["", ".nxs"]), doc="Sample run(s).")

        self.declareProperty(
            MultipleFileProperty("ContainerRuns", action=FileAction.OptionalLoad, extensions=["", ".nxs"]),
            doc="Container run(s) (empty cell)",
        )

        self.declareProperty(
            FileProperty("CorrectionFile", "", action=FileAction.OptionalLoad, extensions=[".txt"]), doc="Correction reference file."
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output workspace containing reduced data."
        )

        self.declareProperty(
            name="NormaliseBy",
            defaultValue="Monitor",
            validator=StringListValidator(["Monitor", "None"]),
            direction=Direction.Input,
            doc="What normalisation approach to use on data.",
        )

        self.declareProperty(name="UseIncidentEnergy", defaultValue=False, doc="Show the energies as incident energies, not transfer ones.")

        self.declareProperty(name="ConvertToWaveNumber", defaultValue=False, doc="Convert axis unit to energy in wave number (cm-1)")

        self.declareProperty(name="NexusInput", defaultValue=True, doc="Whether the input data contains NeXus files.")

    def validateInputs(self):
        issues = dict()
        if self.getProperty("NexusInput").value:
            nexus_err_msg = "Data is expected to be NeXus but extension not found in the provided data path."
            if ".nxs" not in self.getPropertyValue("SampleRuns"):
                issues["SampleRuns"] = nexus_err_msg
            if not self.getProperty("ContainerRuns").isDefault and ".nxs" not in self.getPropertyValue("ContainerRuns"):
                issues["ContainerRuns"] = nexus_err_msg
        return issues

    def PyExec(self):
        self.setup()

        correction_file = self.getPropertyValue("CorrectionFile")
        self.output_ws_name = self.getPropertyValue("OutputWorkspace")

        # load correction
        if correction_file:
            self.water_correction = self.get_water_correction(correction_file)

        empty_cell_files = self.getPropertyValue("ContainerRuns").split(",")
        # empty cell treatment, if there is any
        if empty_cell_files[0] != str():
            self.process_empty_cell(empty_cell_files)

        # sample load and formatting
        sample_files = self.getPropertyValue("SampleRuns").split(",")
        raw_sample_ws = "__" + self.output_ws_name + "_rawS"
        if self._sample_nexus:
            self.preprocess_nexus(sample_files, raw_sample_ws)
        else:
            sample_data = self.load_and_concatenate(sample_files)
            sample_data = self.merge_adjacent_points(sample_data)
            energy, detector_counts, errors, time, temperature = self.get_counts_errors_metadata(sample_data)

            CreateWorkspace(outputWorkspace=raw_sample_ws, DataX=energy, DataY=detector_counts, DataE=errors, UnitX="Energy")
            self.add_metadata(raw_sample_ws, time, temperature)

        self.intermediate_workspaces.append(raw_sample_ws)

        # sample correction by water
        if self.water_correction is not None:
            water_corrected_ws = "{}_Calibrated".format(raw_sample_ws)
            self.correct_data(raw_sample_ws, water_corrected_ws)
            self.intermediate_workspaces.append(water_corrected_ws)

        # clone the last workspace - either the raw or water corrected data - to the destination name
        CloneWorkspace(InputWorkspace=self.intermediate_workspaces[-1], OutputWorkspace=self.output_ws_name)

        # correct by empty cell if there is any
        if self.empty_cell_ws is not None:
            self.subtract_empty_cell()

        if self.convert_to_wavenumber:
            ConvertUnits(InputWorkspace=self.output_ws_name, OutputWorkspace=self.output_ws_name, Target="Energy_inWavenumber")

        # set the properties and group the hidden workspaces
        self.setProperty("OutputWorkspace", self.output_ws_name)
        GroupWorkspaces(OutputWorkspace="__" + self.output_ws_name, InputWorkspaces=self.intermediate_workspaces)

    def add_metadata(self, ws: str, time: List[float], temperature: List[float]):
        """
        Adds metadata from provided loaded lists of values.

        Args:
            ws (str): workspace name where the metadata is to be added
            time (list(float)): list of time values to be added
            temperature (list(float)): list of temperature values to be added
        """
        run = mtd[ws].getRun()
        run.addProperty("time", time, True)
        run.addProperty("temperature", temperature, True)
        run.addProperty("run_title", self._title, True)

    def load_and_concatenate(self, files: List[str]) -> np.ndarray:
        """
        Loads ASCII Lagrange data files as input, loads the interesting data from it and concatenates them into one numpy array

        Args:
            files (list(str)): the ascii data files to load and concatenate together
        Return:
            Loaded values concatenated, as a (nb of points, 3)-shaped numpy array, with values (incident energy, monitor counts,
        detector counts)
        """
        loaded_data = None
        for file in files:
            header_index = -1
            energy_col = -1
            detector_counts_col = -1
            monitor_counts_col = -1
            time_col = -1
            temperature_col = -1

            # we first open the file to find the size of the header and the position of the columns holding the data
            with open(file, "r") as f:
                for index, line in enumerate(f):
                    if line.startswith("TITLE:"):
                        contents = line.split(" ")
                        self._title = " ".join(contents[1:-1])  # the metadata tag and the line break at the end
                    if line == "DATA_:\n":
                        header_index = index
                        next_line = f.__next__().split()

                        if "EI" in next_line:
                            energy_col = next_line.index("EI")
                        else:
                            energy_col = self.ENERGY_COL_DEFAULT
                            self.log().warning(
                                "Could not find energy column in file {}. Defaulting to column {}".format(file, self.ENERGY_COL_DEFAULT)
                            )
                        if "CNTS" in next_line:
                            detector_counts_col = next_line.index("CNTS")
                        else:
                            detector_counts_col = self.DETECTOR_COUNT_COL_DEFAULT
                            self.log().warning(
                                "Could not find detector counts column in file {}. Defaulting to column {}".format(
                                    file, self.DETECTOR_COUNT_COL_DEFAULT
                                )
                            )
                        if "M1" in next_line:
                            monitor_counts_col = next_line.index("M1")
                        else:
                            monitor_counts_col = self.MONITOR_COUNT_COL_DEFAULT
                            self.log().warning(
                                "Could not find monitor counts column in file {}. Defaulting to column {}".format(
                                    file, self.MONITOR_COUNT_COL_DEFAULT
                                )
                            )
                        if "TIME" in next_line:
                            time_col = next_line.index("TIME")
                        else:
                            time_col = self.TIME_COL_DEFAULT
                        if "TT" in next_line:
                            temperature_col = next_line.index("TT")
                        else:
                            temperature_col = self.TEMPERATURE_COL_DEFAULT
                        break
                f.close()

            if header_index < 0:
                self.log().warning("Could not determine header length in file {}. Defaulting to {}".format(file, self.HEADER_SIZE_DEFAULT))
                header_index = self.HEADER_SIZE_DEFAULT
                energy_col = self.ENERGY_COL_DEFAULT
                detector_counts_col = self.DETECTOR_COUNT_COL_DEFAULT
                monitor_counts_col = self.MONITOR_COUNT_COL_DEFAULT
                time_col = self.TIME_COL_DEFAULT
                temperature_col = self.TEMPERATURE_COL_DEFAULT

            # load the relevant data, which position we just determined - angle, monitor count, detector count
            data = np.loadtxt(
                file,
                dtype=float,
                skiprows=header_index + 2,
                usecols=(energy_col, monitor_counts_col, detector_counts_col, time_col, temperature_col),
            )
            loaded_data = np.vstack((loaded_data, data)) if loaded_data is not None else data
        if np.shape(loaded_data)[0] == 0:
            raise RuntimeError("Provided files contain no data in the LAGRANGE format.")
        if loaded_data.ndim == 1:  # data needs to be reshaped in case there is only one scan point
            loaded_data = np.reshape(loaded_data, (1, loaded_data.shape[0]))
        return loaded_data

    def preprocess_nexus(self, file_name: List[str], output_name: str):
        """
        Loads, merges adjacent bins and puts the detector counts workspace in the ADS. The method interfaces to the LoadAndMerge
        algorithm to load NeXus Lagrange data, then processes the loaded workspace to remove all bins that have a smaller
        bin width than EPSILON, uses the ExtractMonitors to separate detector counts from monitors, and finally,
        if requested, normalizes detector counts to monitor.

        Args:
            file_name (list(str)): string containing name(s) of file(s) to be loaded
            output_name (str): name for the output workspace containing detector counts
        """
        # if the user wants to see transfer energy, we subtract by the constant offset
        offset = 0 if self.use_incident_energy else self.INCIDENT_ENERGY_OFFSET
        LoadAndMerge(
            Filename=",".join(file_name),
            LoaderName="LoadILLLagrange",
            LoaderOptions={"InitialEnergyOffset": offset},
            OutputWorkspace=output_name,
            OutputBehaviour="Concatenate",
            SampleLogAsXAxis="Ei",
        )
        if len(file_name) > 1:  # corrects X axis unit if multiple files were concatenated
            mtd[output_name].getAxis(0).setUnit("Energy")
            SortXAxis(InputWorkspace=output_name, OutputWorkspace=output_name, Ordering="Ascending")
        self.merge_adjacent_bins(output_name)
        monitor_name = output_name + "_mon"
        ExtractMonitors(InputWorkspace=output_name, DetectorWorkspace=output_name, MonitorWorkspace=monitor_name)
        if self.normalise_by == "Monitor":
            Divide(LHSWorkspace=output_name, RHSWorkspace=monitor_name, OutputWorkspace=output_name)
        DeleteWorkspace(Workspace=monitor_name)

    def get_counts_errors_metadata(self, data: np.ndarray) -> Tuple[List[float], List[int], List[float], List[float], List[float]]:
        """
        Processes loaded data and metadata, computes and returns correct energy, (optionally) normalized detector counts
        and errors.

        Args:
            data (ndarray): the data to format
        Return:
             5 arrays, with the values being incident energy, normalized detector counts, errors, times, and temperatures
        """

        energy = [0] * len(data)
        detector_counts = [0] * len(data)
        errors = [0] * len(data)
        time = [0] * len(data)
        temperature = [0] * len(data)

        # if the user wants to see transfer energy, we subtract by the constant offset
        offset = 0 if self.use_incident_energy else self.INCIDENT_ENERGY_OFFSET

        for index, line in enumerate(data):
            energy[index] = line[0] - offset
            monitor_counts = line[1]
            det_counts = line[2]
            time[index] = line[3]
            temperature[index] = line[4]

            detector_counts[index] = det_counts
            if self.normalise_by == "Monitor":
                monitor_errors = np.sqrt(monitor_counts)  # the monitor errors are *not* assumed to be zero, and will be propagated
                detector_counts[index] /= monitor_counts
                errors[index] = np.sqrt(det_counts + ((det_counts**2) * monitor_errors**2) / monitor_counts**2) / monitor_counts
            else:
                errors[index] = np.sqrt(detector_counts[index])
        return energy, detector_counts, errors, time, temperature

    def get_water_correction(self, correction_file: str) -> np.ndarray:
        """
        Loads the provided water correction and passes these values.

        Args:
            correction_file (str): path to the file with water correction
        Return:
            Numpy array with loaded correction data
        """
        try:
            correction = np.loadtxt(correction_file)
        except FileNotFoundError:
            self.log().warning("Water correction file {} not found.".format(correction_file))
            correction = None
        except (RuntimeError, ValueError) as e:
            self.log().warning("Water correction file is faulty.")
            self.log().warning(str(e))
            correction = None
        else:
            # if the sample is in energy transfer unit, we need to make the correction consistent by subtracting constant offset
            if not self.use_incident_energy:
                correction[:, 0] -= self.INCIDENT_ENERGY_OFFSET
            self.water_corr_ws = "__{}_Calibration".format(self.output_ws_name)
            CreateWorkspace(OutputWorkspace=self.water_corr_ws, DataX=correction[:, 0], DataY=correction[:, 1], UnitX="Energy")
            self.intermediate_workspaces.append(self.water_corr_ws)
        return correction

    def merge_adjacent_bins(self, ws: str) -> str:
        """
        Searches the x axis for bin centres closer than EPSILON and if true, uses error-weighted average of
        counts and monitors to create a new bin value, and removes the right hand bin that was too close.

        Args:
            ws (str): name of the input workspace
        Return:
            Name of the workspace, either original one, if unchanged, or new workspace with bins merged
        """
        xAxis = mtd[ws].readX(0)
        maskedX = np.ma.array(xAxis, mask=False)
        yAxis = mtd[ws].extractY()
        maskedY = np.ma.array(yAxis, mask=False)
        eAxis = mtd[ws].extractE()
        maskedE = np.ma.array(eAxis, mask=False)
        index = 0
        n_masked = 0
        while index < mtd[ws].blocksize() - 1:
            if abs(xAxis[index + 1] - xAxis[index]) < self.EPSILON:
                # average counts for both data and monitors using error as weights:
                # first data spectrum
                yAxis[0][index] = (yAxis[0][index] * eAxis[0][index] + yAxis[0][index + 1] * eAxis[0][index + 1]) / (
                    eAxis[0][index] + eAxis[0][index + 1]
                )
                eAxis[0][index] = 0.5 * (eAxis[0][index] + eAxis[0][index + 1])
                # then monitor spectrum
                yAxis[1][index] = (yAxis[1][index] * eAxis[1][index] + yAxis[1][index + 1] * eAxis[1][index + 1]) / (
                    eAxis[1][index] + eAxis[1][index + 1]
                )
                eAxis[1][index] = 0.5 * (eAxis[1][index] + eAxis[1][index + 1])
                # mask indices to be removed
                maskedX.mask[index + 1] = True
                maskedY.mask[0][index + 1] = True
                maskedY.mask[1][index + 1] = True
                maskedE.mask[0][index + 1] = True
                maskedE.mask[1][index + 1] = True
                n_masked += 1
                index += 1  # skip the next bin
            index += 1

        if n_masked > 0:
            # Mantid does not allow to change number of bins using the setX, setY, and setE methods
            # A simple alternative is to create a new workspace containing the same metadata but new axes
            CreateWorkspace(
                OutputWorkspace=ws,
                DataX=maskedX.compressed(),
                DataY=maskedY.compressed().reshape(np.shape(yAxis)[0], np.shape(yAxis)[1] - n_masked),
                DataE=maskedE.compressed().reshape(np.shape(eAxis)[0], np.shape(eAxis)[1] - n_masked),
                NSpec=2,
                ParentWorkspace=ws,
                UnitX="Energy",
            )
        return ws

    def merge_adjacent_points(self, data: np.ndarray) -> np.ndarray:
        """
        Merge points that are close to one another together, summing their values

        Args:
            data (ndarray): a (nb of points, 3)-shaped numpy array, with values (incident energy, monitor counts, detector counts)
        Return:
             A (nb of points, 3)-shaped numpy array, with data sorted and merged by their incident energy.
        """
        # create masked array sorted by incident energy
        data_mask = np.ma.masked_array(data[data[:, 0].argsort()], mask=False)

        # merge adjacent data points
        n_masked = 0
        for point_no, point in enumerate(data):
            # The merging behaviour for multiple points needs to be considered in the future.
            # For now, if two points are close enough the latter is removed,
            # then the comparison continues with next point.
            # This could be troublesome if there are a lot of points separated by very short distances,
            # but it normally shouldn't happen on Lagrange data
            try:
                if (data[point_no + 1][0] - point[0]) < self.EPSILON:
                    data_mask.mask[point_no] = True
                    n_masked += 1
            except IndexError:
                break
        return data_mask.compressed().reshape(np.shape(data)[0] - n_masked, np.shape(data)[1])

    def correct_data(self, ws_to_correct: str, corrected_ws: str):
        """
        Apply water correction to the provided data

        Args:
            ws_to_correct (str): the name of the workspace holding the data to be corrected
            corrected_ws (str): the name of the workspace that should hold the corrected value. It will be created.
        """

        # we need to get the data and interpolate it with numpy because Mantid only have spline interpolation and the
        # water correction has a shape that cause the spline to go completely off
        # so we are using numpy instead
        energy = mtd[ws_to_correct].readX(0)

        interpolated_corr = np.interp(energy, self.water_correction[:, 0], self.water_correction[:, 1])

        sample_type = "EC" if "EC" in corrected_ws else "S"
        interpolated_water_ws = "{}_interp{}".format(self.water_corr_ws, sample_type)
        CreateWorkspace(OutputWorkspace=interpolated_water_ws, DataX=energy, DataY=interpolated_corr, UnitX="Energy")
        self.intermediate_workspaces.append(interpolated_water_ws)

        Multiply(LHSWorkspace=ws_to_correct, RHSWorkspace=interpolated_water_ws, OutputWorkspace=corrected_ws)

    def process_empty_cell(self, empty_cell_files: List[str]):
        """
        Process empty cell files by loading the raw data, adding metadata, normalising to monitor (if requested) and
        performing water correction.

        Args:
            empty_cell_files (list(str)): list with paths to empty cell data
        """
        # load and format empty cell
        self.empty_cell_ws = "__" + self.output_ws_name + "_rawEC"
        if self._empty_cell_nexus:
            self.preprocess_nexus(empty_cell_files, self.empty_cell_ws)
        else:
            empty_cell_data = self.load_and_concatenate(empty_cell_files)
            empty_cell_data = self.merge_adjacent_points(empty_cell_data)
            energy, detector_counts, errors, time, temperature = self.get_counts_errors_metadata(empty_cell_data)
            CreateWorkspace(OutputWorkspace=self.empty_cell_ws, DataX=energy, DataY=detector_counts, DataE=errors, UnitX="Energy")
            self.add_metadata(self.empty_cell_ws, time, temperature)

        self.intermediate_workspaces.append(self.empty_cell_ws)

        # correct by water
        if self.water_correction is not None:
            corrected_EC_name = "{}_Calibrated".format(self.empty_cell_ws)
            self.correct_data(self.empty_cell_ws, corrected_EC_name)
            self.intermediate_workspaces.append(corrected_EC_name)
            self.empty_cell_ws = corrected_EC_name

    def subtract_empty_cell(self):
        """
        Subtract the empty cell data from the sample.
        """
        # interpolating, because the empty cell may not have the same binning as the sample
        # let's hope that the spline behaves
        interpolated_empty_cell_ws = "{}_interpS".format(self.empty_cell_ws)
        SplineInterpolation(
            WorkspaceToMatch=self.output_ws_name, WorkspaceToInterpolate=self.empty_cell_ws, OutputWorkspace=interpolated_empty_cell_ws
        )
        self.intermediate_workspaces.append(interpolated_empty_cell_ws)

        Subtract(LHSWorkspace=self.output_ws_name, RHSWorkspace=interpolated_empty_cell_ws, OutputWorkspace=self.output_ws_name)

        final_ws_name = "__{}_final".format(self.output_ws_name)
        CloneWorkspace(InputWorkspace=self.output_ws_name, OutputWorkspace=final_ws_name)
        self.intermediate_workspaces.append(final_ws_name)


AlgorithmFactory.subscribe(LagrangeILLReduction)
