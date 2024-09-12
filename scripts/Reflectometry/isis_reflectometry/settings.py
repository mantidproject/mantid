# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import xml.etree.ElementTree as XML
import os.path


class MissingSettings(Exception):
    pass


class Settings(object):
    __contents = None
    __filename = None

    def __init__(self, filename=None):
        self.__filename = filename
        if not filename:
            filename = os.path.join(os.path.dirname(os.path.realpath(__file__)), "settings.xml")

        self.__check_file(filename)

        doc = None
        try:
            tree = XML.parse(filename)
            doc = tree.getroot()
            self.__extract_to_dictionary(doc)
        except:
            raise ValueError("The file %s does not contain valid XML" % filename)

    def __check_file(self, filename):
        path, extension = os.path.splitext(filename)
        if extension.lower().strip() != ".xml":
            raise ValueError("Wrong file extension. *.xml expected not %s." % extension)
        if not os.path.isfile(filename):
            """Deliberately swallow and re-throw at this point. Consise reinterpreted error, will be much nicer for client code."""
            raise MissingSettings("Settings file %s does not exist so no manual settings will be applied." % filename)

    def __extract_to_dictionary(self, doc):
        temp = dict()
        for elem in doc:
            key = elem.attrib.get("name").strip()
            value = elem.text.strip()
            if not key:
                raise ValueError("Missing name attribute on Setting element")
            if not value:
                raise ValueError("Missing value for Setting element")
            temp[key] = value
        self.__contents = dict(frozenset(list(temp.items())))

    def get_all_entries(self):
        return self.__contents

    def get_named_setting(self, name):
        return self.__contents[name]

    def get_contents_file(self):
        return self.__filename
