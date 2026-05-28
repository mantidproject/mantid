# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Verify that loading SNAP_mini_Definition.xml (pixel_assembly banks) produces
detector geometry that is identical to loading SNAP_mini_RD_Definition.xml
(equivalent instrument built with rectangular_detector banks).

The rectangular_detector version is the ground truth: it uses the existing,
well-tested code path.  The pixel_assembly version exercises the new
PixelAssembly / IVirtualBank path.

Expected instrument layout (both files)
----------------------------------------
  2 panels x 3 columns x 3 banks = 18 banks
  Each bank: 256x256 pixels, Y-fastest fill order
  Total pixels: 1 179 648  (IDs 0 – 1 179 647, stride 65536 per bank)
  Monitors: IDs -1, -2 (included in detectorInfo; di.size() == 1 179 650)
"""

import unittest
from mantid.simpleapi import LoadEmptyInstrument

_N_PIXELS = 18 * 256 * 256  # 1 179 648
_N_TOTAL = _N_PIXELS + 2  # + 2 monitors

# --------------------------------------------------------------------------- #
#  Helpers                                                                     #
# --------------------------------------------------------------------------- #

_RD_WS = None  # rectangular_detector reference  (loaded once)
_PA_WS = None  # pixel_assembly under test        (loaded once)


def _load_once():
    global _RD_WS, _PA_WS
    if _RD_WS is None:
        _RD_WS = LoadEmptyInstrument(
            Filename="SNAP_mini_RD_Definition.xml",
            OutputWorkspace="__snap_mini_rd",
        )
    if _PA_WS is None:
        _PA_WS = LoadEmptyInstrument(
            Filename="SNAP_mini_Definition.xml",
            OutputWorkspace="__snap_mini_pa",
        )


# --------------------------------------------------------------------------- #
#  Tests that exercise the rectangular_detector reference on its own           #
#  These pass today and document the expected geometry.                        #
# --------------------------------------------------------------------------- #


class SnapMiniRDGeometryTest(unittest.TestCase):
    """Sanity-check the reference instrument (rectangular_detector)."""

    @classmethod
    def setUpClass(cls):
        _load_once()
        cls.ws = _RD_WS
        cls.di = cls.ws.detectorInfo()

    def test_detector_count(self):
        """18 banks x 65536 pixels + 2 monitors = 1 179 650 entries in detectorInfo."""
        self.assertEqual(self.di.size(), _N_TOTAL)

    def test_pixel_count(self):
        """Exactly 1 179 648 non-monitor detectors (18 banks x 256x256)."""
        n_pixels = sum(1 for i in range(self.di.size()) if not self.di.isMonitor(i))
        self.assertEqual(n_pixels, _N_PIXELS)

    def test_detector_ids_cover_expected_range(self):
        """All pixel IDs 0 – 1 179 647 are present (monitors have negative IDs)."""
        pixel_ids = sorted(int(self.di.detectorIDs()[i]) for i in range(self.di.size()) if not self.di.isMonitor(i))
        self.assertEqual(len(pixel_ids), _N_PIXELS)
        self.assertEqual(pixel_ids[0], 0)
        self.assertEqual(pixel_ids[-1], _N_PIXELS - 1)

    def test_all_pixel_l2_positive(self):
        """Every pixel is on the same side of the sample (l2 > 0)."""
        for i in range(self.di.size()):
            if self.di.isMonitor(i):
                continue
            self.assertGreater(self.di.l2(i), 0.0, msg=f"detector index {i}")

    def test_panel_distance_roughly_two_metres(self):
        """Panels were placed at +/-2 m from origin; l2 should be near 2 m."""
        l2_values = [self.di.l2(i) for i in range(self.di.size()) if not self.di.isMonitor(i)]
        # With bank offsets up to ~0.24 m, l2 is between ~1.9 and ~2.2 m.
        self.assertTrue(
            all(1.5 < v < 2.5 for v in l2_values),
            msg=f"l2 range [{min(l2_values):.3f}, {max(l2_values):.3f}]",
        )

    def test_bank11_pixel_ids(self):
        """bank11 has idstart=0; first four Y-fastest pixels are IDs 0,1,2,3."""
        ids = set(self.di.detectorIDs())
        for expected in (0, 1, 2, 3):
            self.assertIn(expected, ids)

    def test_bank13_pixel_ids(self):
        """bank13 has idstart=131072; first four pixels are 131072 – 131075."""
        ids = set(self.di.detectorIDs())
        for expected in (131072, 131073, 131074, 131075):
            self.assertIn(expected, ids)


# --------------------------------------------------------------------------- #
#  Tests that compare pixel_assembly against rectangular_detector              #
#  These will FAIL until InstrumentDefinitionParser supports pixel_assembly.   #
# --------------------------------------------------------------------------- #


class PixelAssemblyVsRectangularDetectorTest(unittest.TestCase):
    """
    pixel_assembly (SNAP_mini_Definition.xml) must be geometrically identical
    to rectangular_detector (SNAP_mini_RD_Definition.xml).
    """

    _ABS_TOL = 1e-9  # metres -- positions must agree to sub-nanometre
    # Stride used to sample the full pixel population in position/l2/twoTheta
    # checks — testing every pixel at 1.18M would be very slow.
    _SAMPLE_STRIDE = 1000

    @classmethod
    def setUpClass(cls):
        _load_once()
        cls.di_rd = _RD_WS.detectorInfo()
        cls.di_pa = _PA_WS.detectorInfo()

        # Build ID -> index maps for NON-MONITOR detectors only, so we compare
        # the same physical pixel regardless of storage order in each workspace.
        cls.id_to_idx_rd = {int(cls.di_rd.detectorIDs()[i]): i for i in range(cls.di_rd.size()) if not cls.di_rd.isMonitor(i)}
        cls.id_to_idx_pa = {int(cls.di_pa.detectorIDs()[i]): i for i in range(cls.di_pa.size()) if not cls.di_pa.isMonitor(i)}
        # Sorted pixel IDs used for sampled iteration
        cls.sorted_ids = sorted(cls.id_to_idx_rd)

    # -- basic counts ----------------------------------------------------------

    def test_same_detector_count(self):
        """Both instruments report the same total number of entries."""
        self.assertEqual(self.di_pa.size(), self.di_rd.size())

    def test_same_pixel_count(self):
        """Same number of non-monitor pixels."""
        self.assertEqual(len(self.id_to_idx_pa), len(self.id_to_idx_rd))

    def test_same_detector_ids(self):
        """Same set of pixel detector IDs (monitors excluded): check count, min, max."""
        ids_pa = sorted(self.id_to_idx_pa)
        ids_rd = self.sorted_ids
        self.assertEqual(len(ids_pa), len(ids_rd))
        self.assertEqual(ids_pa[0], ids_rd[0])
        self.assertEqual(ids_pa[-1], ids_rd[-1])

    # -- per-detector geometry (sampled every _SAMPLE_STRIDE pixels) -----------

    def _assert_positions_match(self, det_id):
        idx_rd = self.id_to_idx_rd[det_id]
        idx_pa = self.id_to_idx_pa[det_id]
        pos_rd = self.di_rd.position(idx_rd)
        pos_pa = self.di_pa.position(idx_pa)
        for axis, vrd, vpa in zip("XYZ", pos_rd, pos_pa):
            self.assertAlmostEqual(
                vpa,
                vrd,
                delta=self._ABS_TOL,
                msg=f"detector ID {det_id}: position {axis} mismatch (RD={vrd:.9f}, PA={vpa:.9f})",
            )

    def test_all_positions_match(self):
        """Sampled pixel world positions must agree to within 1 nm."""
        sample = self.sorted_ids[:: self._SAMPLE_STRIDE]
        for det_id in sample:
            with self.subTest(det_id=det_id):
                self._assert_positions_match(det_id)

    def test_all_l2_match(self):
        """Sampled sample-to-detector distances must agree for every tested pixel."""
        sample = self.sorted_ids[:: self._SAMPLE_STRIDE]
        for det_id in sample:
            idx_rd = self.id_to_idx_rd[det_id]
            idx_pa = self.id_to_idx_pa[det_id]
            l2_rd = self.di_rd.l2(idx_rd)
            l2_pa = self.di_pa.l2(idx_pa)
            with self.subTest(det_id=det_id):
                self.assertAlmostEqual(
                    l2_pa,
                    l2_rd,
                    delta=self._ABS_TOL,
                    msg=f"detector ID {det_id}: l2 mismatch (RD={l2_rd:.9f}, PA={l2_pa:.9f})",
                )

    def test_all_two_theta_match(self):
        """Sampled scattering angle 2-theta must agree for every tested pixel."""
        sample = self.sorted_ids[:: self._SAMPLE_STRIDE]
        for det_id in sample:
            idx_rd = self.id_to_idx_rd[det_id]
            idx_pa = self.id_to_idx_pa[det_id]
            tt_rd = self.di_rd.twoTheta(idx_rd)
            tt_pa = self.di_pa.twoTheta(idx_pa)
            with self.subTest(det_id=det_id):
                self.assertAlmostEqual(
                    tt_pa,
                    tt_rd,
                    delta=1e-9,
                    msg=f"detector ID {det_id}: twoTheta mismatch (RD={tt_rd:.9f}, PA={tt_pa:.9f})",
                )

    def test_bank11_referent_pixel_position(self):
        """
        bank11: idstart=0, West panel at x=-2, rotated +90-deg about Y.
        Pixel (ix=0, iy=0) is at bank-local (-0.078795, -0.078795, 0).
        After rotation (+90-deg Y): local (+z -> world +x, local +x -> world -z).
        The pixel_assembly position must match the rectangular_detector ground truth.
        """
        self._assert_positions_match(0)

    def test_bank63_referent_pixel_position(self):
        """East panel bank63 (idstart=720896): first pixel must match RD."""
        self._assert_positions_match(720896)

    def test_pa_uses_less_memory_than_rd(self):
        """pixel_assembly instrument must consume less memory than rectangular_detector."""
        mem_rd = _RD_WS.getInstrument().getMemorySize()
        mem_pa = _PA_WS.getInstrument().getMemorySize()
        ci_rd = _RD_WS.componentInfo()
        ci_pa = _PA_WS.componentInfo()
        print(f"\nRD instrument memory: {mem_rd:>12,} bytes  ({mem_rd / 1024:.1f} kB)")
        print(f"PA instrument memory: {mem_pa:>12,} bytes  ({mem_pa / 1024:.1f} kB)")
        print(f"Reduction:            {mem_rd - mem_pa:>12,} bytes  ({100 * (mem_rd - mem_pa) / mem_rd:.1f}% smaller)")
        print(f"RD component tree: {ci_rd.size()} components")
        print(f"PA component tree: {ci_pa.size()} components")
        self.assertGreater(
            mem_rd,
            mem_pa,
            f"PA ({mem_pa} B) should use less memory than RD ({mem_rd} B)",
        )


if __name__ == "__main__":
    unittest.main()
