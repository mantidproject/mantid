# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
import numpy as np
import unittest
import mantid
from mantid.simpleapi import ExportSpectraMask, DeleteWorkspace


class ExportSpectraMaskTest(unittest.TestCase):
    def __init__(self, method_name):
        unittest.TestCase.__init__(self, method_name)

        self.this_path = os.path.dirname(os.path.realpath(__file__))
        test_path = self.this_path
        for i in range(0, 4):
            test_path, _ = os.path.split(test_path)
        self.test_mod_path = os.path.join(test_path, "plugins/algorithms")
        sys.path.append(self.test_mod_path)

        self.test_files_path = mantid.config.getString("defaultsave.directory")
        self.test_file = os.path.join(self.test_files_path, "test_mask_file")

    def setUp(self):
        """ """

        self.masks = [1, 4, 8, 10, 199, 200]
        if "test_ws" not in mantid.api.mtd:
            test_ws = mantid.simpleapi.CreateSampleWorkspace()
            test_ws.maskDetectors(self.masks)
        if not hasattr(self, "write_f"):
            import ExportSpectraMask as amc

            self.test_write_f = amc.writeISISmasks
            self.test_export_f = amc.export_masks

        return super(ExportSpectraMaskTest, self).setUp()

    def tearDown(self):
        if "test_ws" in mantid.api.mtd:
            DeleteWorkspace("test_ws")
        test_file = self.test_file + ".msk"
        if os.path.isfile(test_file):
            os.remove(test_file)
        return super(ExportSpectraMaskTest, self).tearDown()

    def test_writeISISmasks(self):
        masks = [1, 20, 30, 40, 41, 42, 43]

        test_file = self.test_file

        # Test single row writing
        self.test_write_f(test_file, masks)

        self.assertTrue(os.path.isfile(test_file + ".msk"))
        with open(test_file + ".msk", "r") as tf:
            for line in tf:
                self.assertEqual(line, "1 20 30 40-43\n")
        os.remove(test_file + ".msk")

        # Test multiple row writing
        test_file = test_file + ".msk"
        masks = masks + [46, 47, 49, 50]
        self.test_write_f(test_file, masks, 4)
        self.assertTrue(os.path.isfile(test_file))

        sample = ["1 20 30 40\n", "41-43 46-47\n", "49-50\n"]
        with open(test_file, "r") as tf:
            for line, s in zip(tf, sample):
                self.assertEqual(line, s)

    def test_exportMasks(self):
        r_masks = self.test_export_f("test_ws", "", True)
        self.assertEqual(self.masks, r_masks)

    def test_ExportAlgosWork(self):
        r_masks = ExportSpectraMask("test_ws", ExportMaskOnly=True)
        self.assertTrue((np.array(self.masks) == r_masks).all())

        test_file = self.test_file + ".msk"
        self.assertFalse(os.path.isfile(test_file))

        r_masks = ExportSpectraMask("test_ws", Filename=test_file)
        self.assertTrue(os.path.isfile(test_file))
        self.assertTrue((np.array(self.masks) == r_masks).all())

        default_tf = os.path.join(mantid.config.getString("defaultsave.directory"), "test_ws.msk")
        ExportSpectraMask("test_ws")
        self.assertTrue(os.path.isfile(default_tf))
        os.remove(default_tf)


if __name__ == "__main__":
    unittest.main()
