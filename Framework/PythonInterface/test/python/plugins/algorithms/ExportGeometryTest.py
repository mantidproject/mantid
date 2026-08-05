# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
from mantid.simpleapi import ExportGeometry, LoadEmptyInstrument
from mantid.api import AlgorithmFactory


class ExportGeometryTest(unittest.TestCase):
    def test_export_nested_path_component(self):
        r"""
        'bank7/sixteenpack' is a '/'-qualified path: 'bank7' is a pure positioning frame
        (placed at its parent's location) while 'sixteenpack', nested inside it, carries
        the real detector-bank position. ExportGeometry must resolve to the leaf, not the
        frame, and must report the source and sample positions correctly.
        """
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        component_info = ws.componentInfo()

        with tempfile.TemporaryDirectory() as tmp_dir:
            filename = os.path.join(tmp_dir, "exported.xml")
            ExportGeometry(InputWorkspace=ws, Components="bank7/sixteenpack", Filename=filename, EulerConvention="YZX")
            with open(filename) as handle:
                content = handle.read()

        # source and sample positions
        source_pos = component_info.position(component_info.source())
        sample_pos = component_info.position(component_info.sample())
        self.assertIn('<location z="%f"/>' % source_pos.Z(), content)
        self.assertIn('<location x="%f" y="%f" z="%f"/>' % (sample_pos.X(), sample_pos.Y(), sample_pos.Z()), content)

        # the leaf component, not the 'bank7' positioning frame
        bank_index = component_info.indexOfAny("bank7")
        leaf_index = next(
            int(c)
            for c in component_info.componentsInSubtree(bank_index)
            if int(c) != bank_index and component_info.name(int(c)) == "sixteenpack"
        )
        leaf_pos = component_info.position(leaf_index)
        self.assertNotEqual(leaf_pos, component_info.position(bank_index))
        self.assertIn('name="sixteenpack"', content)
        self.assertIn('x="%f" y="%f" z="%f" name="sixteenpack"' % (leaf_pos.X(), leaf_pos.Y(), leaf_pos.Z()), content)

    def test_missing_component_raises(self):
        ws = LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace="corelli_empty")
        with tempfile.TemporaryDirectory() as tmp_dir:
            filename = os.path.join(tmp_dir, "exported.xml")
            self.assertRaisesRegex(
                RuntimeError,
                "no component",
                ExportGeometry,
                InputWorkspace=ws,
                Components="not_a_real_component",
                Filename=filename,
                EulerConvention="YZX",
            )


if __name__ == "__main__":
    # Only test is Algorithm is loaded
    if AlgorithmFactory.exists("ExportGeometry"):
        unittest.main()
