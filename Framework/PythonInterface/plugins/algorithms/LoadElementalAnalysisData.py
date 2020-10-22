# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import glob
import os
import numpy as np
from mantid import config
from mantid.api import AlgorithmFactory, AnalysisDataService, PythonAlgorithm, TextAxis, WorkspaceGroup, WorkspaceGroupProperty
from mantid.kernel import Direction, IntBoundedValidator
from mantid.simpleapi import ConvertToHistogram, ConvertToPointData, DeleteWorkspace, LoadAscii, WorkspaceFactory


type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}
spectrum_index = {"Delayed": 1, "Prompt": 2, "Total": 3}
num_files_per_detector = 3
run_number_length = 5


class LoadElementalAnalysisData(PythonAlgorithm):
    def category(self):
        """
        Returns category
        """
        return "DataHandling"

    def PyInit(self):
        self.declareProperty(name='Run', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Run number to load')

        self.declareProperty(WorkspaceGroupProperty(name='GroupWorkspace', defaultValue='',
                                                    direction=Direction.Output),
                             doc='Output group workspace for run')

    def validateInputs(self):
        issues = dict()
        checkRun = self.search_user_dirs()
        if not checkRun:
            issues['Run'] = "Cannot find files for run " + self.getPropertyValue("Run")
        return issues

    def PyExec(self):
        run = self.getPropertyValue("Run")
        to_load = self.search_user_dirs()
        workspaces = {
            path: self.get_filename(path, run)
            for path in to_load if self.get_filename(path, run) is not None
        }
        unique_workspaces = {}
        for path, workspace in workspaces.items():
            if workspace not in unique_workspaces.values():
                unique_workspaces[path] = workspace
        workspaces = unique_workspaces

        self.format_workspace(workspaces)
        self.merge_workspaces(workspaces.values())

    def pad_run(self):
        """ Pads run number: i.e. 123 -> 00123; 2695 -> 02695 """
        return str(self.getProperty("Run").value).zfill(run_number_length)

    def search_user_dirs(self):
        """ Finds all files for the run number provided """
        files = []
        for user_dir in config["datasearch.directories"].split(";"):
            path = os.path.join(user_dir, "ral{}.rooth*.dat".format(self.pad_run()))
            files.extend([file for file in glob.iglob(path)])
        return files

    def get_filename(self, path, run):
        """
        Returns the overall workspace name
        """
        try:
            detectors_num = str(int(path.rsplit(".", 2)[1][5]) - 1)
            run_type = type_keys[path.rsplit(".")[1][-2:]]
            return "_".join([detectors_num, run_type, str(run)])
        except KeyError:
            return None

    def format_workspace(self, inputs):
        """ inputs is a dict mapping filepaths to output names """
        for path, output in inputs.items():
            workspace = LoadAscii(path, OutputWorkspace=output)
            workspace.getAxis(0).setUnit("Label").setLabel("Energy", "keV")

    def merge_workspaces(self, workspaces):
        """ where workspaces is a tuple of form:
                (filepath, ws name)
        """
        workspace_name = self.getPropertyValue('GroupWorkspace')
        # detectors is a dictionary of {detector_name : [names_of_workspaces]}
        detectors = {f"{workspace_name}; Detector {x}": [] for x in range(1, 5)}
        # fill dictionary
        for workspace in workspaces:
            detector_number = workspace[0]
            detectors[f"{workspace_name}; Detector {detector_number}"].append(workspace)
        # initialise a group workspace
        overall_ws = WorkspaceGroup()
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
                merged_ws = self.create_merged_workspace(workspace_list)
                ConvertToHistogram(InputWorkspace=merged_ws, OutputWorkspace=detector)
                overall_ws.addWorkspace(AnalysisDataService.retrieve(detector))
        self.setProperty("GroupWorkspace", overall_ws)

    def create_merged_workspace(self, workspace_list):
        if workspace_list:
            # get max number of bins and max X range
            max_num_bins = 0
            for ws_name in workspace_list:
                if ws_name:
                    ws = AnalysisDataService.retrieve(ws_name)
                    max_num_bins = max(ws.blocksize(), max_num_bins)

            # create single ws for the merged data, use original ws as a template
            template_ws = next(ws for ws in workspace_list if ws is not None)
            merged_ws = WorkspaceFactory.create(AnalysisDataService.retrieve(template_ws), NVectors=num_files_per_detector,
                                                XLength=max_num_bins, YLength=max_num_bins)

            # create a merged workspace based on every entry from workspace list
            for i in range(0, num_files_per_detector):
                # load in ws - first check workspace exists
                if workspace_list[i]:
                    ws = AnalysisDataService.retrieve(workspace_list[i])
                    # check if histogram data, and convert if necessary
                    if ws.isHistogramData():
                        ws = ConvertToPointData(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
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

                    #set y axis labels
                    self.set_y_axis_labels(merged_ws, spectrum_index)

                    # remove workspace from ADS
                    DeleteWorkspace(ws)

            return merged_ws

    def set_y_axis_labels(self, workspace, labels):
        """ adds the spectrum_index to the plot labels """

        axis = TextAxis.create(len(labels))

        for index, label in enumerate(labels):
            axis.setLabel(index, label)

        workspace.replaceAxis(1, axis)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadElementalAnalysisData)
