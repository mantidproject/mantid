# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-many-locals,too-many-branches
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction, StringArrayProperty
from mantid.simpleapi import config, CreateWorkspace, GroupWorkspaces, LoadInstrument, LoadParameterFile, UpdateInstrumentFromFile

from IndirectCommon import get_efixed

import ast
import re
import math
import os
import numpy as np

# ==============================================================================


def _find_starts(data, c, l1):
    for i in range(l1, len(data)):
        char = data[i]
        if char.startswith(c):
            line = i
            break
    return line


# ==============================================================================


def _find_tab_starts(data, c, l1):
    for i in range(l1, len(data)):
        char = data[i][1:]
        if char.startswith(c):
            line = i
            break
    return line


# ==============================================================================


def _find_ends(data, c, l1):
    for i in range(l1, len(data)):
        char = data[i]
        if char.endswith(c):
            line = i
            break
    return line


# ==============================================================================


def _make_list(a, l1, l2):
    data = ""
    for m in range(l1, l2 + 1):
        data += a[m]
        alist = data.split(",")
    return alist


# ==============================================================================


def _cdl_find_dimensions(data):
    """
    Gets the number of Q, time and frequency values in given raw data.

    @param data Raw data to search
    """

    num_q_values = _find_tab_starts(data, "NQVALUES", 0)
    num_time_values = _find_tab_starts(data, "NTIMES", 0)
    num_freq_values = _find_tab_starts(data, "NFREQUENCIES", 0)

    q_el = data[num_q_values].split()
    num_q = int(q_el[2])
    t_el = data[num_time_values].split()
    num_t = int(t_el[2])
    f_el = data[num_freq_values].split()
    num_f = int(f_el[2])

    logger.debug(data[2][1:-1])
    logger.debug(data[3][1:-1])
    logger.debug(data[6][1:-1])

    return num_q, num_t, num_f


# ==============================================================================


class LoadNMoldyn3Ascii(PythonAlgorithm):
    _file_name = None
    _file_type = None
    _functions = None
    _out_ws = None

    # -------------------------------------------------------------------------------

    def category(self):
        return "Inelastic\\DataHandling;Simulation"

    # -------------------------------------------------------------------------------

    def summary(self):
        return "Imports functions from CDL and ASCII files output by nMOLDYN 3."

    # -------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=[".cdl", ".dat"]), doc="File path for data")

        self.declareProperty(StringArrayProperty("Functions"), doc="Names of functions to attempt to load from file")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace name")

    # -------------------------------------------------------------------------------

    def validateInputs(self):
        issues = dict()

        sample_filename = self.getPropertyValue("Filename")
        file_type = os.path.splitext(sample_filename)[1]
        function_list = self.getProperty("Functions").value

        if len(function_list) == 0 and file_type == ".cdl":
            issues["Functions"] = "Must specify at least one function when loading a CDL file"

        if len(function_list) > 0 and file_type == ".dat":
            issues["Functions"] = "Cannot specify functions when loading an ASCII file"

        return issues

    # -------------------------------------------------------------------------------

    def PyExec(self):
        # Do setup
        self._setup()

        loaded_ws = None

        # Run nMOLDYN import
        if self._file_type == "cdl":
            loaded_ws = self._cdl_import()
        elif self._file_type == "dat":
            loaders = [self._ascii_3d_import, self._ascii_2d_import]
            for loader in loaders:
                try:
                    logger.information("Attempting to load with loader {0}".format(loader))
                    loaded_ws = loader()
                    break
                except (ValueError, SyntaxError) as err:
                    logger.information("Loader {0} failed to load data: {1}".format(loader, err))
        else:
            raise RuntimeError("Unrecognised file extension: %s" % self._file_type)

        if loaded_ws is None:
            raise RuntimeError("Failed to load any data, check file format")

        # Set the output workspace
        self.setProperty("OutputWorkspace", loaded_ws)

    # -------------------------------------------------------------------------------

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._file_name = self.getPropertyValue("Filename")
        self._file_type = os.path.splitext(self._file_name)[1][1:]

        self._out_ws = self.getPropertyValue("OutputWorkspace")

        raw_functions = self.getProperty("Functions").value
        self._functions = [x.strip() for x in raw_functions]

    # -------------------------------------------------------------------------------

    def _cdl_import(self):
        """
        Import data from CDL file.
        """
        logger.notice("Loading CDL file")

        # Get file base name
        base_filename = os.path.basename(self._file_name)
        base_name = os.path.splitext(base_filename)[0]

        # Open file and get data
        data = []
        with open(self._file_name, "r") as handle:
            for line in handle:
                line = line.rstrip()
                data.append(line)

        len_data = len(data)

        # raw head
        num_spec, nT, nF = _cdl_find_dimensions(data)
        ldata = _find_starts(data, "data:", 0)
        lq1 = _find_starts(data, " q =", ldata)  # start Q values
        lq2 = _find_starts(data, " q =", lq1 - 1)
        Qlist = _make_list(data, lq1, lq2)
        if num_spec != len(Qlist):
            raise RuntimeError("Error reading Q values")
        Qf = Qlist[0].split()
        Q = [float(Qf[2]) / 10.0]
        for m in range(1, num_spec - 1):
            Q.append(float(Qlist[m]) / 10.0)

        Q.append(float(Qlist[num_spec - 1][:-1]) / 10.0)
        logger.information("Q values = " + str(Q))

        lt1 = _find_starts(data, " time =", lq2)  # start T values
        lt2 = _find_ends(data, ";", lt1)
        Tlist = _make_list(data, lt1, lt2)
        if nT != len(Tlist):
            raise RuntimeError("Error reading Time values")

        Tf = Tlist[0].split()
        T = [float(Tf[2])]
        for m in range(1, nT - 1):
            T.append(float(Tlist[m]))

        T.append(float(Tlist[nT - 1][:-1]))
        T.append(2 * T[nT - 1] - T[nT - 2])
        logger.information("T values = " + str(T[:2]) + " to " + str(T[-3:]))

        lf1 = _find_starts(data, " frequency =", lq2)  # start F values
        lf2 = _find_ends(data, ";", lf1)
        Flist = _make_list(data, lf1, lf2)
        if nF != len(Flist):
            raise RuntimeError("Error reading Freq values")

        Ff = Flist[0].split()
        F = [float(Ff[2])]
        for m in range(1, nF - 1):
            F.append(float(Flist[m]))

        F.append(float(Flist[nF - 1][:-1]))
        F.append(2 * F[nF - 1] - T[nF - 2])
        logger.information("F values = " + str(F[:2]) + " to " + str(F[-3:]))

        # Function
        output_ws_list = list()
        for func in self._functions:
            start = []
            lstart = lt2
            if func[:3] == "Fqt":
                nP = nT
                xEn = np.array(T)
                zero_error = np.zeros(nT)
                x_unit = "TOF"
            elif func[:3] == "Sqw":
                nP = nF
                xEn = np.array(F)
                zero_error = np.zeros(nF)
                x_unit = "Energy"
            else:
                raise RuntimeError("Failed to parse function string " + func)

            for n in range(0, num_spec):
                for m in range(lstart, len_data):
                    char = data[m]
                    if char.startswith("  // " + func):
                        start.append(m)
                        lstart = m + 1
            lend = _find_ends(data, ";", lstart)
            start.append(lend + 1)

            # Throw error if we couldn't find the function
            if len(start) < 2:
                raise RuntimeError("Failed to parse function string " + func)

            Qaxis = ""
            for n in range(0, num_spec):
                logger.information(str(start))
                logger.information("Reading : " + data[start[n]])

                Slist = _make_list(data, start[n] + 1, start[n + 1] - 1)
                if n == num_spec - 1:
                    Slist[nP - 1] = Slist[nP - 1][:-1]
                S = []
                for m in range(0, nP):
                    S.append(float(Slist[m]))
                if nP != len(S):
                    raise RuntimeError("Error reading S values")
                else:
                    logger.information("S values = " + str(S[:2]) + " to " + str(S[-2:]))
                if n == 0:
                    Qaxis += str(Q[n])
                    data_x = xEn
                    data_y = np.array(S)
                    data_e = zero_error
                else:
                    Qaxis += "," + str(Q[n])
                    data_x = np.append(data_x, xEn)
                    data_y = np.append(data_y, np.array(S))
                    data_e = np.append(data_e, zero_error)

            function_ws_name = base_name + "_" + func
            CreateWorkspace(
                OutputWorkspace=function_ws_name,
                DataX=data_x,
                DataY=data_y,
                DataE=data_e,
                Nspec=num_spec,
                UnitX=x_unit,
                VerticalAxisUnit="MomentumTransfer",
                VerticalAxisValues=Qaxis,
            )

            output_ws_list.append(function_ws_name)

        out_ws = GroupWorkspaces(InputWorkspaces=output_ws_list, OutputWorkspace=self._out_ws)

        return out_ws

    # -------------------------------------------------------------------------------

    def _ascii_3d_import(self):
        """
        Import 3D ASCII data (e.g. I(Q, t)).
        """
        logger.notice("Loading ASCII data")

        # Read file
        data = []
        x_axis = ("time", "ns")
        v_axis = ("q", "ang^-1")
        with open(self._file_name, "r") as handle:
            for line in handle:
                line = line.strip()

                # Ignore empty lines
                if line == "":
                    continue

                # Data line (if not comment)
                elif line.strip()[0] != "#":
                    line_values = np.array([ast.literal_eval(t.strip()) if "nan" not in t.lower() else np.nan for t in line.split()])
                    data.append(line_values)

        if x_axis is None or v_axis is None:
            raise ValueError("Data is not in expected format for 3D data")
        logger.debug("X axis: {0}".format(x_axis))
        logger.debug("V axis: {0}".format(v_axis))

        # Get axis and Y values
        data = np.swapaxes(np.array(data), 0, 1)
        x_axis_values = data[0, 1:]
        v_axis_values = data[1:, 0]
        y_values = np.ravel(data[1:, 1:])

        # Create the workspace

        wks = CreateWorkspace(
            OutputWorkspace=self._out_ws,
            DataX=x_axis_values,
            DataY=y_values,
            NSpec=v_axis_values.size,
            UnitX=x_axis[1],
            EnableLogging=False,
        )

        # Load the MolDyn instrument
        q_max = v_axis_values[-1]
        instrument = "MolDyn"
        reflection = "2" if q_max <= 2.0 else "4"
        self._load_instrument_and_parameter_file(wks.name(), instrument, "simul", reflection)

        # Process angles
        efixed = get_efixed(wks.name())
        logger.information("Qmax={0}, Efixed={1}".format(q_max, efixed))
        wave = 1.8 * math.sqrt(25.2429 / efixed)
        qw = wave * v_axis_values / (4.0 * math.pi)
        theta = 2.0 * np.degrees(np.arcsin(qw))
        self._update_instrument_theta(wks.name(), instrument, theta)

        return wks

    @staticmethod
    def _load_instrument_and_parameter_file(workspace_name: str, instrument_name: str, analyser: str, reflection: str) -> None:
        idf_directory = config["instrumentDefinition.directory"]
        idf_filepath = f"{idf_directory}{instrument_name}_Definition.xml"
        LoadInstrument(Workspace=workspace_name, Filename=idf_filepath, RewriteSpectraMap=True)
        ipf_filepath = f"{idf_directory}{instrument_name}_{analyser}_{reflection}_Parameters.xml"
        LoadParameterFile(Workspace=workspace_name, Filename=ipf_filepath)

    @staticmethod
    def _update_instrument_theta(workspace_name: str, instrument_name: str, theta: np.ndarray[float]) -> None:
        work_directory = config["defaultsave.directory"]
        filename = f"{instrument_name}_angles.txt"
        path = os.path.join(work_directory, filename)
        handle = open(path, "w")
        head = "spectrum,theta"
        handle.write(f"{head} \n")
        for n in range(0, len(theta)):
            handle.write(f"{n + 1}   {str(theta[n])}\n")
        handle.close()
        UpdateInstrumentFromFile(Workspace=workspace_name, Filename=path, MoveMonitors=False, IgnorePhi=False, AsciiHeader=head)

    # -------------------------------------------------------------------------------

    def _ascii_2d_import(self):
        """
        Import 2D ASCII data (e.g. DoS).
        """
        logger.notice("Loading ASCII data")

        # Regex
        x_axis_regex = re.compile(r"\s*columns-1\s*=\s*([A-z0-9\-\s]+)\s*\(([A-z0-9-\s*]+)\)")
        y_axis_regex = re.compile(r"\s*columns-2\s*=\s*([A-z\-\s]+)")

        # Read file
        data = []
        x_axis = None
        y_axis = None
        with open(self._file_name, "r") as handle:
            for line in handle:
                line = line.strip()

                # Ignore empty lines
                if line == "":
                    continue

                x_axis_match = x_axis_regex.match(line)
                y_axis_match = y_axis_regex.match(line)

                # Line (X) header
                if x_axis_match:
                    x_axis = (x_axis_match.group(1).strip(), x_axis_match.group(2))

                # Data (Y) header
                elif y_axis_match:
                    y_axis = y_axis_match.group(1)

                # Data line (if not comment)
                elif line.strip()[0] != "#":
                    line_values = np.array([ast.literal_eval(t.strip()) if "nan" not in t.lower() else np.nan for t in line.split()])
                    data.append(line_values)

        if x_axis is None or y_axis is None:
            raise ValueError("Data is not in expected format for 2D data")
        logger.debug("X axis: {0}".format(x_axis))
        logger.debug("Y axis: {0}".format(y_axis))

        # Get axis and Y values
        data = np.array(data)
        x_axis_values = data[:, 0]
        y_values = data[:, 1]

        # Create the workspace
        wks = CreateWorkspace(
            OutputWorkspace=self._out_ws,
            DataX=x_axis_values,
            DataY=y_values,
            NSpec=1,
            UnitX=x_axis[1],
            WorkspaceTitle=y_axis,
            YUnitLabel=y_axis,
            EnableLogging=False,
        )

        return wks


# ==============================================================================


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadNMoldyn3Ascii)
