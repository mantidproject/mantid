# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import OrderedDict

import glob
import os
import numpy as np
from mantid import config
import mantid.simpleapi as mantid

type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}
spectrum_index = {"Delayed": 1, "Prompt": 2, "Total": 3}
num_files_per_detector = 3


class LModel(object):
    def __init__(self):
        self.run = 0
        self.num_loaded_detectors = OrderedDict()
        self.loaded_runs = OrderedDict()
        self.last_loaded_runs = []

    def _load(self, inputs):
        """inputs is a dict mapping filepaths to output names"""
        for path, output in inputs.items():
            workspace = mantid.LoadAscii(path, OutputWorkspace=output)
            workspace.getAxis(0).setUnit("Label").setLabel("Energy", "keV")

    def load_run(self):
        to_load = search_user_dirs(self.run)
        if not to_load:
            return None
        workspaces = {filename: get_filename(filename, self.run) for filename in to_load if get_filename(filename, self.run) is not None}
        unique_workspaces = {}
        for path, workspace in workspaces.items():
            if workspace not in unique_workspaces.values():
                unique_workspaces[path] = workspace
        workspaces = unique_workspaces
        # get number of detectors loaded - should be 4, but legacy data may contain less
        loaded_detectors = {}
        for ws_name in workspaces.values():
            loaded_detectors[ws_name[0]] = 1
        num_loaded_detectors = len(loaded_detectors)

        if mantid.AnalysisDataService.Instance().doesExist(str(self.run)):
            run_workspace = mantid.AnalysisDataService.Instance().retrieve(str(self.run))
            mantid.DeleteWorkspace(run_workspace)

        self._load(workspaces)
        self.loaded_runs.update({self.run: merge_workspaces(self.run, workspaces.values())})
        self.num_loaded_detectors[self.run] = num_loaded_detectors
        self.last_loaded_runs.append(self.run)
        return self.loaded_runs[self.run]

    def cancel(self):
        return

    def loadData(self, inputs):
        return

    def set_run(self, run):
        self.run = run


def pad_run(run):
    """Pads run number: i.e. 123 -> 00123; 2695 -> 02695"""
    return str(run).zfill(5)


def search_user_dirs(run):
    files = []
    for user_dir in config["datasearch.directories"].split(";"):
        path = os.path.join(user_dir, "ral{}.rooth*.dat".format(pad_run(run)))
        files.extend([file for file in glob.iglob(path)])

    return files


# merge each detector workspace into one
def merge_workspaces(run, workspaces):
    """where workspaces is a tuple of form:
    (filepath, ws name)
    """
    d_string = "{}; Detector {}"
    # detectors is a dictionary of {detector_name : [names_of_workspaces]}
    detectors = {d_string.format(run, x): [] for x in range(1, 5)}
    # fill dictionary
    for workspace in workspaces:
        detector_number = get_detector_num_from_ws(workspace)
        detectors[d_string.format(run, detector_number)].append(workspace)
    # initialise a group workspace
    tmp = mantid.CreateSampleWorkspace()
    overall_ws = mantid.GroupWorkspaces(tmp, OutputWorkspace=str(run))
    # merge each workspace list in detectors into a single workspace
    for detector, workspace_list in detectors.items():
        if workspace_list:
            # sort workspace list according to type_index
            sorted_workspace_list = [None] * num_files_per_detector
            # sort workspace list according to type_index
            for workspace in workspace_list:
                data_type = workspace.rsplit("_")[1]
                sorted_workspace_list[spectrum_index[data_type] - 1] = workspace
            workspace_list = sorted_workspace_list
            # create merged workspace
            merged_ws = create_merged_workspace(workspace_list)
            # add merged ws to ADS
            mantid.mtd.add(detector, merged_ws)
            mantid.ConvertToHistogram(InputWorkspace=detector, OutputWorkspace=detector)
            overall_ws.add(detector)

    mantid.AnalysisDataService.remove("tmp")
    # return list of [run; Detector detectorNumber], in ascending order of detector number
    detector_list = sorted(list(detectors))
    return detector_list


# creates merged workspace, based on workspaces from workspace list
# returns a merged workspace in point data format.
def create_merged_workspace(workspace_list):
    if workspace_list:
        # get max number of bins and max X range
        max_num_bins = 0
        for ws_name in workspace_list:
            if ws_name:
                ws = mantid.mtd[ws_name]
                max_num_bins = max(ws.blocksize(), max_num_bins)

        # create single ws for the merged data, use original ws as a template
        template_ws = next(ws for ws in workspace_list if ws is not None)
        merged_ws = mantid.WorkspaceFactory.create(
            mantid.mtd[template_ws], NVectors=num_files_per_detector, XLength=max_num_bins, YLength=max_num_bins
        )

        # create a merged workspace based on every entry from workspace list
        for i in range(0, num_files_per_detector):
            # load in ws - first check workspace exists
            if workspace_list[i]:
                ws = mantid.mtd[workspace_list[i]]
                # check if histogram data, and convert if necessary
                if ws.isHistogramData():
                    ws = mantid.ConvertToPointData(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
                # find max x val
                max_x = np.max(ws.readX(0))
                # get current number of bins
                num_bins = ws.blocksize()
                # pad bins
                X_padded = np.empty(max_num_bins)
                X_padded.fill(max_x)
                X_padded[:num_bins] = ws.readX(0)
                Y_padded = np.zeros(max_num_bins)
                Y_padded[:num_bins] = ws.readY(0)
                E_padded = np.zeros(max_num_bins)
                E_padded[:num_bins] = ws.readE(0)

                # set row of merged workspace
                merged_ws.setX(i, X_padded)
                merged_ws.setY(i, Y_padded)
                merged_ws.setE(i, E_padded)

                # remove workspace from ADS
                mantid.AnalysisDataService.remove(ws.getName())

        return merged_ws


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
    for workspace_list in workspaces:
        out.append(sorted([ws_name for ws_name in workspace_list]))
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
