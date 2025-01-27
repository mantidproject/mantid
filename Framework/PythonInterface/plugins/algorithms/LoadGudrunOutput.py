# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import FileProperty, WorkspaceProperty, PythonAlgorithm, AlgorithmFactory, FileAction, PropertyMode
from mantid.kernel import Direction
from mantid.simpleapi import LoadAscii, CreateWorkspace

import itertools


class LoadGudrunOutput(PythonAlgorithm):
    def name(self):
        return "LoadGudrunOutput"

    def category(self):
        return "DataHandling"

    def summary(self):
        return "Loads the common outputs created from Gudrun"

    def PyInit(self):
        self.declareProperty(
            FileProperty(
                name="InputFile", defaultValue="", action=FileAction.Load, extensions=[".dcs01", ".mdsc01", ".mint01", ".mdor01", ".mgor01"]
            ),
            doc="Gudrun output file to be loaded.",
        )
        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="If No OutputWorkspace is provided, then the workpsace name will be obtained from the meta data in the input file.",
        )

    def PyExec(self):
        input_file = self.getProperty("InputFile").value
        output_ws = self.getPropertyValue("OutputWorkspace")
        if not output_ws:
            output_ws = self.get_title(input_file)
        number_of_columns, data_line_start = self.find_number_of_columns(input_file)
        self.load_gudrun_file(input_file, output_ws, number_of_columns, data_line_start)
        self.setProperty("OutputWorkspace", output_ws)

    def get_title(self, input_file):
        """
        Return the title from the file meta data
        :param input_file: file to get meta data from
        :return: (title)
        """
        with open(input_file, "r") as gudrun_file:
            first_line = gudrun_file.readline()
            first_line = first_line[2:]
            return first_line.replace(".", "-")

    def find_number_of_columns(self, input_file):
        """
        Evaluate how many columns of data there are in the file
        :param input_file: The file to check
        :return: (The number of columns of data, the first line of data)
        """
        with open(input_file, "r") as gudrun_file:
            data_line_start = 0
            while gudrun_file.readline().startswith("#"):
                # skip over lines that are commented
                data_line_start += 1
            row = self.format_data_row(gudrun_file.readline().split(" "))
            return len(row), data_line_start

    def load_gudrun_file(self, input_file, output_workspace, number_of_columns, first_data_line):
        """
        Loads the gudrun file using the mantid LoadAscii algorithm
        :param input_file: The file to load
        :param output_workspace: The workspace to be the result of the load
        :param number_of_columns: The number of columns in the file being loaded
        :param first_data_line: The first line to expect data on
        :return: The outputWorkspace of the Load Algorithm
        """

        if number_of_columns % 2 == 0:
            print(number_of_columns)
            raise ValueError(
                "Incorrect data format: The input file {} must have an odd number "
                "of columns in the format X , Y , E, Y1, E1, Y2, E2, ...".format(input_file)
            )
        elif number_of_columns == 3:
            LoadAscii(Filename=input_file, OutputWorkspace=output_workspace, CommentIndicator="#")
        else:
            self.load_multi_column_file(input_file, output_workspace, first_data_line)

    def load_multi_column_file(self, input_file, output_workspace, first_data_line):
        """
        Load a file that has an odd number of columns that is more than 3
        :param input_file: The file to extract the data from
        :param output_workspace: tThe file to put the data into
        :param first_data_line: The first line to expect data on
        :return: The outputWorkspace pointer
        """
        with open(input_file, "r") as gudrun_file:
            data_rows = [self.format_data_row(line.split(" ")) for line in gudrun_file.readlines()[first_data_line:]]
        x_data, y_data, e_data = self.create_2d_data_arrays(data_rows)
        n_spec = int((len(data_rows[0]) - 1) / 2)
        CreateWorkspace(OutputWorkspace=output_workspace, DataX=x_data, DataY=y_data, DataE=e_data, NSpec=n_spec)

    def format_data_row(self, data_row):
        """
        Remove special characters and empty lines from data list
        :param data_row: The data to format
        :return: a formatted data row
        """
        formatted_row = [data.rstrip() for data in data_row if data != ""]
        return formatted_row

    def create_2d_data_arrays(self, all_data):
        """
        Create 1d x data array and 2d y and e data arrays
        :param all_data: All the data to create arrays from. Assumed format of:
                         X, Y, E, Y1, E1, Y2, E2 ... (first bin)
                         X, Y, E, Y1, E1, Y2, E2 ... (second bin)
        :return: ([x_data], [y, y1, y2, y3], [e, e1, e2, e3])
        """
        row_length = len(all_data[0])
        x_data = []
        # Create empty 2d lists
        y_data = [[] for _ in range(int((row_length - 1) / 2.0))]
        e_data = [[] for _ in range(int((row_length - 1) / 2.0))]
        for data_row in all_data:
            x_data.append(float(data_row[0]))
            for row_index in range(1, len(data_row)):
                if row_index % 2 == 1:
                    # y_data
                    data_array_index = int((row_index / 2.0) - 0.5)
                    y_data[data_array_index].append(float(data_row[row_index]))
                else:
                    # e_data
                    data_array_index = int((row_index / 2.0) - 1.0)
                    e_data[data_array_index].append(float(data_row[row_index]))
        # collapse 2d lists
        y_data = list(itertools.chain.from_iterable(y_data))
        e_data = list(itertools.chain.from_iterable(e_data))
        return x_data, y_data, e_data


AlgorithmFactory.subscribe(LoadGudrunOutput)
