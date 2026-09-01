# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
from unittest import mock

import numpy as np
from mantid.simpleapi import Load, MDNormSCDPreprocessIncoherent


class MDNormSCDPreprocessIncoherentTest(unittest.TestCase):
    def testMaskingBranchPassesRootComponentInstrumentName(self):
        r"""
        Regression coverage for the masking branch's instrument-name lookup: the algorithm must
        call LoadMask with Instrument=componentInfo().name(componentInfo().root()) rather than the
        deprecated getInstrument().getName(). Instead of only comparing the two APIs on a bare
        workspace, this drives MDNormSCDPreprocessIncoherent with a MaskFile so the masking branch
        actually executes, spies on the LoadMask call it issues, and asserts the Instrument value
        it received.

        The end-to-end mask cannot be applied here: the historical CNCS_7860 file's geometry has
        51203 detectors, while LoadMask(Instrument="CNCS", ...) builds its reference workspace from
        the environment's bundled (newer) CNCS IDF, which has 51204 - so MaskDetectors always
        raises "Instrument's detector numbers mismatch" independent of the instrument-name API.
        The spy therefore intercepts LoadMask, records the Instrument argument, and aborts the run
        before MaskDetectors, which is exactly the branch behaviour under test.
        """
        # Baseline the value the masking branch is expected to derive.
        van = Load(Filename="CNCS_7860", OutputWorkspace="__van_probe")
        expected_instrument = van.getInstrumentName()

        captured = {}

        def spy_load_mask(*args, **kwargs):
            captured.update(kwargs)
            # Abort the algorithm here: the following MaskDetectors call would fail on the
            # unrelated 51203/51204 detector-count mismatch described above.
            raise RuntimeError("stop after LoadMask")

        mask_handle, mask_path = tempfile.mkstemp(suffix=".xml")
        with os.fdopen(mask_handle, "w") as mask_file:
            mask_file.write("<?xml version='1.0'?>\n<detector-masking>\n<group>\n<detids>1</detids>\n</group>\n</detector-masking>\n")

        try:
            with mock.patch("MDNormSCDPreprocessIncoherent.LoadMask", side_effect=spy_load_mask):
                # The masking branch runs, calls the spied LoadMask, and the algorithm then fails.
                with self.assertRaises(RuntimeError):
                    MDNormSCDPreprocessIncoherent(
                        Filename="CNCS_7860",
                        MomentumMin=1,
                        MomentumMax=1.5,
                        MaskFile=mask_path,
                        SolidAngleOutputWorkspace="__SA",
                        FluxOutputWorkspace="__Flux",
                    )
        finally:
            os.remove(mask_path)

        # LoadMask was reached, and the Instrument it received is the root component's name
        # (the behaviour-preserving replacement for the deprecated getInstrument().getName()).
        self.assertEqual(captured.get("Instrument"), expected_instrument)

    def testCNCS(self):
        # CNCS_7860 is not an incoherent scatterer but for this test
        # it doesn't matter
        SA, Flux = MDNormSCDPreprocessIncoherent(Filename="CNCS_7860", MomentumMin=1, MomentumMax=1.5)

        # Just compare 10 points of the Flux
        flux_cmp = np.array(
            [
                0.00000000e00,
                7.74945234e-04,
                4.96143098e-03,
                1.18914010e-02,
                1.18049991e-01,
                7.71872176e-01,
                9.93078957e-01,
                9.96312349e-01,
                9.98450129e-01,
                1.00000002e00,
            ]
        )
        np.testing.assert_allclose(Flux.extractY()[0][::1000], flux_cmp)
        self.assertEqual(Flux.getXDimension().name, "Momentum")
        self.assertEqual(Flux.getXDimension().getUnits(), "Angstrom^-1")
        self.assertEqual(Flux.blocksize(), 10000)
        self.assertEqual(Flux.getNumberHistograms(), 1)

        # Compare every 20-th bin of row 64
        SA_cmp = np.array(
            [
                0.11338311,
                0.18897185,
                0.15117748,
                0.11338311,
                0.03779437,
                0.07558874,
                0.15117748,
                0.18897185,
                0.03779437,
                0.15117748,
                0.11338311,
                0.07558874,
                0.03779437,
                0.0,
                0.56691555,
                0.26456059,
                0.11338311,
                0.07558874,
                0.11338311,
                0.0,
            ]
        )
        np.testing.assert_allclose(SA.extractY().reshape((-1, 128))[::20, 64], SA_cmp)
        self.assertEqual(SA.getXDimension().name, "Momentum")
        self.assertEqual(SA.getXDimension().getUnits(), "Angstrom^-1")
        self.assertEqual(SA.blocksize(), 1)
        self.assertEqual(SA.getNumberHistograms(), 51200)
        self.assertEqual(SA.getNEvents(), 51200)


if __name__ == "__main__":
    unittest.main()
