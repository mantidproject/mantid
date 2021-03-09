# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import xml.etree.ElementTree as ET
import xml.dom.minidom as MD
import Muon.GUI.Common.utilities.run_string_utils as run_string_utils

from Muon.GUI.Common.muon_group import MuonGroup, MuonDiff
from Muon.GUI.Common.muon_pair import MuonPair


def _create_XML_subElement_for_groups(root_node, groups):
    group_nodes = []
    for group in groups:
        child = ET.SubElement(root_node, 'group', name=group.name)
        id_string = run_string_utils.run_list_to_string(group.detectors)
        ids = ET.SubElement(child, 'ids', val=id_string)
        child.extend(ids)
        period_string = run_string_utils.run_list_to_string(group.periods)
        periods = ET.SubElement(child, 'periods', val=period_string)
        child.extend(periods)
        group_nodes += [child]
    return group_nodes


def _create_XML_subElement_for_pairs(root_node, pairs):
    pair_nodes = []
    for pair in pairs:
        child = ET.SubElement(root_node, 'pair', name=pair.name)
        fwd_group = ET.SubElement(child, 'forward-group', val=pair.forward_group)
        bwd_group = ET.SubElement(child, 'backward-group', val=pair.backward_group)
        alpha = ET.SubElement(child, 'alpha', val=str(pair.alpha))
        child.extend(fwd_group)
        child.extend(bwd_group)
        child.extend(alpha)
        pair_nodes += [child]
    return pair_nodes


def _create_XML_subElement_for_diffs(root_node, diffs):
    diff_nodes = []
    for diff in diffs:
        child = ET.SubElement(root_node, 'diff', name=diff.name)
        fwd_group = ET.SubElement(child, 'positive-group', val=diff.forward_group)
        bwd_group = ET.SubElement(child, 'negative-group', val=diff.backward_group)
        group_or_pair = ET.SubElement(child, 'group-or-pair', val=diff.group_or_pair)
        periods = ET.SubElement(child, 'periods', val=diff.periods)
        child.extend(fwd_group)
        child.extend(bwd_group)
        child.extend(group_or_pair)
        child.extend(periods)
        diff_nodes += [child]
    return diff_nodes


def save_grouping_to_XML(groups,  pairs, diffs, filename, save=True, description=''):
    """
    Save a set of muon group and pair parameters to XML format file. Fewer checks are performed
    than with the XML loading.

    :param groups: A list of MuonGroup objects to save.
    :param pairs: A list of MuonPair objects to save.
    :param diffs: A list of MuonDiff objects to save
    :param filename: The name of the XML file to save to.
    :param save: Whether to actually save the file.
    :return: the XML tree (used in testing).
    """
    # some basic checks
    if filename == "":
        raise AttributeError("File must be specified for saving to XML")
    if os.path.splitext(filename)[-1].lower() != ".xml":
        raise AttributeError("File extension must be XML")
    if sum([0 if isinstance(group, MuonGroup) else 1 for group in groups]) > 0:
        raise AttributeError("groups must be MuonGroup type")
    if sum([0 if isinstance(pair, MuonPair) else 1 for pair in pairs]) > 0:
        raise AttributeError("pairs must be MuonPair type")
    if sum([0 if isinstance(diff, MuonDiff) else 1 for diff in diffs]) > 0:
        raise AttributeError("diffs must be MuonDiff type")
    root = ET.Element("detector-grouping")
    if description:
        root.set('description', description)

    # handle groups
    _create_XML_subElement_for_groups(root, groups)
    # handle pairs
    _create_XML_subElement_for_pairs(root, pairs)
    #handle diffs
    _create_XML_subElement_for_diffs(root, diffs)

    if save:
        prettyXML = MD.parseString(ET.tostring(root)).toprettyxml(indent="\t")
        with open(filename, "w") as xmlfile:
            xmlfile.write(prettyXML)
    return ET.ElementTree(root)


def load_grouping_from_XML(filename):
    """
    Load group/pair data from an XML file (which can be produced using the save_grouping_to_XML() function

    :param filename: Full filepath to an xml file.
    :return: (groups, pairs), lists of MuonGroup, MuonPair objects respectively.
    """
    tree = ET.parse(filename)
    root = tree.getroot()
    description = root.get('description')
    if not description:
        description = filename
    try:
        default = root.find('default').get('name')
    except (AttributeError, KeyError):
        default = ''

    group_names, group_ids, periods = _get_groups_from_XML(root)
    pair_names, pair_groups, pair_alphas = _get_pairs_from_XML(root)
    diff_names, diff_groups = _get_diffs_from_XML(root)
    groups, diffs, pairs = [], [], []

    for i, group_name in enumerate(group_names):
        period = periods[i] if periods and i<len(periods) else [1]
        groups += [MuonGroup(group_name=group_name,
                             detector_ids=group_ids[i], periods=period)]

    for i, pair_name in enumerate(pair_names):
        pairs += [MuonPair(pair_name=pair_name,
                           forward_group_name=pair_groups[i][0],
                           backward_group_name=pair_groups[i][1],
                           alpha=pair_alphas[i])]

    for i, diff_name in enumerate(diff_names):
        diffs +=[MuonDiff(diff_name,diff_groups[i][0],
                          diff_groups[i][1], diff_groups[i][2], diff_groups[i][3])]

    return groups, pairs, diffs, description, default


def _get_groups_from_XML(root):
    names, ids, periods = [], [], []
    for child in root:
        if child.tag == "group":
            names += [child.attrib['name']]
            ids += [run_string_utils.run_string_to_list(child.find('ids').attrib['val'])]
            try:
                periods += [run_string_utils.run_string_to_list(child.find('periods').attrib['val'])]
            except AttributeError:
                # Using an old style xml file where grouping information is not stored
                pass
    return names, ids, periods


def _get_diffs_from_XML(root):
    names, groups = [], []
    for child in root:
        if child.tag == "diff":
            names += [child.attrib['name']]
            groups += [[child.find('positive-group').attrib['val'], child.find('negative-group').attrib['val'],
                        child.find('group-or-pair').attrib['val'], child.find('periods').attrib['val'] ]]
    return names, groups


def _get_pairs_from_XML(root):
    names, groups, alphas = [], [], []
    for child in root:
        if child.tag == "pair":
            names += [child.attrib['name']]
            groups += [[child.find('forward-group').attrib['val'], child.find('backward-group').attrib['val']]]
            alphas += [child.find('alpha').attrib['val']]
    return names, groups, alphas
