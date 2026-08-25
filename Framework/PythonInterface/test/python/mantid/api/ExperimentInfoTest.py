# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

###############################################################################
# This has to be tested through a workspace as it cannot be created in
# Python
###############################################################################
from testhelpers import run_algorithm
from mantid.geometry import Instrument
from mantid.api import Sample, Run
from mantid.simpleapi import CopySample, CreateSampleShape, CreateSampleWorkspace, LoadEmptyInstrument


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

    # -------------------------------------------------------------------------
    # Lab frame sample shape
    # -------------------------------------------------------------------------

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

    def test_lab_frame_sample_shape_is_unchanged_when_there_is_no_goniometer(self):
        ws = self._ws_with_offset_sphere()

        lab_shape = ws.getLabFrameSampleShape()

        self._assert_centred_on(lab_shape, [2.0, 0.0, 0.0])
        np.testing.assert_allclose(lab_shape.getAppliedRotation(), np.eye(3), atol=1e-12)

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


if __name__ == "__main__":
    unittest.main()
