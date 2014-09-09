import unittest
import os
from mantid.simpleapi import *
from isis_reflectometry import settings

'''
RAII Test helper class. Equivalent to the ScopedFileHelper.

If this proves useful. It would be sensible to make it more accessible for other testing classes.
'''
class TempFile(object):

    __tempFile = None

    def __init__(self, contents, extension):
        import tempfile
        self.__tempFile = tempfile.NamedTemporaryFile(delete=False, suffix=extension)
        self.__tempFile.write(contents)
        self.__tempFile.close()

    def __del__(self):
        self.clear()

    def clear(self):
        if os.path.isfile(self.__tempFile.name):
            os.remove(self.__tempFile.name)

    def pathToFile(self):
        return self.__tempFile.name

'''
Determine if all tests should be skipped. Check for the expat module to decide.
'''
def skipAllTests():
    skiptests = False
    try:
        import xml.parsers.expat
    except ImportError:
        skiptests = True
    return skiptests


if not skipAllTests():
    '''
    Test suite for the Settings
    '''
    class SettingsTest(unittest.TestCase):

        def test_avalid_file(self):
            fileObject = TempFile(contents="<SettingList><Setting name='test_setting'>test</Setting></SettingList>", extension=".xml")
            configuration = settings.Settings( fileObject.pathToFile() )
            entries = configuration.get_all_entries()
            self.assertEqual(len(entries), 1, "There is only one setting entry") # Quick check

        def test_bad_file_extension_throws(self):
            bad_extension = ".txt "
            fileObject = TempFile(contents="<SettingList><Setting name='test_setting'>test</Setting></SettingList>", extension=bad_extension)
            self.assertRaises(ValueError, settings.Settings, fileObject.pathToFile() )

        def test_bad_file_location_throws(self):
            missing_file = "fictional_file.xml"
            self.assertRaises(settings.MissingSettings, settings.Settings, missing_file)

        def test_bad_xml_format_throws(self):
            fileObject = TempFile(contents="<SettingList>invalid xml", extension=".xml")
            self.assertRaises(ValueError, settings.Settings, fileObject.pathToFile() )

        def test_sanity_check_missing_attribute_name_throws(self):
            fileObject = TempFile(contents="<SettingList><Setting>test</Setting></SettingList>", extension=".xml")
            self.assertRaises(ValueError, settings.Settings, fileObject.pathToFile() )

        def test_sanity_check_missing_attribute_value_throws(self):
            fileObject = TempFile(contents="<SettingList><Setting name='test_setting'></Setting></SettingList>", extension=".xml")
            self.assertRaises(ValueError, settings.Settings, fileObject.pathToFile() )

        def test_get_entries(self):
            fileObject = TempFile(contents="<SettingList><Setting name='a'>1</Setting><Setting name='b'>2</Setting></SettingList>", extension=".xml")
            configuration = settings.Settings( fileObject.pathToFile() )
            entries = configuration.get_all_entries()
            self.assertEqual(len(entries), 2)
            self.assertEqual(int(entries['a']), 1)
            self.assertEqual(int(entries['b']), 2)

        def test_get_filename(self):
            fileObject = TempFile(contents="<SettingList></SettingList>", extension=".xml")
            configuration = settings.Settings( fileObject.pathToFile() )
            self.assertEqual(configuration.get_contents_file(), fileObject.pathToFile())

        def test_get_named_setting(self):
            fileObject = TempFile(contents="<SettingList><Setting name='a'>1</Setting></SettingList>", extension=".xml")
            configuration = settings.Settings( fileObject.pathToFile() )
            self.assertEqual(configuration.get_named_setting('a'), '1')
            self.assertRaises(KeyError, configuration.get_named_setting, 'b')

if __name__ == '__main__':
    unittest.main()
