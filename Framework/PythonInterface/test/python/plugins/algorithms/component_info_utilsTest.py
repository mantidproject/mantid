# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import unittest
import warnings
from mantid.simpleapi import CreateSampleWorkspace, LoadEmptyInstrument

from plugins.algorithms.component_info_utils import (
    find_grid_detector_indices,
    find_rectangular_detector_indices,
    get_assembly_children,
    get_detector_id,
    resolve_component_index,
)


class ComponentInfoUtilsTest(unittest.TestCase):
    def test_resolve_component_index_resolves_nested_path(self):
        r"""
        Regression test for a bug where resolve_component_index resolved a
        '/'-qualified component name (e.g. 'bank7/sixteenpack') to the first
        path segment that is globally unique ('bank7') instead of walking down to the
        leaf component ('sixteenpack') that the full path actually identifies. In the
        CORELLI instrument (and others with the same convention) an intermediate path
        segment is a pure positioning frame placed at <location/> (i.e. at its parent's
        location), while the real detector-bank position lives on the leaf 'sixteenpack'
        component nested inside it. Resolving to the frame instead of the leaf silently
        substitutes the wrong position/rotation, which is what made
        CorelliPowderCalibrationCreate fit component offsets around the wrong starting
        point (systemtest CorelliPowderCalibrationTest).
        """
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        component_info = ws.componentInfo()

        resolved_index = resolve_component_index("bank7/sixteenpack", component_info)
        bank_index = component_info.indexOfAny("bank7")

        # must resolve to the leaf, not to the intermediate positioning frame
        self.assertEqual(component_info.name(resolved_index), "sixteenpack")
        self.assertNotEqual(resolved_index, bank_index)

        # the frame sits at <location/> (its parent's position); the leaf carries the real offset
        self.assertFalse(np.allclose(component_info.position(bank_index), component_info.position(resolved_index)))

    def test_resolve_component_index_bare_name(self):
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        component_info = ws.componentInfo()

        resolved_index = resolve_component_index("bank7", component_info)
        self.assertEqual(component_info.name(resolved_index), "bank7")

    def test_resolve_component_index_missing_raises_value_error(self):
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        component_info = ws.componentInfo()

        self.assertRaises(ValueError, resolve_component_index, "not_a_real_component", component_info)
        self.assertRaises(ValueError, resolve_component_index, "bank7/not_a_real_child", component_info)

    def _assert_same_banks_as_legacy(self, ws):
        component_info = ws.componentInfo()
        # the deprecated Instrument search is the reference these helpers must reproduce, order included
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            instrument = ws.getInstrument()
            legacy_rect_names = [bank.getFullName() for bank in instrument.findRectDetectors()]
            legacy_grid_names = [bank.getFullName() for bank in instrument.findGridDetectors()]

        self.assertEqual([component_info.fullName(index) for index in find_rectangular_detector_indices(component_info)], legacy_rect_names)
        self.assertEqual([component_info.fullName(index) for index in find_grid_detector_indices(component_info)], legacy_grid_names)

    def test_find_bank_indices_matches_legacy_on_sample_workspace(self):
        ws = CreateSampleWorkspace(NumBanks=3, BankPixelWidth=4, NumMonitors=2, OutputWorkspace="sample_banks")
        self.assertEqual(len(find_rectangular_detector_indices(ws.componentInfo())), 3)
        self._assert_same_banks_as_legacy(ws)

    def test_find_bank_indices_matches_legacy_on_sxd(self):
        ws = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="sxd_empty")
        self.assertGreater(len(find_rectangular_detector_indices(ws.componentInfo())), 0)
        self._assert_same_banks_as_legacy(ws)

    def test_find_bank_indices_empty_for_tube_instrument(self):
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        self.assertEqual(find_rectangular_detector_indices(ws.componentInfo()), [])
        self._assert_same_banks_as_legacy(ws)

    def test_get_assembly_children_and_detector_id(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=3, OutputWorkspace="sample_banks")
        component_info = ws.componentInfo()
        detector_info = ws.detectorInfo()
        bank_index = component_info.indexOfAny("bank1")

        columns = get_assembly_children(component_info, bank_index)
        self.assertEqual(columns, [int(child) for child in component_info.children(bank_index)])
        pixel_index = get_assembly_children(component_info, columns[0])[0]
        self.assertEqual(get_detector_id(component_info, detector_info, pixel_index), detector_info.detid(pixel_index))

        # a pixel is not an assembly, and a bank is not a detector
        self.assertRaises(RuntimeError, get_assembly_children, component_info, pixel_index)
        self.assertRaises(RuntimeError, get_detector_id, component_info, detector_info, bank_index)


if __name__ == "__main__":
    unittest.main()
