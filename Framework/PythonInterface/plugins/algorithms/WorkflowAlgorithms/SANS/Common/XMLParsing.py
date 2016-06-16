try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET


def get_named_elements_from_ipf_file(ipf_file, names_to_search, value_type):
    """
    Args:
        ipf_file: The path to the IPF
        names: A list of search names
    Returns: A map of the search names and the found information
    """
    output = {}
    number_of_elements_to_search = len(names_to_search)
    for event, element in ET.iterparse(ipf_file):
        if element.tag == "parameter" and "name" in element.keys():
            if element.get("name") in names_to_search:
                sub_element = element.find("value")
                value = sub_element.get("val")
                output.update({element.get("name"), value_type(value)})
                element.clear()
                if len(number_of_elements_to_search) == len(output):
                    break
    return output
