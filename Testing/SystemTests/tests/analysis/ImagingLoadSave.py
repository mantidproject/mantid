import os
import unittest
import stresstesting

import mantid.simpleapi as msapi
from mantid import config

def required_larmor_test_files():
    data_root = config['datasearch.directories'].split(';')[0]
    fits_files_dir = os.path.join(data_root, 'wavelength_dependent_images')

    # subdir and filename components
    fits_files_comps = [
        ('angle0', 'LARMOR00005471_Metals_000_00000.fits'),
        ('angle0', 'LARMOR00005471_Metals_000_00001.fits'),
        ('angle0', 'LARMOR00005471_Metals_000_00002.fits'),
        ('angle0', 'LARMOR00005471_Metals_000_00003.fits'),
        ('angle0', 'LARMOR00005471_Metals_000_00004.fits'),
        ('angle0', 'LARMOR00005471_Metals_000_00005.fits'),
        ('angle1', 'LARMOR00005329_Metals_000_00000.fits'),
        ('angle1', 'LARMOR00005329_Metals_000_00001.fits'),
        ('angle1', 'LARMOR00005329_Metals_000_00002.fits'),
        ('angle2', 'LARMOR00005330_Metals_000_01343.fits'),
        ('angle2', 'LARMOR00005330_Metals_000_01344.fits'),
        ('angle5', 'LARMOR00005333_Metals_000_00690.fits')
        ]

    file_paths = [os.path.join(fits_files_dir, comps[0], comps[1])\
                  for comps in fits_files_comps]

    return file_paths

#pylint: disable=too-many-public-methods
class ImagingLoadSaveTests(unittest.TestCase):
    """
    Tests load/save images. This is just around FITS format at the
    moment. There should be tests on other formats and conversions
    when we have the algorithms Load/SaveImage.
    """

    def setUp(self):
        # Sharing some files with the ImggAggregateWavelengths system test
        self._fits_paths = required_larmor_test_files()


    def test_load_all_indiv(self):
        """
        Load a few files with different options and check they load correctly
        """

        group_name = 'all_fits_rect'
        for fpath in self._fits_paths:
            group = msapi.LoadFITS(Filename=fpath, LoadAsRectImg=True, OutputWorkspace=group_name)

        group_norect_name = 'all_fits_norect'
        for fpath in self._fits_paths:
            group_norect = msapi.LoadFITS(Filename=fpath, LoadAsRectImg=False, OutputWorkspace=group_norect_name)

        self.assertEquals(group.size(), group_norect.size())
        for idx in range(0, group.size()):
            img_a = group.getItem(idx)
            size_a = img_a.getNumberHistograms() * img_a.blocksize()
            img_b = group_norect.getItem(idx)
            size_b = img_b.getNumberHistograms() * img_b.blocksize()
            self.assertEquals(size_a, size_b)
            self.assertEquals(img_a.getTitle(), img_b.getTitle())

            for row in [0, 100, 200, 300, 400, 511]:
                self.assertEquals(img_a.readY(row)[0], img_b.readY(row*img_a.blocksize() + 0))

        msapi.DeleteWorkspace(group_name)
        msapi.DeleteWorkspace(group_norect_name)

    def test_load_all_at_once(self):
        """
        A batch load, as when stacks are loaded at once
        """
        all_filepaths = ','.join(self._fits_paths)
        group = msapi.LoadFITS(Filename=all_filepaths, LoadAsRectImg=True)

        self.assertEquals(group.size(), len(self._fits_paths))

        msapi.DeleteWorkspace(group)

    def test_load_save_load(self):
        """
        Check that nothing is lost in a Load/Save/Load cycle
        """
        group_name = 'all_indiv'
        for fpath in self._fits_paths:
            msapi.LoadFITS(Filename=fpath, LoadAsRectImg=True, OutputWorkspace=group_name)
        group = msapi.mtd[group_name]

        for idx in range(0, group.size()):
            # Save
            img_loaded = group.getItem(idx)
            save_filename = 'test.fits'
            msapi.SaveFITS(Filename=save_filename, InputWorkspace=img_loaded)

            # Re-load and compare
            reload_name = 'indiv_fits_reloaded'
            grp_reloaded = msapi.LoadFITS(Filename=save_filename, LoadAsRectImg=True, OutputWorkspace=reload_name)
            self.assertEquals(1, grp_reloaded.size())
            img_reloaded = grp_reloaded.getItem(0)

            self.assertEquals(img_loaded.getNumberHistograms(), img_reloaded.getNumberHistograms())
            self.assertEquals(img_loaded.blocksize(), img_reloaded.blocksize())

            (comp_result, tbl_messages) = msapi.CompareWorkspaces(img_loaded, img_reloaded)
            num_rows = tbl_messages.rowCount()
            txt_messages = [tbl_messages.row(idx) for idx in range(0, num_rows)]
            self.assertTrue(comp_result,
                            "Workspace comparison failed. Details: {0}".format(txt_messages))
            msapi.DeleteWorkspace(grp_reloaded)

        msapi.DeleteWorkspace(group_name)

# Runs the unittest tests defined above in the mantid stress testing framework
class ImagingAggregateWavelengths(stresstesting.MantidStressTest):

    _success = False

    def __init__(self, *args, **kwargs):
        # super(ImagingAggregateWavelengths, self).__init__(*args, **kwargs)
        # old-style
        stresstesting.MantidStressTest.__init__(self, *args, **kwargs)
        self._success = False

    def requiredFiles(self):
        return set(required_larmor_test_files())

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(ImagingLoadSaveTests, "test") )
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        self._success = res.wasSuccessful()

    def validate(self):
        return self._success
