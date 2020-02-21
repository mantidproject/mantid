# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import print_function

import mantid.simpleapi as mantid

from Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils


class LoadModel(lutils.LModel):
    def __init__(self):
        super(LoadModel, self).__init__()

    def execute(self):
        print("bii")
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
        print("fff", run1, run2)
        out = run1+" "+run2
        mantid.Plus(run1, run2, OutputWorkspace=out)
        return out

    def co_load_run(self, workspace):


        run = lutils.hyphenise(self.co_runs)
        print(self.workspace,run, ":D")
        self.last_loaded_runs.append(run)
        to_add = [
            self.add_runs(run1, run2, run)
            for run1, run2 in zip(*lutils.flatten_run_data(self.workspace, workspace))
        ]
        overall_ws = mantid.GroupWorkspaces(to_add[0], OutputWorkspace=str(run))
        overall_ws.add(to_add[1])
        # need to finsih adding the other WS to group
        # need to work out how to add n>3 ws correctly
