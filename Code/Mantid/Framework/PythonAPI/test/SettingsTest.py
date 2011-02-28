import unittest
import os

from MantidFramework import *
mtd.initialise()

class SettingsTest(unittest.TestCase):

    __dirs_to_rm = []
    __init_dir_list = ''

    def test_welcome(self):
        self.assertEquals(mtd.settings.welcomeMessage(), 
                          'Welcome to Mantid - Manipulation and Analysis Toolkit for Instrument Data')

    def test_getting_search_paths(self):
        """Retrieve the search paths
        """
        paths = mtd.settings.getDataSearchDirs()
        self.assertEquals(type(paths), list)
        self.assert_(len(paths) > 0)

    def test_setting_data_search_paths_via_string(self):
        """Set data search paths via a string
        """
        updated = self._setup_test_areas()
        mtd.settings.setDataSearchDirs(updated)
        # Have they been updated - The stored values come back with trailing slashes
        self.assertEquals(mtd.settings['datasearch.directories'], updated)

        self._clean_up_test_areas()

    def test_setting_data_search_paths_via_string(self):
        """Set data search paths via a string
        """
        updated = self._setup_test_areas()
        updated_list = updated.split(';')

        self.assertEquals(len(updated_list), 2)
        self.assertEquals(type(updated_list), list)
        mtd.settings.setDataSearchDirs(updated_list)

        # Have they been updated - The stored values come back with trailing slashes
        self.assertEquals(mtd.settings['datasearch.directories'], updated)

        self._clean_up_test_areas()

    def _setup_test_areas(self):
        """Set data search paths via a list
        """
        self.__init_dir_list = mtd.settings['datasearch.directories']
        # Set new paths - Make a temporary directory so that I know where it is
        test_path = os.path.join(os.getcwd(), "tmp")
        try:
            os.mkdir(test_path)
            self.__dirs_to_rm.append(test_path)
        except OSError:
            pass

        test_path_two = os.path.join(os.getcwd(), "tmp_2")
        try:
            os.mkdir(test_path_two)
            self.__dirs_to_rm.append(test_path_two)
        except OSError:
            pass
        
        updated = test_path + '/;' + test_path_two + '/'
        return updated

    def _clean_up_test_areas(self):
        mtd.settings['datasearch.directories'] = self.__init_dir_list
        
        # Remove temp directories
        for p in self.__dirs_to_rm:
            try:
                os.rmdir(p)
            except OSError:
                pass
# -----------------------------------

if __name__ == '__main__':
    unittest.main()
