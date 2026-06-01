# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
import numpy as np

from unittest.mock import patch, MagicMock
from scipy.spatial.transform import Rotation

from mantidqtinterfaces.TexturePlanner.helpers.exporter import OrientationExporter

file_path = "mantidqtinterfaces.TexturePlanner.helpers.exporter"

WS_REFERENCE = "__texture_planning_reference_ws"
COPY_KWARGS = dict(CopyName=False, CopyEnvironment=False, CopyLattice=False)


def _make_orientation(R=None, include=True):
    orient = MagicMock()
    orient.R = R if R is not None else Rotation.identity()
    orient.include = include
    return orient


def _make_model(orientations=None, n_output_points=1, instr="ENGINX", axes="xyz", senses="1,1,1", init_R=None, updated_mesh_ws="neutral"):
    model = MagicMock()
    if orientations is None:
        orientations = {0: _make_orientation()}
    model.orientations.values.return_value = list(orientations.values())
    model.n_output_points = n_output_points
    model.instr = instr
    model.orientation_kwargs = {"Axes": axes, "Senses": senses}
    model.workspaces.WS_REFERENCE = WS_REFERENCE
    model.workspaces.updated_mesh_ws = updated_mesh_ws
    model.workspaces.init_R = init_R if init_R is not None else Rotation.identity()
    return model


class TestOrientationExporter_Included(unittest.TestCase):
    def test_filters_out_orientations_with_include_false(self):
        kept = _make_orientation(include=True)
        dropped = _make_orientation(include=False)
        model = _make_model(orientations={0: kept, 1: dropped, 2: kept})
        exp = OrientationExporter(model)

        self.assertEqual(list(exp._included()), [kept, kept])


@patch(file_path + ".logger")
class TestOrientationExporter_OutputAsSscanss(unittest.TestCase):
    def _run(self, model, filename="out"):
        with tempfile.TemporaryDirectory() as tmp:
            exp = OrientationExporter(model)
            exp.output_as_sscanss(tmp, filename)
            save_file = os.path.join(tmp, filename + ".angles")
            with open(save_file) as f:
                return f.read(), save_file

    @patch(file_path + ".convert_to_sscanss_frame")
    def test_writes_header_and_one_line_per_orientation_per_output_point(self, mock_convert, mock_logger):
        mock_convert.return_value = (10.0, 20.0, 30.0)
        model = _make_model(orientations={0: _make_orientation(), 1: _make_orientation()}, n_output_points=2)

        content, _ = self._run(model)

        lines = content.splitlines()
        self.assertEqual(lines[0], "xyz")
        self.assertEqual(len(lines), 1 + 2 * 2)
        for line in lines[1:]:
            self.assertEqual(line, "10.0\t20.0\t30.0")

    @patch(file_path + ".convert_to_sscanss_frame")
    def test_rounds_angles_to_two_decimals(self, mock_convert, mock_logger):
        mock_convert.return_value = (1.23456, 2.34567, 3.45678)
        model = _make_model()

        content, _ = self._run(model)

        self.assertIn("1.23\t2.35\t3.46", content)

    @patch(file_path + ".convert_to_sscanss_frame")
    def test_passes_rotation_matrix_to_convert(self, mock_convert, mock_logger):
        mock_convert.return_value = (0.0, 0.0, 0.0)
        R = Rotation.from_euler("x", 45, degrees=True)
        model = _make_model(orientations={0: _make_orientation(R=R)})

        self._run(model)

        np.testing.assert_allclose(mock_convert.call_args.args[0], R.as_matrix())

    @patch(file_path + ".convert_to_sscanss_frame")
    def test_logs_notice_on_success(self, mock_convert, mock_logger):
        mock_convert.return_value = (0.0, 0.0, 0.0)
        model = _make_model()

        _, save_file = self._run(model, filename="out")

        mock_logger.notice.assert_called_once()
        self.assertIn(save_file, mock_logger.notice.call_args.args[0])


@patch(file_path + ".logger")
class TestOrientationExporter_OutputAsMatrix(unittest.TestCase):
    def _run(self, model, filename="out"):
        with tempfile.TemporaryDirectory() as tmp:
            exp = OrientationExporter(model)
            exp.output_as_matrix(tmp, filename)
            save_file = os.path.join(tmp, filename + ".txt")
            with open(save_file) as f:
                return f.read(), save_file

    def test_writes_9_tab_separated_values_per_line(self, mock_logger):
        R = Rotation.identity()
        model = _make_model(orientations={0: _make_orientation(R=R)})

        content, _ = self._run(model)

        line = content.splitlines()[0]
        values = line.split("\t")
        self.assertEqual(len(values), 9)
        np.testing.assert_allclose([float(v) for v in values], np.eye(3).reshape(-1))

    def test_writes_one_line_per_orientation_per_output_point(self, mock_logger):
        model = _make_model(orientations={0: _make_orientation(), 1: _make_orientation()}, n_output_points=3)

        content, _ = self._run(model)

        self.assertEqual(len(content.splitlines()), 2 * 3)

    def test_logs_notice_with_save_file(self, mock_logger):
        model = _make_model()

        _, save_file = self._run(model)

        mock_logger.notice.assert_called_once()
        self.assertIn(save_file, mock_logger.notice.call_args.args[0])


@patch(file_path + ".logger")
class TestOrientationExporter_OutputAsEuler(unittest.TestCase):
    def _run(self, model, filename="out"):
        with tempfile.TemporaryDirectory() as tmp:
            exp = OrientationExporter(model)
            exp.output_as_euler(tmp, filename)
            save_file = os.path.join(tmp, filename + ".txt")
            with open(save_file) as f:
                return f.read(), save_file

    def _make_orient_with_mock_R(self, euler_return):
        orient = MagicMock()
        orient.include = True
        orient.R.as_euler.return_value = np.asarray(euler_return)
        return orient

    def test_uses_axes_from_orientation_kwargs(self, mock_logger):
        orient = self._make_orient_with_mock_R([10.0, 20.0, 30.0])
        model = _make_model(orientations={0: orient}, axes="zyx", senses="1,1,1")

        self._run(model)

        orient.R.as_euler.assert_called_once_with("zyx", degrees=True)

    def test_applies_senses_per_axis(self, mock_logger):
        orient = self._make_orient_with_mock_R([10.0, 20.0, 30.0])
        model = _make_model(orientations={0: orient}, senses="1,-1,1")

        content, _ = self._run(model)

        values = content.splitlines()[0].split("\t")
        self.assertEqual([float(v) for v in values], [10.0, -20.0, 30.0])

    def test_writes_one_line_per_orientation_per_output_point(self, mock_logger):
        orient = self._make_orient_with_mock_R([0.0, 0.0, 0.0])
        model = _make_model(orientations={0: orient, 1: orient}, n_output_points=2)

        content, _ = self._run(model)

        self.assertEqual(len(content.splitlines()), 2 * 2)


@patch(file_path + ".ADS")
@patch(file_path + ".CopySample")
@patch(file_path + ".LoadEmptyInstrument")
class TestOrientationExporter_BuildReferenceWs(unittest.TestCase):
    def test_loads_empty_instrument_into_reference_wsname(self, mock_load_instr, mock_copy, mock_ads):
        model = _make_model(instr="ENGINX")
        exp = OrientationExporter(model)

        exp._build_reference_ws(WS_REFERENCE)

        mock_load_instr.assert_called_once_with(InstrumentName="ENGINX", OutputWorkspace=WS_REFERENCE)

    def test_copies_sample_from_updated_mesh_ws(self, mock_load_instr, mock_copy, mock_ads):
        model = _make_model(updated_mesh_ws="neutral")
        exp = OrientationExporter(model)

        exp._build_reference_ws(WS_REFERENCE)

        mock_copy.assert_called_once_with(InputWorkspace="neutral", OutputWorkspace=WS_REFERENCE, **COPY_KWARGS)


@patch(file_path + ".RotateSampleShape")
@patch(file_path + ".ADS")
class TestOrientationExporter_BakeInitialRotationIfCsg(unittest.TestCase):
    def _set_shape_type(self, mock_ads, type_name):
        shape = MagicMock()
        type(shape).__name__ = type_name
        mock_ads.retrieve.return_value.sample.return_value.getShape.return_value = shape

    def test_no_rotate_when_shape_is_not_csg(self, mock_ads, mock_rotate):
        self._set_shape_type(mock_ads, "MeshObject")
        model = _make_model(init_R=Rotation.from_euler("x", 45, degrees=True))
        exp = OrientationExporter(model)

        exp._bake_initial_rotation_if_csg(WS_REFERENCE)

        mock_rotate.assert_not_called()

    def test_no_rotate_when_csg_but_identity_init_R(self, mock_ads, mock_rotate):
        self._set_shape_type(mock_ads, "CSGObject")
        model = _make_model(init_R=Rotation.identity())
        exp = OrientationExporter(model)

        exp._bake_initial_rotation_if_csg(WS_REFERENCE)

        mock_rotate.assert_not_called()

    def test_rotates_when_csg_and_nonzero_init_R(self, mock_ads, mock_rotate):
        self._set_shape_type(mock_ads, "CSGObject")
        init_R = Rotation.from_euler("x", 90, degrees=True)
        model = _make_model(init_R=init_R)
        exp = OrientationExporter(model)

        exp._bake_initial_rotation_if_csg(WS_REFERENCE)

        mock_rotate.assert_called_once()
        ws_arg, rot_str = mock_rotate.call_args.args
        self.assertEqual(ws_arg, WS_REFERENCE)
        parts = rot_str.split(",")
        self.assertAlmostEqual(float(parts[0]), 90.0)
        np.testing.assert_allclose([float(parts[1]), float(parts[2]), float(parts[3])], [1.0, 0.0, 0.0], atol=1e-12)
        self.assertEqual(parts[4], "1")


@patch(file_path + ".ADS")
class TestOrientationExporter_ResetGoniometerToIdentity(unittest.TestCase):
    def test_sets_goniometer_R_to_identity(self, mock_ads):
        gonio = MagicMock()
        mock_ads.retrieve.return_value.run.return_value.getGoniometer.return_value = gonio

        OrientationExporter._reset_goniometer_to_identity(WS_REFERENCE)

        mock_ads.retrieve.assert_called_once_with(WS_REFERENCE)
        gonio.setR.assert_called_once()
        np.testing.assert_array_equal(gonio.setR.call_args.args[0], np.eye(3))


@patch(file_path + ".logger")
@patch(file_path + ".ADS")
@patch(file_path + ".SaveNexus")
class TestOrientationExporter_OutputAsReferenceWorkspace(unittest.TestCase):
    def _make_exporter(self):
        model = _make_model()
        exp = OrientationExporter(model)
        exp._build_reference_ws = MagicMock()
        exp._bake_initial_rotation_if_csg = MagicMock()
        exp._reset_goniometer_to_identity = MagicMock()
        return exp

    def test_orchestrates_build_then_bake_then_reset_then_save(self, mock_save, mock_ads, mock_logger):
        exp = self._make_exporter()

        with tempfile.TemporaryDirectory() as tmp:
            exp.output_as_reference_workspace(tmp, "out")
            expected_path = os.path.join(tmp, "out.nxs")

            exp._build_reference_ws.assert_called_once_with(WS_REFERENCE)
            exp._bake_initial_rotation_if_csg.assert_called_once_with(WS_REFERENCE)
            exp._reset_goniometer_to_identity.assert_called_once_with(WS_REFERENCE)
            mock_save.assert_called_once_with(InputWorkspace=WS_REFERENCE, Filename=expected_path)
            mock_logger.notice.assert_called_once()

    def test_removes_ref_ws_from_ads_when_it_exists(self, mock_save, mock_ads, mock_logger):
        exp = self._make_exporter()
        mock_ads.doesExist.return_value = True

        with tempfile.TemporaryDirectory() as tmp:
            exp.output_as_reference_workspace(tmp, "out")

        mock_ads.doesExist.assert_called_with(WS_REFERENCE)
        mock_ads.remove.assert_called_once_with(WS_REFERENCE)

    def test_does_not_remove_ref_ws_if_absent(self, mock_save, mock_ads, mock_logger):
        exp = self._make_exporter()
        mock_ads.doesExist.return_value = False

        with tempfile.TemporaryDirectory() as tmp:
            exp.output_as_reference_workspace(tmp, "out")

        mock_ads.remove.assert_not_called()

    def test_cleans_up_ref_ws_even_when_build_raises(self, mock_save, mock_ads, mock_logger):
        exp = self._make_exporter()
        exp._build_reference_ws.side_effect = RuntimeError("boom")
        mock_ads.doesExist.return_value = True

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(RuntimeError):
                exp.output_as_reference_workspace(tmp, "out")

        mock_ads.remove.assert_called_once_with(WS_REFERENCE)
        mock_save.assert_not_called()


if __name__ == "__main__":
    unittest.main()
