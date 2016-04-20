import unittest
import stresstesting

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
        if not self.data_wsg or _data_wsname not in mtd:
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
        return data_vol.astype('float32')

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

    def test_crop_errors(self):
        import IMAT.prep as iprep

        coords = None
        with self.assertRaises(ValueError):
            iprep.filters.crop_vol(self.data_vol, coords)

        coords = [0, 0, 0]
        with self.assertRaises(ValueError):
            iprep.filters.crop_vol(self.data_vol, coords)

        coords = [0, 0, 0, 0, 0]
        with self.assertRaises(ValueError):
            iprep.filters.crop_vol(self.data_vol, coords)

        coords = [0, 0, 10, 10]
        with self.assertRaises(ValueError):
            iprep.filters.crop_vol(self.data_vol[1, :, :], coords)


    def test_crop_empty(self):
        import IMAT.prep as iprep

        coords = [0, 0, 0, 0]
        cropped = iprep.filters.crop_vol(self.data_vol, coords)

        self.assertTrue(isinstance(self.data_vol, np.ndarray))
        self.assertTrue(isinstance(cropped, np.ndarray),
                        msg="the result of cropping with empty (0) coordinates should be a "
                        "numpy array")

        self.assertEqual(cropped.shape, self.data_vol.shape,
                         msg="the result of cropping with empty (0) coordinates should have "
                         "the appropriate dimensions. Found {0} instead of {1}".
                         format(cropped.shape, self.data_vol.shape))

        peek_positions = [[3,57], [57, 4]]
        for pos in peek_positions:
            (pos_x, pos_y) = pos
            cropped_coord_equals = cropped[:, pos_y, pos_x] == self.data_vol[:, pos_y, pos_x]
            self.assertTrue(cropped_coord_equals.all(),
                            msg="cropping should not change values (found differences at "
                            "coordinates: {0}, {1})".format(pos_x, pos_y))

    def test_crop_coordinates_skips(self):
        import IMAT.prep as iprep

        coords = [50, 40, 0, 0]
        cropped = iprep.filters.crop_vol(self.data_vol, coords)

        self.assertTrue(isinstance(self.data_vol, np.ndarray))
        self.assertTrue(isinstance(cropped, np.ndarray),
                        msg="the result of cropping with inconsistent) coordinates should be a "
                        "numpy array")

        self.assertEqual(cropped.shape, self.data_vol.shape,
                         msg="the result of cropping with inconsistent coordinates should have "
                         "the appropriate dimensions (same as original). Found {0} instead of {1}".
                         format(cropped.shape, self.data_vol.shape))

    def test_crop_ok(self):
        import IMAT.prep as iprep

        coords = [2, 2, 100, 100]
        cropped = iprep.filters.crop_vol(self.data_vol, coords)

        self.assertTrue(isinstance(self.data_vol, np.ndarray))
        self.assertTrue(isinstance(cropped, np.ndarray),
                        msg="the result of cropping should be a numpy array")

        expected_shape = (self.data_vol.shape[0], coords[3] - coords[1] + 1, coords[2] - coords[0] + 1)
        self.assertEqual(cropped.shape, expected_shape,
                         msg="the result of cropping should have the appropriate dimensions")

        orig_cropped_equals = self.data_vol[:, coords[1]:coords[3]+1, coords[0]:coords[2]+1] == cropped
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
                                    msg="Circular mask: wrong value found outside. Expected: {0}, "
                                    "found: {1}".format(some_val, peek_out))

        for coords in [(3, 200, 200), (2, 50, 20), (1, 300, 100), (0, 400, 200)]:
            peek_in = masked[coords]
            expected_val = self.data_vol[coords]
            self.assertAlmostEquals(peek_in, expected_val,
                                    msg="Circular mask: wrong value found inside. Expected: "
                                    "{0}, found: {1}".format(expected_val, peek_in))

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

    def test_remove_stripes_err(self):
        import IMAT.prep as iprep

        method = 'wavelet-fourier'

        # expect pywt to be missing
        with self.assertRaises(ImportError):
            iprep.filters.remove_stripes_ring_artifacts(self.data_vol, method)

    def disabled_test_remove_stripes_ok(self):
        """
        This tests the local implementation of the wavelet-fourier stripe removal
        method. It will only run if pywt is available
        """
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

    def test_config_pre(self):
        """
        Basic consistency check of the pre-processing config and some default
        settings
        """
        import IMAT.tomorec.configs as cfgs

        pre = cfgs.PreProcConfig()
        self.assertEquals(pre.input_dir, None)
        self.assertEquals(pre.input_dir_flat, None)
        self.assertEquals(pre.input_dir_dark, None)
        self.assertEquals(pre.max_angle, 360)
        self.assertEquals(pre.rotation, -1)
        self.assertEquals(pre.normalize_flat_dark, True)
        self.assertEquals(pre.crop_coords, None)
        self.assertEquals(pre.scale_down, 0)
        self.assertEquals(pre.median_filter_size, 3)
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

    def test_rotate_raises(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()
        alg_conf = cfgs.ToolAlgorithmConfig()
        post_conf = cfgs.PostProcConfig()
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)

        pre_conf.rotation = 1
        # absolutely invalid data
        with self.assertRaises(ValueError):
            cmd.rotate_stack([], pre_conf)

        # wrong data type or dimensions (for samples / flats / darks
        with self.assertRaises(ValueError):
            cmd.rotate_stack(np.ones((3, 2)), pre_conf)

        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, pre_conf, [1])

        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, pre_conf, None, np.zeros((3, 3)))

        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, pre_conf, None, [0, 1])

        # invalid configurations
        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, None)

        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, [])

        with self.assertRaises(ValueError):
            cmd.rotate_stack(self.data_vol, conf)

    def test_rotate_imgs_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()
        pre_conf.rotation = 1

        (rotated, white, dark) = cmd.rotate_stack(self.data_vol, pre_conf)
        np.testing.assert_allclose(rotated, self.data_vol,
                                   err_msg="Epected rotated data volume not to change when "
                                   "the rotation option is disabled")
        self.assertEquals(white, None, msg="When the white stack is None, it should still be "
                          "None after rotation")
        self.assertEquals(dark, None, msg="When the dark stack is None, it should still be "
                          "None after rotation")

        pre_conf.rotation = 1
        (rotated_90, white, dark) = cmd.rotate_stack(self.data_vol, pre_conf)
        coordinates =  [(3, 510, 0), (2,2,3), (1,0,0), (0, 500, 5)]
        expected_vals = [-0.810005187988, 0.656108379364, -0.531451165676, 0.430478185415]
        for coord, expected in zip(coordinates, expected_vals):
            real_val = rotated_90[coord]
            self.assertAlmostEquals(real_val, expected,
                                    msg="Rotation: wrong value found at coordinate {0},{1},{2}. "
                                    "Expected: {3}, found: {4}".format(coord[0], coord[1], coord[2],
                                                                       expected, real_val))

    def test_normalize_air_raises(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()
        alg_conf = cfgs.ToolAlgorithmConfig()
        post_conf = cfgs.PostProcConfig()
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)

        # absolutely invalid data
        with self.assertRaises(ValueError):
            cmd.normalize_air_region([], pre_conf)

        # wrong data dimensions
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(np.ones((3, 2)), pre_conf)

        # invalid configurations
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, alg_conf)

        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, post_conf)

        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, conf)

        # wrong air-regions
        pre_conf.normalize_air_region = [3]
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, pre_conf)

        pre_conf.normalize_air_region = (3, 0, 100, 10)
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, pre_conf)

        pre_conf.normalize_air_region = [3, 0, 100]
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, pre_conf)

        pre_conf.normalize_air_region = [0.1, 0, 50, 50]
        with self.assertRaises(ValueError):
            cmd.normalize_air_region(self.data_vol, pre_conf)

    def test_normalize_air_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()

        normalized = cmd.normalize_air_region(self.data_vol, pre_conf)
        np.testing.assert_allclose(normalized, self.data_vol,
                                   err_msg="Epected normalized data volume not to changed")

    def test_normalize_flat_raises(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()
        alg_conf = cfgs.ToolAlgorithmConfig()
        post_conf = cfgs.PostProcConfig()
        conf = cfgs.ReconstructionConfig(pre_conf, alg_conf, post_conf)

        # absolutely invalid data
        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark([], pre_conf, np.ones((10, 23)), None)

        # wrong data dimensions
        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark(np.ones((3, 2)), pre_conf, np.ones((10, 23)), None)

        # wrong dimensions of the flat image
        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark(self.data_vol, pre_conf, np.ones((10, 23)), None)

        # invalid configurations
        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark(self.data_vol, alg_conf, None, None)

        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark(self.data_vol, post_conf, None, None)

        with self.assertRaises(ValueError):
            cmd.normalize_flat_dark(self.data_vol, conf, None, None)

    def test_normalize_flat_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        import IMAT.tomorec.configs as cfgs
        pre_conf = cfgs.PreProcConfig()

        # ignored, with just info message
        norm = cmd.normalize_flat_dark(self.data_vol, pre_conf, None, None)

        # ignored, with just info message
        norm = cmd.normalize_flat_dark(self.data_vol, pre_conf, 45, None)

        for img_idx in range(0, self.data_vol.shape[0]):
            fake_white = self.data_vol[img_idx, :, :]
            norm = cmd.normalize_flat_dark(self.data_vol, pre_conf, fake_white, None)
            np.testing.assert_allclose(norm[img_idx, : :], np.ones(fake_white.shape),
                                       err_msg="Epected normalized data volume not to changed "
                                       "wheh using fake flat image, with index {0}".format(img_idx))

    def test_read_stack_fails_ok(self):
        import IMAT.tomorec.reconstruction_command as cmd
        cmd = cmd.ReconstructionCommand()

        # The images are not .tiff but .fits so the loader should raise
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
        self._success = res.wasSuccessful()

    def validate(self):
        return self._success
