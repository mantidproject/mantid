# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

import glob
import os

from six import iteritems

from mantid import config
import mantid.simpleapi as mantid

type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}


class LModel(object):
    def __init__(self):
        self.run = 0
        self.loaded_runs = {}
        self.last_loaded_runs = []

    def _load(self, inputs):
        """ inputs is a dict mapping filepaths to output names """
        for path, output in iteritems(inputs):
            workspace = mantid.LoadAscii(path, OutputWorkspace=output)
            workspace.getAxis(0).setUnit("Label").setLabel("Energy", "keV")

    def load_run(self):
        to_load = search_user_dirs(self.run)
        if not to_load:
            return None
        workspaces = {filename: get_filename(
            filename, self.run) for filename in to_load if get_filename(filename, self.run) is not None}
        self._load(workspaces)
        self.loaded_runs[self.run] = group_by_detector(self.run, workspaces.values())
        self.last_loaded_runs.append(self.run)
        return self.loaded_runs[self.run]

    def output(self):
        return

    def cancel(self):
        return

    def loadData(self, inputs):
        return

    def set_run(self, run):
        self.run = run


def pad_run(run):
    """ Pads run number: i.e. 123 -> 00123; 2695 -> 02695 """
    return str(run).zfill(5)


def search_user_dirs(run):
    files = []
    for user_dir in config["datasearch.directories"].split(";"):
        path = os.path.join(user_dir, "ral{}.rooth*.dat".format(pad_run(run)))
        files.extend([file for file in glob.iglob(path)])
    return files


def group_by_detector(run, workspaces):
    """ where workspaces is a tuple of form:
            (filepath, ws name)
    """
    d_string = "{}; Detector {}"
    detectors = {d_string.format(run, x): [] for x in range(1, 5)}
    for workspace in workspaces:
        detector_number = get_detector_num_from_ws(workspace)
        detectors[d_string.format(run, detector_number)].append(workspace)
    for detector, workspace_list in iteritems(detectors):
        mantid.GroupWorkspaces(workspace_list, OutputWorkspace=str(detector))
    detector_list = sorted(list(detectors))
    group_grouped_workspaces(run, detector_list)
    return detector_list


def group_grouped_workspaces(name, workspaces):
    tmp = mantid.CreateSampleWorkspace()
    overall = mantid.GroupWorkspaces(tmp, OutputWorkspace=str(name))
    for workspace in workspaces:
        overall.add(workspace)
    mantid.AnalysisDataService.remove("tmp")


def get_detector_num_from_ws(name):
    """
    Gets the detector number from the workspace name:
        i.e the first character
    """
    return name[0]


def get_detectors_num(path):
    """
    Gets the detector number from the filepath
    """
    return str(int(path.rsplit(".", 2)[1][5]) - 1)


def get_end_num(path):
    """
    Gets the end numbers (form: roothXXXX) from the filepath
    """
    return path.rsplit(".")[1][-9:]


def get_run_type(path):
    """
    Gets the run type (i.e. Total/Delayed/Prompt) from the filepath
    """
    return type_keys[path.rsplit(".")[1][-2:]]


def get_filename(path, run):
    """
    Returns the overall workspace name
    """
    try:
        return "_".join([get_detectors_num(path), get_run_type(path), str(run)])
    except KeyError:
        return None


def replace_workspace_name_suffix(name, suffix):
    detector, run_type = name.split("_", 2)[:2]
    return "_".join([detector, run_type, suffix])


def flatten_run_data(*workspaces):
    out = []
    for workspace in workspaces:
        detectors = [mantid.mtd[detector] for detector in workspace]
        out.append(sorted([_workspace.getName() for detector in detectors for _workspace in detector]))
    return out


def hyphenise(vals):
    out = []
    if vals:
        vals = [str(val) for val in sorted(list(set(vals)))]
        diffs = [int(vals[i + 1]) - int(vals[i]) for i in range(len(vals) - 1)]
        first = last = vals[0]
        for i, diff in enumerate(diffs):
            if diff != 1:
                out.append("-".join([first, last]) if first != last else first)
                first = vals[i + 1]
            last = vals[i + 1]
        out.append("-".join([first, last]) if first != last else vals[-1])
    return ", ".join(out)
