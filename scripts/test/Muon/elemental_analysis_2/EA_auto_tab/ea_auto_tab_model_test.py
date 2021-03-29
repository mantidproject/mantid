import unittest
import unittest.mock as mock
from mantid.simpleapi import CreateEmptyTableWorkspace , DeleteWorkspace
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import EAAutoTabModel
from mantid.api import mtd


class EAAutoTabModelTest(unittest.TestCase):

    def setUp(self):
        self.model = EAAutoTabModel(None)

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_update_match_table_when_table_has_more_than_6_rows(self , mock_notify_subscribers):
        correct_table_entries = [['Ag', 20, 2],
                                 ['Au', 18, 2],
                                 ['Fe', 14, 1],
                                 ['Cu', 10, 0],
                                 ['Al', 8, 0],
                                 ['Si', 6, 1]]
        self.model.table_entries = None
        #Create source tables
        likelyhood_table = CreateEmptyTableWorkspace(OutputWorkspace = "likelyhood")
        likelyhood_table.addColumn("str", "Element")
        likelyhood_table.addColumn("int", "Likelihood")
        likelyhood_table.addRow(["Ag", 20])
        likelyhood_table.addRow(["Au", 18])
        likelyhood_table.addRow(["Fe", 14])
        likelyhood_table.addRow(["Cu", 10])
        likelyhood_table.addRow(["Al", 8])
        likelyhood_table.addRow(["Si", 6])
        likelyhood_table.addRow(["C", 4])
        likelyhood_table.addRow(["O", 3])

        all_matches_table = CreateEmptyTableWorkspace(OutputWorkspace = "all_matches")
        all_matches_table.addColumn("str" , "Element")
        all_matches_table.addColumn("float" , "Center")
        all_matches_table.addRow(["Ag" , 4])
        all_matches_table.addRow(["C", 4])
        all_matches_table.addRow(["Fe", 4])
        all_matches_table.addRow(["Si", 4])
        all_matches_table.addRow(["Ag", 4])
        all_matches_table.addRow(["Au", 4])
        all_matches_table.addRow(["Au", 4])
        all_matches_table.addRow(["O", 4])

        #Run function
        self.model.update_match_table("likelyhood" , "all_matches")

        #Assert statements
        self.assertEqual(self.model.table_entries , correct_table_entries)
        mock_notify_subscribers.assert_called_once()

        #Delete tables from ADS
        self.delete_if_present("likelyhood")
        self.delete_if_present("all_matches")

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_update_match_table_when_table_has_less_than_6_rows(self, mock_notify_subscribers):
        correct_table_entries = [['Ag', 20, 2],
                                 ['Au', 18, 2],
                                 ['Fe', 14, 1],
                                 ['Cu', 10, 2]]

        self.model.table_entries = None
        # Create source tables
        likelyhood_table = CreateEmptyTableWorkspace(OutputWorkspace="likelyhood")
        likelyhood_table.addColumn("str", "Element")
        likelyhood_table.addColumn("int", "Likelihood")
        likelyhood_table.addRow(["Ag", 20])
        likelyhood_table.addRow(["Au", 18])
        likelyhood_table.addRow(["Fe", 14])
        likelyhood_table.addRow(["Cu", 10])

        all_matches_table = CreateEmptyTableWorkspace(OutputWorkspace="all_matches")
        all_matches_table.addColumn("str", "Element")
        all_matches_table.addColumn("float", "Center")
        all_matches_table.addColumn("int", "sigma")
        all_matches_table.addRow(["Ag", 4, 1])
        all_matches_table.addRow(["Cu", 4, 2])
        all_matches_table.addRow(["Fe", 4, 3])
        all_matches_table.addRow(["Cu", 4, 4])
        all_matches_table.addRow(["Ag", 4, 5])
        all_matches_table.addRow(["Au", 4, 6])
        all_matches_table.addRow(["Au", 4, 7])

        # Run function
        self.model.update_match_table("likelyhood", "all_matches")

        # Assert statements
        self.assertEqual(self.model.table_entries, correct_table_entries)
        mock_notify_subscribers.assert_called_once_with()

        # Delete tables from ADS
        self.delete_if_present("likelyhood")
        self.delete_if_present("all_matches")

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_run_peak_algorithms(self,mock_notify_subscribers):

        # Run function
        self.model.run_peak_algorithms("a")

        # Assert statements
        mock_notify_subscribers.assert_called_once_with()


if __name__ == '__main__':
    unittest.main()
