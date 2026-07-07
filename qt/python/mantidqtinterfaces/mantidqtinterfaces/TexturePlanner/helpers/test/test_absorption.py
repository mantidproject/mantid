# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from unittest.mock import patch, MagicMock, call
from scipy.spatial.transform import Rotation

from mantidqtinterfaces.TexturePlanner.helpers.absorption import AbsorptionCalculator

file_path = "mantidqtinterfaces.TexturePlanner.helpers.absorption"

WS_MC_INPUT = "__mc_ws"
WS_MC_OUTPUT = "__abs_ws"
WS_DATA = "__texture_planning_ws"


def _make_wsm(offset=(0.0, 0.0, 0.0), init_R=None, gauge_volume_str="<gv/>"):
    wsm = MagicMock()
    wsm.WS_MC_INPUT = WS_MC_INPUT
    wsm.WS_MC_OUTPUT = WS_MC_OUTPUT
    wsm.wsname = WS_DATA
    wsm.mesh_ws = "mesh_ws"
    wsm.offset = offset
    wsm.init_R = init_R if init_R is not None else Rotation.identity()
    wsm.gauge_volume_str = gauge_volume_str
    wsm.attenuation_kwargs = {"point": 1.5, "unit": "dSpacing"}
    return wsm


def _make_model(R=None, n_orientations=1, spec_inds=None):
    model = MagicMock()
    model.workspaces = _make_wsm()
    orient = MagicMock()
    orient.R = R if R is not None else Rotation.identity()
    model.orientations = MagicMock()
    model.orientations.__getitem__.side_effect = lambda i: orient
    model.orientations.keys.return_value = list(range(n_orientations))
    model.geometry.spec_inds = spec_inds or []
    return model, orient


@patch(file_path + ".ConvertUnits")
class TestAbsorptionCalculator_CreateMcWs(unittest.TestCase):
    def test_converts_units_to_wavelength_into_mc_input(self, mock_convert):
        wsm = _make_wsm()
        mc_ws = MagicMock()
        mock_convert.return_value = mc_ws

        result = AbsorptionCalculator._create_mc_ws(wsm)

        mock_convert.assert_called_once_with(InputWorkspace=WS_DATA, Target="Wavelength", OutputWorkspace=WS_MC_INPUT)
        self.assertIs(result, mc_ws)

    def test_resets_goniometer_to_identity(self, mock_convert):
        wsm = _make_wsm()
        mc_ws = MagicMock()
        gonio = MagicMock()
        mc_ws.run.return_value.getGoniometer.return_value = gonio
        mock_convert.return_value = mc_ws

        AbsorptionCalculator._create_mc_ws(wsm)

        gonio.setR.assert_called_once()
        np.testing.assert_array_equal(gonio.setR.call_args.args[0], np.eye(3))


@patch(file_path + ".define_gauge_volume")
@patch(file_path + ".RotateSampleShape")
@patch(file_path + ".CopySample")
class TestAbsorptionCalculator_SetMcSampleState(unittest.TestCase):
    def test_copies_shape_and_material_from_mesh_ws(self, mock_copy, mock_rotate, mock_define_gv):
        wsm = _make_wsm()
        mc_ws = MagicMock()

        AbsorptionCalculator._set_mc_sample_state(wsm, mc_ws, Rotation.identity())

        mock_copy.assert_called_once_with(
            InputWorkspace="mesh_ws",
            OutputWorkspace=WS_MC_INPUT,
            CopyShape=True,
            CopyMaterial=True,
            CopyEnvironment=False,
            CopyLattice=False,
        )

    def test_applies_translate_with_offset(self, mock_copy, mock_rotate, mock_define_gv):
        wsm = _make_wsm(offset=(1.0, 2.0, 3.0))
        mc_ws = MagicMock()

        AbsorptionCalculator._set_mc_sample_state(wsm, mc_ws, Rotation.identity())

        wsm.translate_shape.assert_called_once_with(mc_ws, 1.0, 2.0, 3.0)

    def test_skips_rotate_when_combined_rotation_is_identity(self, mock_copy, mock_rotate, mock_define_gv):
        wsm = _make_wsm(init_R=Rotation.identity())
        mc_ws = MagicMock()

        AbsorptionCalculator._set_mc_sample_state(wsm, mc_ws, Rotation.identity())

        mock_rotate.assert_not_called()

    def test_rotates_with_composed_R_times_init_R(self, mock_copy, mock_rotate, mock_define_gv):
        init_R = Rotation.from_euler("y", 30, degrees=True)
        R = Rotation.from_euler("x", 60, degrees=True)
        wsm = _make_wsm(init_R=init_R)
        mc_ws = MagicMock()

        AbsorptionCalculator._set_mc_sample_state(wsm, mc_ws, R)

        mock_rotate.assert_called_once()
        ws_arg, rot_str = mock_rotate.call_args.args
        self.assertEqual(ws_arg, WS_MC_INPUT)

        parts = rot_str.split(",")
        ang = float(parts[0])
        vec = np.array([float(parts[1]), float(parts[2]), float(parts[3])])
        self.assertEqual(parts[4], "1")

        expected = (R * init_R).as_rotvec(degrees=True)
        expected_ang = np.linalg.norm(expected)
        expected_vec = expected / expected_ang
        self.assertAlmostEqual(ang, expected_ang)
        np.testing.assert_allclose(vec, expected_vec, atol=1e-12)

    def test_defines_gauge_volume_with_str_from_wsm(self, mock_copy, mock_rotate, mock_define_gv):
        wsm = _make_wsm(gauge_volume_str="<gv/>")
        mc_ws = MagicMock()

        AbsorptionCalculator._set_mc_sample_state(wsm, mc_ws, Rotation.identity())

        mock_define_gv.assert_called_once_with(mc_ws, "<gv/>")


@patch(file_path + ".read_attenuation_coefficient_at_value")
@patch(file_path + ".MonteCarloAbsorption")
class TestAbsorptionCalculator_CalcForIndex(unittest.TestCase):
    def _make_calculator(self, spec_inds, n_hist=4):
        model, orient = _make_model(spec_inds=spec_inds)
        calc = AbsorptionCalculator(model)
        calc._create_mc_ws = MagicMock()
        mc_ws = MagicMock()
        mc_ws.getNumberHistograms.return_value = n_hist
        calc._create_mc_ws.return_value = mc_ws
        calc._set_mc_sample_state = MagicMock()
        return calc, model, orient, mc_ws

    def test_orchestrates_create_then_set_state_then_mc_then_set_transmission(self, mock_mc, mock_read):
        calc, model, orient, mc_ws = self._make_calculator(spec_inds=[0, 1, 2, 3])
        mock_read.return_value = np.array([0.1, 0.2, 0.3, 0.4])

        calc.calc_for_index(0)

        calc._create_mc_ws.assert_called_once_with(model.workspaces)
        calc._set_mc_sample_state.assert_called_once_with(model.workspaces, mc_ws, orient.R)
        mock_mc.assert_called_once_with(**calc.mc_kwargs)

    def test_passes_transmission_slice_from_starting_ind(self, mock_mc, mock_read):
        calc, model, _, _ = self._make_calculator(spec_inds=[2, 3])
        mock_read.return_value = np.array([0.1, 0.2, 0.3, 0.4])

        calc.calc_for_index(0)

        mock_read.assert_called_once_with(
            WS_MC_OUTPUT,
            model.workspaces.attenuation_kwargs["point"],
            model.workspaces.attenuation_kwargs["unit"],
        )
        set_call = model.orientations.set_transmission_at_index.call_args
        np.testing.assert_array_equal(set_call.args[0], np.array([0.3, 0.4]))
        self.assertEqual(set_call.args[1], 0)

    def test_zero_transmission_when_mc_absorption_raises(self, mock_mc, mock_read):
        calc, model, _, _ = self._make_calculator(spec_inds=[1, 2, 3, 4], n_hist=5)
        mock_mc.side_effect = RuntimeError("outside gauge volume")

        calc.calc_for_index(0)

        mock_read.assert_not_called()
        set_call = model.orientations.set_transmission_at_index.call_args
        np.testing.assert_array_equal(set_call.args[0], np.zeros(4))
        self.assertEqual(set_call.args[1], 0)

    @patch(file_path + ".logger")
    def test_logs_warning_when_mc_absorption_raises(self, mock_logger, mock_mc, mock_read):
        calc, _, _, _ = self._make_calculator(spec_inds=[0])
        mock_mc.side_effect = RuntimeError("boom")

        calc.calc_for_index(0)

        mock_logger.warning.assert_called_once()


class TestAbsorptionCalculator_Init(unittest.TestCase):
    def test_mc_kwargs_built_from_workspace_constants(self):
        model, _ = _make_model()

        calc = AbsorptionCalculator(model)

        self.assertEqual(
            calc.mc_kwargs,
            {
                "InputWorkspace": WS_MC_INPUT,
                "OutputWorkspace": WS_MC_OUTPUT,
                "EventsPerPoint": 50,
                "MaxScatterPtAttempts": int(1e4),
                "SimulateScatteringPointIn": "SampleOnly",
                "ResimulateTracksForDifferentWavelengths": False,
            },
        )


class TestAbsorptionCalculator_CalcAll(unittest.TestCase):
    def test_iterates_over_all_orientation_keys(self):
        model = MagicMock()
        model.orientations.keys.return_value = [0, 2, 5]
        calc = AbsorptionCalculator(model)
        calc.calc_for_index = MagicMock()

        calc.calc_all()

        self.assertEqual(calc.calc_for_index.call_args_list, [call(0), call(2), call(5)])

    def test_no_calls_when_no_orientations(self):
        model = MagicMock()
        model.orientations.keys.return_value = []
        calc = AbsorptionCalculator(model)
        calc.calc_for_index = MagicMock()

        calc.calc_all()

        calc.calc_for_index.assert_not_called()


if __name__ == "__main__":
    unittest.main()
