# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from unittest.mock import patch, MagicMock

from mantidqtinterfaces.TexturePlanner.helpers.detector_geometry import DetectorGeometry

file_path = "mantidqtinterfaces.TexturePlanner.helpers.detector_geometry"


def _make_wsm(ws="ws", ungrouped_ws="ungrouped_ws", gauge_volume_str=None, scattering_centre=(0, 0, 0)):
    wsm = MagicMock()
    wsm.ws = ws
    wsm.ungrouped_ws = ungrouped_ws
    wsm.wsname = "__texture_planning_ws"
    wsm.gauge_volume_str = gauge_volume_str
    wsm.scattering_centre = np.asarray(scattering_centre, dtype=float)
    return wsm


def _make_model(wsm=None, grouping_path="/calib/ENGINX_grouping.xml"):
    model = MagicMock()
    model.workspaces = wsm if wsm is not None else _make_wsm()
    model.instrument.get_grouping_path.return_value = grouping_path
    return model


class TestDetectorGeometry_Init(unittest.TestCase):
    def assertEmptyArray(self, arr):
        return self.assertTrue(arr.size == 0)

    def test_default_attributes(self):
        dg = DetectorGeometry(_make_model())

        self.assertEmptyArray(dg.det_k)
        self.assertEmptyArray(dg.detQs_lab)
        self.assertEqual(len(dg.spec_inds), 0)
        self.assertEmptyArray(dg._det_positions)
        self.assertEmptyArray(dg._source_position)


class TestDetectorGeometry_GetGroupingPath(unittest.TestCase):
    def test_delegates_to_instrument_helper(self):
        model = _make_model(grouping_path="/calib/GRP.xml")
        dg = DetectorGeometry(model)

        self.assertEqual(dg._get_grouping_path(), "/calib/GRP.xml")
        model.instrument.get_grouping_path.assert_called_once_with()


@patch(file_path + ".define_gauge_volume")
@patch(file_path + ".GroupDetectors")
class TestDetectorGeometry_ApplyGroupingToWss(unittest.TestCase):
    def test_syncs_ws_into_ungrouped_before_regroup_when_ws_present(self, mock_group, mock_define_gv):
        wsm = _make_wsm(ws="ws", ungrouped_ws="ungrouped_ws")
        mock_group.return_value = "new_ws"

        DetectorGeometry._apply_grouping_to_wss(wsm, "/path/grp.xml")

        # the sync must preserve the sample's initial rotation, so it goes through the dedicated
        # workspace-manager helper rather than a bare CopySample (which drops init_R for a CSG shape)
        wsm.copy_sample_preserving_initial_rotation.assert_called_once_with("ws", "ungrouped_ws")

    def test_skips_sync_when_ws_is_none(self, mock_group, mock_define_gv):
        wsm = _make_wsm(ws=None)
        mock_group.return_value = "new_ws"

        DetectorGeometry._apply_grouping_to_wss(wsm, "/path/grp.xml")

        wsm.copy_sample_preserving_initial_rotation.assert_not_called()

    def test_regroups_from_ungrouped_into_wsname_and_assigns_back_to_wsm(self, mock_group, mock_define_gv):
        wsm = _make_wsm()
        mock_group.return_value = "new_ws"

        result = DetectorGeometry._apply_grouping_to_wss(wsm, "/path/grp.xml")

        mock_group.assert_called_once_with(
            InputWorkspace="ungrouped_ws",
            MapFile="/path/grp.xml",
            OutputWorkspace=wsm.wsname,
        )
        self.assertEqual(wsm.ws, "new_ws")
        self.assertEqual(result, "new_ws")

    def test_reapplies_gauge_volume_when_set(self, mock_group, mock_define_gv):
        wsm = _make_wsm(gauge_volume_str="<gv/>")
        mock_group.return_value = "new_ws"

        DetectorGeometry._apply_grouping_to_wss(wsm, "/path/grp.xml")

        mock_define_gv.assert_called_once_with("new_ws", "<gv/>")

    def test_does_not_define_gauge_volume_when_unset(self, mock_group, mock_define_gv):
        wsm = _make_wsm(gauge_volume_str=None)
        mock_group.return_value = "new_ws"

        DetectorGeometry._apply_grouping_to_wss(wsm, "/path/grp.xml")

        mock_define_gv.assert_not_called()


@patch(file_path + ".LoadDetectorsGroupingFile")
class TestDetectorGeometry_Recompute(unittest.TestCase):
    def _make_dg_with_stubs(self, n_hist=4, inds_with_dets=(0, 1, 2, 3)):
        model = _make_model()
        dg = DetectorGeometry(model)
        dg._get_grouping_path = MagicMock(return_value="/path/grp.xml")
        group_ws = MagicMock()
        group_ws.getNumberHistograms.return_value = n_hist
        group_ws.spectrumInfo.return_value.hasDetectors.side_effect = lambda i: i in inds_with_dets
        group_ws.spectrumInfo.return_value.position.side_effect = lambda i: np.array([float(i), 0.0, 0.0])
        group_ws.componentInfo.return_value.sourcePosition.return_value = np.array([-1.0, 0.0, 0.0])
        dg._apply_grouping_to_wss = MagicMock(return_value=group_ws)
        dg.recompute_scattering_geometry = MagicMock()
        return dg, group_ws

    def test_delegates_to_apply_grouping_to_wss(self, mock_load_grp):
        dg, _ = self._make_dg_with_stubs()
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1], [2], [3]])
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        dg._apply_grouping_to_wss.assert_called_once_with(dg._model.workspaces, "/path/grp.xml")

    def test_loads_detector_grouping_file_without_ads(self, mock_load_grp):
        dg, _ = self._make_dg_with_stubs()
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1], [2]])
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        mock_load_grp.assert_called_once_with(InputFile="/path/grp.xml", OutputWorkspace="tmp_grp", StoreInADS=False)

    def test_ignore_first_group_when_null_group_present(self, mock_load_grp):
        dg, _ = self._make_dg_with_stubs()
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[0, 1, 1, 2, 2, 3, 2]])  # group will be array of int group labels
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        self.assertEqual(dg.spec_inds, [1, 2, 3])

    def test_all_specs_included_when_no_null_group(self, mock_load_grp):
        specs_with_dets = (0, 1, 2, 3)
        dg, _ = self._make_dg_with_stubs(inds_with_dets=specs_with_dets)
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1, 2, 3, 2, 3]])
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        self.assertEqual(dg.spec_inds, list(specs_with_dets))

    def test_captures_det_positions_from_starting_ind(self, mock_load_grp):
        dg, group_ws = self._make_dg_with_stubs(n_hist=4)
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[0], [1], [2]])  # null group -> starting_ind=1
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        # positions captured for spec 1,2,3 (skipping null group at 0)
        np.testing.assert_array_equal(dg._det_positions, np.array([[1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]]))

    def test_skips_leading_spectra_without_detectors(self, mock_load_grp):
        # a custom grouping file can leave the leading group empty (no detectors); these must be
        # skipped so spectrumInfo.position is never asked for a detector-less spectrum
        dg, group_ws = self._make_dg_with_stubs(n_hist=4, inds_with_dets=(1, 2, 3))
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1], [2], [3]])  # no null group -> base starting_ind=0
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        self.assertEqual(dg.spec_inds, [1, 2, 3])
        np.testing.assert_array_equal(dg._det_positions, np.array([[1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]]))

    def test_skips_any_spectra_without_detectors(self, mock_load_grp):
        # not sure it is likely that any of the other spectra might end up without detectors but guard in case
        # we will make it so the first and last spectra don't have any detectors attached
        dg, group_ws = self._make_dg_with_stubs(n_hist=4, inds_with_dets=(1, 2))
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1, 1, 1, 2, 2, 2, 3, 3, 3]])  # no 0 ind -> no null group
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        self.assertEqual(dg.spec_inds, [1, 2])
        np.testing.assert_array_equal(dg._det_positions, np.array([[1.0, 0.0, 0.0], [2.0, 0.0, 0.0]]))

    def test_handles_no_detectors(self, mock_load_grp):
        # check sensible things happen if no spectra have detectors
        dg, group_ws = self._make_dg_with_stubs(n_hist=4, inds_with_dets=())
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1, 1, 1, 2, 2, 2, 3, 3, 3]])  # no 0 ind -> no null group
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        self.assertEqual(dg.spec_inds, [])
        self.assertEqual(len(dg._det_positions), 0)

    def test_captures_source_position_from_component_info(self, mock_load_grp):
        dg, _ = self._make_dg_with_stubs()
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1], [2]])
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        np.testing.assert_array_equal(dg._source_position, np.array([-1.0, 0.0, 0.0]))

    def test_finishes_by_calling_recompute_scattering_geometry(self, mock_load_grp):
        dg, _ = self._make_dg_with_stubs()
        tmp_grp = MagicMock()
        tmp_grp.extractY.return_value = np.array([[1], [2]])
        mock_load_grp.return_value = tmp_grp

        dg.recompute()

        dg.recompute_scattering_geometry.assert_called_once_with()


class TestDetectorGeometry_RecomputeScatteringGeometry(unittest.TestCase):
    def assertEmptyArray(self, arr):
        return self.assertTrue(arr.size == 0)

    def test_returns_early_when_no_det_positions(self):
        dg = DetectorGeometry(_make_model())
        # untouched, so _det_positions is None

        dg.recompute_scattering_geometry()

        self.assertEmptyArray(dg.det_k)
        self.assertEmptyArray(dg.detQs_lab)

    def test_det_k_is_unit_vectors_from_scattering_centre_to_detectors(self):
        wsm = _make_wsm(scattering_centre=(0.0, 0.0, 0.0))
        model = _make_model(wsm=wsm)
        dg = DetectorGeometry(model)
        dg._det_positions = np.array([[2.0, 0.0, 0.0], [0.0, 3.0, 0.0]])
        dg._source_position = np.array([-1.0, 0.0, 0.0])
        dg.spec_inds = [0, 1]

        dg.recompute_scattering_geometry()

        np.testing.assert_allclose(dg.det_k, np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]))

    def test_det_k_uses_scattering_centre_as_origin(self):
        wsm = _make_wsm(scattering_centre=(1.0, 0.0, 0.0))
        model = _make_model(wsm=wsm)
        dg = DetectorGeometry(model)
        dg._det_positions = np.array([[3.0, 0.0, 0.0]])
        dg._source_position = np.array([-1.0, 0.0, 0.0])
        dg.spec_inds = [
            0,
        ]

        dg.recompute_scattering_geometry()

        # vector from scattering centre (1,0,0) to det (3,0,0) is (2,0,0) -> normalised (1,0,0)
        np.testing.assert_allclose(dg.det_k, np.array([[1.0, 0.0, 0.0]]))

    def test_detQs_lab_is_kd_minus_ki_normalised(self):
        wsm = _make_wsm(scattering_centre=(0.0, 0.0, 0.0))
        model = _make_model(wsm=wsm)
        dg = DetectorGeometry(model)
        # one detector perpendicular to beam: K_d=(0,1,0), beam K_i=(1,0,0) -> Q=(-1,1,0)/sqrt(2)
        dg._det_positions = np.array([[0.0, 1.0, 0.0]])
        dg._source_position = np.array([-1.0, 0.0, 0.0])
        dg.spec_inds = [
            0,
        ]

        dg.recompute_scattering_geometry()

        expected = np.array([[-1.0, 1.0, 0.0]]) / np.sqrt(2.0)
        np.testing.assert_allclose(dg.detQs_lab, expected)


if __name__ == "__main__":
    unittest.main()
