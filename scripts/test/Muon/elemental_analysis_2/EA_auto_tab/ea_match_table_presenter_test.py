import unittest
import unittest.mock as mock
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter import EAMatchTablePresenter


class EAMatchTablePresenterTest(unittest.TestCase):

    def setUp(self):
        self.presenter = EAMatchTablePresenter(mock.Mock)

    def test_something(self):
        self.assertEqual(True, False)


if __name__ == '__main__':
    unittest.main()
