import os
import unittest
import stresstesting

from mantid.simpleapi import ImggAggregateWavelengths
from mantid import config

class ImagingAggregateTests(unittest.TestCase):
    """
    Tests aggregation of wavelengths/energy bands, which at the moment
    is done by the algorithm ImggAggregateWavelengths.
    """
    def setUp(self):
        # example files to make a stack of images
        data_root = config['datasearch.directories'].split(';')[0]
        self._raw_files_dir = os.path.join(data_root, 'wavelength_dependent_images')
        self._raw_files_subdirs = [ 'wavelength_dependent_images/angle0/',
                                    'wavelength_dependent_images/angle1/',
                                    'wavelength_dependent_images/angle2/',
                                    'wavelength_dependent_images/angle5/'
                                  ]

        self._out_dir = os.path.join(os.getcwd(), 'summed_wavelengths_metals')

        self._expected_out_dir = 'bands_uniform_idx_0_to_6'
        self._expected_out_fnames = [
            'sum_projection_00000_bands_idx_0_to_6.fits',
            'sum_projection_00001_bands_idx_0_to_3.fits',
            'sum_projection_00002_bands_idx_0_to_2.fits',
            'sum_projection_00003_bands_idx_0_to_1.fits'
            ]
        self._expected_out_fnames = [os.path.join(self._expected_out_dir, exp)\
                                     for exp in self._expected_out_fnames]

    def tearDown(self):
        pass

    def _cleanup_dirs_files(self):
        # All these should be there at the end of the test
        for fname in self._expected_out_fnames:
            fpath = os.path.join(self._out_dir, fname)
            os.remove(fpath)
        os.rmdir(os.path.join(self._out_dir, self._expected_out_dir))
        os.rmdir(self._out_dir)

    def test_input_output_path_errors(self):
        self.assertRaises(ValueError,
                          ImggAggregateWavelengths,
                          InputPath = self._raw_files_dir + '_should_fail',
                          OutputPath = self._out_dir)

    def test_run_ok(self):
        if not os.path.exists(self._out_dir):
            os.mkdir(self._out_dir)

        num_proj, num_bands = ImggAggregateWavelengths(InputPath = self._raw_files_dir,
                                                       OutputPath = self._out_dir)

        self.assertEquals(num_proj, 4)
        self.assertEquals(num_bands, 6)
        self._cleanup_dirs_files()

# Runs the unittest tests defined above in the mantid stress testing framework
class ImagingAggregateWavelengths(stresstesting.MantidStressTest):

    _success = False

    def __init__(self, *args, **kwargs):
        # super(ImagingAggregateWavelengths, self).__init__(*args, **kwargs)
        # old-style
        stresstesting.MantidStressTest.__init__(self, *args, **kwargs)
        self._success = False

        self._raw_in_files = [ 'wavelength_dependent_images/angle0/foo.txt',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00000.fits',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00001.fits',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00002.fits',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00003.fits',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00004.fits',
                               'wavelength_dependent_images/angle0/LARMOR00005471_Metals_000_00005.fits',
                               'wavelength_dependent_images/angle1/LARMOR00005329_Metals_000_00000.fits',
                               'wavelength_dependent_images/angle1/LARMOR00005329_Metals_000_00001.fits',
                               'wavelength_dependent_images/angle1/LARMOR00005329_Metals_000_00002.fits',
                               'wavelength_dependent_images/angle2/LARMOR00005330_Metals_000_01343.fits',
                               'wavelength_dependent_images/angle2/LARMOR00005330_Metals_000_01344.fits',
                               'wavelength_dependent_images/angle5/LARMOR00005333_Metals_000_00690.fits',
                               'wavelength_dependent_images/angle5/bogus.txt',
                               'wavelength_dependent_images/angle5/more_bogus',
                             ]

    def requiredFiles(self):
        return set(self._raw_in_files)

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(ImagingAggregateTests, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        self._success = res.wasSuccessful()

    def validate(self):
        return self._success
