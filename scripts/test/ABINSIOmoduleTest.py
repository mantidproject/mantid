import unittest
from mantid.simpleapi import *
import numpy as np
from AbinsModules import IOmodule, AbinsConstants


def old_modules():
    """" Check if Python and numpy  has proper version."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSIOmoduleTest because Python is too old.")

    is_numpy_old = AbinsConstants.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping ABINSIOmoduleTest because numpy is too old.")

    return is_python_old or is_numpy_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """

    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls

    return decorate


@skip_if(old_modules)
class ABINSIOmoduleTest(unittest.TestCase):

    # noinspection PyMethodMayBeStatic
    def _save_stuff(self):
        saver = IOmodule(input_filename="Cars.foo", group_name="Volksvagen")

        # add some attributes
        saver.add_attribute("Fuel", 100)
        saver.add_attribute("Speed", 200)

        # add some datasets
        saver.add_data("Passengers", np.array([4]))
        saver.add_data("FireExtinguishers", np.array([2]))

        # add some mode complex data sets
        wheels = [{"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  2},
                  {"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  3},
                  {"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  5},
                  {"Winter":   False, "Punctured": True,  "Brand": "Mercedes", "Age":  7}]
        chairs = {"AdjustableHeadrests": True, "ExtraPadding": True}

        saver.add_data("wheels", wheels)
        saver.add_data("chairs", chairs)

        # save attributes and datasets
        saver.save()

    def _save_wrong_attribute(self):
        _poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        _poor_saver.add_attribute("BadPassengers", np.array([4]))
        with self.assertRaises(ValueError):
            _poor_saver.save()

    def _save_wrong_dataset(self):
        _poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        _poor_saver.add_data("BadPassengers", 4)
        with self.assertRaises(ValueError):
            _poor_saver.save()

    def _wrong_filename(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_loader = IOmodule(input_filename=1, group_name="goodgroup")

    def _wrong_groupname(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_loader = IOmodule(input_filename="goodfile", group_name=1)

    def _wrong_file(self):
        poor_loader = IOmodule(input_filename="bum", group_name="nice_group")
        with self.assertRaises(IOError):
            poor_loader.load(list_of_attributes="one_attribute")

    def _loading_attributes(self):
        data = self.loader.load(list_of_attributes=["Fuel", "Speed"])
        attr_data = data["attributes"]

        self.assertEqual(100, attr_data["Fuel"])
        self.assertEqual(200, attr_data["Speed"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes=["NiceFuel"])

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes=1)

        with self.assertRaises(ValueError):
            self.loader.load(list_of_attributes=[1, "Speed"])

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

        self.assertEqual([{"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 2},
                          {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 3},
                          {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 5},
                          {"Winter": False, "Punctured": True,  "Brand": "Mercedes", "Age":  7}],
                         data["datasets"]["wheels"])

        self.assertEqual({"AdjustableHeadrests": True, "ExtraPadding": True},
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
