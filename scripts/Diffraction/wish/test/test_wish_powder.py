import unittest
from unittest.mock import patch
import tempfile
import shutil
from mantid.simpleapi import AnalysisDataService
from Diffraction.wish.wishPowder import WishPowder

WISHPOWDER_PATH = "Diffraction.wish.wishPowder"


class SXDTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_dir = tempfile.mkdtemp()
        cls.wish_init_kwargs = {"calib_dir": cls._test_dir, "user_dir": cls._test_dir}
        cls.wish = WishPowder(**cls.cls.wish_init_kwargs)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.test_dir)

    def tearDown(self):
        AnalysisDataService.clear()

    @patch(WISHPOWDER_PATH + ".mantid.Load")
    @patch(WISHPOWDER_PATH + ".WishPowder.create_vanadium")
    def test_get_van_ws_create_new(self, mock_create_van, mock_load):
        wsname_van = "van"
        mock_create_van.return_value = wsname_van
        self.wish.get_van_ws()

        self.assertEqual(self.wish.van_ws, wsname_van)
        mock_create_van.assert_called_once()
        mock_load.assert_not_called()

    @patch(WISHPOWDER_PATH + ".path.isfile")
    @patch(WISHPOWDER_PATH + ".mantid.Load")
    @patch(WISHPOWDER_PATH + ".WishPowder.create_vanadium")
    def test_get_van_ws_loads_existing(self, mock_create_van, mock_load, mock_isfile):
        wsname_van = "van"
        mock_create_van.return_value = wsname_van
        self.wish.get_van_ws()

        self.assertEqual(self.wish.van_ws, wsname_van)
        mock_create_van.assert_not_called()
        mock_load.assert_not_called()


if __name__ == "__main__":
    unittest.main()
