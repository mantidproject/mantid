from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class ConvertTest(unittest.TestCase):
    """
    This test actually tests the saver and loader modules too, but
    it isn't focussed on them
    """

    def __init__(self, *args, **kwargs):
        super(ConvertTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        self.config = ReconstructionConfig.empty_init()
        self.config.func.verbosity = 0

    def create_saver(self):
        from imgdata.saver import Saver
        return Saver(self.config)

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

        try:
            self.delete_files(prefix='converted')
        except OSError:
            # no reconstructed images were saved
            pass

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
                filenames.append(base_name + str(i) + '.' + file_format)

            for f in filenames:
                self.assertTrue(os.path.isfile(f))

        else:
            filename = base_name + '.' + file_format
            self.assertTrue(os.path.isfile(filename))

    def test_convert_fits_fits_nostack(self):
        self.do_convert(
            img_format='fits',
            convert_format='fits',
            stack=False,
            parallel=False)

    def test_convert_fits_tiff_nostack(self):
        self.do_convert(
            img_format='fits',
            convert_format='tiff',
            stack=False,
            parallel=False)

    def do_convert(self, img_format, convert_format, stack, parallel=False):
        # this just converts between the formats, but not NXS!
        # create some images
        images = th.gen_img_shared_array()
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            saver._output_path = os.path.dirname(f.name)
            saver._img_format = img_format
            saver._save_preproc = True
            saver._data_as_stack = stack
            saver._overwrite_all = True

            # save them out
            saver.save_preproc_images(images)

            preproc_output_path = saver._output_path + '/pre_processed'

            # convert them
            from convert import convert
            conf = self.config
            conf.func.input_path = preproc_output_path
            conf.func.in_format = saver._img_format
            converted_output_path = saver._output_path + '/converted'
            conf.func.output_path = converted_output_path
            conf.func.out_format = convert_format
            conf.func.data_as_stack = stack
            conf.func.convert_prefix = 'converted'
            convert.execute(conf)

            # load them back
            # compare data to original
            from imgdata import loader
            # this odes not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                converted_output_path,
                None,
                None,
                convert_format,
                parallel_load=parallel)

            th.assert_equals(sample, images)
            th.assert_equals(flat_loaded, flat)
            th.assert_equals(dark_loaded, dark)

            self.assert_files_exist(converted_output_path + '/converted',
                                    convert_format, saver._data_as_stack,
                                    images.shape[0])

    def test_convert_fits_nxs_stack(self):
        # NXS is only supported for stack
        self.do_convert_to_nxs(
            img_format='fits', convert_format='nxs', stack=True)

    def test_convert_tiff_nxs_stack(self):
        # NXS is only supported for stack
        self.do_convert_to_nxs(
            img_format='tiff', convert_format='nxs', stack=True)

    def do_convert_to_nxs(self, img_format, convert_format, stack):
        # this saves out different formats to a nxs stack
        # create some images
        parallel = False
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
            saver._save_preproc = True
            saver._overwrite_all = True

            # save them out
            saver.save_preproc_images(images)
            preproc_output_path = saver._output_path + '/pre_processed'

            # convert them
            from convert import convert
            conf = self.config
            conf.func.input_path = preproc_output_path
            conf.func.in_format = saver._img_format
            converted_output_path = saver._output_path + '/converted'
            conf.func.output_path = converted_output_path
            conf.func.out_format = convert_format
            conf.func.data_as_stack = stack
            conf.func.convert_prefix = 'converted'
            convert.execute(conf)

            # load them back
            # compare data to original
            from imgdata import loader
            # this odes not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                converted_output_path,
                None,
                None,
                convert_format,
                parallel_load=parallel)

            th.assert_equals(sample, images)
            th.assert_equals(flat_loaded, flat)
            th.assert_equals(dark_loaded, dark)

            self.assert_files_exist(converted_output_path + '/converted',
                                    convert_format, saver._data_as_stack,
                                    images.shape[0])

    def test_convert_nxs_fits_nostack(self):
        self.do_convert_from_nxs(
            img_format='nxs', convert_format='fits', stack=False)

    def test_convert_nxs_tiff_nostack(self):
        self.do_convert_from_nxs(
            img_format='nxs', convert_format='tiff', stack=False)

    def do_convert_from_nxs(self, img_format, convert_format, stack):
        # this saves out a nexus stack and then loads it in different formats
        # create some images
        parallel = False
        images = th.gen_img_shared_array()
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            saver._output_path = os.path.dirname(f.name)
            saver._img_format = img_format
            # force saving out as STACK because we're saving NXS files
            saver._data_as_stack = True
            saver._save_preproc = True
            saver._overwrite_all = True

            # save them out
            saver.save_preproc_images(images)
            preproc_output_path = saver._output_path + '/pre_processed'

            # convert them
            from convert import convert
            conf = self.config
            conf.func.input_path = preproc_output_path
            conf.func.in_format = saver._img_format
            converted_output_path = saver._output_path + '/converted'
            conf.func.output_path = converted_output_path
            conf.func.out_format = convert_format
            conf.func.data_as_stack = stack
            conf.func.convert_prefix = 'converted'
            convert.execute(conf)
            # load them back
            # compare data to original
            from imgdata import loader
            # this does not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                converted_output_path,
                None,
                None,
                convert_format,
                parallel_load=parallel)

            th.assert_equals(sample, images)
            # None because when loading images the flat and dark are not considered
            # The flat and dark are only returned from load if loading a nexus file, or the paths are provided,
            # and in this case we're not loading nexus, and there are no paths provided!
            th.assert_equals(flat_loaded, None)
            th.assert_equals(dark_loaded, None)

            self.assert_files_exist(converted_output_path + '/converted',
                                    convert_format, stack, images.shape[0])


if __name__ == '__main__':
    unittest.main()
