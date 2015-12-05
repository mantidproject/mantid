import stresstesting
import unittest

import numpy as np

from mantid.api import WorkspaceGroup, MatrixWorkspace, mtd
import mantid.simpleapi as sapi

#pylint: disable=too-many-public-methods
class ImagingIMATTomoTests(unittest.TestCase):
    """
    General IMAT/tomography tests, covering pre-/post-processing
    methods of tomographic reconstruction scripts
    """

    # group of image workspaces
    data_wsg = None

    # data volume from a 'stack' of images
    data_vol = None

    # This should rather use setUpClass() - not supported in Python 2.6 (rhel6)
    def setUp(self):
        # example files to make a stack of images
        _raw_files = ['LARMOR00005328_Metals_000_SummedImg_1.fits',
                      'LARMOR00005329_Metals_000_SummedImg_2.fits',
                      'LARMOR00005330_Metals_000_SummedImg_3.fits',
                      'LARMOR00005331_Metals_000_SummedImg_4.fits',
                      'LARMOR00005332_Metals_000_SummedImg_5.fits'
                     ]

        # a name for the stack / group of workspaces
        _data_wsname = 'small_img_stack'

        self.__class__.test_input_dir = 'some_imaging_test_path_in'
        self.__class__.test_output_dir = 'some_imaging_test_path_out'

        if not self.__class__.data_wsg:
            filename_string = ",".join(_raw_files)
            # Load all images into a workspace group, one matrix workspace per image
            self.__class__.data_wsg = sapi.LoadFITS(Filename=filename_string,
                                                    LoadAsRectImg=True,
                                                    OutputWorkspace=_data_wsname)
            self.__class__.data_vol = self._ws_group_to_data_vol(self.data_wsg)

        # double-check before every test that the input workspaces are available and of the
        # correct types
        if not self.data_wsg or not _data_wsname in mtd:
            raise RuntimeError("Input workspace not available")

        # this could use assertIsInstance (new in version 2.7)
        self.assertTrue(isinstance(self.data_wsg, WorkspaceGroup))
        img_workspaces = [ self.data_wsg.getItem(i) for i in range(0, self.data_wsg.size()) ]
        for wksp in img_workspaces:
            self.assertTrue(isinstance(wksp, MatrixWorkspace))

    def tearDown(self):
        import os
        if os.path.exists(self.test_input_dir):
            os.rmdir(self.test_input_dir)
        import shutil
        if os.path.exists(self.test_output_dir):
            shutil.rmtree(self.test_output_dir)

    # Remember: use this when rhel6/Python 2.6 is deprecated
    # @classmethod
    # def setUpClass(cls):

    # Remember: use this when rhel6/Python 2.6 is deprecated
    # @classmethod
    # def tearDownClass(cls):
    #    sapi.DeleteWorkspace(cls.data_wsg)

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

        # In the Mantid workspaces we have the usual double/float64 values
        # but reconstruction tools effectively work on float32
        return data_vol.astype(dtype='float32')

    def test_scale_down_errors(self):
        import IMAT.prep as iprep

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self.data_vol, 9)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self.data_vol, 1000)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self.data_vol, 513)

        with self.assertRaises(ValueError):
            iprep.filters.scale_down(self.data_vol, 2, method='fail-now')

    def test_scale_down_ok(self):
        import IMAT.prep as iprep

        dummy_shape = (7, 256, 256)
        dummy = np.ones(dummy_shape, dtype='float')
        scaled = iprep.filters.scale_down(dummy, 2)
        self.assertEquals(scaled.shape, (dummy_shape[0], dummy_shape[1]/2, dummy_shape[2]/2))

        scaled = iprep.filters.scale_down(self.data_vol, 2)

        self.assertEquals(len(scaled.shape), 3)
        self.assertEquals(self.data_vol.shape[0], scaled.shape[0])
        self.assertEquals(self.data_vol.shape[1]/2, scaled.shape[1])
        self.assertEquals(self.data_vol.shape[2]/2, scaled.shape[2])

    def test_crop_ok(self):
        import IMAT.prep as iprep

        coords = [2, 2, 100, 100]
        cropped = iprep.filters.crop_vol(self.data_vol, coords)

        self.assertTrue(isinstance(self.data_vol, np.ndarray))
        self.assertTrue(isinstance(cropped, np.ndarray),
                        msg="the result of cropping should be a numpy array")

        expected_shape = (self.data_vol.shape[0], coords[3]-coords[1], coords[2]-coords[0])
        self.assertEqual(cropped.shape, expected_shape,
                         msg="the result of cropping should have the appropriate dimensions")

        orig_cropped_equals = self.data_vol[:, coords[1]:coords[3], coords[0]:coords[2]] == cropped
        self.assertTrue(orig_cropped_equals.all())

    def test_correct_import_excepts(self):
        import IMAT.tomorec.tool_imports as tti

        with self.assertRaises(ImportError):
            tti.import_tomo_tool('astra')

        with self.assertRaises(ImportError):
            tti.import_tomo_tool('tomopy')

    def test_circular_mask_raises(self):
        import IMAT.prep as iprep

        with self.assertRaises(ValueError):
            iprep.filters.circular_mask('fail!')

        with self.assertRaises(ValueError):
            iprep.filters.circular_mask(np.zeros((2,3)))

        with self.assertRaises(ValueError):
            iprep.filters.circular_mask(np.zeros((2,1,2,1)))

    def test_circular_mask_ok(self):
        import IMAT.prep as iprep

        masked = iprep.filters.circular_mask(self.data_vol, ratio=0.0)
        np.testing.assert_allclose(masked, self.data_vol,
                                   err_msg="An empty circular mask not behaving as expected")

        masked = iprep.filters.circular_mask(self.data_vol, ratio=1.0, mask_out_val=0)
        self.assertEquals(masked[2, 2, 3], 0,
                          msg="Circular mask: wrong values outside")

        some_val = -1.23456
        masked = iprep.filters.circular_mask(self.data_vol, ratio=1.0, mask_out_val=some_val)

        for coords in [(3, 510, 0), (2,2,3), (1,0,0), (0, 500, 5)]:
            peek_out = masked[coords]
            self.assertAlmostEquals(peek_out, some_val,
                              msg="Circular mask: wrong value found outside. Expected: {0}, found: {1}".
                              format(some_val, peek_out))

        for coords in [(3, 200, 200), (2, 50, 20), (1, 300, 100), (0, 400, 200)]:
            peek_in = masked[coords]
            expected_val = self.data_vol[coords]
            self.assertAlmostEquals(peek_in, expected_val,
                              msg="Circular mask: wrong value found inside. Expected: {0}, found: {1}".
                              format(expected_val, peek_in))

    def test_remove_stripes_ok(self):
        import IMAT.prep as iprep

        method = 'wavelet-fourier'
        stripped = iprep.filters.remove_stripes_ring_artifacts(self.data_vol,
                                                               method)

        self.assertTrue(isinstance(self.data_vol, np.ndarray))
        self.assertTrue(isinstance(stripped, np.ndarray),
                        msg="the result of remove_stripes should be a numpy array")
        self.assertEquals(stripped.shape, self.data_vol.shape,
                          msg="the result of remove_stripes should be a numpy array")
        expected_val = -0.25781357
        val = stripped[0, 100, 123]
        self.assertAlmostEquals(val, expected_val,
                                msg="Expected the results of stripe removal (method {0} not to change "
                                "with respect to previous executions".format(method))

    def test_remove_stripes_raises(self):
        import IMAT.prep as iprep

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts('fail!')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(np.zeros((2,2,2)), 'fail-method')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(np.zeros((3,3)))

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(np.zeros((1,1,2,2)))

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol, '')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol,
                                                        'funny-method-doesnt-exist')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol,
                                                        'fourier-wavelet')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol,
                                                        'fw')

        with self.assertRaises(ValueError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol,
                                                        'wf')

    def test_config_pre(self):
        """
        Basic consistency check of the pre-processing config and some default
        settings
        """
        import IMAT.tomorec.configs as cfgs

        pre = cfgs.PreProcConfig()
        self.assertEquals(pre.input_dir, None)
        self.assertEquals(pre.max_angle, 360)
        self.assertEquals(pre.normalize_flat_dark, True)
        self.assertEquals(pre.crop_coords, None)
        self.assertEquals(pre.scale_down, 0)
        self.assertEquals(pre.stripe_removal_method, 'wavelet-fourier')
        self.assertEquals(pre.save_preproc_imgs, True)

    def test_config_alg(self):
        """
        Basic consistency check of the tool/algorithm config and some default
        values
        """
        import IMAT.tomorec.configs as cfgs

        alg = cfgs.ToolAlgorithmConfig()
        self.assertEquals(alg.tool, alg.DEF_TOOL)
        self.assertEquals(alg.algorithm, alg.DEF_ALGORITHM)
        self.assertEquals(alg.num_iter, None)
        self.assertEquals(alg.regularization, None)

    def test_config_post(self):
        """
        Basic consistency check of the post-processing config and some default
        values
        """
        import IMAT.tomorec.configs as cfgs

        post = cfgs.PostProcConfig()
        self.assertEquals(post.output_dir, None)
        self.assertEquals(post.circular_mask, 0.94)

    def test_config_all(self):
        """
        Basic consistency check of the tomographic reconstruction config and some
        default values
        """
        import IMAT.tomorec.configs as cfgs

        pre_conf = cfgs.PreProcConfig()
        alg_conf = cfgs.ToolAlgorithmConfig()
        post_conf = cfgs.PostProcConfig()
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)

        self.assertEquals(conf.preproc_cfg, pre_conf)
        self.assertEquals(conf.alg_cfg, alg_conf)
        self.assertEquals(conf.postproc_cfg, post_conf)

        print conf

    def test_recon_fails_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        with self.assertRaises(ValueError):
            cmd.do_recon('', cmd_line='')

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()
        alg_conf = cfgs.ToolAlgorithmConfig()
        post_conf = cfgs.PostProcConfig()
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)
        with self.assertRaises(ValueError):
            cmd.do_recon(conf, cmd_line='irrelevant')

        pre_conf.input_dir = self.test_input_dir
        import IMAT.tomorec.io as tomoio
        tomoio.make_dirs_if_needed(self.test_input_dir)
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)
        with self.assertRaises(ValueError):
            cmd.do_recon(conf, cmd_line='irrelevant')

        post_conf.output_dir = self.test_output_dir
        tomoio.make_dirs_if_needed(self.test_output_dir)
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)
        # should fail because no images found in input dir
        with self.assertRaises(RuntimeError):
            cmd.do_recon(conf, cmd_line='irrelevant')

        import os
        self.assertTrue(os.path.exists(self.test_input_dir))
        self.assertTrue(os.path.exists(os.path.join(self.test_output_dir,
                                                    '0.README_reconstruction.txt')))
        self.assertTrue(os.path.exists(self.test_output_dir))

    def test_read_in_fails_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        with self.assertRaises(RuntimeError):
            cmd.read_in_stack(self.test_input_dir, 'tiff')


# Just run the unittest tests defined above
class ImagingIMATScriptsTest(stresstesting.MantidStressTest):

    _success = False

    def __init__(self, *args, **kwargs):
        # super(ImagingIMATScriptsTest, self).__init__(*args, **kwargs)
        # old-style
        stresstesting.MantidStressTest.__init__(self, *args, **kwargs)
        self._success = False

    def runTest(self):
        # Disable for Python 2.6 (which we still have on rhel6)
        import sys
        vers = sys.version_info
        if vers < (2,7,0):
            from mantid import logger
            logger.warning("Not running this test as it requires Python >= 2.7. Version found: {0}".
                           format(vers))
            self._success = True
            return

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
