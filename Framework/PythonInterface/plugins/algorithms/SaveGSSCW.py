# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import mantid
import os
import numpy as np
from mantid.simpleapi import ConvertToPointData


class SaveGSSCW(mantid.api.PythonAlgorithm):
    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["SaveGSSCW"]

    def name(self):
        return "SaveGSSCW"

    def summary(self):
        return "Save constant wavelength powder diffraction data to a GSAS file in FXYE format"

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input), "Workspace to save")
        self.declareProperty(
            mantid.api.FileProperty("OutputFilename", "", action=mantid.api.FileAction.Save, extensions=[".gss"]),
            doc="Name of the GSAS file to save to",
        )

    def validateInputs(self):
        """Validate input properties
        (virtual method)
        """
        issues = dict()

        wkspParamName = "InputWorkspace"
        # FIXME - need to find out the unit of the workspace
        allowedUnits = ["Degrees"]

        # check for units
        wksp = self.getProperty(wkspParamName).value
        units = wksp.getAxis(0).getUnit().unitID()
        if units not in allowedUnits:
            allowedUnits = ["'%s'" % unit for unit in allowedUnits]
            allowedUnits = ", ".join(allowedUnits)
            issues[wkspParamName] = "Only support units %s" % allowedUnits
            print(f"[ERROR] Workspace {wksp.name()} with unit {units} is not supported.Allowed units are {allowedUnits}")

        # check output file name: whether user can access and write files
        output_file_name_property = "OutputFilename"
        gsas_name = self.getProperty(output_file_name_property).value

        try:
            temp_file = open(gsas_name, "w")
        except PermissionError as per_err:
            # error message
            issues[output_file_name_property] = f"User is not allowed to write file {gsas_name} due to {per_err}"
        else:
            # delete the temp file
            temp_file.close()
            os.remove(gsas_name)

        return issues

    def PyExec(self):
        """
        Main method to execute the algorithm
        """
        # Get input
        wksp = self.getProperty("InputWorkspace").value
        assert wksp, "Input workspace cannot be None"

        # process workspace to make it a PointData workspace
        if wksp.isHistogramData():
            wksp_name = wksp.name()
            wksp = ConvertToPointData(InputWorkspace=wksp_name, OutputWorkspace=wksp_name)

        gsas_content = self._write_gsas_fxye(wksp)

        # Get output and write file
        gsas_name = self.getProperty("OutputFilename").value
        with open(gsas_name, "w") as gsas_file:
            gsas_file.write(gsas_content)

    def _write_gsas_fxye(self, workspace):
        """
        Write GSAS file from a workspace to FXYE format

        Example:

        BANK 1 2438 488 CONST 1000.000    5.000 0 0 ESD
          287.51   16.96  281.38   13.52  279.80   11.83  282.77   11.89  279.73   13.43
          270.69   16.45  270.27   13.23  271.90   11.66  275.57   11.74  286.05   13.66
          303.35   17.42  295.64   13.86  292.17   12.09  292.92   12.10  288.03   13.63
          277.50   16.66  278.01   13.42  274.65   11.72  267.41   11.56  266.29   13.15
          271.27   16.47  273.06   13.29  272.29   11.67  268.96   11.60  260.08   12.93
          245.63   15.67  250.87   12.73  258.06   11.36  267.18   11.56  272.39   13.29

        Returns
        -------
        str
            GSAS file content

        """
        # generate header
        header = self._create_gsas_header(workspace)

        # generate body
        body = self._create_gsas_body(workspace)

        # construct the file
        gsas_content = self.empty_line(80) + "\n" + header + body

        return gsas_content

    @staticmethod
    def empty_line(width):
        """Create an empty line

        Parameters
        ----------
        width: int
            line width

        Returns
        -------
        str
            A line with space only

        """
        space = ""
        line = f"{space:{width}s}"

        return line

    @staticmethod
    def _create_gsas_header(workspace):
        """Create GSAS header

        Example:
        BANK 1 2438 488 CONST 1000.000    5.000 0 0 ESD

        Returns
        -------
        str
            header

        """
        # data points
        num_data_points = len(workspace.readY(0))
        # number of lines
        num_lines = num_data_points // 5
        if num_data_points % 5 > 0:
            num_lines += 1
        # min 2theta and 2theta step as centi-degree
        min_2theta = workspace.readX(0)[0] * 100
        delta_2theta = np.mean(workspace.readX(0)[1:] - workspace.readX(0)[:-1]) * 100
        # check whether the workspace's 2theta has constant step
        delta_2theta_std = np.std(workspace.readX(0)[1:] - workspace.readX(0)[:-1])
        if delta_2theta_std > 1e-5:
            raise RuntimeError("2theta steps are not constant")

        min_2theta_str = "%9s" % f"{min_2theta:6.3f}"
        delta_2theta_str = "%9s" % f"{delta_2theta:6.3f}"
        header = f"BANK 1{num_data_points:5}{num_lines:4} CONST{min_2theta_str}{delta_2theta_str} 0 0 ESD"

        # enforce to 80 character with line change
        header = "{:80s}\n".format(header)

        return header

    @staticmethod
    def _create_gsas_body(workspace):
        """Create GSAS file's body containing data only

        Returns
        -------
        str
            data body
        """
        # Get Y and E
        vec_y = workspace.readY(0)
        vec_e = workspace.readE(0)
        num_data_points = len(vec_y)

        # Write
        gsas_data = ""
        gsas_line = ""
        for item_index in range(num_data_points):
            # check whether any value exceeds the space list, i.e., larger than 99999.99
            if vec_y[item_index] > 99999.99 or vec_e[item_index] > 99999.99:
                raise RuntimeError(
                    f"{item_index}-th data point: Y = {vec_y[item_index]}, E = {vec_e[item_index]}"
                    f". Exceeding limit 99999.99 as GSAS F5.2 format limit."
                )
            # format Y_i and E_i
            y_value = "%8s" % f"{vec_y[item_index]:5.2f}"
            e_value = "%8s" % f"{vec_e[item_index]:5.2f}"
            gsas_line += f"{y_value}{e_value}"
            # new line?
            if (item_index + 1) % 5 == 0:
                # check line width
                assert len(gsas_line) == 80
                # append
                gsas_data += f"{gsas_line}\n"
                # reset gsas line
                gsas_line = ""

        # remove end of line if it is the last character
        if gsas_line == "":
            # reset gsas line, then the last character must be '\n'
            gsas_data = gsas_data[:-1]
        else:
            # in this case, gsas line cannot empty
            gsas_data += "{:80s}".format(gsas_line)

        return gsas_data


# Register the algorithm
mantid.api.AlgorithmFactory.subscribe(SaveGSSCW)
