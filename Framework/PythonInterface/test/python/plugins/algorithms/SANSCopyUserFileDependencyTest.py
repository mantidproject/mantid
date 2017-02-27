import unittest
from mantid.simpleapi import *
from mantid.api import AlgorithmManager, MatrixWorkspace
from SANSCopyUserFileDependency import UserFileDependencyExtractor
import shutil

class SANSCopyUserFileDependency(unittest.TestCase):
    def _save_set_of_files(self, name, save_only_some_dependencies = False):
        text_in_file = ("TUBECALIBFILE=A.nxs\n"
                        "MON/DIRECT=B.txt\n"
                        "! use for front, this command has to come second!\n"
                        "MON/TRANS/SPECTRUM=2\n"
                        "TRANS/TRANSPEC=4/SHIFT=-55\n"
                        "MASKFILE=C.xml,D.xml\n"
                        "!TRANS/TRANSPEC=3\n"
                        "MON/SPECTRUM=2\n")

        user_file = self._create_file_path(name)
        with open(user_file, 'w') as f:
            f.write(text_in_file)

        if save_only_some_dependencies:
            expected_hits = ["A.nxs",
                             "B.txt",
                             "D.xml"]
        else:
            expected_hits = ["A.nxs",
                             "B.txt",
                             "C.xml",
                             "D.xml"]

        dependencies = []
        # Also save the dependencies
        for expected_hit in expected_hits:
            dependencies.append(self._create_file_path(expected_hit))

        for dependency in dependencies:
            with open(dependency, 'w') as f:
                f.write("test")
        to_remove = dependencies
        to_remove.append(user_file)
        return set(expected_hits), to_remove

    def _create_file_path(self, name):
        temp_save_dir = config['defaultsave.directory']

        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()
        full_path = os.path.join(temp_save_dir, name)
        return full_path

    def _create_target_directory(self, name):
        temp_save_dir = config['defaultsave.directory']
        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()
        directory = os.path.join(temp_save_dir, name)
        if not os.path.isdir(directory):
            os.makedirs(directory)
        return directory

    def _remove_files(self, file_paths):
        for file_path in file_paths:
            if os.path.exists(file_path):
                os.remove(file_path)

    def _remove_target_directory(self, name):
        if os.path.isdir(name):
            shutil.rmtree(name)

    def test_that_files_are_copied(self):
        # Arrange
        user_file_name = "user_file_test_for_copy.txt"
        expected, to_remove = self._save_set_of_files(user_file_name)
        target_directory = self._create_target_directory("copy_user_file_sans_unit_test")

        # Act
        copier = AlgorithmManager.createUnmanaged("SANSCopyUserFileDependency")
        copier.initialize()
        copier.setChild(True)
        copier.setProperty("Filename", user_file_name)
        copier.setProperty("OutputDirectory", target_directory)
        copier.execute()

        # Assert
        expected_files = [os.path.join(target_directory, file_name) for file_name in expected]
        for expected_file in expected_files:
            self.assertTrue(os.path.exists(expected_file), "New file in new folder should exist")

        # Clean up
        self._remove_files(to_remove)
        self._remove_target_directory(target_directory)

    def test_that_raises_when_user_file_cannot_be_found(self):
        #Arrange
        user_file_name = "does_not_exist.txt"
        target_directory = "copy_user_file_sans_unit_test"
        #import time
        #time.sleep(10)
        copier = AlgorithmManager.createUnmanaged("SANSCopyUserFileDependency")
        copier.initialize()
        copier.setChild(True)

        # Act + Assert
        msg = ""
        raises_exception = False
        try:
            copier.setProperty("Filename", user_file_name)
        except ValueError:
            msg = "Should raise because there is no valid file name specified"
            raises_exception = True
        self.assertTrue(msg)
        self.assertTrue(raises_exception)

    def test_that_raises_when_a_dependency_cannot_be_found(self):
        #Arrange
        user_file_name = "user_file_test_for_copy.txt"
        save_only_some_dependencies = True
        expected, to_remove = self._save_set_of_files(user_file_name, save_only_some_dependencies)
        target_directory = self._create_target_directory("copy_user_file_sans_unit_test")
        missing_depedency = "C.xml"

        #Act
        copier = AlgorithmManager.createUnmanaged("SANSCopyUserFileDependency")
        copier.initialize()
        copier.setChild(True)
        copier.setProperty("Filename", user_file_name)
        copier.setProperty("OutputDirectory", target_directory)

        raises_exception = False
        error_message = ""
        try:
            copier.execute()
        except RuntimeError as e:
            raises_exception = True
            error_message = str(e)

        # Assert
        self.assertTrue(raises_exception)
        self.assertTrue(missing_depedency in error_message)

        # Cleanup
        self._remove_files(to_remove)
        self._remove_target_directory(target_directory)

    def test_that_raises_when_output_directory_does_not_exist(self):
        #Arrange
        user_file_name = "user_file_test_for_copy.txt"
        expected, to_remove = self._save_set_of_files(user_file_name)
        target_directory = "copy_user_file_sans_unit_test"

        #Act
        copier = AlgorithmManager.createUnmanaged("SANSCopyUserFileDependency")
        copier.initialize()
        copier.setChild(True)
        copier.setProperty("Filename", user_file_name)

        # Act + Assert
        msg = ""
        raises_exception = False
        try:
            copier.setProperty("OutputDirectory", target_directory)
        except ValueError:
            msg = "Should raise because there is no valid file name specified"
            raises_exception = True
        self.assertTrue(msg)
        self.assertTrue(raises_exception)

        # Cleanup
        self._remove_files(to_remove)


class UserFileDependencyExtractorTest(unittest.TestCase):
    def _save_user_file(self, file_path):
        text_in_file = ("TUBECALIBFILE=TUBE_SANS2D_BOTH_31681_25Sept15.nxs\n"
                        "MON/DIRECT=DIRECT_SANS2D_REAR_33117_4m_14mm_15Nov15.txt\n"
                        "! use for front, this command has to come second!\n"
                        "MON/DIRECT/FRONT=DIRECT_SANS2D_REAR_33117_4m_14mm_15Nov15.txt\n"
                        "MON/TRANS/SPECTRUM=2\n"
                        "TRANS/TRANSPEC=4/SHIFT=-55\n"
                        "MASKFILE=MASK_SANS2D_REAR_Edges_16Mar2015.xml,MASK_SANS2D_FRONT_Edges_16Mar2015.xml\n"
                        "!TRANS/TRANSPEC=3\n"
                        "MON/SPECTRUM=2\n"
                        "FIT/TRANS/LOG 1.75 16.5\n"
                        "!PRINT FIT/TRANS/OFF\n"
                        "!FIT/TRANS/OFF\n")
        expected_hits = ["TUBE_SANS2D_BOTH_31681_25Sept15.nxs",
                         "DIRECT_SANS2D_REAR_33117_4m_14mm_15Nov15.txt",
                         "MASK_SANS2D_REAR_Edges_16Mar2015.xml",
                         "MASK_SANS2D_FRONT_Edges_16Mar2015.xml"]

        with open(file_path, 'w') as f:
            f.write(text_in_file)

        return set(expected_hits)

    def test_that_find_all_files_in_user_file(self):
        # Arrange
        temp_save_dir = config['defaultsave.directory']
        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()
        full_path = os.path.join(temp_save_dir, "dummy_user_file.txt")
        expected = self._save_user_file(full_path)

        # Act
        extractor = UserFileDependencyExtractor()
        dependencies = extractor.get_all_dependencies(full_path)

        # Assert
        # Make sure that all dependencies are found
        for dependency in dependencies:
            self.assertTrue(dependency in expected)

        # Make sure that no duplicates are found
        self.assertTrue(len(set(dependencies)) == len(expected))

        if os.path.exists(full_path):
            os.remove(full_path)

if __name__ == '__main__':
    unittest.main()
