# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

import numbers

from mantid.api import NumericAxis, PythonAlgorithm, TextAxis, WorkspaceGroup, WorkspaceProperty
from mantid.kernel import Direction, StringListValidator, StringMandatoryValidator, logger
from mantid.simpleapi import AlgorithmFactory, ConjoinWorkspaces, DeleteWorkspace, ExtractSingleSpectrum, \
    RenameWorkspace, mtd


class ConjoinSpectra(PythonAlgorithm):
    """
    Conjoins spectra from several workspaces into a single workspace

    Spectra to be conjoined must be equally binned in order for ConjoinSpectra
    to work. If necessary use RebinToWorkspace first.
    """

    def category(self):
        return "Transforms\\Merging"

    def seeAlso(self):
        return ["AppendSpectra", "ConjoinWorkspaces"]

    def name(self):
        return "ConjoinSpectra"

    def summmary(self):
        return "Joins individual spectra from a range of workspaces into a single workspace for plotting or further analysis."

    def PyInit(self):
        self.declareProperty("InputWorkspaces", "", validator=StringMandatoryValidator(),
                             doc="Comma seperated list of workspaces to use, group workspaces " +
                                 "will automatically include all members.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name the workspace that will contain the result")
        self.declareProperty("WorkspaceIndex", 0,
                             doc="The workspace index of the spectra in each workspace to extract. Default: 0")
        self.declareProperty("LabelUsing", "", doc="The name of a log value used to label the resulting spectra. " +
                                                   "Default: The source workspace name")
        labelValueOptions = ["Mean", "Median", "Maximum", "Minimum", "First Value"]
        self.declareProperty("LabelValue", "Mean", validator=StringListValidator(labelValueOptions),
                             doc="How to derive the value from a time series property")

    def PyExec(self):
        # get parameter values
        ws_output = self.getPropertyValue("OutputWorkspace")
        ws_index = int(self.getPropertyValue("WorkspaceIndex"))
        ws_string = self.getPropertyValue("InputWorkspaces").strip()
        label_using = self.getPropertyValue("LabelUsing").strip()
        label_value = self.getPropertyValue("LabelValue")
        # import sys
        # sys.path.append("c:\\users\\qbr77747\\apps\\miniconda3\\lib\\site-packages")
        # import pydevd
        # pydevd.settrace('localhost', port=44444, stdoutToServer=True, stderrToServer=True)

        # internal values
        ws_temp = "__ConjoinSpectra_temp"
        loop_index = 0

        # get the workspace list
        all_ws_names = self.make_workspace_list(ws_string)
        if mtd.doesExist(ws_output):
            DeleteWorkspace(Workspace=ws_output)

        axis = self.make_output_axis(all_ws_names, label_using, label_value)
        for ws_name in all_ws_names:
            # extract the spectrum
            ExtractSingleSpectrum(InputWorkspace=ws_name, OutputWorkspace=ws_temp,
                                  WorkspaceIndex=ws_index)

            label = self.make_label(ws_name, ws_index, label_using, label_value)
            axis.setValue(loop_index, label)
            loop_index += 1
            if mtd.doesExist(ws_output):
                ConjoinWorkspaces(InputWorkspace1=ws_output,
                                  InputWorkspace2=ws_temp,
                                  CheckOverlapping=False)
                if mtd.doesExist(ws_temp):
                    DeleteWorkspace(Workspace=ws_temp)
            else:
                RenameWorkspace(InputWorkspace=ws_temp, OutputWorkspace=ws_output)

        ws_out = mtd[ws_output]
        # replace the spectrum axis
        ws_out.replaceAxis(1, axis)

        self.setProperty("OutputWorkspace", ws_out)

    def make_workspace_list(self, wsString):
        all_ws_names = []
        for ws_name in wsString.split(","):
            ws = mtd[ws_name.strip()]
            # if we cannot find the ws then stop the algorithm execution
            if ws is None:
                raise RuntimeError("Cannot find workspace '" + ws_name.strip() + "', aborting")
            if isinstance(ws, WorkspaceGroup):
                all_ws_names.extend(ws.getNames())
            else:
                all_ws_names.append(ws_name)
        return all_ws_names

    def make_output_axis(self, all_ws_names, label_using, label_value):
        """
        Gets the log value from the first workspace to check if it can be parsed as a number.

        If it can be parsed as a number then NumericAxis is used. Otherwise it is defaulted to TextAxis.

        :param all_ws_names: List of workspace names. The full list is needed to determine the length of the Axis
        :param label_using: The LabelUsing property - specifies a log entry,
                            whose value from each workspace will be used as the label.
        :param label_value: The value of the LabelValue property
        """
        label = self.make_label(all_ws_names[0], 0, label_using, label_value)

        if isinstance(label, numbers.Number):
            # if the label successfully parses as a number, then we create a NumericAxis to hold the labels
            na= NumericAxis.create(len(all_ws_names))
            # na.setUnit("TOF") # FIXME this TOF seems to make ALL the difference
            return na
        else:
            # if the label fails parsing, then it is something that isn't a number, so we default to TextAxis
            return TextAxis.create(len(all_ws_names))

    def make_label(self, ws_name, index, label_using, label_value):
        label = ""
        if label_value != "":
            label = self.get_log_value(mtd[ws_name.strip()], label_using, label_value)
        if label == "":
            label = ws_name + "_" + str(index)
        return label

    def get_log_value(self, ws, label_using, label_value):
        label = 0.0
        run = ws.getRun()
        try:
            prop = run.getProperty(label_using)
            try:
                stats = prop.getStatistics()
                if (label_value == "Mean"):
                    label = stats.mean
                elif (label_value == "Median"):
                    label = stats.median
                elif (label_value == "Maximum"):
                    label = stats.maximum
                elif (label_value == "Minimum"):
                    label = stats.minimum
                elif (label_value == "Last Value"):
                    label = prop.value[-1] + 0.0
                else:
                    label = prop.value[0] + 0.0
            except:
                # this is not a time series property - just return the value
                label = prop.value
        except RuntimeError:
            # failed to find the property
            # log and pass out an empty string
            logger.information("Could not find log " + label_using + " in workspace " +
                               str(ws) + " using workspace label instead.")
        return label


AlgorithmFactory.subscribe(ConjoinSpectra)
