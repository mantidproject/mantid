# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


import unittest
import os
import tempfile

from mantid.simpleapi import LoadPLN

from plugins.algorithms.ANSTO.ansto_common import (
    seq_to_list,
    list_to_seq,
    hdf_files_from_runs,
    replace_variants,
    encode_run_list,
    expanded_runs,
    SingleRun,
    find_cycle_folders,
    split_run_index,
    ScratchFolder,
)


class AnstoCommonSearchTests(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def test_seq_to_list(self):
        iseqn = "2-7,8,1,9-10"
        elist = [2, 3, 4, 5, 6, 7, 8, 1, 9, 10]
        flist = seq_to_list(iseqn)
        self.assertListEqual(flist, elist)

        oseqn = list_to_seq(elist)
        self.assertEqual(oseqn, "1-10")

    def test_expanded_runs(self):
        allruns = "142::12345-12346:1;22345:20"
        exp_runs = expanded_runs(allruns)
        self.assertEqual(len(exp_runs), 3)
        act_runs = [
            SingleRun(142, 12345, 1),
            SingleRun(142, 12346, 1),
            SingleRun(None, 22345, 20),
        ]
        self.assertListEqual(exp_runs, act_runs)

    def test_encode_run_list(self):
        file_list = [
            "/fpath/PLN0012345.nx.hdf:0",
            "/fpath/PLN0012345.nx.hdf:1",
            "/fpath/PLN0012345.nx.hdf:2",
            "/fpath/PLN0022220.nx.hdf:7",
            "/fpath/PLN0022221.nx.hdf:7",
            "/fpath/PLN0022222.nx.hdf:7",
        ]
        exp_value = "12345:0-2;22220-22222:7"
        ret_value = encode_run_list(file_list)
        self.assertEqual(ret_value, exp_value)

    def test_replace_variants(self):
        search_path = ["/cycle/[10-12]/data/[src,bin]"]
        tags = ["[10-12]", "[src,bin]"]
        exp_paths = [
            "/cycle/10/data/src",
            "/cycle/11/data/src",
            "/cycle/12/data/src",
            "/cycle/10/data/bin",
            "/cycle/11/data/bin",
            "/cycle/12/data/bin",
        ]
        ret_paths = replace_variants(search_path, tags)
        self.assertEqual(len(ret_paths), 6)
        self.assertCountEqual(ret_paths, exp_paths)

    def test_replace_cycle(self):
        inp_paths = [
            "/cycle/10/data/src",
            "/cycle/NNN/data/src",
            "/cycle/12/data/src",
            "/cycle/10/data/bin",
            "/cycle/NNN/data/bin",
            "/cycle/12/data/bin",
        ]
        exp_paths = ["/cycle/138/data/src", "/cycle/138/data/bin"]
        ret_paths = find_cycle_folders(138, inp_paths)
        self.assertCountEqual(ret_paths, exp_paths)

    def test_find_hdf_files(self):
        ret_values = hdf_files_from_runs("44464", [], "PLN", ".hdf")
        self.assertEqual(len(ret_values), 1)
        filename = os.path.basename(ret_values[0])
        self.assertEqual(filename, "PLN0044464.hdf:0")

        try:
            ret_values = hdf_files_from_runs("44464xx", [], "PLN", ".hdf")
            self.assertTrue(False, "Expected runtime exception for missing file")
        except RuntimeError:
            pass

    def test_scratch_folder(self):
        # create a temp folder as the scratch folder
        with tempfile.TemporaryDirectory() as tmpdir:
            scratch = ScratchFolder(tmpdir)

            # load a file using the scratch folder method
            erun = hdf_files_from_runs("44464", [], "PLN", ".hdf")[0]
            source, ds_index = split_run_index(erun)
            nxsfile = scratch.build_temp_fpath(source, 0, "")

            loadopts = {"BinaryEventPath": "./PLN0044464.bin"}
            loaded = scratch.load_run_from_scratch(source, 0, LoadPLN, loadopts, "test", params=[], event_dirs=[], filter=None)
            self.assertTrue(loaded, "Unable to load file {}".format(source))

            # confirm the file is in the scratch folder
            ok = os.path.isfile(nxsfile)
            self.assertTrue(ok, "Missing scratch file {}".format(nxsfile))

            # rename the tmp file
            newfile = nxsfile.replace("44464.", "55555.")
            os.rename(nxsfile, newfile)

            # load renamed file which is only available in the scratch folder
            newsrc = source.replace("44464.", "55555.")
            loaded = scratch.load_run_from_scratch(newsrc, 0, LoadPLN, loadopts, "test", params=[], event_dirs=[], filter=None)
            self.assertTrue(loaded, "Unable to load file {}".format(newsrc))


if __name__ == "__main__":
    unittest.main()
