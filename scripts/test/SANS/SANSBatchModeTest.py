# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from ISISCommandInterface import *
from isis_reduction_steps import UserFile
import SANSBatchMode as bm


class TestSettingUserFileInBatchMode(unittest.TestCase):
    def _create_minimal_user_files(self, amount):
        temp_save_dir = config["defaultsave.directory"]
        if temp_save_dir == "":
            temp_save_dir = os.getcwd()
        base_name = "batch_test_file_"
        file_names = []
        minimal_user_file = "LOQ\n" "L/QXY 0 0.2 0.0125/lin\n"
        for element in range(0, amount):
            file_name = os.path.join(temp_save_dir, base_name + str(element) + ".txt")
            file_names.append(file_name)
            with open(file_name, "w") as ff:
                ff.write(minimal_user_file)
        return file_names

    def _delete_minimal_user_files(self, file_names):
        for file_name in file_names:
            if os.path.exists(file_name):
                os.remove(file_name)

    def _prepare_reducer(self, current_user_file, original_user_file):
        # Set up original reducer in order to store a copy
        Clean()
        LOQ()
        ReductionSingleton().user_settings = UserFile(original_user_file)
        ReductionSingleton().user_settings.execute(ReductionSingleton())
        original_settings = copy.deepcopy(ReductionSingleton().reference())
        original_prop_man_settings = ReductionSingleton().settings.clone("TEMP_SETTINGS")

        # Set up current reducer
        Clean()
        LOQ()
        ReductionSingleton().user_settings = UserFile(current_user_file)
        ReductionSingleton().user_settings.execute(ReductionSingleton())
        return original_settings, original_prop_man_settings

    def test_user_file_is_set_to_original_when_new_user_file_does_not_exist(self):
        # Arrange
        user_files = self._create_minimal_user_files(3)
        new_user_file = ""  # Create an invalid new user file
        current_user_file = user_files[1]
        original_user_file = user_files[2]
        original_settings, original_prop_man_settings = self._prepare_reducer(
            current_user_file=current_user_file, original_user_file=original_user_file
        )
        # Act
        reducer_address = str(ReductionSingleton().reference())
        bm.setUserFileInBatchMode(
            new_user_file=new_user_file,
            current_user_file=current_user_file,
            original_user_file=original_user_file,
            original_settings=original_settings,
            original_prop_man_settings=original_prop_man_settings,
        )
        # Assert
        self.assertTrue(str(ReductionSingleton().reference()) != reducer_address, ("The physical reducer should change."))
        self.assertEqual(
            ReductionSingleton().user_settings.filename,
            original_user_file,
            ("The reducer should use the original user file," "since we don't provide a valid new user file."),
        )
        # Clean up
        self._delete_minimal_user_files(user_files)

    def test_user_file_is_set_to_new_user_file_when_it_exists_and_is_different_from_orig_and_current(self):
        # Arrange
        user_files = self._create_minimal_user_files(3)
        new_user_file = user_files[0]
        current_user_file = user_files[1]
        original_user_file = user_files[2]
        original_settings, original_prop_man_settings = self._prepare_reducer(
            current_user_file=current_user_file, original_user_file=original_user_file
        )
        # Act
        reducer_address = str(ReductionSingleton().reference())
        bm.setUserFileInBatchMode(
            new_user_file=new_user_file,
            current_user_file=current_user_file,
            original_user_file=original_user_file,
            original_settings=original_settings,
            original_prop_man_settings=original_prop_man_settings,
        )
        # Assert
        self.assertTrue(str(ReductionSingleton().reference()) != reducer_address, "The physical reducer should not change.")
        self.assertEqual(ReductionSingleton().user_settings.filename, new_user_file, "The reducer should use the new user file.")
        # Clean up
        self._delete_minimal_user_files(user_files)

    def test_user_file_is_set_to_new_user_file_when_it_exists_but_reduction_dimensionality_remains_unchanged(self):
        # Arrange
        user_files = self._create_minimal_user_files(3)
        new_user_file = user_files[0]
        current_user_file = user_files[1]
        original_user_file = user_files[2]
        original_settings, original_prop_man_settings = self._prepare_reducer(
            current_user_file=current_user_file, original_user_file=original_user_file
        )
        ReductionSingleton().to_Q.output_type = "2D"
        # Act

        bm.setUserFileInBatchMode(
            new_user_file=new_user_file,
            current_user_file=current_user_file,
            original_user_file=original_user_file,
            original_settings=original_settings,
            original_prop_man_settings=original_prop_man_settings,
        )
        # Assert
        self.assertTrue(ReductionSingleton().to_Q.output_type == "2D", "The reducer should retain the same dimensionality.")
        # Clean up
        self._delete_minimal_user_files(user_files)

    def test_reducer_is_not_reset_when_new_file_is_the_same_as_the_current_file(self):
        # Arrange
        user_files = self._create_minimal_user_files(3)
        new_user_file = user_files[1]
        current_user_file = user_files[1]
        original_user_file = user_files[2]
        original_settings, original_prop_man_settings = self._prepare_reducer(
            current_user_file=current_user_file, original_user_file=original_user_file
        )
        # Act
        reducer_address = str(ReductionSingleton().reference())
        bm.setUserFileInBatchMode(
            new_user_file=new_user_file,
            current_user_file=current_user_file,
            original_user_file=original_user_file,
            original_settings=original_settings,
            original_prop_man_settings=original_prop_man_settings,
        )
        # Assert
        self.assertTrue(str(ReductionSingleton().reference()) == reducer_address, "The physical reducer should change.")
        self.assertEqual(ReductionSingleton().user_settings.filename, current_user_file, "The reducer should use the current user file.")
        # Clean up
        self._delete_minimal_user_files(user_files)

    def test_reducer_is_not_reset_if_file_to_set_is_original_and_current(self):
        # Arrange
        user_files = self._create_minimal_user_files(3)
        new_user_file = user_files[1]
        current_user_file = user_files[1]
        original_user_file = user_files[1]
        original_settings, original_prop_man_settings = self._prepare_reducer(
            current_user_file=current_user_file, original_user_file=original_user_file
        )
        # Act
        reducer_address = str(ReductionSingleton().reference())
        bm.setUserFileInBatchMode(
            new_user_file=new_user_file,
            current_user_file=current_user_file,
            original_user_file=original_user_file,
            original_settings=original_settings,
            original_prop_man_settings=original_prop_man_settings,
        )
        # Assert
        self.assertTrue(str(ReductionSingleton().reference()) == reducer_address, "The physical reducer should not change.")
        self.assertEqual(ReductionSingleton().user_settings.filename, current_user_file, "The reducer should use the current user file.")
        # Clean up
        self._delete_minimal_user_files(user_files)


class TestGeometrySettings(unittest.TestCase):
    def test_that_can_get_geometry_properties(self):
        LOQ()
        reducer = ReductionSingleton()
        geometry_settings = bm.get_geometry_properties(reducer)
        self.assertTrue("Geometry" in geometry_settings)
        self.assertTrue("SampleHeight" in geometry_settings)
        self.assertTrue("SampleWidth" in geometry_settings)
        self.assertTrue("SampleThickness" in geometry_settings)

        self.assertEqual(geometry_settings["Geometry"], "Disc")
        self.assertEqual(geometry_settings["SampleHeight"], 1.0)
        self.assertEqual(geometry_settings["SampleWidth"], 1.0)
        self.assertEqual(geometry_settings["SampleThickness"], 1.0)


if __name__ == "__main__":
    unittest.main()
