from __future__ import print_function

import mantid.simpleapi as mantid

from six import iteritems

from Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils


class LoadModel(object):
    def __init__(self):
        self.alg = None
        self.co_load_ws = None
        self.loaded_runs = {}
        self.co_runs = []
        self.run = 0

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def output(self):
        return

    def execute(self):
        if self.run not in self.loaded_runs:
            self.load_run()
        current_ws = self.loaded_runs[self.run]
        if self.co_runs:
            if self.co_load_ws:
                self.co_load_run(current_ws)
            else:
                self.co_load_ws = current_ws

    def load_run(self):
        to_load = lutils.search_user_dirs(self.run)
        if not to_load:
            try:
                self.co_runs.remove(self.run)
            except ValueError:
                pass
            return
        workspaces = {f: lutils.get_filename(
            f) for f in to_load if lutils.get_filename(f) is not None}
        self._load(workspaces)
        self.loaded_runs[self.run] = lutils.group_by_detector(
            self.run, workspaces.values())

    def co_load_run(self, workspace):
        to_add = []
        for l, r in zip(lutils.flatten_run_data(
                self.co_load_ws), lutils.flatten_run_data(workspace)):
            out = "{}_co_add".format(l)
            mantid.Plus(l, r, OutputWorkspace=out)
            to_add.append(out)
        self.co_load_ws = lutils.group_by_detector(
            lutils.hyphenise(self.co_runs), to_add)

    # inputs is a dict mapping filepaths to output names
    def _load(self, inputs):
        if self.alg is not None:
            raise RuntimeError("Loading already in progress")
        self.alg = mantid.AlgorithmManager.create("LoadAscii")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        for path, output in iteritems(inputs):
            self.alg.setProperty("Filename", path)
            self.alg.setProperty("OutputWorkspace", output)
            self.alg.execute()
            mantid.AnalysisDataService.addOrReplace(
                output, self.alg.getProperty("OutputWorkspace").value)
        self.alg = None

    def set_run(self, run):
        self.run = run

    def loadData(self, inputs):
        pass
