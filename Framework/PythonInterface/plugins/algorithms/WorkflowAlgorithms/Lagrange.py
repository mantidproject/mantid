# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import DataProcessorAlgorithm, MultipleFileProperty, FileAction, MatrixWorkspaceProperty, FileProperty
from mantid.kernel import Direction
from mantid.simpleapi import *
import numpy as np


class ILLLagrange(DataProcessorAlgorithm):

    progress = None
    output_ws_name = None
    empty_cell_ws = None
    water_correction = None
    intermediate_workspaces = None

    @staticmethod
    def category():
        return 'ILL\\Indirect'

    @staticmethod
    def summary():
        return 'Reduces Lagrange data.'

    @staticmethod
    def seeAlso():
        return []

    @staticmethod
    def name():
        return 'ILLLagrange'

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('SampleRuns', action=FileAction.Load, extensions=['']),
                             doc='Sample run(s).')
        self.declareProperty(MultipleFileProperty('ContainerRuns', action=FileAction.OptionalLoad, extensions=['']),
                             doc='Container run(s) (empty cell)')
        self.declareProperty(FileProperty('CorrectionFile', '', action=FileAction.OptionalLoad, extensions=['.txt']),
                             doc='Correction reference file.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace containing reduced data.')

        # the list of all the intermediate workspaces to group at the end
        self.intermediate_workspaces = []

    def PyExec(self):

        correction_file = self.getPropertyValue('CorrectionFile')
        self.output_ws_name = self.getPropertyValue('OutputWorkspace')

        # load correction
        if correction_file:
            self.load_correction(correction_file)

        empty_cell_files = self.getPropertyValue('ContainerRuns').split(',')

        # empty cell treatment, if there is any
        if empty_cell_files[0]:
            # load and format empty cell
            empty_cell_data = self.load_and_concatenate(empty_cell_files)
            empty_cell_data = self.merge_adjacent_points(empty_cell_data)
            energy, detector_counts, errors = self.format_values(empty_cell_data)

            self.empty_cell_ws = "__" + self.output_ws_name + "_rawEC"

            CreateWorkspace(outputWorkspace=self.empty_cell_ws,
                            DataX=energy,
                            DataY=detector_counts,
                            DataE=errors,
                            UnitX="meV")

            self.intermediate_workspaces.append(self.empty_cell_ws)

            # correct by water
            if self.water_correction is not None:
                correctedECname = "__" + self.output_ws_name + "_waterCorrectedEC"
                self.correct_data(self.empty_cell_ws, correctedECname)

                self.intermediate_workspaces.append(correctedECname)

        # sample load and formatting
        sample_files = self.getPropertyValue('SampleRuns').split(',')
        sample_data = self.load_and_concatenate(sample_files)
        sample_data = self.merge_adjacent_points(sample_data)
        energy, detector_counts, errors = self.format_values(sample_data)

        raw_sample_ws = "__" + self.output_ws_name + "_rawSample"
        CreateWorkspace(outputWorkspace=raw_sample_ws,
                        DataX=energy,
                        DataY=detector_counts,
                        DataE=errors,
                        UnitX="meV")

        self.intermediate_workspaces.append(raw_sample_ws)

        # sample correction by water
        if self.water_correction is not None:

            water_corrected_ws = "__" + self.output_ws_name + "_waterCorrectedSample"
            self.correct_data(raw_sample_ws, water_corrected_ws)

            self.intermediate_workspaces.append(water_corrected_ws)

        # clone the last workspace - either the raw or water corrected data - to the destination name
        CloneWorkspace(InputWorkspace=self.intermediate_workspaces[-1],
                       OutputWorkspace=self.output_ws_name)

        # correct by empty cell if there is any
        if self.empty_cell_ws is not None:
            self.subtract_empty_cell()

        # set the properties and group the hidden workspace
        self.setProperty('OutputWorkspace', self.output_ws_name)
        GroupWorkspaces(OutputWorkspace='__' + self.output_ws_name, InputWorkspaces=self.intermediate_workspaces)

    def load_and_concatenate(self, files):
        """
        Taking Lagrange data files as input, load the interesting data from it and concatenate them into one numpy array
        @param files the ascii data files to load and concatenate together
        @return the values concatenated, as a (nb of points, 3)-shaped numpy array,
        with values (incident energy, monitor counts, detector counts)
        """
        loaded_data = None
        for file in files:
            header_index = -1
            energy_col = -1
            detector_counts_col = -1
            monitor_counts_col = -1

            # we first open the file to find the size of the header and the position of the columns holding the data
            with open(file, "r") as f:
                for index, line in enumerate(f):
                    if line == "DATA_:\n":
                        header_index = index
                        next_line = f.__next__().split()

                        if "EI" in next_line:
                            energy_col = next_line.index("EI")
                        else:
                            energy_col = 1
                            self.log().warning("Could not find energy column in file {}. "
                                               "Defaulting to column 1".format(file))
                        if "CNTS" in next_line:
                            detector_counts_col = next_line.index("CNTS")
                        else:
                            detector_counts_col = 5
                            self.log().warning("Could not find detector counts column in file {}. "
                                               "Defaulting to column 5".format(file))
                        if "M1" in next_line:
                            monitor_counts_col = next_line.index("M1")
                        else:
                            monitor_counts_col = 2
                            self.log().warning("Could not find monitor counts column in file {}. "
                                               "Defaulting to column 2".format(file))
                        break
                f.close()

            if header_index < 0:
                self.log().warning("Could not determine header length in file {}. Defaulting to 36".format(file))
                header_index = 36
                energy_col = 1
                detector_counts_col = 5
                monitor_counts_col = 2

            # load the relevant data, which position we just determined - angle, monitor count, detector count
            data = np.loadtxt(file,
                              dtype=float,
                              skiprows=header_index + 2,
                              usecols=(energy_col, monitor_counts_col, detector_counts_col))

            loaded_data = np.vstack((loaded_data, data)) if loaded_data is not None else data

        return loaded_data

    @staticmethod
    def format_values(data):
        """
        Normalize detector counts by monitor counts and set errors
        @param data the data to format
        @return 3 arrays, with the values being incident energy, normalized counts, errors
        """

        # we offset by the incident energy
        incident_energy = 4.5

        energy = [0]*len(data)
        detector_counts = [0]*len(data)
        errors = [0]*len(data)

        for index, line in enumerate(data):
            energy[index] = line[0] - incident_energy
            detector_counts[index] = line[2] / line[1]
            errors[index] = np.sqrt(line[2]) / line[1]

        return energy, detector_counts, errors

    @staticmethod
    def merge_adjacent_points(data):
        """
        Merge points that are close to one another together, summing their values
        @param data a (nb of points, 3)-shaped numpy array, with values (incident energy, monitor counts, detector counts)
        @return a (nb of points, 3)-shaped numpy array, with data sorted and merged by their incident energy.
        """
        # max difference between two identical points
        # two points closer than that will be merged
        epsilon = 1e-2

        # sort by incident energy
        data = data[data[:, 0].argsort()]

        # sum adjacent data points
        current_writing_index = 0
        for point in data:
            if point[0] - data[current_writing_index][0] < epsilon:
                data[current_writing_index][1] = point[1]
                data[current_writing_index][2] = point[2]
                # TODO decide on a merging behaviour for multiple points
                # for now, if two points are close enough there are merged,
                # and their mean energy is taken as the new energy for the new point,
                # then this new energy is compared with next point.
                # this could be troublesome if there are a lot of points separated by very short distances,
                # but it normally shouldn't happen on Lagrange data
            else:
                current_writing_index += 1
                data[current_writing_index] = point

        return data[:current_writing_index + 1]

    def load_correction(self, file):
        """
        Load correction data
        @param file the .txt file from which to load the correction
        """
        self.water_correction = np.loadtxt(file)

    def correct_data(self, ws_to_correct, corrected_ws):
        """
        Apply water correction to the provided data
        @param ws_to_correct the name of the workspace holding the data to be corrected
        @param corrected_ws the name of the workspace that should hold the corrected value. It will be created.
        """

        # we need to get the data and interpolate it with numpy because Mantid only have spline interpolation and the
        # water correction has a shape that cause the spline to go completely off
        # so we are using numpy instead
        energy = mtd[ws_to_correct].readX(0)

        interpolated_corr = np.interp(energy, self.water_correction[:, 0], self.water_correction[:, 1])

        CreateWorkspace(OutputWorkspace="__interp_water" + self.output_ws_name,
                        DataX=energy,
                        DataY=interpolated_corr)

        Multiply(LHSWorkspace=ws_to_correct,
                 RHSWorkspace="__interp_water" + self.output_ws_name,
                 OutputWorkspace=corrected_ws)

        DeleteWorkspace(Workspace="__interp_water" + self.output_ws_name)

    def subtract_empty_cell(self):
        """
        Subtract the empty cell data from the sample.
        """
        # interpolating, because the empty cell may not have the same binning as the sample
        # let's hope that the spline behaves
        SplineInterpolation(WorkspaceToMatch=self.output_ws_name,
                            WorkspaceToInterpolate=self.empty_cell_ws,
                            OutputWorkspace="__interp" + self.empty_cell_ws)

        Subtract(LHSWorkspace=self.output_ws_name,
                 RHSWorkspace="__interp" + self.empty_cell_ws,
                 OutputWorkspace=self.output_ws_name)

        DeleteWorkspace(Workspace="__interp" + self.empty_cell_ws)


AlgorithmFactory.subscribe(ILLLagrange)
