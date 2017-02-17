from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class DataTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(DataTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        self.r = ReconstructionConfig.empty_init()
        self.r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(self.r)

    def create_saver(self):
        from imgdata.saver import Saver
        return Saver(self.r)

    def assert_files_exist(self,
                           base_name,
                           file_format,
                           stack=True,
                           num_images=1):
        import os

        if not stack:
            # generate a list of filenames with 000000 numbers appended
            filenames = []
            for i in range(num_images):
                filenames.append(base_name + '00000' + str(i) + '.' +
                                 file_format)

            for f in filenames:
                self.assertTrue(os.path.isfile(f))

        else:
            filename = base_name + '_stack.' + file_format
            self.assertTrue(os.path.isfile(filename))

    def delete_files(self, prefix=''):
        import tempfile
        import os
        import shutil
        with tempfile.NamedTemporaryFile() as f:
            from imgdata.loader import get_file_names
            full_path = os.path.join(os.path.dirname(f.name), prefix)
            shutil.rmtree(full_path)

    def tearDown(self):
        """
        Cleanup, Make sure all files are deleted from tmp
        """
        try:
            self.delete_files(prefix='pre_processed')
        except OSError:
            # no preprocessed images were saved
            pass
        try:
            self.delete_files(prefix='reconstructed')
        except OSError:
            # no reconstructed images were saved
            pass

    def test_preproc_fits_par(self):
        self.do_preproc('fits', stack=False, parallel=True)

    def test_preproc_fits_seq(self):
        self.do_preproc('fits', stack=False, parallel=False)

    def test_preproc_fits_stack_par(self):
        self.do_preproc('fits', stack=True, parallel=True)

    def test_preproc_fits_stack_seq(self):
        self.do_preproc('fits', stack=True, parallel=False)

    def test_preproc_tiff_par(self):
        self.do_preproc('tiff', stack=False, parallel=True)

    def test_preproc_tiff_seq(self):
        self.do_preproc('tiff', stack=False, parallel=False)

    def test_preproc_tiff_stack_par(self):
        self.do_preproc('tiff', stack=True, parallel=True)

    def test_preproc_tiff_stack_seq(self):
        self.do_preproc('tiff', stack=True, parallel=False)

    def do_preproc(self, img_format, stack, parallel=False):
        images = th.gen_img_shared_array()
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            saver._output_path = os.path.dirname(f.name)
            saver._img_format = img_format
            saver._data_as_stack = stack
            saver._overwrite_all = True

            saver.save_preproc_images(images, flat, dark)

            preproc_output_path = saver._output_path + '/pre_processed/'

            from imgdata import loader
            # this odes not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                preproc_output_path,
                None,
                None,
                saver._img_format,
                parallel_load=parallel,
                h=self.h)

            th.assert_equals(sample, images)
            th.assert_equals(flat_loaded, flat)
            th.assert_equals(dark_loaded, dark)

            self.assert_files_exist(preproc_output_path + 'out_preproc_image',
                                    saver._img_format, saver._data_as_stack,
                                    images.shape[0])

    def test_save_nxs_par(self):
        self.do_preproc_nxs(parallel=True)

    def test_save_nxs_seq(self):
        self.do_preproc_nxs(parallel=False)

    def do_preproc_nxs(self, img_format='nxs', stack=True, parallel=False):
        images = th.gen_img_shared_array()
        # this is different from do_preproc as we need to
        # save out flat and dark images, and they will be loaded
        # back in
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            saver._output_path = os.path.dirname(f.name)
            saver._img_format = img_format
            saver._data_as_stack = stack
            saver._overwrite_all = True

            saver.save_preproc_images(images, flat, dark)

            preproc_output_path = saver._output_path + '/pre_processed/'

            from imgdata import loader
            # this odes not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                preproc_output_path,
                None,
                None,
                saver._img_format,
                parallel_load=parallel,
                h=self.h)

            th.assert_equals(sample, images)
            th.assert_equals(flat_loaded, flat)
            th.assert_equals(dark_loaded, dark)

            self.assert_files_exist(preproc_output_path + 'out_preproc_image',
                                    saver._img_format, saver._data_as_stack,
                                    images.shape[0])

    def test_do_recon_fits(self):
        self.do_recon(img_format='fits', stack=True, horiz_slices=False)

    def test_do_recon_tiff(self):
        self.do_recon(img_format='tiff', stack=True, horiz_slices=False)

    def test_do_recon_fits_horiz(self):
        self.do_recon(img_format='fits', stack=True, horiz_slices=True)

    def test_do_recon_tiff_horiz(self):
        self.do_recon(img_format='tiff', stack=True, horiz_slices=True)

    def do_recon(self, img_format, stack, horiz_slices=False):
        images = th.gen_img_shared_array()
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            saver._output_path = os.path.dirname(f.name)
            saver._img_format = img_format
            saver._data_as_stack = stack
            saver._overwrite_all = True
            saver._save_horiz_slices = horiz_slices

            saver.save_recon_output(images)

            recon_output_path = saver._output_path + '/reconstructed/'

            self.assert_files_exist(recon_output_path + 'recon_slice',
                                    saver._img_format, saver._data_as_stack,
                                    images.shape[0])

            if horiz_slices:
                self.assert_files_exist(
                    recon_output_path + 'horiz_slices/recon_horiz',
                    saver._img_format, saver._data_as_stack, images.shape[0])


if __name__ == '__main__':
    unittest.main()
