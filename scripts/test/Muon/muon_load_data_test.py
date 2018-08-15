import unittest

from Muon.GUI.Common.muon_load_data import MuonLoadData

class MuonLoadDataTest(unittest.TestCase):

    def setUp(self):
        self.data = MuonLoadData()

    def assert_empty(self, load_data):
        self.assertEqual(load_data.get_parameter("run"), [])
        self.assertEqual(load_data.get_parameter("workspace"), [])
        self.assertEqual(load_data.get_parameter("filename"), [])

        self.assertEqual(load_data.num_items(), 0)

    def test_load_data_initialized_as_empty(self):
        self.assert_empty(self.data)

    def test_data_added_correctly(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")

        self.assertEqual(data.num_items(), 1)
        self.assertEqual(data.contains_n(run=1234), 1)
        self.assertEqual(data.contains_n(workspace=[1]), 1)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)

    def test_data_cleared_correctly(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.clear()

        self.assert_empty(data)

    def test_add_then_remove_single_item_leaves_data_empty(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(run=1234)
        self.assert_empty(data)

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(workspace=[1])
        self.assert_empty(data)

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(filename="C:\\dir1\\file1.nxs")
        self.assert_empty(data)

    def test_remove_several_entries_at_once(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1234, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_data(run=1234)

        self.assert_empty(data)

    def test_remove_applies_OR_to_keyword_arguments(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_data(run=1234, workspace=[2], filename="C:\\dir1\\file3.nxs")

        self.assert_empty(data)

    def test_counting_entries_with_keyword_argument_gives_correct_count(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        self.assertEqual(data.contains_n(run=1234), 2)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)
        self.assertEqual(data.contains_n(run=9999), 0)

    def test_counting_entries_applies_OR_behaviour_to_keyword_arguments(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")
        data.add_data(run=1236, workspace=[4], filename="C:\\dir1\\file4.nxs")

        self.assertEqual(data.contains_n(run=1234, workspace=[1]), 2)
        self.assertEqual(data.contains_n(run=1234, workspace=[2]), 3)
        self.assertEqual(data.contains_n(run=1234, workspace=[2], filename="C:\\dir1\\file4.nxs"), 4)

    def test_iterator_behaviour_of_data(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs"},
                 {"run": 1235, "workspace": [2], "filename": "C:\\dir1\\file2.nxs"},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs"}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_remove_last_added_data_remove_the_correct_item(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_last_added_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs"},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs"}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))



if __name__ == "__main__":
    unittest.main(verbosity=2)