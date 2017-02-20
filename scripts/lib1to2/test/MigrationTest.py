"""Base class for all Migration tests. Includes common functionality for preparing/removing the files
"""
import unittest
import os

from lib1to2 import migrate

class MigrationTest(unittest.TestCase):

    test_filename = None
    test_backupname = None
    backup_ext = '.mantidbackup'

    def create_test_file(self, contents):
        """Writes a test file out to a temporary location"""
        self.test_filename = 'MigrationTest_SimpleAPIFunctionCallReplace.py'
        self.test_backupname = self.test_filename + self.backup_ext
        _temp_file = file(self.test_filename, 'w')
        _temp_file.write(contents)
        _temp_file.close()

    def remove_test_files(self):
        """Remove the test file"""
        try:
            os.remove(self.test_filename)
            os.remove(self.test_backupname)
        except OSError:
            pass
        self.test_filename = None
        self.test_backupname = None

    def do_migration(self, input_contents):
        """Run the migration process for the input string"""
        self.create_test_file(input_contents)
        retcode = migrate.run(self.test_filename)
        self.assertEquals(retcode, 0)

    def check_outcome(self, original_input, expected_output):
        """Checks the outcome of a migration"""
        self.assertTrue(os.path.exists(self.test_backupname))
        self.compare_file_contents(self.test_backupname, original_input)
        self.compare_file_contents(self.test_filename, expected_output)

    def compare_file_contents(self, filename, expected_contents):
        """Compare the file contents with the string"""
        migrated_file = file(filename, 'r')
        migrated_string = migrated_file.read()
        migrated_file.close()
        self.assertEquals(migrated_string, expected_contents)
