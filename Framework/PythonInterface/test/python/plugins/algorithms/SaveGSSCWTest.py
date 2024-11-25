# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import mantid
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace, SaveGSSCW
import numpy as np
import os
import unittest


hb2a_exp_data = np.array(
    [
        [10.02500, 287.51184276, 13.43454614],
        [10.07500, 278.30585847, 9.30794030],
        [10.12500, 292.87303075, 13.55254760],
        [10.17500, 275.63281074, 13.12537194],
        [10.22500, 270.69168828, 13.00861236],
        [10.27500, 262.77480661, 12.82209765],
        [10.32500, 277.35681869, 13.14797416],
        [10.37500, 274.59594429, 13.12070174],
        [10.42500, 280.21305288, 13.19471733],
        [10.47500, 303.34965091, 13.80288705],
        [10.52500, 282.70648260, 13.32691140],
        [10.57500, 300.87222859, 13.81951915],
        [10.62500, 296.89787456, 13.69487774],
        [10.67500, 289.70021610, 13.42010336],
        [10.72500, 277.49926017, 13.28980428],
        [10.77500, 256.52015685, 12.60727791],
        [10.82500, 300.00488275, 13.70757297],
        [10.87500, 279.89145922, 13.23841175],
        [10.92500, 247.69887743, 12.40045414],
        [10.97500, 271.26941540, 13.05145583],
        [11.02500, 282.93767404, 13.29357157],
        [11.07500, 264.96899532, 12.86803196],
        [11.12500, 266.72777934, 12.93819774],
        [11.17500, 267.86568113, 12.96292887],
        [11.22500, 245.63113455, 12.45399289],
        [11.27500, 263.01979365, 12.84935776],
        [11.32500, 243.96967945, 12.35388494],
        [11.37500, 274.70220810, 13.11082107],
        [11.42500, 268.77756452, 13.05299243],
        [11.47500, 273.67819710, 13.07685009],
        [11.52500, 263.64522942, 12.87991230],
        [11.57500, 272.43577672, 13.07733576],
    ]
)


class SaveGSSCWTest(unittest.TestCase):
    def saveFilePath(self, wkspname):
        dataDir = mantid.config.getString("defaultsave.directory")
        return os.path.join(dataDir, wkspname + ".gss")

    def cleanup(self, filename, wkspname):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(wkspname):
            mantid.api.AnalysisDataService.remove(wkspname)

    def testSaveGSS2Theta(self):
        """Test to Save one spectrum without and instrument

        - generate a workspace from test data
        - create a GSAS file from this workspace
        - verify the GSAS data with test data

        As the user requirement:
        1. the header of GSAS file shall follow the GSAS doc
        2. all data read from GSAS shall be exactly same as test data
        """
        # set up test parameters: workspace name, output file name,
        ws_name = "SaveGSSCW_2theta"
        gsas_file_name = self.saveFilePath(ws_name)

        # create input workspace
        self._create_2theta_workspace(ws_name, hb2a_exp_data)
        ws = AnalysisDataService[ws_name]
        assert ws
        print(f"Workspace {ws_name} of type {type(ws)}")

        try:
            SaveGSSCW(InputWorkspace=ws_name, OutputFilename=gsas_file_name)
            self.check_gsas(gsas_file_name, expected_data=hb2a_exp_data)
        finally:
            self.cleanup(gsas_file_name, ws_name)

    def test_point_data(self):
        """Test with point data"""
        # Create a workspace as histogram
        x = np.arange(300, 16667, 15.0)
        y = np.random.random(len(x) - 1)  # histogram
        e = np.sqrt(y)

        ws_name = "TestHistogramWS"
        CreateWorkspace(OutputWorkspace=ws_name, DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="Degrees", YUnitlabel="stuff")

        # Save to GSAS
        gsas_file_name = self.saveFilePath(ws_name)
        try:
            SaveGSSCW(InputWorkspace=ws_name, OutputFilename=gsas_file_name)
            # check file existence
            self.assertTrue(os.path.exists(gsas_file_name))
        finally:
            # clean
            self.cleanup(gsas_file_name, ws_name)

    def check_gsas(self, filename, expected_data):
        """Check output GSAS file

        Parameters
        ----------
        filename: str
            GSAS filename
        expected_data: numpy.ndarray
            data to verify against

        Returns
        -------
        None

        """
        # File exists?
        assert os.path.exists(filename), f"Generated GSAS file {filename} cannot be found"

        # Contents
        gsas_file = open(filename, "r")
        contents = gsas_file.readlines()
        gsas_file.close()

        # Check total number of lines: 1 + 1 + ceil(32/5) = 9
        assert len(contents) == 9, f"GSAS file contains {len(contents)} lines but not 9.\n{contents}"

        # Check size
        for li, line in enumerate(contents):
            # readlines() will keep the line-change sign
            if len(line) == 81:
                assert line[-1] == "\n", (
                    f"Line {li}: {line} contains {len(line)} characters but not 80." f"Its last character is [{line[-1]}]"
                )
            else:
                assert len(line) == 80, (
                    f"Line {li}: {line} contains {len(line)} characters but not 80." f"Its last character is [{line[-1]}]"
                )

        # line 0 is emtpy
        assert contents[0].strip() == "", "First line must be empty but not >{}<".format(contents[0].strip())

        # line 1 is for information
        expected_header = "BANK 1   32   7 CONST 1002.500    5.000 0 0 ESD"
        assert contents[1].startswith(expected_header), "Comparison:\n{}\n{}".format(contents[1], expected_header)

        for line_index in range(7):
            for item_index in range(5):
                point_index = line_index * 5 + item_index
                # get Y and E
                start_char_pos = item_index * 16
                y_str = contents[line_index + 2][start_char_pos : start_char_pos + 8]
                e_str = contents[line_index + 2][start_char_pos + 8 : start_char_pos + 16]
                # verify
                if point_index < expected_data.shape[0]:
                    # shall be two float to compare with
                    y_i = float(y_str)
                    e_i = float(e_str)
                    self.assertAlmostEqual(y_i, expected_data[point_index][1], places=2)
                    self.assertAlmostEqual(e_i, expected_data[point_index][2], places=2)
                else:
                    # outside of given data range: two empty string
                    assert y_str.strip() == e_str.strip() == ""

    def _create_2theta_workspace(self, ws_name, data):
        """Create a 1D matrix workspace with unit of 2theta Degrees

        Parameters
        ----------
        ws_name: str
            workspace name for output
        data: numpy.ndarray
            numpy array with 3 columns (2theta, intensity, error) to create workspace

        """
        # Check input
        self.assertEqual(data.shape[1], 3)

        # From 15 degree to 105 degree with 1 degree space
        x = data[:, 0]
        y = data[:, 1]
        e = data[:, 2]

        CreateWorkspace(
            OutputWorkspace=ws_name,
            DataX=x,
            DataY=y,
            DataE=e,
            NSpec=1,
            UnitX="Degrees",
            YUnitlabel="Counts",
            WorkspaceTitle="Test SaveGSSCW",
        )


if __name__ == "__main__":
    unittest.main()
