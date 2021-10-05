# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils


class LoadModel(lutils.LModel):
    def __init__(self):
        super(LoadModel, self).__init__()

    def execute(self):
        if self.run not in self.loaded_runs:
            self.load_run()
        else:
            self.last_loaded_runs.append(self.run)


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
        else:
            self.last_loaded_runs.append(self.run)
        if self.run not in self.co_runs:
            self.co_runs.append(self.run)
            if self.workspace:
                self.co_load_run(self.loaded_runs[self.run])
            else:
                self.workspace = self.loaded_runs[self.run]

    def add_runs(self, run1, run2, suffix):
        # prevent new suffix being appended to old one
        out = suffix+";" + run1.split(";")[1]
        mantid.Plus(run1, run2, OutputWorkspace=out)
        return out

    def co_load_run(self, workspace):
        run = lutils.hyphenise(self.co_runs)
        to_add = [
            self.add_runs(run1, run2, run)
            for run1, run2 in zip(*lutils.flatten_run_data(self.workspace, workspace))
        ]
        self.loaded_runs[run] = to_add
        self.add_co_load_to_group(to_add,run)

    def add_co_load_to_group(self, to_add,run):
        overall_ws = mantid.GroupWorkspaces(to_add[0], OutputWorkspace=str(run))
        for index in range(1, len(to_add)):
            overall_ws.add(to_add[index])
