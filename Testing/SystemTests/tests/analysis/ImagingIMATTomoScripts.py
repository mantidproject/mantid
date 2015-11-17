import stresstesting
import unittest

import numpy as np

from mantid.api import WorkspaceGroup, MatrixWorkspace, mtd
from mantid.api import mtd
import mantid.simpleapi as sapi

#pylint: disable=too-many-public-methods
class ImagingIMATTomoTests(unittest.TestCase):
    """
    General IMAT/tomography tests, covering pre-/post-processing
    methods of tomographic reconstruction scripts
    """

    @classmethod
    def setUpClass(cls):
        # example files to make a stack of images
        cls._raw_files = ['LARMOR00005328_Metals_000_SummedImg_1.fits',
                          'LARMOR00005329_Metals_000_SummedImg_2.fits',
                          'LARMOR00005330_Metals_000_SummedImg_3.fits',
                          'LARMOR00005331_Metals_000_SummedImg_4.fits',
                          'LARMOR00005332_Metals_000_SummedImg_5.fits',
        ]

        # data volume from a 'stack' of images
        cls._data_wsname = 'small_img_stack'

        filename_string = ",".join(cls._raw_files)
        # Load all images into a workspace group, one matrix workspace per image
        cls._data_wsg = sapi.LoadFITS(Filename=filename_string,
                                      LoadAsRectImg=True,
                                      OutputWorkspace=cls._data_wsname)

        cls._data_vol = cls._ws_group_to_data_vol(cls._data_wsg)

    @classmethod
    def tearDownClass(cls):
        sapi.DeleteWorkspace(cls._data_wsg)

    @staticmethod
    def _ws_group_to_data_vol(ws_group):
        """
        Makes a 3d volume as a numpy array from a group of workspace2d workspaces

        @param ws_group :: workspace group as produced by LoadFITS, with all the
        matrix workspaces having the same number of spectra and bins
        """
        zdim = ws_group.size()
        if zdim < 1:
            raise ValueError("Got workspace group with 0 workspaces!")
        ws1 = ws_group.getItem(0)
        ydim = ws1.getNumberHistograms()
        xdim = ws1.blocksize()
        data_vol = np.zeros( (zdim, ydim, xdim) )

        for zidx, wksp in enumerate(ws_group):
            for yidx in range(0, wksp.getNumberHistograms()):
                data_vol[zidx, yidx, :] = wksp.readY(yidx)

        return data_vol


    def setUp(self):
        # double-check before every test that the input workspaces are available and of the
        # correct types
        if not self._data_wsg or not self._data_wsname in mtd:
            raise RuntimeError("Input workspace not available")

        # this could use assertIsInstance (new in version 2.7)
        self.assertTrue(isinstance(self._data_wsg, WorkspaceGroup))
        img_workspaces = [ self._data_wsg.getItem(i) for i in range(0, self._data_wsg.size()) ]
        for wksp in img_workspaces:
            self.assertTrue(isinstance(wksp, MatrixWorkspace))

    def test_crop_ok(self):
        import IMAT.prep as iprep

        coords = [2, 2, 100, 100]
        cropped = iprep.filters.crop_vol(self._data_vol, coords)

        self.assertTrue(isinstance(self._data_vol, np.ndarray))
        self.assertTrue(isinstance(cropped, np.ndarray))

        expected_shape = (self._data_vol.shape[0], coords[3]-coords[1], coords[2]-coords[0])
        self.assertEqual(cropped.shape, expected_shape)

        orig_cropped_equals = self._data_vol[:, coords[1]:coords[3], coords[0]:coords[2]] == cropped
        self.assertTrue(orig_cropped_equals.all())

    def test_correct_import_excepts(self):
        import IMAT.tomorec.tool_imports as tti

        self.assertRaises(ImportError, astra_nope = tti.import_tomo_tool('astra'))
        self.assertRaises(ImportError, tomopy_nope = tti.import_tomo_tool('tomopy'))

    def test_circular_mask_ok(self):
        import IMAT.prep as iprep

        masked = iprep.filters.circular_mask(self._data_vol, ratio=0.0)
        np.testing.assert_allclose(masked, self._data_vol,
                                   err_msg="An empty circular mask not behaving as expected")

        masked = iprep.filters.circular_mask(self._data_vol, ratio=1.0, mask_out_val=0)
        self.assertEquals(masked[2, 2, 3], 0,
                          msg="Circular mask: wrong values outside")

        some_val = -1.23456
        masked = iprep.filters.circular_mask(self._data_vol, ratio=1.0, mask_out_val=some_val)

        for coords in [(3, 510, 0), (2,2,3), (1,0,0), (0, 500, 5)]:
            peek_out = masked[coords]
            self.assertEquals(peek_out, some_val,
                              msg="Circular mask: wrong value found outside. Expected: {0}, found: {1}".
                              format(some_val, peek_out))

        for coords in [(3, 200, 200), (2, 50, 20), (1, 300, 100), (0, 400, 200)]:
            peek_in = masked[coords]
            expected_val = self._data_vol[coords]
            self.assertEquals(peek_in, expected_val,
                              msg="Circular mask: wrong value found inside. Expected: {0}, found: {1}".
                              format(expected_val, peek_in))

    def test_scale_down_errors(self):
        import IMAT.prep as iprep

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self._data_vol, 9)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self._data_vol, 1000)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self._data_vol, 513)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self._data_vol, 2, method='fail-now')


    def test_scale_down_ok(self):
        import IMAT.prep as iprep

        scaled = iprep.filters.scale_down(self._data_vol, 2)

        self.assertEquals(len(scaled.shape), 3)
        self.assertEquals(self._data_vol.shape[0], scaled.shape[0])
        self.assertEquals(self._data_vol.shape[1]/2, scaled.shape[1])
        self.assertEquals(self._data_vol.shape[2]/2, scaled.shape[2])

    def test_remove_sinogram_stripes(self):
        pass

# Just run the unittest tests defined above
class ImagingIMATScriptsTest(stresstesting.MantidStressTest):

    _success = False

    def __init__(self, *args, **kwargs):
        # super(ImagingIMATScriptsTest, self).__init__(*args, **kwargs)
        # old-style
        stresstesting.MantidStressTest.__init__(self, *args, **kwargs)
        self._success = False

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(ImagingIMATTomoTests, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success
