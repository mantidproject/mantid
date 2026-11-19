# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest

import numpy as np

###############################################################################
# This has to be tested through a workspace as it cannot be created in
# Python
###############################################################################
from testhelpers import run_algorithm, WorkspaceCreationHelper
from mantid.geometry import ComponentInfo, Instrument
from mantid.kernel import DateAndTime
from mantid.api import Sample, Run
from mantid.simpleapi import (
    CopySample,
    CreateSampleShape,
    CreateSampleWorkspace,
    LoadEmptyInstrument,
    MoveInstrumentComponent,
    RotateInstrumentComponent,
    SetSample,
    SetUB,
)


class ExperimentInfoTest(unittest.TestCase):
    _expt_ws = None

    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm("CreateWorkspace", DataX=[1, 2, 3, 4, 5], DataY=[1, 2, 3, 4, 5], NSpec=1, child=True)
            ws = alg.getProperty("OutputWorkspace").value
            ws.run().addProperty("run_number", 48127, True)
            self.__class__._expt_ws = ws

    def test_information_access(self):
        inst = self._expt_ws.getInstrument()
        self.assertTrue(isinstance(inst, Instrument))
        self.assertEqual(self._expt_ws.getRunNumber(), 48127)

    def test_sample_access_returns_sample_object(self):
        sample = self._expt_ws.sample()
        self.assertTrue(isinstance(sample, Sample))

    def test_run_access_returns_run_object(self):
        run = self._expt_ws.run()
        self.assertTrue(isinstance(run, Run))

    def test_get_energy_mode(self):
        emode = self._expt_ws.getEMode()
        self.assertEqual(emode, 0)

    def test_detectorInfo(self):
        detInfo = self._expt_ws.detectorInfo()
        # No instrument in test workspace, so size is 0.
        self.assertEqual(detInfo.size(), 0)

    def test_getReferenceFrame(self):
        refFrame = self._expt_ws.getReferenceFrame()
        self.assertEqual(refFrame.vecPointingUp()[0], 0)  # x
        self.assertEqual(refFrame.vecPointingUp()[1], 1)  # y
        self.assertEqual(refFrame.vecPointingUp()[2], 0)  # z

    def test_baseComponentInfo_keeps_original_geometry_after_moves(self):
        ws = CreateSampleWorkspace(NumBanks=2, BankPixelWidth=3, OutputWorkspace="base_component_info_ws")
        component_info = ws.componentInfo()
        base_component_info = ws.baseComponentInfo()
        self.assertTrue(isinstance(base_component_info, ComponentInfo))
        self.assertEqual(base_component_info.size(), component_info.size())
        bank_index = component_info.indexOfAny("bank1")
        original_position = component_info.position(bank_index)
        original_rotation = component_info.rotation(bank_index)
        self.assertEqual(base_component_info.position(bank_index), original_position)

        MoveInstrumentComponent(Workspace=ws, ComponentName="bank1", X=0.1, Y=0.2, Z=0.3, RelativePosition=True)
        RotateInstrumentComponent(Workspace=ws, ComponentName="bank1", X=0, Y=1, Z=0, Angle=30)

        self.assertNotEqual(ws.componentInfo().position(bank_index), original_position)
        self.assertNotEqual(ws.componentInfo().rotation(bank_index), original_rotation)
        self.assertEqual(ws.baseComponentInfo().position(bank_index), original_position)
        self.assertEqual(ws.baseComponentInfo().rotation(bank_index), original_rotation)

    def test_setSample(self):
        sample = Sample()
        sample.setThickness(12.5)

        self._expt_ws.setSample(sample)
        held_sample = self._expt_ws.sample()

        self.assertNotEqual(id(held_sample), id(sample))
        self.assertEqual(held_sample.getThickness(), sample.getThickness())

    def test_setRun(self):
        run = Run()
        run.addProperty("run_property", 1, True)

        self._expt_ws.setRun(run)
        held_run = self._expt_ws.run()

        self.assertNotEqual(id(held_run), id(run))
        self.assertTrue(held_run.hasProperty("run_property"))

    def test_get_instrument_name_none(self):
        inst_name = self._expt_ws.getInstrumentName()
        self.assertEqual(inst_name, "")

    def test_get_instrument_name_with_instrument(self):
        ws = LoadEmptyInstrument(InstrumentName="SNAP")
        inst_name = ws.getInstrumentName()
        self.assertEqual(inst_name, "SNAP")

    def test_instrument_metadata_access(self):
        ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(1, 1)
        self.assertIsInstance(ws.instrumentValidFromDate(), DateAndTime)
        self.assertIsInstance(ws.instrumentValidToDate(), DateAndTime)
        self.assertIsInstance(ws.instrumentFilename(), str)
        self.assertEqual(ws.instrumentXmlText(), "Fake XML")
        self.assertIsInstance(ws.instrumentDefaultView(), str)
        self.assertIsInstance(ws.instrumentDefaultAxis(), str)

    def test_instrument_metadata_matches_a_loaded_IDF(self):
        # A real IDF exercises values the fake instrument leaves empty.
        ws = LoadEmptyInstrument(InstrumentName="ARCS")
        self.assertIn("ARCS", ws.instrumentFilename())
        self.assertTrue(ws.instrumentFilename().endswith(".xml"))
        self.assertEqual(ws.instrumentDefaultView(), "CYLINDRICAL_Y")
        self.assertGreater(len(ws.instrumentXmlText()), 0)
        self.assertLess(ws.instrumentValidFromDate(), ws.instrumentValidToDate())

    @staticmethod
    def _ws_with_offset_sphere():
        """A sphere sitting on +x, so any rotation visibly moves it."""
        ws = CreateSampleWorkspace()
        CreateSampleShape(ws, '<sphere id="offset"><centre x="2.0" y="0.0" z="0.0"/><radius val="0.5"/></sphere>')
        return ws

    def test_shape_reports_an_identity_applied_rotation_by_default(self):
        ws = self._ws_with_offset_sphere()
        np.testing.assert_allclose(ws.sample().getShape().getAppliedRotation(), np.eye(3), atol=1e-12)

    def _assert_centred_on(self, shape, expected):
        centre = shape.getBoundingBox().centrePoint()
        np.testing.assert_allclose([centre.X(), centre.Y(), centre.Z()], expected, atol=1e-6)

    def test_lab_frame_sample_shape_applies_the_goniometer_to_an_unbaked_shape(self):
        ws = self._ws_with_offset_sphere()
        # 90 degrees about z takes the sphere from +x to +y.
        ws.run().getGoniometer().setR(np.array([[0.0, -1.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0]]))

        lab_shape = ws.getLabFrameSampleShape()

        self._assert_centred_on(lab_shape, [0.0, 2.0, 0.0])
        # and the workspace's own sample is untouched
        self._assert_centred_on(ws.sample().getShape(), [2.0, 0.0, 0.0])

    def test_lab_frame_sample_shape_does_not_rotate_an_already_baked_shape_twice(self):
        """CopySample bakes the destination goniometer in; the shape must not turn again."""
        rotation = np.array([[0.0, -1.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])
        source = self._ws_with_offset_sphere()
        dest = CreateSampleWorkspace()
        dest.run().getGoniometer().setR(rotation)
        CopySample(InputWorkspace=source, OutputWorkspace=dest, CopyName=False, CopyEnvironment=False, CopyLattice=False)

        # The bake has already moved it to +y and the shape says so.
        np.testing.assert_allclose(dest.sample().getShape().getAppliedRotation(), rotation, atol=1e-6)
        self._assert_centred_on(dest.sample().getShape(), [0.0, 2.0, 0.0])

        lab_shape = dest.getLabFrameSampleShape()

        # Nothing outstanding, so it stays on +y rather than turning on to -x.
        self._assert_centred_on(lab_shape, [0.0, 2.0, 0.0])

    def test_applied_rotation_is_a_copy_that_outlives_its_shape(self):
        rotation = np.array([[0.0, -1.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])
        ws = self._ws_with_offset_sphere()
        ws.run().getGoniometer().setR(rotation)

        applied = ws.getLabFrameSampleShape().getAppliedRotation()

        np.testing.assert_allclose(applied, rotation, atol=1e-6)
        # a view of the temporary shape's matrix would be read-only
        self.assertTrue(applied.flags.writeable)

    @staticmethod
    def _ws_with_a_sample_in_a_can():
        """An upright cylinder in an upright can, so a quarter turn about z visibly lays both down."""
        ws = CreateSampleWorkspace()
        SetSample(
            ws,
            Geometry={"Shape": "Cylinder", "Height": 4.0, "Radius": 1.0, "Center": [0.0, 0.0, 0.0]},
            Material={"ChemicalFormula": "V", "NumberDensity": 0.07},
            ContainerGeometry={"Shape": "HollowCylinder", "Height": 4.0, "InnerRadius": 1.0, "OuterRadius": 1.3, "Center": [0.0, 0.0, 0.0]},
            ContainerMaterial={"ChemicalFormula": "Al", "NumberDensity": 0.06},
        )
        ws.run().getGoniometer().setR(np.array([[0.0, -1.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0]]))
        return ws

    def _assert_upright(self, shape):
        self.assertAlmostEqual(shape.getBoundingBox().width().Y(), 0.04, places=6)

    def test_a_sample_in_an_environment_is_left_as_defined(self):
        ws = self._ws_with_a_sample_in_a_can()

        self._assert_upright(ws.getLabFrameSampleShape())
        self._assert_upright(ws.getLabFrameSample().getShape())

    def _get_holding_parent(self, parent, accessor):
        """Checks the reference rather than reading through it: freed memory often still holds the old values."""
        before = sys.getrefcount(parent)
        child = accessor(parent)
        self.assertEqual(sys.getrefcount(parent), before + 1, "the child does not keep its parent alive")
        return child

    def test_the_lab_frame_environment_keeps_the_temporary_sample_alive(self):
        sample = self._ws_with_a_sample_in_a_can().getLabFrameSample()

        environment = self._get_holding_parent(sample, lambda s: s.getEnvironment())
        container = self._get_holding_parent(environment, lambda e: e.getContainer())
        self._get_holding_parent(container, lambda c: c.getShape())

    def test_the_lab_frame_lattice_keeps_the_temporary_sample_alive(self):
        ws = CreateSampleWorkspace()
        SetUB(ws, a=4.2, b=5.0, c=6.1)

        lattice = self._get_holding_parent(ws.getLabFrameSample(), lambda s: s.getOrientedLattice())
        self.assertAlmostEqual(lattice.a(), 4.2)


if __name__ == "__main__":
    unittest.main()
