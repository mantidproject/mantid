# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FrameworkManagerImpl, InstrumentDataService, InstrumentDataServiceImpl
from mantid.geometry import Instrument
from mantid.simpleapi import LoadEmptyInstrument


class InstrumentDataServiceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManagerImpl.Instance()

    def tearDown(self):
        InstrumentDataService.clear()

    def test_alias_is_instance_type(self):
        self.assertTrue(isinstance(InstrumentDataService, InstrumentDataServiceImpl))

    def test_only_limited_interface_is_exposed(self):
        self.assertTrue(hasattr(InstrumentDataService, "doesExist"))
        self.assertTrue(hasattr(InstrumentDataService, "retrieve"))
        self.assertTrue(hasattr(InstrumentDataService, "remove"))
        self.assertTrue(hasattr(InstrumentDataService, "clear"))
        self.assertTrue(hasattr(InstrumentDataService, "size"))
        self.assertTrue(hasattr(InstrumentDataService, "getObjectNames"))

        self.assertFalse(hasattr(InstrumentDataService, "add"))
        self.assertFalse(hasattr(InstrumentDataService, "addOrReplace"))
        self.assertFalse(hasattr(InstrumentDataService, "__getitem__"))
        self.assertFalse(hasattr(InstrumentDataService, "__setitem__"))

    def test_empty_service_methods(self):
        self.assertEqual(InstrumentDataService.getObjectNames(), [])
        self.assertFalse(InstrumentDataService.doesExist("does_not_exist"))
        InstrumentDataService.remove("does_not_exist")

    def test_add_retrieve_remove(self):
        # Check that the service is empty to start with.
        self.assertEqual(InstrumentDataService.size(), 0)

        # Add an instrument to the service and check it is there.
        pg3 = LoadEmptyInstrument(InstrumentName="PG3")
        self.assertEqual(InstrumentDataService.size(), 1)
        self.assertEqual(len(InstrumentDataService.getObjectNames()), 1)

        # Check contains work for getObjectNames
        self.assertEqual(len(InstrumentDataService.getObjectNames("TOPAZ")), 0)
        self.assertEqual(len(InstrumentDataService.getObjectNames("POWGEN")), 1)

        # Check the retrieved instrument
        instrument_cache_name = InstrumentDataService.getObjectNames()[0]
        self.assertTrue(instrument_cache_name.startswith("POWGEN"))
        self.assertTrue(InstrumentDataService.doesExist(instrument_cache_name))

        # Retrieve the instrument and check it is the same one.
        retrieved_pg3 = InstrumentDataService.retrieve(instrument_cache_name)
        self.assertIsInstance(retrieved_pg3, Instrument)
        self.assertEqual(retrieved_pg3.getName(), "POWGEN")

        # Check that the object is removed and the service is empty again.
        InstrumentDataService.remove(instrument_cache_name)
        self.assertEqual(InstrumentDataService.size(), 0)
        pg3.delete()

    def test_retrieve_missing_raises_keyerror(self):
        with self.assertRaises(KeyError):
            InstrumentDataService.retrieve("does_not_exist")


if __name__ == "__main__":
    unittest.main()
