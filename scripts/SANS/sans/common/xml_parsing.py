""" The elements of this module are used to extract information from IDF and IPF files."""

# pylint: disable=invalid-name

try:
    import xml.etree.cElementTree as eTree
except ImportError:
    import xml.etree.ElementTree as eTree


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
        if element.tag == "parameter" and "name" in element.keys():
            if element.get("name") in names_to_search:
                sub_element = element.find("value")
                value = sub_element.get("val")
                output.update({element.get("name"): value_type(value)})
                element.clear()
                if number_of_elements_to_search == len(output):
                    break
    return output


def get_monitor_names_from_idf_file(idf_file):
    """
    Gets the monitor names from the IDF

    :param idf_file: the path to the IDF
    :return: a NumberAsString vs Monitor Name map
    """
    def get_tag(tag_in):
        return "{http://www.mantidproject.org/IDF/1.0}" + tag_in
    output = {}
    tag = "idlist"
    idname = "idname"
    id_tag = "id"
    for _, element in eTree.iterparse(idf_file):
        if element.tag == get_tag(tag) and idname in element.keys():
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
    return output
