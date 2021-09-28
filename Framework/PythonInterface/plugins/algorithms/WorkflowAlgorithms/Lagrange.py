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


def load_and_concatenate(files):
    """
    Taking Lagrange data files as input, load the interesting data from it and concatenate them into one numpy array
    @param files the ascii data files to load and concatenate together
    @return the values concatenated, as a (nb of points, 3)-shaped numpy array,
    with values (incident energy, monitor counts, detector counts)
    """
    loaded_data = None
    for file in files:
        # load columns 1, 2 and 5 of the file - angle, monitor count, detector count
        data = np.loadtxt(file, dtype=float, skiprows=38, usecols=(1, 2, 5))

        loaded_data = np.vstack((loaded_data, data)) if loaded_data is not None else data

    return loaded_data


def format_values(data):
    """
    Normalize detector counts by monitor counts and set errors
    @param data the data to format
    @return a (nb of points, 3)-shaped numpy array, with values (incident energy, normalized counts, errors)
    """
    for line in data:
        line[1], line[2] = line[2] / line[1], np.sqrt(line[2]) / line[1]
    return data


def merge_and_average(data):
    """
    Merge points that are close to one another together, averaging their values
    @param data the data, a (nb of points, 3)-shaped numpy array.
    """
    # max difference between two identical points
    epsilon = 1e-2

    # sort by incident energy
    data = data[data[:, 0].argsort()]

    # merge adjacent data points together
    current_sum = 0
    current_writing_index = 0
    for point in data:
        if point[0] - data[current_writing_index][0] < epsilon:
            data[current_writing_index] = (data[current_writing_index] * current_sum + point) / (current_sum + 1)
            # TODO how to average errors ?
            # TODO decide on a merging behaviour for multiple points
            current_sum += 1
        else:
            current_sum = 1
            current_writing_index += 1
            data[current_writing_index] = point

    return data[:current_writing_index + 1]


class LagrangeTMP(DataProcessorAlgorithm):

    progress = None
    sample = None
    empty_cell = None
    correction_data = None
    output = None

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
        return 'LagrangeTMP'

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('SampleRuns', action=FileAction.Load, extensions=['']),
                             doc='Sample run(s).')
        self.declareProperty(MultipleFileProperty('ContainerRuns', action=FileAction.OptionalLoad, extensions=['']),
                             doc='Container run(s) (empty cell)')
        self.declareProperty(FileProperty('CorrectionFile', '', action=FileAction.OptionalLoad, extensions=['.txt']),
                             doc='Correction reference file.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

    def PyExec(self):

        correction_file = self.getPropertyValue('CorrectionFile')

        # load correction
        if correction_file:
            self.load_correction(correction_file)

        empty_cell_file = self.getPropertyValue('ContainerRuns').split(',')

        # empty cell treatment, if there is any
        if empty_cell_file[0]:
            self.empty_cell = load_and_concatenate(empty_cell_file)
            self.empty_cell = format_values(self.empty_cell)
            self.empty_cell = merge_and_average(self.empty_cell)

            if self.correction_data is not None:
                self.correct_data(self.empty_cell)

        # sample load and formatting
        sample_file = self.getPropertyValue('SampleRuns').split(',')
        self.sample = load_and_concatenate(sample_file)
        self.sample = format_values(self.sample)
        self.sample = merge_and_average(self.sample)

        # sample corrections
        if self.correction_data is not None:
            self.correct_data(self.sample)

        if self.empty_cell is not None:
            self.subtract_empty_cell()

        # putting the values in a workspace
        CreateWorkspace(outputWorkspace=self.getPropertyValue('OutputWorkspace'), DataX=self.sample[:, 0],
                        DataY=self.sample[:, 1], DataE=self.sample[:, 2])

        self.setProperty('OutputWorkspace', mtd[self.getPropertyValue('OutputWorkspace')])

    def load_correction(self, file):
        """
        Load correction data
        @param file the .txt file from which to load the correction
        """
        self.correction_data = np.loadtxt(file)

    def correct_data(self, data):
        """
        Apply water correction to the provided data
        @param data the data to be corrected, as a numpy array of shape (nb of points, 3)
        """
        # interpolating, because the correction's binning is not the same as the data
        interpolated_corr = np.interp(data[:, 0], self.correction_data[:, 0], self.correction_data[:, 1])
        for data_point, corr_factor in zip(data, interpolated_corr):
            data_point[1] *= corr_factor
            # TODO what about errors ?

    def subtract_empty_cell(self):
        """
        Subtract the empty cell data from the sample.
        """
        # interpolating, because the empty cell may not have the same binning as the sample
        interpolated_empty = np.interp(self.sample[:, 0], self.empty_cell[:, 0], self.empty_cell[:, 1])
        interpolated_empty_errors = np.interp(self.sample[:, 0], self.empty_cell[:, 0], self.empty_cell[:, 2])

        self.sample[:, 1] = self.sample[:, 1] - interpolated_empty
        self.sample[:, 2] = np.sqrt(self.sample[:, 2] ** 2 + interpolated_empty_errors ** 2)


AlgorithmFactory.subscribe(LagrangeTMP)
