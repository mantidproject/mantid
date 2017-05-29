from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class ConfigsTest(unittest.TestCase):
    """
    There's not much to test on the configs as they are simple containers for the arguments.
    """

    def test_preproc(self):
        from configs.preproc_config import PreProcConfig
        preproc = PreProcConfig()
        self._compare_dict_to_str(preproc.__dict__, str(preproc))

    def test_postproc(self):
        from configs.postproc_config import PostProcConfig
        postproc = PostProcConfig()
        self._compare_dict_to_str(postproc.__dict__, str(postproc))

    def test_functional(self):
        from configs.functional_config import FunctionalConfig
        fc = FunctionalConfig()
        self._compare_dict_to_str(fc.__dict__, str(fc))

        # running without --input-path
        fc.input_path = '23'
        npt.assert_raises(ValueError, fc.update, fc)
        # running --save-preproc without output path
        fc.save_preproc = True
        fc.output_path = None
        npt.assert_raises(ValueError, fc.update, fc)
        # remove the output path raise
        fc.output_path = 'some/dir'
        # running recon without COR
        npt.assert_raises(ValueError, fc.update, fc)

    def _compare_dict_to_str(self, class_dict, class_str):
        dictlen = len(class_dict)
        strlen = len(class_str.split('\n'))
        self.assertEquals(
            dictlen, strlen,
            "Different size of __dict__({0}) and __str__({1}). Some of the attributes are not documented!".
            format(dictlen, strlen))


if __name__ == '__main__':
    unittest.main()
