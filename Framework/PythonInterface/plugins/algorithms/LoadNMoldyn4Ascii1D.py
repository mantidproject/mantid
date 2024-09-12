# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace, GroupWorkspaces
from mantid.kernel import *
from mantid.api import *

import numpy as np
import scipy.constants as sc
import ast
import fnmatch
import re
import os

FUNC_NAME_REGEX = re.compile(r"#\s+variable name:\s+((.*)_.*)")
AXIS_REGEX = re.compile(r"#\s+axis:\s+([A-z]+)")
UNIT_REGEX = re.compile(r"#\s+units:\s+(.*)")
SLICE_HEADER_REGEX = re.compile(r"#slice:\[([0-9]+)[A-z]*\]")


class LoadNMoldyn4Ascii1D(PythonAlgorithm):
    data_directory = None

    def category(self):
        return "Simulation; Inelastic\\DataHandling"

    def summary(self):
        return "Imports 1D dos and vac functions from an nMolDyn 4 output file," "convoluting it with a resolution function if required."

    def PyInit(self):
        self.declareProperty(FileProperty("Directory", "", action=FileAction.Directory), doc=("Path to directory containing dat files"))

        self.declareProperty(StringArrayProperty("Functions"), doc="Names of functions to attempt to load from file")

        self.declareProperty(
            "ResolutionConvolution", "No", StringListValidator(["No", "TOSCA"]), doc="Use resolution function to 'smear' dos data?"
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace name")

    def validateInputs(self):
        issues = dict()

        if len(self.getProperty("Functions").value) == 0:
            issues["Functions"] = "Must specify at least one function to load"

        return issues

    def PyExec(self):
        self.data_directory = self.getPropertyValue("Directory")
        # Converts the specified functions into full filenames and finds them in directory
        data_files = [os.path.splitext(f)[0] for f in fnmatch.filter(os.listdir(self.data_directory), "*.dat")]
        logger.debug("All data files: {0}".format(data_files))
        chosen_functions = [x for x in self.getProperty("Functions").value]
        func_names = [f for f in data_files if f.replace(",", "") in chosen_functions]
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=len(func_names))
        logger.debug("Functions to load: {0}".format(func_names))
        loaded_function_workspaces = []
        out_ws_name = self.getPropertyValue("OutputWorkspace")

        for function in func_names:
            prog_reporter.report("Loading {0} function".format(function))
            # Loads the two axes
            y_axis = self.read_axis(function)
            x_axis = self.read_axis(y_axis[3])

            # Converts the x-axis units and sets all axis properties
            x_axis = self.axis_conversion(x_axis[0], x_axis[1], y_axis[3])
            y_data = y_axis[0]
            y_name = y_axis[2]
            x_data = x_axis[0]
            x_unit = x_axis[1]
            x_name = x_axis[2]

            # Convolutes the data if required
            if self.getPropertyValue("ResolutionConvolution") == "TOSCA" and x_name == "frequency":
                resolutions = self.gaussians(x_data, self.TOSCA_resfunction)
                y_data = self.convolutor(y_data, resolutions, x_data)
                logger.information("Function " + str(y_name) + " will be convoluted")

            # Create the workspace for function
            ws_title = out_ws_name + "(" + function + ")"
            CreateWorkspace(OutputWorkspace=ws_title, DataX=x_data, DataY=y_data, UnitX=x_unit, WorkspaceTitle=ws_title)

            loaded_function_workspaces.append(ws_title)

        if len(loaded_function_workspaces) == 0:
            raise RuntimeError("Failed to load any functions for data")
        GroupWorkspaces(InputWorkspaces=loaded_function_workspaces, OutputWorkspace=out_ws_name)
        # Set the output workspace
        self.setProperty("OutputWorkspace", out_ws_name)

    def read_axis(self, func_name):
        # Loads an axis file from the directory.

        # Axis file loaded is directory\func_name.dat
        # Returns a tuple of (Numpy array of data, unit, func_name, x-axis name)

        file_name = os.path.join(self.data_directory, "{0}.dat".format(func_name))
        x_axis = 0
        unit = ""
        header_data = [unit, func_name, x_axis]
        with open(file_name, "rU") as f_handle:
            while True:
                line = f_handle.readline()
                if not line:
                    break
                if len(line[0]) == 0:
                    pass
                # Parse metadata from file
                elif line[0] == "#":
                    # Read header data
                    header_data = self._parse_header_data(line, header_data)
                    # Read data from file
                    slice_match = SLICE_HEADER_REGEX.match(line)
                    if slice_match:
                        length = int(slice_match.group(1))
                        data = self.slice_read(f_handle, length)
            f_handle.close()
            data = self.del_symmetry(data)
        return (data, header_data[0], header_data[1], header_data[2])

    def _parse_header_data(self, line, header_data):
        """
        Attempts to parse current line as unit, function_name or x_axis
        """
        # Attempt to match unit
        unit_match = UNIT_REGEX.match(line)
        if unit_match:
            header_data[0] = unit_match.group(1)
        # Attempt to match fucntion name
        func_match = FUNC_NAME_REGEX.match(line)
        if func_match:
            header_data[1] = func_match.group(1)
        # Attempt to match x_axis
        axis_match = AXIS_REGEX.match(line)
        if axis_match:
            header_data[2] = axis_match.group(1)

        return header_data

    def slice_read(self, file_handle, length):
        # Loads the actual data from a data file

        # file_handle is opened file, length is the number of data points loaded
        # Returns numpy array of data

        data = []
        i = 0
        while i < length:
            line = file_handle.readline()
            if not line:
                break
            data.append(ast.literal_eval(line))
            i += 1
        data = np.asarray(data)
        return data

    def del_symmetry(self, data):
        # If axis loaded is symmetrical, ie it goes from -xHz to xHz,
        # chops the data in half, discarding physically meaningless data points

        if abs(data[0]) == abs(data[-1]):
            half_length = int(float(len(data)) / 2 - 0.5)
            data = data[half_length:]
        return data

    def axis_conversion(self, data, unit, name):
        # Converts the x-axis units to a format Mantid accepts,
        # frequency to wavenumber, time to time of flight

        logger.debug("Axis for conversion: name={0}, unit={1}".format(name, unit))
        if name == "frequency" and unit == "THz":
            logger.information("Axis {0} will be converted to wave number in cm^-1".format(name))
            unit = "Energy_inWavenumber"
            data *= sc.tera
            data *= sc.value("Planck constant in eV s")
            data *= 8065.54
        elif name == "time" and unit == "ps":
            logger.information("Axis {0} will be converted to time in microsecond".format(name))
            unit = "TOF"
            data *= sc.micro
        else:
            unit = "Empty"
        return (data, unit, name)

    def TOSCA_resfunction(self, x_data):
        # Approximate resolution function for Tosca delE = f(E)
        # Used in convolution

        x_data = np.array(x_data)
        delE = x_data * (16 * np.exp(-x_data * 0.0295) + 0.000285 * x_data + 1.058)
        delE /= 100
        return delE

    def gaussianfunc(self, x_data, point_x, resfunction, fwhm):
        # Outputs a gaussian peak for a point x on the wavenumber axis, with
        # the width of the Gaussian a function of wavenumber(x)
        # Used in convolution

        x_data = np.array(x_data)
        coeff_arr = -((x_data - point_x) ** 2) / (2 * fwhm**2)
        g_of_x = np.exp(coeff_arr)

        return g_of_x

    def integrator(self, y_data, g_of_x, x_data):
        # Numerically integrates two functions
        # Used in convolution

        h_of_x = 0
        for i in range(len(x_data)):
            h_of_x += y_data[i] * g_of_x[i]
        return h_of_x

    def gaussians(self, x_data, resfunction):
        # Outputs a list of gaussian functions, one for each value of x
        # Used in convolution

        all_gx = []
        fwhm_arr = fwhm_arr = resfunction(x_data) / 2.35482
        all_gx = [self.gaussianfunc(x_data, x_data[i], resfunction, fwhm_arr[i]) for i in range(len(x_data))]
        return all_gx

    def convolutor(self, y_data, all_gx, x_data):
        # Convolutes two functions, ouputting a numpy array of the convolution

        g_of_x = []
        for i in range(len(x_data)):
            g_of_x.append(self.integrator(y_data, all_gx[i], x_data))
        return np.asarray(g_of_x)


AlgorithmFactory.subscribe(LoadNMoldyn4Ascii1D)
