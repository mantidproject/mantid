import unittest
import six

import Muon.GUI.Common.load_utils as load_utils
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


class SaveGroupingToXMLTest(unittest.TestCase):

    def test_AttributeError_thrown_if_no_file_given(self):
        with self.assertRaises(AttributeError):
            load_utils.save_grouping_to_XML([], [], "")

    def test_AttributeError_thrown_file_extension_is_not_XML(self):
        with self.assertRaises(AttributeError):
            load_utils.save_grouping_to_XML([], [], "file.txt")

    def test_AttributeError_thrown_if_groups_are_not_MuonGroup_object(self):
        with self.assertRaises(AttributeError):
            load_utils.save_grouping_to_XML([MuonGroup(group_name="mygroup1"),
                                             "mygroup2"], [], "file.xml", False)

    def test_AttributeError_thrown_if_pairs_are_not_MuonPair_object(self):
        with self.assertRaises(AttributeError):
            load_utils.save_grouping_to_XML([], [MuonPair(pair_name="mypair"),
                                                 "mypair2"], "file.xml", False)

    def test_that_group_name_saved_correctly(self):
        group = MuonGroup(group_name="mygroup")

        tree = load_utils.save_grouping_to_XML([group], [], "file.xml", False)

        groups_xml = tree.findall("group")
        group_xml = groups_xml[0]

        self.assertEqual(len(groups_xml), 1)
        self.assertEqual(group_xml.attrib['name'], 'mygroup')

    def test_that_group_detector_ids_saved_correctly(self):
        group = MuonGroup(group_name="mygroup", detector_IDs=[1, 2, 3, 4, 5])

        tree = load_utils.save_grouping_to_XML([group], [], "file.xml", False)

        group_xml = tree.findall("group")[0]

        self.assertEqual(group_xml.attrib['name'], 'mygroup')
        self.assertEqual(group_xml.find("ids").attrib["val"], '1-5')

    def test_that_pair_name_saved_correctly(self):
        pair = MuonPair(pair_name="mypair")

        tree = load_utils.save_grouping_to_XML([], [pair], "file.xml", False)

        pairs_xml = tree.findall("pair")
        pair_xml = pairs_xml[0]

        self.assertEqual(len(pairs_xml), 1)
        self.assertEqual(pair_xml.attrib['name'], 'mypair')

    def test_that_pair_group_names_saved_correctly(self):
        pair = MuonPair(pair_name="mypair", group1_name="fwd", group2_name="bwd")

        tree = load_utils.save_grouping_to_XML([], [pair], "file.xml", False)

        pair_xml = tree.findall("pair")[0]

        self.assertEqual(pair_xml.find("forward-group").attrib["val"], 'fwd')
        self.assertEqual(pair_xml.find("backward-group").attrib["val"], 'bwd')

    def test_that_pair_alpha_saved_correctly(self):
        pair = MuonPair(pair_name="mypair", group1_name="fwd", group2_name="bwd", alpha=1.5)

        tree = load_utils.save_grouping_to_XML([], [pair], "file.xml", False)

        pair_xml = tree.findall("pair")[0]

        self.assertEqual(pair_xml.find("alpha").attrib["val"], '1.5')

    def test_that_pair_alpha_saved_with_3_significant_figures_and_rounded_correctly(self):
        pair = MuonPair(pair_name="mypair", group1_name="fwd", group2_name="bwd", alpha=1.1239)

        tree = load_utils.save_grouping_to_XML([], [pair], "file.xml", False)

        pair_xml = tree.findall("pair")[0]

        self.assertEqual(pair_xml.find("alpha").attrib["val"], '1.124')

    def test_saving_multiple_groups(self):
        group1 = MuonGroup(group_name="mygroup1")
        group2 = MuonGroup(group_name="mygroup2")
        group3 = MuonGroup(group_name="mygroup3")

        tree = load_utils.save_grouping_to_XML([group1, group2, group3], [], "file.xml", False)

        groups_xml = tree.findall("group")
        group_names = [group_xml.attrib['name'] for group_xml in groups_xml]

        self.assertEqual(len(groups_xml), 3)
        six.assertCountEqual(self, group_names, ["mygroup1", "mygroup2", "mygroup3"])

    def test_saving_multiple_pairs(self):
        pair1 = MuonPair(pair_name="mypair1")
        pair2 = MuonPair(pair_name="mypair2")
        pair3 = MuonPair(pair_name="mypair3")

        tree = load_utils.save_grouping_to_XML([], [pair1, pair2, pair3], "file.xml", False)

        pairs_xml = tree.findall("pair")
        pair_names = [pair_xml.attrib['name'] for pair_xml in pairs_xml]

        self.assertEqual(len(pairs_xml), 3)
        six.assertCountEqual(self, pair_names, ["mypair1", "mypair2", "mypair3"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
