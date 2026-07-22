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

FILE_PATH = "mantidqtinterfaces.TexturePlanner.helpers.exporter"

WS_REFERENCE = "__texture_planning_reference_ws"


def _make_orientation(R=None, include=True, transmission=None):
    orient = MagicMock()
    orient.R = R if R is not None else Rotation.identity()
    orient.include = include
    orient.transmission = transmission
    return orient


def _make_model(orientations=None, instr="ENGINX", axes="xyz", senses="1,1,1", init_R=None, updated_mesh_ws="neutral"):
    model = MagicMock()
    if orientations is None:
        orientations = {0: _make_orientation()}
    model.orientations.values.return_value = list(orientations.values())
    model.instrument.get_instrument.return_value = instr
    model.orientations.orientation_kwargs = {"Axes": axes, "Senses": senses}
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


@patch(FILE_PATH + ".logger")
class TestOrientationExporter_OutputAsSscanss(unittest.TestCase):
    def _run(self, model, filename="out"):
        with tempfile.TemporaryDirectory() as tmp:
            exp = OrientationExporter(model)
            exp.output_as_sscanss(tmp, filename)
            save_file = os.path.join(tmp, filename + ".angles")
            with open(save_file) as f:
                return f.read(), save_file

    @patch(FILE_PATH + ".convert_to_sscanss_frame")
    def test_writes_header_and_one_line_per_orientation(self, mock_convert, mock_logger):
        mock_convert.return_value = (10.0, 20.0, 30.0)
        model = _make_model(orientations={0: _make_orientation(), 1: _make_orientation()})

        content, _ = self._run(model)

        lines = content.splitlines()
        self.assertEqual(lines[0], "xyz")
        self.assertEqual(len(lines), 1 + 2)
        for line in lines[1:]:
            self.assertEqual(line, "10.0\t20.0\t30.0")

    @patch(FILE_PATH + ".convert_to_sscanss_frame")
    def test_rounds_angles_to_two_decimals(self, mock_convert, mock_logger):
        mock_convert.return_value = (1.23456, 2.34567, 3.45678)
        model = _make_model()

        content, _ = self._run(model)

        self.assertIn("1.23\t2.35\t3.46", content)

    @patch(FILE_PATH + ".convert_to_sscanss_frame")
    def test_passes_rotation_matrix_to_convert(self, mock_convert, mock_logger):
        mock_convert.return_value = (0.0, 0.0, 0.0)
        R = Rotation.from_euler("x", 45, degrees=True)
        model = _make_model(orientations={0: _make_orientation(R=R)})

        self._run(model)

        np.testing.assert_allclose(mock_convert.call_args.args[0], R.as_matrix())

    @patch(FILE_PATH + ".convert_to_sscanss_frame")
    def test_logs_notice_on_success(self, mock_convert, mock_logger):
        mock_convert.return_value = (0.0, 0.0, 0.0)
        model = _make_model()

        _, save_file = self._run(model, filename="out")

        mock_logger.notice.assert_called_once()
        self.assertIn(save_file, mock_logger.notice.call_args.args[0])


@patch(FILE_PATH + ".logger")
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

    def test_writes_one_line_per_orientation(self, mock_logger):
        model = _make_model(orientations={0: _make_orientation(), 1: _make_orientation()})

        content, _ = self._run(model)

        self.assertEqual(len(content.splitlines()), 2)

    def test_logs_notice_with_save_file(self, mock_logger):
        model = _make_model()

        _, save_file = self._run(model)

        mock_logger.notice.assert_called_once()
        self.assertIn(save_file, mock_logger.notice.call_args.args[0])


@patch(FILE_PATH + ".logger")
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

    def test_writes_one_line_per_orientation(self, mock_logger):
        orient = self._make_orient_with_mock_R([0.0, 0.0, 0.0])
        model = _make_model(orientations={0: orient, 1: orient})

        content, _ = self._run(model)

        self.assertEqual(len(content.splitlines()), 2)


@patch(FILE_PATH + ".logger")
class TestOrientationExporter_OutputTransmissionWeighting(unittest.TestCase):
    def _run(self, model, filename="out"):
        with tempfile.TemporaryDirectory() as tmp:
            exp = OrientationExporter(model)
            exp.output_transmission_weighting(tmp, filename)
            save_file = os.path.join(tmp, filename + "_transmission_weighting.txt")
            content = None
            if os.path.exists(save_file):
                with open(save_file) as f:
                    content = f.read()
            return content, save_file

    def test_writes_one_row_per_included_orientation(self, mock_logger):
        orientations = {
            0: _make_orientation(transmission=np.array([0.5, 0.8])),
            1: _make_orientation(transmission=np.array([0.4, 0.9])),
            2: _make_orientation(transmission=np.array([0.2, 0.6])),
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        self.assertEqual(len(content.splitlines()), 3)

    def test_normalises_per_orientation_minima_against_the_largest(self, mock_logger):
        # per-orientation minima: 0.5, 0.4, 0.2 -> largest is 0.5 -> weights 0.5/min
        orientations = {
            0: _make_orientation(transmission=np.array([0.5, 0.8])),
            1: _make_orientation(transmission=np.array([0.4, 0.9])),
            2: _make_orientation(transmission=np.array([0.2, 0.6])),
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        weights = [float(line) for line in content.splitlines()]
        np.testing.assert_allclose(weights, [0.5 / 0.5, 0.5 / 0.4, 0.5 / 0.2])

    def test_least_absorbing_orientation_has_weight_one(self, mock_logger):
        orientations = {
            0: _make_orientation(transmission=np.array([0.4])),
            1: _make_orientation(transmission=np.array([0.7])),  # least absorbing
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        weights = [float(line) for line in content.splitlines()]
        self.assertEqual(min(weights), 1.0)
        self.assertTrue(all(w >= 1.0 for w in weights))

    def test_excludes_orientations_with_include_false(self, mock_logger):
        orientations = {
            0: _make_orientation(transmission=np.array([0.5]), include=True),
            1: _make_orientation(transmission=np.array([0.1]), include=False),
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        # only the single included orientation contributes -> one row, normalised to itself
        self.assertEqual(content.splitlines(), ["1.0"])

    def test_skips_and_warns_when_transmission_missing(self, mock_logger):
        orientations = {
            0: _make_orientation(transmission=np.array([0.5])),
            1: _make_orientation(transmission=None),
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        self.assertIsNone(content)
        mock_logger.warning.assert_called_once()
        mock_logger.notice.assert_not_called()

    def test_skips_and_warns_when_an_orientation_has_zero_transmission(self, mock_logger):
        orientations = {
            0: _make_orientation(transmission=np.array([0.5, 0.8])),
            1: _make_orientation(transmission=np.array([0.0, 0.0])),  # outside gauge volume
        }
        model = _make_model(orientations=orientations)

        content, _ = self._run(model)

        self.assertIsNone(content)
        mock_logger.warning.assert_called_once()

    def test_logs_notice_with_save_file_on_success(self, mock_logger):
        model = _make_model(orientations={0: _make_orientation(transmission=np.array([0.5]))})

        _, save_file = self._run(model)

        mock_logger.notice.assert_called_once()
        self.assertIn(save_file, mock_logger.notice.call_args.args[0])


@patch(FILE_PATH + ".LoadEmptyInstrument")
class TestOrientationExporter_BuildReferenceWs(unittest.TestCase):
    def test_loads_empty_instrument_into_reference_wsname(self, mock_load_instr):
        model = _make_model(instr="ENGINX")
        exp = OrientationExporter(model)

        exp._build_reference_ws(WS_REFERENCE)

        mock_load_instr.assert_called_once_with(InstrumentName="ENGINX", OutputWorkspace=WS_REFERENCE)

    def test_copies_sample_preserving_initial_rotation_from_updated_mesh_ws(self, mock_load_instr):
        model = _make_model(updated_mesh_ws="neutral")
        exp = OrientationExporter(model)

        exp._build_reference_ws(WS_REFERENCE)

        # the reference ws keeps the sample's initial rotation (which a plain CopySample would strip
        # from a CSG shape) via the shared workspace-manager helper, sourcing from the
        # identity-goniometer neutral mesh ws and writing into the freshly loaded empty-instrument ws
        model.workspaces.copy_sample_preserving_initial_rotation.assert_called_once_with("neutral", mock_load_instr.return_value)


@patch(FILE_PATH + ".logger")
@patch(FILE_PATH + ".ADS")
@patch(FILE_PATH + ".SaveNexus")
class TestOrientationExporter_OutputAsReferenceWorkspace(unittest.TestCase):
    def _make_exporter(self):
        model = _make_model()
        exp = OrientationExporter(model)
        exp._build_reference_ws = MagicMock()
        return exp

    def test_orchestrates_build_then_save(self, mock_save, mock_ads, mock_logger):
        exp = self._make_exporter()

        with tempfile.TemporaryDirectory() as tmp:
            exp.output_as_reference_workspace(tmp, "out")
            expected_path = os.path.join(tmp, "out.nxs")

            # _build_reference_ws now preserves the initial rotation itself (via the shared helper), so
            # the orchestration is simply build-then-save
            exp._build_reference_ws.assert_called_once_with(WS_REFERENCE)
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
