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
from mantid.api import AlgorithmFactory, AnalysisDataService, PythonAlgorithm, TextAxis, WorkspaceGroup, \
    WorkspaceGroupProperty
from mantid.kernel import Direction, IntBoundedValidator
from mantid.simpleapi import ConvertToHistogram, ConvertToPointData, DeleteWorkspace, LoadAscii, WorkspaceFactory, \
    CropWorkspaceRagged


TYPE_KEYS = {"10": "Prompt", "20": "Delayed", "99": "Total"}
SPECTRUM_INDEX = {"Delayed": 1, "Prompt": 2, "Total": 3}
NUM_FILES_PER_DETECTOR = 3
RUN_NUMBER_LENGTH = 5


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

        self.declareProperty(name="Directory", defaultValue="", direction=Direction.Output,
                             doc="provides the directory where the run files were acquired")

    def validateInputs(self):
        issues = dict()
        check_run = self.search_user_dirs()
        if not check_run:
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
        self.merge_and_crop_workspaces(workspaces.values())

    def pad_run(self):
        """ Pads run number: i.e. 123 -> 00123; 2695 -> 02695 """
        return str(self.getProperty("Run").value).zfill(RUN_NUMBER_LENGTH)

    def search_user_dirs(self):
        """ Finds all files for the run number provided """
        files = []
        for user_dir in config.getDataSearchDirs():
            path = os.path.join(user_dir, "ral{}.rooth*.dat".format(self.pad_run()))
            files.extend([file for file in glob.iglob(path)])
            if files:
                self.setProperty("Directory", user_dir)
                return files

    def get_filename(self, path, run):
        """
        Returns the overall workspace name
        """
        filename = os.path.basename(path)
        try:
            detectors_num = str(int(filename.rsplit(".", 2)[1][5]) - 1)
            run_type = TYPE_KEYS[filename.rsplit(".")[1][-2:]]
            return "_".join([detectors_num, run_type, str(run)])
        except KeyError:
            return None

    def format_workspace(self, inputs):
        """ inputs is a dict mapping filepaths to output names """
        for path, output in inputs.items():
            workspace = LoadAscii(path, OutputWorkspace=output)
            workspace.setE(0, np.sqrt(workspace.dataY(0)))
            workspace.getAxis(0).setUnit("Label").setLabel("Energy", "keV")

    def merge_and_crop_workspaces(self, workspaces):
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
                sorted_workspace_list = [None] * NUM_FILES_PER_DETECTOR
                # sort workspace list according to type_index
                for workspace in workspace_list:
                    data_type = workspace.rsplit("_")[1]
                    sorted_workspace_list[SPECTRUM_INDEX[data_type] - 1] = workspace
                workspace_list = sorted_workspace_list
                # create merged workspace
                merged_ws = self.create_merged_workspace(workspace_list)
                ConvertToHistogram(InputWorkspace=merged_ws, OutputWorkspace=detector)
                minX, maxX = [], []
                ws = AnalysisDataService.retrieve(detector)
                for i in range(ws.getNumberHistograms()):
                    xdata = ws.readX(i)
                    minX.append(xdata[0])
                    if i == 2:
                        maxX.append(xdata[-1])
                    else:
                        maxX.append(xdata[-1] - 1)
                CropWorkspaceRagged(InputWorkspace=detector, OutputWorkspace=detector, xmin = minX, xmax = maxX)
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
            merged_ws = WorkspaceFactory.create(AnalysisDataService.retrieve(template_ws),
                                                NVectors=NUM_FILES_PER_DETECTOR, XLength=max_num_bins,
                                                YLength=max_num_bins)

            # create a merged workspace based on every entry from workspace list
            for i in range(0, NUM_FILES_PER_DETECTOR):
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

                    # set y axis labels
                    self.set_y_axis_labels(merged_ws, SPECTRUM_INDEX)

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
