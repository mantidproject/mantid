import unittest
from mantid.simpleapi import *
import numpy as np

try:
    import h5py
except ImportError:
    logger.warning("Failure of IOmodule test because h5py is unavailable.")
    exit(1)

from AbinsModules import IOmodule

class ABINSIOmoduleTest(unittest.TestCase):

    def _save_stuff(self):
        saver = IOmodule(input_filename="Cars.foo", group_name="Volksvagen")

        # add some attributes
        saver.addAttribute("Fuel", 100)
        saver.addAttribute("Speed", 200)


        # add some datasets
        saver.addData("Passengers", np.array([4]))
        saver.addData("FireExtinguishers", np.array([2]))

        # add some mode complex data sets
        wheels = [{"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":2},
                {"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":3},
                {"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":5},
                {"Winter":False, "Punctured":True, "Brand":"Mercedes", "Age":7}]
        chairs = {"AdjustableHeadrests":True, "ExtraPadding":True}

        saver.addData("wheels", wheels)
        saver.addData("chairs", chairs)


        # save attributes and datasets
        saver.save()


    def _save_wrong_attribute(self):
        _poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        _poor_saver.addAttribute("BadPassengers", np.array([4]))
        with self.assertRaises(ValueError):
            _poor_saver.save()


    def _save_wrong_dataset(self):
        _poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        _poor_saver.addData("BadPassengers", 4)
        with self.assertRaises(ValueError):
            _poor_saver.save()


    def _wrong_filename(self):
        with self.assertRaises(ValueError):
            poor_loader = IOmodule(input_filename=1, group_name="goodgroup")


    def _wrong_groupname(self):
        with self.assertRaises(ValueError):
            poor_loader = IOmodule(input_filename="goodfile", group_name=1)


    def _wrong_file(self):
        poor_loader = IOmodule(input_filename="bum", group_name="nice_group")
        with self.assertRaises(IOError):
           poor_loader.load(list_of_attributes = "one_attribute")


    def _loading_attributes(self):
        data = self.loader.load(list_of_attributes = ["Fuel", "Speed"])
        attr_data = data["attributes"]

        self.assertEqual(100, attr_data["Fuel"])
        self.assertEqual(200, attr_data["Speed"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes = ["NiceFuel"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes=1)

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes = [1,"Speed"])


    def _loading_datasets(self):
        data = self.loader.load(list_of_datasets=["Passengers", "FireExtinguishers"])
 
        self.assertEqual(np.array([4]), data["datasets"]["Passengers"])
        self.assertEqual(np.array([2]), data["datasets"]["FireExtinguishers"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_datasets=["NicePassengers"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_datasets=1)

        with self.assertRaises(ValueError):
            self.loader.load(list_of_datasets=[1, "Passengers"])


    def _loading_structured_datasets(self):
        """
        Loads more complicated data from the hdf file.
        """

        data = self.loader.load(list_of_datasets=["wheels", "chairs"])

        self.assertEqual([{"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":2},
                          {"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":3},
                          {"Winter":False, "Punctured":False, "Brand":"Mercedes", "Age":5},
                          {"Winter":False, "Punctured":True, "Brand":"Mercedes", "Age":7}],
                         data["datasets"]["wheels"])

        self.assertEqual({"AdjustableHeadrests":True, "ExtraPadding":True},
                         data["datasets"]["chairs"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_datasets=["WrongDataSet"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_datasets=1)


    def runTest(self):

        self._save_stuff()

        self._save_wrong_attribute()
        self._save_wrong_dataset()

        self.loader = IOmodule(input_filename="Cars.foo", group_name="Volksvagen")

        self._wrong_filename()
        self._wrong_groupname()
        self._wrong_file()

        self._loading_attributes()
        self._loading_datasets()
        self._loading_structured_datasets()

if __name__ == '__main__':
    unittest.main()
