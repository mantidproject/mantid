# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import unittest
from mantid.simpleapi import LoadEmptyInstrument

from plugins.algorithms.component_info_utils import resolve_component_index


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


if __name__ == "__main__":
    unittest.main()
