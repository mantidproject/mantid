# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,  WorkspaceGroup
from mantid.kernel import Direction, StringArrayProperty, StringListValidator, V3D, StringArrayLengthValidator
import numpy as np


class DNSMergeRuns(PythonAlgorithm):
    """
    Merges given runs into one matrix workspace.
    This algorithm is written for the DNS @ MLZ,
    but can be adjusted for other instruments if needed.
    """
    properties_to_compare = ['omega', 'slit_i_left_blade_position',
                             'slit_i_right_blade_position', 'slit_i_lower_blade_position',
                             'slit_i_upper_blade_position', 'polarisation', 'polarisation_comment', 'flipper']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.workspace_names = []
        self.xaxis = '2theta'
        self.outws_name = None

    def category(self):
        """
        Returns category
        """
        return 'Workflow\\MLZ\\DNS'

    def seeAlso(self):
        return [ "LoadDNSLegacy" ]

    def name(self):
        """
        Returns name
        """
        return "DNSMergeRuns"

    def summary(self):
        return "Merges runs performed at different detector bank positions into one matrix workspace."

    def PyInit(self):

        validator = StringArrayLengthValidator()
        validator.setLengthMin(1)                               # group of workspaces may be given

        self.declareProperty(StringArrayProperty(name="WorkspaceNames", direction=Direction.Input, validator=validator),
                             doc="List of Workspace names to merge.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace name to save the merged data.")
        H_AXIS = ["2theta", "|Q|", "d-Spacing"]
        self.declareProperty("HorizontalAxis", "2theta", StringListValidator(H_AXIS),
                             doc="X axis in the merged workspace")
        return

    def _expand_groups(self, input_list):
        """
            returns names of the grouped workspaces
        """
        workspaces = []
        for wsname in input_list:
            wks = api.AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                workspaces.extend(wks.getNames())
            else:
                workspaces.append(wsname)
        self.log().notice("Workspaces: " + str(workspaces))
        return workspaces

    def validateInputs(self):
        issues = dict()
        input_list = self.getProperty("WorkspaceNames").value

        for wsname in input_list:
            if not api.AnalysisDataService.doesExist(wsname):
                issues["WorkspaceNames"] = "Workspace " + wsname + " does not exist"

        workspace_names = self._expand_groups(input_list)
        if len(workspace_names) < 2:
            issues["WorkspaceNames"] = "At least 2 workspaces required."

        ws0 = api.AnalysisDataService.retrieve(workspace_names[0])
        ndims = ws0.getNumDims()
        nhists = ws0.getNumberHistograms()
        nblocks = ws0.blocksize()
        # workspaces must exist and have the same dimensions
        for wsname in workspace_names[1:]:
            wks = api.AnalysisDataService.retrieve(wsname)
            if wks.getNumDims() != ndims:
                issues["WorkspaceNames"] = "Number of dimensions for workspace " + wks.name() + \
                    " does not match to one for " + ws0.name()
            if wks.getNumberHistograms() != nhists:
                issues["WorkspaceNames"] = "Number of histohrams for workspace " + wks.name() + \
                    " does not match to one for " + ws0.name()
            if wks.blocksize() != nblocks:
                issues["WorkspaceNames"] = "Number of blocks for workspace " + wks.name() + \
                    " does not match to one for " + ws0.name()
        # workspaces must have the same wavelength and normalization
        result = api.CompareSampleLogs(workspace_names, 'wavelength,normalized', 0.01)
        if len(result) > 0:
            issues["WorkspaceNames"] = "Cannot merge workspaces with different " + result

        return issues

    def _merge_workspaces(self):
        """
        merges given workspaces into one
            @param norm If True, normalization workspaces will be merged
        """
        arr = []
        beamDirection = V3D(0, 0, 1)
        # merge workspaces, existance has been checked by _can_merge function
        for ws_name in self.workspace_names:
            wks = api.AnalysisDataService.retrieve(ws_name)
            samplePos = wks.getInstrument().getSample().getPos()
            n_hists = wks.getNumberHistograms()
            two_theta = np.array([wks.getDetector(i).getTwoTheta(samplePos, beamDirection) for i in range(0, n_hists)])
            # round to approximate hardware accuracy 0.05 degree ~ 1 mrad
            two_theta = np.round(two_theta, 4)
            dataY = np.rot90(wks.extractY())[0]
            dataE = np.rot90(wks.extractE())[0]
            for i in range(n_hists):
                arr.append([two_theta[i], dataY[i], dataE[i]])
        data = np.array(arr)
        # sum up intensities for dublicated angles
        data_sorted = data[np.argsort(data[:, 0])]
        # unique values
        unX = np.unique(data_sorted[:, 0])
        if len(data_sorted[:, 0]) - len(unX) > 0:
            arr = []
            self.log().information("There are dublicated 2Theta angles in the dataset. Sum up the intensities.")
            # we must sum up the values
            for i in range(len(unX)):
                idx = np.where(np.fabs(data_sorted[:, 0] - unX[i]) < 1e-4)
                new_y = sum(data_sorted[idx][:, 1])
                err = data_sorted[idx][:, 2]
                new_e = np.sqrt(np.dot(err, err))
                arr.append([unX[i], new_y, new_e])
            data = np.array(arr)

        # define x axis
        wks = api.AnalysisDataService.retrieve(self.workspace_names[0])
        wavelength = wks.getRun().getProperty('wavelength').value
        if self.xaxis == "2theta":
            data[:, 0] = np.round(np.degrees(data[:, 0]), 2)
            unitx = "Degrees"
        elif self.xaxis == "|Q|":
            data[:, 0] = np.fabs(4.0*np.pi*np.sin(0.5*data[:, 0])/wavelength)
            unitx = "MomentumTransfer"
        elif self.xaxis == "d-Spacing":
            data[:, 0] = np.fabs(0.5*wavelength/np.sin(0.5*data[:, 0]))
            unitx = "dSpacing"
        else:
            message = "The value for X axis " + self.xaxis + " is invalid! Cannot merge."
            self.log().error(message)
            raise RuntimeError(message)

        data_sorted = data[np.argsort(data[:, 0])]
        api.CreateWorkspace(dataX=data_sorted[:, 0], dataY=data_sorted[:, 1], dataE=data_sorted[:, 2],
                            UnitX=unitx, OutputWorkspace=self.outws_name)
        outws = api.AnalysisDataService.retrieve(self.outws_name)
        # assume that all input workspaces have the same YUnits and YUnitLabel
        wks = api.AnalysisDataService.retrieve(self.workspace_names[0])
        outws.setYUnit(wks.YUnit())
        outws.setYUnitLabel(wks.YUnitLabel())

        return

    def PyExec(self):
        # Input
        input_list = self.getProperty("WorkspaceNames").value
        self.outws_name = self.getProperty("OutputWorkspace").valueAsStr
        self.xaxis = self.getProperty("HorizontalAxis").value

        self.workspace_names = self._expand_groups(input_list)
        self.log().information("Workspaces to merge: %i" % (len(self.workspace_names)))
        # produce warnings is some optional sample logs do not match
        result = api.CompareSampleLogs(self.workspace_names, self.properties_to_compare, 1e-2)

        self._merge_workspaces()

        outws = api.AnalysisDataService.retrieve(self.outws_name)
        api.CopyLogs(self.workspace_names[0], outws)
        # remove logs which do not match
        if result:
            api.RemoveLogs(outws, result)

        self.setProperty("OutputWorkspace", outws)
        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSMergeRuns)
