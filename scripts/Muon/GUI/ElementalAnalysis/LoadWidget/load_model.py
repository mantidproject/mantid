from __future__ import print_function
import os
import glob

import mantid.simpleapi as mantid
from mantid import config

from six import iteritems


class LoadModel(object):
    def __init__(self):
        self.alg = None
        self.run = 0
        self._type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def output(self):
        return

    def execute(self):
        to_load = self.search_user_dirs(self.run)
        if to_load:
            self.load_sequential(to_load)

    def set_run(self, run):
        self.run = run

    # Pads run number: i.e. 123 -> 00123; 2695- > 02695
    def pad_run(self, run):
        return str(run).zfill(5)

    def search_user_dirs(self, run):
        files = []
        for user_dir in config["datasearch.directories"].split(";"):
            path = os.path.join(user_dir,
                                "ral{}.rooth*.dat".format(self.pad_run(run)))
            for g in glob.iglob(path):
                files.append(g)
        return files

    def load_sequential(self, files):
        if self.alg is not None:
            raise RuntimeError("Loading already in progress")
        self.alg = mantid.AlgorithmManager.create("LoadAscii")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        workspaces = [(f, self.get_filename(f))
                      for f in files if self.get_filename(f) is not None]
        print(workspaces)
        for f, n in workspaces:
            self.alg.setProperty("Filename", f)
            self.alg.setProperty("OutputWorkspace", n)
            self.alg.execute()
            mantid.AnalysisDataService.addOrReplace(
                n, self.alg.getProperty("OutputWorkspace").value)
        self.group_by_detector(workspaces)
        self.alg = None

    def get_filename(self, name):
        s = name.rsplit(".")[1]
        num, end = s[-2:], s[-9:]
        try:
            return "{}_{}_{}".format(self.run, self._type_keys[num], end)
        except KeyError:
            return None

    def group_by_detector(self, workspaces):
        d_string = "Run {}; Detector {}"
        detectors = {d_string.format(self.run, x): [] for x in range(1, 5)}
        for w, name in workspaces:
            detector_number = int(w.rsplit(".", 2)[1][5]) - 1
            detectors[d_string.format(self.run, detector_number)].append(name)
        for d, v in iteritems(detectors):
            mantid.GroupWorkspaces(v, OutputWorkspace=str(d))

    def load_concurrent(self, output, files):
        if self.alg is not None:
            raise RuntimeError("Loading already in progress")
        self.alg = mantid.AlgorithmManager.create("Load")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        load_string = "+".join(files)
        self.alg.setProperty("Filename", load_string)
        self.alg.setProperty("OutputWorkspace", output)
        self.alg.execute()
        mantid.AnalysisDataService.addOrReplace(
            load_string, self.alg.getProperty("OutputWorkspace").value)
        self.alg = None

    def loadData(self, inputs):
        pass
