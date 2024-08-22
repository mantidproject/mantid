# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import base64
import tempfile
import os
import unittest
import numpy as np
import xml.etree.ElementTree as etree
from mantid.simpleapi import SaveMDHistoToVTK, CreateMDHistoWorkspace, CreateMDWorkspace, AddSampleLog, SetUB, BinMD, FakeMDEventData


class SaveMDHistoToVTKTest(unittest.TestCase):
    def test_simple(self):
        tmp_filename = tempfile.mktemp(".vts")
        input_data = [np.nan, 1, 2, 3, 4, 5, 6, 7]
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-1,1,-2,2,-3,3",
            SignalInput=input_data,
            ErrorInput=input_data,
            NumberOfBins="2,2,2",
            Names="[H,0,0],[0,K,0],[0,0,L]",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )

        SaveMDHistoToVTK(ws, tmp_filename)

        # manually calculate points
        expected_points = []
        for z in (-3, 0, 3):
            for y in (-2, 0, 2):
                for x in (-1, 0, 1):
                    expected_points += [x, y, z]

        self.check_result(tmp_filename, expected_points)

    def test_non_orthogonal(self):
        tmp_filename = tempfile.mktemp(".vts")
        input_data = [np.nan, 1, 2, 3, 4, 5, 6, 7]
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-1,1,-2,2,-3,3",
            SignalInput=input_data,
            ErrorInput=input_data,
            NumberOfBins="2,2,2",
            Names="[H,0,0],[0,K,0],[0,0,L]",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )

        AddSampleLog(ws, LogName="sample")  # easy was to create ExperimentInfo
        SetUB(ws, gamma=120)

        SaveMDHistoToVTK(ws, tmp_filename)

        expected_cob = np.array([[1, 0.5, 0, 0], [0, np.sin(np.pi / 3), 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]])

        B = ws.getExperimentInfo(0).sample().getOrientedLattice().getB()
        B = B / np.linalg.norm(B, axis=0)

        # manually calculate points
        expected_points = []
        for z in (-3, 0, 3):
            for y in (-2, 0, 2):
                for x in (-1, 0, 1):
                    expected_points += np.dot(B, [x, y, z]).tolist()

        self.check_result(tmp_filename, expected_points, expected_cob=expected_cob)

    def test_non_orthogonal_W_MATRIX(self):
        tmp_filename = tempfile.mktemp(".vts")
        input_data = [np.nan, 1, 2, 3, 4, 5, 6, 7]
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-1,1,-2,2,-3,3",
            SignalInput=input_data,
            ErrorInput=input_data,
            NumberOfBins="2,2,2",
            Names="[H,H,0],[H,-H,0],[0,0,L]",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )

        AddSampleLog(ws, LogName="sample")  # easy was to create ExperimentInfo
        SetUB(ws, gamma=120)

        # aligning the W_MATRIX for a HH0 slice with a lattice with gamma=120 should result in an identity change of basis matrix
        ws.getExperimentInfo(0).run().addProperty("W_MATRIX", [1.0, 1.0, 0.0, 1.0, -1.0, 0.0, 0.0, 0.0, 1.0], True)

        SaveMDHistoToVTK(ws, tmp_filename)

        # manually calculate points
        expected_points = []
        for z in (-3, 0, 3):
            for y in (-2, 0, 2):
                for x in (-1, 0, 1):
                    expected_points += [x, y, z]

        self.check_result(tmp_filename, expected_points, axes=("[H,H,0]", "[H,-H,0]", "[0,0,L]"))

    def test_non_orthogonal_W_MATRIX2(self):
        tmp_filename = tempfile.mktemp(".vts")
        input_data = [np.nan, 1, 2, 3, 4, 5, 6, 7]
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-1,1,-2,2,-3,3",
            SignalInput=input_data,
            ErrorInput=input_data,
            NumberOfBins="2,2,2",
            Names="[H,0,0],[H,H,0],[0,0,L]",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )

        AddSampleLog(ws, LogName="sample")  # easy was to create ExperimentInfo
        SetUB(ws)

        ws.getExperimentInfo(0).run().addProperty("W_MATRIX", [1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0], True)

        SaveMDHistoToVTK(ws, tmp_filename)

        expected_cob = np.array([[1, 1 / np.sqrt(2), 0, 0], [0, 1 / np.sqrt(2), 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]])

        # manually calculate points
        expected_points = []
        for z in (-3, 0, 3):
            for y in (-2, 0, 2):
                for x in (-1, 0, 1):
                    expected_points += [x + y / np.sqrt(2), y / np.sqrt(2), z]

        self.check_result(tmp_filename, expected_points, expected_cob=expected_cob, axes=("[H,0,0]", "[H,H,0]", "[0,0,L]"))

    def test_non_orthogonal_basis_vector(self):
        tmp_filename = tempfile.mktemp(".vts")
        ws = CreateMDWorkspace(
            Dimensions=3,
            Extents="-10,10,-10,10,-10,10",
            Names="[H,0,0],[0,K,0],[0,0,L]",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )

        AddSampleLog(ws, LogName="sample")  # easy was to create ExperimentInfo
        SetUB(ws)

        FakeMDEventData(ws, PeakParams="10,0.5,0.5,0.5,0.1")

        ws_slice = BinMD(
            InputWorkspace=ws,
            AxisAligned=False,
            BasisVector0="[H,0,0],r.l.u.,1,0,0",
            BasisVector1="[H,H,0],r.l.u.,1,1,0",
            BasisVector2="[0,0,L],r.l.u.,0,0,1",
            OutputExtents="-1,1,-2,2,-3,3",
            OutputBins="2,2,2",
            NormalizeBasisVectors=False,
        )

        SaveMDHistoToVTK(ws_slice, tmp_filename)

        expected_cob = np.array(
            [[1 / np.sqrt(2), 1 / np.sqrt(5), 0, 0], [1 / np.sqrt(2), 2 / np.sqrt(5), 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
        )

        # manually calculate points
        expected_points = []
        for z in (-3, 0, 3):
            for y in (-2, 0, 2):
                for x in (-1, 0, 1):
                    expected_points += [x / np.sqrt(2) + y / np.sqrt(5), x / np.sqrt(2) + y * 2 / np.sqrt(5), z]

        self.check_result(
            tmp_filename,
            expected_points,
            expected_cob=expected_cob,
            expected_signal=(0, 0, 0, 0, 0, 0, 3, 7),
            axes=("[H,0,0]", "[H,H,0]", "[0,0,L]"),
        )

    def check_result(
        self,
        filename,
        expected_points,
        expected_signal=(np.nan, 1, 2, 3, 4, 5, 6, 7),
        expected_cob=np.eye(4),
        axes=("[H,0,0]", "[0,K,0]", "[0,0,L]"),
    ):
        assert os.path.isfile(filename)
        # parse document
        root = etree.parse(filename)

        # check ChangeOfBasisMatrix
        cob = root.findall(".//DataArray[@Name='ChangeOfBasisMatrix']")
        assert len(cob) == 1
        cob = np.array(cob[0].text.split(), dtype=float)
        np.testing.assert_allclose(cob, expected_cob.flatten(), atol=1e-15)

        # check BoundingBoxInModelCoordinates
        bb = root.findall(".//DataArray[@Name='BoundingBoxInModelCoordinates']")
        assert len(bb) == 1
        bb = np.array(bb[0].text.split(), dtype=float)
        np.testing.assert_equal(bb, [-1.0, 1.0, -2.0, 2.0, -3.0, 3.0])

        # check axes labels
        axes_labels = root.findall(".//Array")
        assert len(axes_labels) == 3

        x = axes_labels[0]
        assert x.attrib["Name"] == "AxisTitleForX"
        data = "".join(chr(int(c)) for c in x.text.split()[:-1])
        assert data == axes[0]

        y = axes_labels[1]
        assert y.attrib["Name"] == "AxisTitleForY"
        data = "".join(chr(int(c)) for c in y.text.split()[:-1])
        assert data == axes[1]

        z = axes_labels[2]
        assert z.attrib["Name"] == "AxisTitleForZ"
        data = "".join(chr(int(c)) for c in z.text.split()[:-1])
        assert data == axes[2]

        # check signal array
        signal = root.findall(".//DataArray[@Name='Signal']")
        assert len(signal) == 1
        signal = signal[0]
        assert float(signal.attrib["RangeMin"]) == np.nanmin(expected_signal)
        assert float(signal.attrib["RangeMax"]) == np.nanmax(expected_signal)
        data = base64.b64decode(signal.text)
        byte_count = np.frombuffer(data[:8], dtype=np.uint64)
        assert byte_count == 64
        s = np.frombuffer(data[8:], dtype=np.float64)
        np.testing.assert_equal(s, expected_signal)

        # check ghost array
        ghost = root.findall(".//DataArray[@Name='vtkGhostType']")
        assert len(ghost) == 1
        ghost = ghost[0]
        data = base64.b64decode(ghost.text)
        byte_count = np.frombuffer(data[:8], dtype=np.uint64)
        assert byte_count == 8
        g = np.frombuffer(data[8:], dtype=np.uint8)
        expected_ghost = np.full_like(expected_signal, 0)
        expected_ghost[np.isnan(expected_signal)] = 32
        np.testing.assert_equal(g, expected_ghost)

        # check boundary points
        points = root.findall(".//DataArray[@Name='Points']")
        assert len(points) == 1
        points = points[0]
        assert points.attrib["NumberOfComponents"] == "3"
        data = base64.b64decode(points.text)
        byte_count = np.frombuffer(data[:8], dtype=np.uint64)
        assert byte_count == 648  # 3 x 3 x 3 x 3 X 8
        g = np.frombuffer(data[8:], dtype=np.float64)
        np.testing.assert_allclose(g, expected_points, atol=1e-15)

        # cleanup
        os.remove(filename)


if __name__ == "__main__":
    unittest.main()
