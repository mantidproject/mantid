# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function, unicode_literals)

import inspect
import os
import testhelpers
import unittest

from mantid.kernel import (ConfigService, ConfigServiceImpl, config,
                           std_vector_str, FacilityInfo, InstrumentInfo)
import six

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
        ConfigService.updateFacilities(os.path.join(ConfigService.getInstrumentDirectory(),"unit_testing/UnitTestFacilities.xml"))
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

    def test_setting_paths(self):
        def do_test(paths):
            config.setDataSearchDirs(paths)
            newpaths = config.getDataSearchDirs()
            # Clean up here do that if the assert fails
            # it doesn't bring all the other tests down
            self._clean_up_test_areas()
            self.assertEqual(len(newpaths), 2)
            self.assertTrue('tmp' in newpaths[0])
            self.assertTrue('tmp_2' in newpaths[1])

        new_path_list = self._setup_test_areas()
        # test with list
        do_test(new_path_list)

        # reset test areas or the directories don't exist to be added
        new_path_list = self._setup_test_areas()
        # test with single string
        do_test(';'.join(new_path_list))


    def test_appending_paths(self):
        new_path_list = self._setup_test_areas()
        try:
            config.appendDataSearchDir(six.text_type(new_path_list[0]))
            updated_paths = config.getDataSearchDirs()
        finally:
            self._clean_up_test_areas()

        self.assertEqual(4, len(updated_paths))

    def test_setting_log_channel_levels(self):
        testhelpers.assertRaisesNothing(self, config.setLogLevel, 4, True)

    def test_properties_documented(self):
        # location of the rst file relative to this file this will break if either moves
        doc_filename = os.path.split(inspect.getfile(self.__class__))[0]
        doc_filename = os.path.join(doc_filename, '../../../../../../docs/source/concepts/PropertiesFile.rst')
        doc_filename = os.path.abspath(doc_filename)

        # read in the user documentation
        print ('Parsing', doc_filename)
        documented_keys = []
        with open(doc_filename) as handle:
            text = handle.read()

        # these will be ignored - the list should get shorter over time
        hidden_prefixes = ['CheckMantidVersion.DownloadURL',  # shouldn't be changed by users
                           'CheckMantidVersion.GitHubReleaseURL', # shouldn't be changed by users
                           'UpdateInstrumentDefinitions.URL', # shouldn't be changed by users
                           'docs.html.root', # shouldn't be changed by users
                           'errorreports.rooturl', # shouldn't be changed by users
                           'usagereports.rooturl', # shouldn't be changed by users
                           'workspace.sendto.SansView.arguments', 'workspace.sendto.SansView.saveusing', # related to SASview in menu
                           'workspace.sendto.SansView.target', 'workspace.sendto.SansView.visible', # related to SASview in menu
                           'workspace.sendto.name.SansView', # related to SASview in menu
                           'catalog.oncat.token.accessToken', 'catalog.oncat.token.expiresIn', 'catalog.oncat.token.refreshToken', 'catalog.oncat.token.scope', 'catalog.oncat.token.tokenType', # Shouldn't be changed by users.

                           ########## TODO should be documented!
                           'filefinder.casesensitive',
                           'graph1d.autodistribution',
                           'groupingFiles.directory',
                           'icatDownload.directory', 'icatDownload.mountPoint',
                           'instrument.view.geometry',
                           'interfaces.categories.hidden',
                           'loading.multifile', 'loading.multifilelimit',
                           'maskFiles.directory',
                           'pythonalgorithms.refresh.allowed',
                           'sliceviewer.nonorthogonal',
                           'usersearch.directories',

                           ########## TODO should these be documented?
                           'curvefitting.defaultPeak', 'curvefitting.findPeaksFWHM', 'curvefitting.findPeaksTolerance', 'curvefitting.guiExclude',
                           'logging.channels.consoleChannel.class', 'logging.channels.consoleChannel.formatter', 'logging.formatters.f1.class', 'logging.formatters.f1.pattern', 'logging.formatters.f1.times', 'logging.loggers.root.channel.channel1', 'logging.loggers.root.channel.class',
                           'MantidOptions.ReusePlotInstances',
                           'mantidqt.python_interfaces', 'mantidqt.python_interfaces_directory',
                           'paraview.ignore', 'paraview.path', 'paraview.pythonpaths', 'pvplugins.directory',
                           'python.plugins.directories',
                       ]

        # create the list of things
        undocumented = []
        properties_defined = ConfigService.keys()
        for property in properties_defined:
            property_tag = '``{}``'.format(property)

            if property_tag not in text:
                for hidden in hidden_prefixes:
                    if property.startswith(hidden):
                        break
                else:
                    undocumented.append(property)

        # everything should be documented
        if len(undocumented) > 0:
            raise AssertionError('{} undocumented properties: {}'.format(len(undocumented), undocumented))


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
