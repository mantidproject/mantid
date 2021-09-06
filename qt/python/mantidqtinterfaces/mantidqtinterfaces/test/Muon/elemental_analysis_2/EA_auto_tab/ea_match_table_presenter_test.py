import unittest
import unittest.mock as mock
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter import EAMatchTablePresenter


class EAMatchTablePresenterTest(unittest.TestCase):

    def setUp(self):
        self.presenter = EAMatchTablePresenter(mock.Mock())

    def tearDown(self):
        self.presenter = None

    @mock.patch(
        "scientific_interfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter.EAMatchTablePresenter.find_entry_index")
    @mock.patch("scientific_interfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter.EAMatchTablePresenter.remove_entry")
    def test_update_table(self, mock_remove_entry, mock_find_entry_index):
        mock_find_entry_index.return_value = -1

        self.presenter.update_table("mock_entry")

        # Assert statements
        mock_remove_entry.assert_not_called()
        self.presenter.view.add_entry_to_table.assert_called_once_with("mock_entry")
        self.assertEqual(["mock_entry"], self.presenter.table_entries)

    @mock.patch(
        "scientific_interfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter.EAMatchTablePresenter.find_entry_index")
    @mock.patch("scientific_interfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter.EAMatchTablePresenter.remove_entry")
    def test_update_table_when_entry_is_present(self, mock_remove_entry, mock_find_entry_index):
        index = 3
        mock_find_entry_index.return_value = index

        self.presenter.update_table("mock_entry")

        # Assert statements
        mock_remove_entry.assert_called_once_with(index)
        self.presenter.view.add_entry_to_table.assert_called_once_with("mock_entry")
        self.assertEqual(["mock_entry"], self.presenter.table_entries)

    def test_remove_entry(self):
        self.presenter.table_entries = ["spam", "eggs", "ham"]

        self.presenter.remove_entry(1)

        # Assert statements
        self.assertEqual(self.presenter.table_entries, ["spam", "ham"])
        self.presenter.view.remove_row.assert_called_once_with(1)

    def test_find_entry_index(self):
        # first test, when entry is present
        self.presenter.table_entries = [["mock_run", "mock_detector", "mock_elements"],
                                        ["fake_run", "fake_detector", "fake_elements"], ["spam", "eggs", "ham"]]

        index = self.presenter.find_entry_index(["fake_run", "fake_detector",
                                                 "This doesn't matter because it doesn't check this"])
        self.assertEqual(index, 1)

        # second test, when entry is absent
        index = self.presenter.find_entry_index(["run", "detector",
                                                 "This doesn't matter because it doesn't check this"])
        self.assertEqual(index, -1)

    def test_clear_table(self):
        self.presenter.table_entries = [["mock_run", "mock_detector", "mock_elements"],
                                        ["fake_run", "fake_detector", "fake_elements"], ["spam", "eggs", "ham"]]

        self.presenter.clear_table()

        # Assert statements
        self.assertEqual(self.presenter.table_entries, [])
        self.presenter.view.clear_table.assert_called_once()


if __name__ == '__main__':
    unittest.main()
