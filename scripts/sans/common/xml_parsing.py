# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The elements of this module are used to extract information from IDF and IPF files."""

# pylint: disable=invalid-name

try:
    import xml.etree.cElementTree as eTree
except ImportError:
    import xml.etree.ElementTree as eTree
from mantid.kernel import DateAndTime


def get_named_elements_from_ipf_file(ipf_file, names_to_search, value_type):
    """
    Gets a named element from the IPF

    This is useful for detector names etc.
    :param ipf_file: the path to the IPF
    :param names_to_search: the names we want to search for on the XML file.
    :param value_type: the type we expect for the names.
    :return: a ElementName vs Value map
    """
    """
    Args:
        ipf_file: The path to the IPF
        names_to_search: A list of search names
        value_type: the type of an item
    Returns: A map of the search names and the found information
    """
    output = {}
    number_of_elements_to_search = len(names_to_search)

    for _, element in eTree.iterparse(ipf_file):
        if element.tag == "parameter" and "name" in list(element.keys()):
            # Ideally we would break the for loop if we have found all the elements we are looking for.
            # BUT: a not completed generator eTree.iterparse emits a ResourceWarning if we don't finish the generator.
            #  There is also no method to close the file manually, hence we run through the whole file. Note that there
            # is an existing bug report here: https://bugs.python.org/issue25707
            if number_of_elements_to_search != len(output) and element.get("name") in names_to_search:
                sub_element = element.find("value")
                value = sub_element.get("val")
                output.update({element.get("name"): value_type(value)})
                element.clear()

    return output


def get_monitor_names_from_idf_file(idf_file, invalid_monitor_names=None):
    """
    Gets the monitor names from the IDF

    :param idf_file: the path to the IDF
    :param invalid_monitor_names: a list of invalid monitor names, which is required since some monitors are
                                  dummy monitors which exist in the IDF but not in the workspace.
    :return: a NumberAsString vs Monitor Name map
    """

    def get_tag(tag_in):
        return "{http://www.mantidproject.org/IDF/1.0}" + tag_in

    output = {}
    tag = "idlist"
    idname = "idname"
    id_tag = "id"
    for _, element in eTree.iterparse(idf_file):
        # Get the names from the ID list
        if element.tag == get_tag(tag) and idname in list(element.keys()):
            name = element.get(idname)
            if "monitor" in name:
                sub_element = element.find(get_tag(id_tag))
                # We can have two situations here:
                # 1. either monitors are separate, e.g. <idlist idname="monitor1"> <id val="1" /> </idlist>, ..
                # 2. or in a range, e.g. <idlist idname="monitors"> <id start="1" end="8" /> </idlist>
                val = sub_element.get("val")
                start = sub_element.get("start")
                end = sub_element.get("end")
                if val:
                    output.update({val: name})
                    element.clear()
                elif start and end:
                    for index in range(int(start), int(end) + 1):
                        monitor_id = "monitor" + str(index)
                        output.update({str(index): monitor_id})
                    element.clear()
                else:
                    continue

    # Remove any monitor entries where the
    if invalid_monitor_names:
        output = {key: value for key, value in list(output.items()) if value not in invalid_monitor_names}
    return output


def get_valid_to_time_from_idf_string(idf_string):
    tree_root = eTree.fromstring(idf_string)
    valid_to_date = tree_root.attrib["valid-to"]
    return DateAndTime(valid_to_date)
