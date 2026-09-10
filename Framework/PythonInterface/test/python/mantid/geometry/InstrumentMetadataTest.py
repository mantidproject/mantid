# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import DateAndTime
from mantid.geometry import InstrumentMetadata
from testhelpers import can_be_instantiated, WorkspaceCreationHelper


class InstrumentMetadataTest(unittest.TestCase):
    __testws = None

    def setUp(self):
        if self.__testws is None:
            self.__class__.__testws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(1, 1)

    def test_InstrumentMetadata_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(InstrumentMetadata))

    def test_instrumentMetadata_returns_an_InstrumentMetadata(self):
        self.assertIsInstance(self.__testws.instrumentMetadata(), InstrumentMetadata)

    def test_filename(self):
        self.assertIsInstance(self.__testws.instrumentMetadata().filename(), str)

    def test_xmlText(self):
        xml_text = self.__testws.instrumentMetadata().xmlText()
        self.assertIsInstance(xml_text, str)
        self.assertEqual(xml_text, "Fake XML")

    def test_validDates(self):
        metadata = self.__testws.instrumentMetadata()
        self.assertIsInstance(metadata.validFromDate(), DateAndTime)
        self.assertIsInstance(metadata.validToDate(), DateAndTime)

    def test_defaultView_and_defaultAxis(self):
        metadata = self.__testws.instrumentMetadata()
        self.assertIsInstance(metadata.defaultView(), str)
        self.assertIsInstance(metadata.defaultAxis(), str)

    def test_metadata_matches_a_loaded_IDF(self):
        # A real IDF exercises values the fake instrument leaves empty.
        from mantid.simpleapi import DeleteWorkspace, LoadEmptyInstrument

        ws = LoadEmptyInstrument(InstrumentName="ARCS", OutputWorkspace="__InstrumentMetadataTest_ARCS")
        try:
            metadata = ws.instrumentMetadata()
            self.assertTrue(metadata.filename().endswith(".xml"))
            self.assertIn("ARCS", metadata.filename())
            self.assertEqual(metadata.defaultView(), "CYLINDRICAL_Y")
            self.assertGreater(len(metadata.xmlText()), 0)
            self.assertLess(metadata.validFromDate(), metadata.validToDate())
        finally:
            DeleteWorkspace(ws)


if __name__ == "__main__":
    unittest.main()
