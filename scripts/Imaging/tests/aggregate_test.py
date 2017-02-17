from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class AggregateTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(AggregateTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(r)

    def create_saver(self):
        from imgdata.saver import Saver
        return Saver(self.h.config)

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

        try:
            self.delete_files(prefix='aggregated')
        except OSError:
            # no reconstructed images were saved
            pass

        try:
            self.delete_files(prefix='aggregate')
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
                print(base_name + str(i) + '.' + file_format)
                filenames.append(base_name + str(i) + '.' + file_format)

            for f in filenames:
                self.assertTrue(os.path.isfile(f))

        else:
            filename = base_name + '_stack.' + file_format
            self.assertTrue(os.path.isfile(filename))

    def test_aggregate_single_folder_sum(self):
        self.do_aggregate_single_folder('fits', 'fits', 'sum')

    def test_aggregate_single_folder_avg(self):
        self.do_aggregate_single_folder('fits', 'fits', 'avg')

    def do_aggregate_single_folder(self,
                                   img_format,
                                   convert_format,
                                   mode='sum'):
        # this just converts between the formats, but not NXS!
        # create some images
        import numpy as np
        images = th.gen_img_shared_array()
        if 'sum' == mode:
            expected = images.sum(axis=0, dtype=np.float32)
        else:
            expected = images.mean(axis=0, dtype=np.float32)

        aggregate_angles = 7
        stack = False
        parallel = False
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            aggregate_path = os.path.dirname(f.name) + '/aggregate'
            # save out 5 'angles'
            for i in range(aggregate_angles):
                angle_path = aggregate_path + '/angle' + str(i)
                saver._output_path = angle_path
                saver._img_format = img_format
                saver._data_as_stack = stack
                saver._overwrite_all = True
                # do the actual saving out, directories will be created here
                saver.save(images, angle_path, 'out_angle', flat, dark)

            # aggregate them
            from aggregate import aggregate
            conf = self.h.config
            conf.func.aggregate = ['0', '10', mode]
            # select angles 0 - 4 (aggregate_angles is 5 so we subtract 1)
            conf.func.aggregate_angles = ['0', str(aggregate_angles - 1)]
            conf.func.aggregate_single_folder_output = True
            conf.func.input_path = aggregate_path
            conf.func.in_format = saver._img_format
            aggregate_output_path = os.path.dirname(f.name) + '/aggregated'
            conf.func.output_path = aggregate_output_path
            conf.func.overwrite_all = True
            conf.func.convert_prefix = 'aggregated'
            aggregate.execute(conf)

            # load them back
            # compare data to original
            from imgdata import loader
            # this does not load any flats or darks as they were not saved out
            sample, flat_loaded, dark_loaded = loader.load(
                aggregate_output_path,
                None,
                None,
                saver._img_format,
                parallel_load=parallel,
                h=self.h)

            for i in sample:
                th.assert_equals(i, expected)

            self.assert_files_exist(
                aggregate_output_path + '/out_' + mode + '_0_10_',
                saver._img_format, saver._data_as_stack, aggregate_angles)

    def test_aggregate_not_single_folder_sum(self):
        self.do_aggregate_not_single_folder('fits', 'fits', 'sum')

    def test_aggregate_not_single_folder_avg(self):
        self.do_aggregate_not_single_folder('fits', 'fits', 'avg')

    def do_aggregate_not_single_folder(self,
                                       img_format,
                                       convert_format,
                                       mode='sum'):
        # this just converts between the formats, but not NXS!
        # create some images
        import numpy as np
        images = th.gen_img_shared_array()
        if 'sum' == mode:
            expected = images.sum(axis=0, dtype=np.float32)
        else:
            expected = images.mean(axis=0, dtype=np.float32)
        aggregate_angles = 7
        stack = False
        parallel = False
        flat = None
        dark = None
        saver = self.create_saver()
        import tempfile
        import os
        with tempfile.NamedTemporaryFile() as f:
            aggregate_path = os.path.dirname(f.name) + '/aggregate'
            # keep the angle paths for the load later
            angle_paths = []
            # save out 5 'angles'
            for i in range(aggregate_angles):
                angle_paths.append(aggregate_path + '/angle' + str(i))
                saver._output_path = angle_paths[i]
                saver._img_format = img_format
                saver._data_as_stack = stack
                saver._overwrite_all = True
                # do the actual saving out, directories will be created here
                saver.save(images, angle_paths[i], 'out_angle', flat, dark)

            # aggregate them
            from aggregate import aggregate
            conf = self.h.config
            conf.func.aggregate = ['0', '10', mode]
            # select angles 0 - 4 (starts from 0 so -1)
            conf.func.aggregate_angles = ['0', str(aggregate_angles - 1)]
            conf.func.aggregate_single_folder_output = False
            conf.func.input_path = aggregate_path
            conf.func.in_format = saver._img_format
            aggregate_output_path = os.path.dirname(f.name) + '/aggregated'
            conf.func.output_path = aggregate_output_path
            conf.func.overwrite_all = True
            conf.func.convert_prefix = 'aggregated'
            aggregate.execute(conf)

            # load them back
            # compare data to original
            from imgdata import loader
            # this does not load any flats or darks as they were not saved out
            for i in range(aggregate_angles):
                angle_path = os.path.dirname(
                    f.name) + '/aggregated/angle_' + mode + str(i)

                sample, flat_loaded, dark_loaded = loader.load(
                    angle_path,
                    None,
                    None,
                    saver._img_format,
                    parallel_load=parallel,
                    h=self.h)

                for i in sample:
                    th.assert_equals(i, expected)

                # we leave it as '0_1' here and leave the num_images parameter to one
                # this means that an additional 0 will be appended, and will get
                # the correct file name. This is a workaroud to not specify
                # an additional if statement
                self.assert_files_exist(angle_path + '/out_' + mode + '0_1',
                                        saver._img_format,
                                        saver._data_as_stack)

        #TODO aggregate ALL angles test


if __name__ == '__main__':
    unittest.main()
