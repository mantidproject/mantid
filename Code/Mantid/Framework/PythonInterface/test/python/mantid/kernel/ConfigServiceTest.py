import unittest
import os

from mantid.kernel import (ConfigService, ConfigServiceImpl, config,
                           std_vector_str, FacilityInfo, InstrumentInfo)

class ConfigServiceTest(unittest.TestCase):

    __dirs_to_rm = []
    __init_dir_list = ''

    def test_singleton_returns_instance_of_ConfigService(self):
        self.assertTrue(isinstance(config, ConfigServiceImpl))

    def test_getLocalFilename(self):
        local = config.getLocalFilename().lower()
        self.assertTrue('local' in local)

    def test_getUserFilename(self):
        user = config.getUserFilename().lower()
        self.assertTrue('user' in user)

    def test_getFacilityReturns_A_FacilityInfo_Object(self):
        facility = config.getFacility()
        self.assertTrue(isinstance(facility, FacilityInfo))

    def test_getFacility_With_Name_Returns_A_FacilityInfo_Object(self):
        facility = config.getFacility("ISIS")
        self.assertTrue(isinstance(facility, FacilityInfo))
        self.assertRaises(RuntimeError, config.getFacility, "MadeUpFacility")

    def test_getFacilities_Returns_A_FacilityInfo_List(self):
        facilities = config.getFacilities()
        self.assertTrue(isinstance(facilities[0], FacilityInfo))

    def test_getFacilities_and_Facility_Names_are_in_sync_and_non_empty(self):
        facilities = config.getFacilities()
        names = config.getFacilityNames()

        self.assertTrue(len(names)>0)
        self.assertEquals(len(names),len(facilities))
        for i in range(len(names)):
            self.assertEquals(names[i],facilities[i].name())

    def test_update_and_set_facility(self):
        self.assertFalse("TEST" in config.getFacilityNames())
        ConfigService.updateFacilities(os.path.join(ConfigService.getInstrumentDirectory(),"IDFs_for_UNIT_TESTING/UnitTestFacilities.xml"))
        ConfigService.setFacility("TEST")
        self.assertEquals(config.getFacility().name(), "TEST")
        self.assertRaises(RuntimeError, config.getFacility, "SNS")

    def test_getInstrumentReturns_A_InstrumentInfo_Object(self):
        self.assertTrue(isinstance(config.getInstrument("WISH"), InstrumentInfo))
        self.assertRaises(RuntimeError, config.getInstrument, "MadeUpInstrument")

    def test_service_acts_like_dictionary(self):
        test_prop = "algorithms.retained"
        self.assertTrue(config.hasProperty(test_prop))
        dictcall = config[test_prop]
        fncall = config.getString(test_prop)
        self.assertEquals(dictcall, fncall)
        self.assertNotEqual(config[test_prop], "")

        old_value = fncall
        config.setString(test_prop, "1")
        self.assertEquals(config.getString(test_prop), "1")
        config[test_prop] =  "2"
        self.assertEquals(config.getString(test_prop), "2")

        config.setString(test_prop, old_value)

    def test_getting_search_paths(self):
        """Retrieve the search paths
        """
        paths = config.getDataSearchDirs()
        self.assertEquals(type(paths), std_vector_str)
        self.assert_(len(paths) > 0)

    def test_setting_paths_via_single_string(self):
        new_path_list = self._setup_test_areas()
        path_str = ';'.join(new_path_list)
        config.setDataSearchDirs(path_str)
        paths = config.getDataSearchDirs()
        # Clean up here do that if the assert fails
        # it doesn't bring all the other tests down
        self._clean_up_test_areas()

        self.assertTrue(len(paths), 2)
        self.assertTrue('tmp' in paths[0])
        self.assertTrue('tmp_2' in paths[1])
        self._clean_up_test_areas()


    def _setup_test_areas(self):
        """Create a new data search path string
        """
        self.__init_dir_list = config['datasearch.directories']
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

        return [test_path, test_path_two]

    def _clean_up_test_areas(self):
        config['datasearch.directories'] = self.__init_dir_list

        # Remove temp directories
        for p in self.__dirs_to_rm:
            try:
                os.rmdir(p)
            except OSError:
                pass

if __name__ == '__main__':
    unittest.main()
