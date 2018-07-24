<<<<<<< c59d22493b303d5cda5a4d2e982865ac476fe85c
from __future__ import print_function

import mantid.simpleapi as mantid

from Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils


class LoadModel(lutils.LModel):
||||||| merged common ancestors
class LoadModel(object):
=======
import mantid.simpleapi as mantid

from six import iteritems

class LoadModel(object):
>>>>>>> refs #23023 add load algorithm (threaded) to browse
    def __init__(self):
<<<<<<< c59d22493b303d5cda5a4d2e982865ac476fe85c
        super(LoadModel, self).__init__()

    def execute(self):
        if self.run not in self.loaded_runs:
            self.load_run()


class CoLoadModel(lutils.LModel):
    def __init__(self):
        super(CoLoadModel, self).__init__()
        self.workspace = None
        self.co_runs = []

    def wipe_co_runs(self):
        self.co_runs = []
        self.workspace = None

    def execute(self):
        if self.run not in self.loaded_runs:
            current_ws = self.load_run()
            if current_ws is None:
                return
            self.loaded_runs[self.run] = current_ws
        if self.run not in self.co_runs:
            self.co_runs.append(self.run)
            if self.workspace:
                self.co_load_run(self.loaded_runs[self.run])
            else:
                self.workspace = self.loaded_runs[self.run]

    def add_runs(self, l, r, suffix):
        # prevent new suffix being appended to old one
        out = lutils.replace_workspace_name_suffix(l, suffix)
        mantid.Plus(l, r, OutputWorkspace=out)
        return out

    def co_load_run(self, workspace):
        run = lutils.hyphenise(self.co_runs)
        to_add = [self.add_runs(l, r, run) for l, r in zip(*lutils.flatten_run_data(
            self.workspace, workspace))]
        self.workspace = lutils.group_by_detector(run, to_add)
||||||| merged common ancestors
        pass
=======
        self.alg = None
        self.filename = ""

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def output(self):
        return

    def execute(self):
        if self.filename == "":
            self.cancel()
            return
        self.alg = mantid.AlgorithmManager.create("Load")   
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        self.alg.setProperty("Filename", self.filename)

        #l = lambda x: "WS_{}".format(x)
        self.alg.setProperty("OutputWorkspace", "WS_{}".format(self.filename))
        self.alg.execute()
        mantid.AnalysisDataService.addOrReplace("WS_{}".format(self.filename), self.alg.getProperty("OutputWorkspace").value)

    def loadData(self, inputs):
        self.filename = inputs["Filename"]
        

    
>>>>>>> refs #23023 add load algorithm (threaded) to browse
