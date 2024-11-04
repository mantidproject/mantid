# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, AlgorithmManager, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction, StringArrayProperty
from mantid.simpleapi import GroupWorkspaces

import numpy as np
import scipy.constants as sc
import ast
import fnmatch
import re
import os

# ------------------------------------------------------------------------------

VARIABLE_REGEX = re.compile(r"#\s+variable name:\s+(.*)")
TYPE_REGEX = re.compile(r"#\s+type:\s+([A-z]+)")
AXIS_REGEX = re.compile(r"#\s+axis:\s+([A-z]+)\|([A-z]+)")
UNIT_REGEX = re.compile(r"#\s+units:\s+(.*)")

SLICE_1D_HEADER_REGEX = re.compile(r"#slice:\[([0-9]+)[A-z]*\]")
SLICE_2D_HEADER_REGEX = re.compile(r"#slice:\[([0-9]+)[A-z]*,\s+([0-9]+)[A-z]*\]")

# ------------------------------------------------------------------------------


class LoadNMoldyn4Ascii(PythonAlgorithm):
    _axis_cache = None
    _data_directory = None

    # ------------------------------------------------------------------------------

    def category(self):
        return "Inelastic\\DataHandling;Simulation"

    def summary(self):
        return "Imports functions from .dat files output by nMOLDYN 4."

    # ------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(FileProperty("Directory", "", action=FileAction.Directory), doc="Path to directory containg .dat files")

        self.declareProperty(StringArrayProperty("Functions"), doc="Names of functions to attempt to load from file")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace name")

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        issues = dict()

        if len(self.getProperty("Functions").value) == 0:
            issues["Functions"] = "Must specify at least one function to load"

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):
        self._axis_cache = {}
        self._data_directory = self.getPropertyValue("Directory")

        # Convert the simplified function names to the actual file names
        data_directory_files = [os.path.splitext(f)[0] for f in fnmatch.filter(os.listdir(self._data_directory), "*.dat")]
        logger.debug("All data files: {0}".format(data_directory_files))
        functions_input = [x.strip().replace(",", "") for x in self.getProperty("Functions").value]
        functions = [f for f in data_directory_files if f.replace(",", "") in functions_input]
        logger.debug("Functions to load: {0}".format(functions))

        loaded_function_workspaces = []
        for func_name in functions:
            try:
                # Load the intensity data
                function = self._load_function(func_name)
                # Load (or retrieve) the axis data
                v_axis = self._load_axis(function[2][0])
                x_axis = self._load_axis(function[2][1])

                # Perform axis unit conversions
                v_axis = self._axis_conversion(*v_axis)
                x_axis = self._axis_conversion(*x_axis)

                # Create the workspace for function
                create_workspace = AlgorithmManager.Instance().create("CreateWorkspace")
                create_workspace.initialize()
                create_workspace.setLogging(False)
                create_workspace.setProperty("OutputWorkspace", func_name)
                create_workspace.setProperty("DataX", x_axis[0])
                create_workspace.setProperty("DataY", function[0])
                create_workspace.setProperty("NSpec", v_axis[0].size)
                create_workspace.setProperty("UnitX", x_axis[1])
                create_workspace.setProperty("YUnitLabel", function[1])
                create_workspace.setProperty("VerticalAxisValues", v_axis[0])
                create_workspace.setProperty("VerticalAxisUnit", v_axis[1])
                create_workspace.setProperty("WorkspaceTitle", func_name)
                create_workspace.execute()

                loaded_function_workspaces.append(func_name)

            except ValueError as rerr:
                logger.warning("Failed to load function {0}. Error was: {1}".format(func_name, str(rerr)))

        # Process the loaded workspaces
        out_ws_name = self.getPropertyValue("OutputWorkspace")
        if len(loaded_function_workspaces) == 0:
            raise RuntimeError("Failed to load any functions for data")
        GroupWorkspaces(InputWorkspaces=loaded_function_workspaces, OutputWorkspace=out_ws_name)

        # Set the output workspace
        self.setProperty("OutputWorkspace", out_ws_name)

    # ------------------------------------------------------------------------------

    def _load_function(self, function_name):
        """
        Loads a function from the data directory.

        @param function_name Name of the function to load
        @return Tuple of (Numpy array of data, unit, (v axis name, x axis name))
        @exception ValueError If function is not found
        """
        function_filename = os.path.join(self._data_directory, "{0}.dat".format(function_name))
        if not os.path.isfile(function_filename):
            raise ValueError('File for function "{0}" not found'.format(function_name))

        data = None
        axis = (None, None)
        unit = None

        with open(function_filename, "rU") as f_handle:
            while True:
                line = f_handle.readline()
                if not line:
                    break

                # Ignore empty lines
                if len(line[0]) == 0:
                    pass

                # Parse header lines
                elif line[0] == "#":
                    variable_match = VARIABLE_REGEX.match(line)
                    if variable_match and variable_match.group(1) != function_name:
                        raise ValueError("Function name differs from file name")

                    axis_match = AXIS_REGEX.match(line)
                    if axis_match:
                        axis = (axis_match.group(1), axis_match.group(2))

                    unit_match = UNIT_REGEX.match(line)
                    if unit_match:
                        unit = unit_match.group(1)

                    slice_match = SLICE_2D_HEADER_REGEX.match(line)
                    if slice_match:
                        dimensions = (int(slice_match.group(1)), int(slice_match.group(2)))
                        # Now parse the data
                        data = self._load_2d_slice(f_handle, dimensions)

        return (data, unit, axis)

    # ------------------------------------------------------------------------------

    def _load_axis(self, axis_name):
        """
        Loads an axis by name from the data directory.

        @param axis_name Name of axis to load
        @return Tuple of (Numpy array of data, unit, name)
        @exception ValueError If axis is not found
        """
        if axis_name in self._axis_cache:
            return self._axis_cache[axis_name]

        axis_filename = os.path.join(self._data_directory, "{0}.dat".format(axis_name))
        if not os.path.isfile(axis_filename):
            raise ValueError('File for axis "{0}" not found'.format(axis_name))

        data = None
        unit = None

        with open(axis_filename, "rU") as f_handle:
            while True:
                line = f_handle.readline()
                if not line:
                    break

                # Ignore empty lines
                if len(line[0]) == 0:
                    pass

                # Parse header lines
                elif line[0] == "#":
                    variable_match = VARIABLE_REGEX.match(line)
                    if variable_match and variable_match.group(1) != axis_name:
                        raise ValueError("Axis name differs from file name")

                    unit_match = UNIT_REGEX.match(line)
                    if unit_match:
                        unit = unit_match.group(1)

                    slice_match = SLICE_1D_HEADER_REGEX.match(line)
                    if slice_match:
                        length = int(slice_match.group(1))
                        # Now parse the data
                        data = self._load_1d_slice(f_handle, length)

        return (data, unit, axis_name)

    # ------------------------------------------------------------------------------

    def _load_1d_slice(self, f_handle, length):
        """
        Loads a 1D slice from the open file.

        @param f_handle Handle to the open file with the iterator at the slice header
        @param length Length of data
        @return Numpy array of length [length]
        """
        data = np.ndarray(shape=(length), dtype=float)

        for idx in range(length):
            line = f_handle.readline()

            # End of file or empty line (either way end of data)
            if not line or len(line) == 0:
                break

            data[idx] = ast.literal_eval(line)

        return data

    # ------------------------------------------------------------------------------

    def _load_2d_slice(self, f_handle, dimensions):
        """
        Loads a 2D slice from the open file.

        @param f_handle Handle to the open file with the iterator at the slice header
        @param dimensions Tuple containing dimensions (rows/vertical axis, cols/x axis)
        @return Numpy array of shape [dimensions]
        """
        data = np.ndarray(shape=dimensions, dtype=float)

        for v_idx in range(dimensions[0]):
            line = f_handle.readline()

            # End of file or empty line (either way end of data)
            if not line or len(line) == 0:
                break

            values = [ast.literal_eval(s) for s in line.split()]
            data[v_idx] = np.array(values)

        return data

    # ------------------------------------------------------------------------------

    def _axis_conversion(self, data, unit, name):
        """
        Converts an axis to a Mantid axis type (possibly performing a unit
        conversion).

        @param data The axis data as Numpy array
        @param unit The axis unit as read from the file
        @param name The axis name as read from the file
        @return Tuple containing updated axis details
        """
        logger.debug("Axis for conversion: name={0}, unit={1}".format(name, unit))

        # Q (nm**-1) to Q (Angstrom**-1)
        if name.lower() == "q" and unit.lower() == "inv_nm":
            logger.information("Axis {0} will be converted to Q in Angstrom**-1".format(name))
            unit = "MomentumTransfer"
            data /= sc.nano  # nm to m
            data *= sc.angstrom  # m to Angstrom

        # Frequency (THz) to Energy (meV)
        elif name.lower() == "frequency" and unit.lower() == "thz":
            logger.information("Axis {0} will be converted to energy in meV".format(name))
            unit = "Energy"
            data *= sc.tera  # THz to Hz
            data *= sc.value("Planck constant in eV s")  # Hz to eV
            data /= sc.milli  # eV to meV

        # Time (ps) to TOF (s)
        elif name.lower() == "time" and unit.lower() == "ps":
            logger.information("Axis {0} will be converted to time in microsecond".format(name))
            unit = "TOF"
            data *= sc.micro  # ps to us

        # No conversion
        else:
            unit = "Empty"

        return (data, unit, name)


# ------------------------------------------------------------------------------


AlgorithmFactory.subscribe(LoadNMoldyn4Ascii)
