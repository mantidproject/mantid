import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty, StringListValidator, V3D
import numpy as np

import mlzutils


class DNSMergeRuns(PythonAlgorithm):
    """
    Merges given runs into one matrix workspace.
    This algorithm is written for the DNS @ MLZ,
    but can be adjusted for other instruments if needed.
    """
    properties_to_compare = ['omega', 'slit_i_left_blade_position',
                             'slit_i_right_blade_position', 'slit_i_lower_blade_position',
                             'slit_i_upper_blade_position', 'polarisation', 'flipper']
    sample_logs = properties_to_compare + ['wavelength', 'huber', 'T1', 'T2', 'Tsp', 'Ei']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.workspace_names = []
        self.wavelength = 0
        self.xaxis = '2theta'
        self.outws_name = None
        self.is_normalized = False

    def category(self):
        """
        Returns category
        """
        return 'PythonAlgorithms\\MLZ\\DNS'

    def name(self):
        """
        Returns name
        """
        return "DNSMergeRuns"

    def summary(self):
        return "Merges runs performed at different detector bank positions into one matrix workspace."

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="WorkspaceNames",
                                                 direction=Direction.Input),
                             doc="List of Workspace names to merge.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace name to save the merged data.")
        H_AXIS = ["2theta", "|Q|", "d-Spacing"]
        self.declareProperty("HorizontalAxis", "2theta", StringListValidator(H_AXIS),
                             doc="X axis in the merged workspace")
        self.declareProperty("Normalize", False, "If checked, the merged data will be normalized, "
                             "otherwise the separate normalization workspace will be created.")
        return

    def _can_merge(self):
        """
        checks whether it is possible to merge the given list of workspaces
        """
        # list of workspaces must not be empty
        if not self.workspace_names:
            message = "No workspace names has been specified! Nothing to merge."
            self.log().error(message)
            raise RuntimeError(message)

        # workspaces must exist
        mlzutils.ws_exist(self.workspace_names, self.log())

        # all workspaces must be either normalized or not normalized, but not mixed
        self._are_normalized()

        # if data are not normalized, normalization workspaces must exist
        if not self.is_normalized:
            wslist = [wsname + '_NORM' for wsname in self.workspace_names]
            mlzutils.ws_exist(wslist, self.log())

        # they must have the same dimensions
        mlzutils.same_dimensions(self.workspace_names)

        # and the same wavelength
        self._same_wavelength()

        # algorithm must warn if some properties_to_compare are different
        ws1 = api.AnalysisDataService.retrieve(self.workspace_names[0])
        run1 = ws1.getRun()
        for wsname in self.workspace_names[1:]:
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            mlzutils.compare_properties(run1, run, self.properties_to_compare, self.log())
        return True

    def _same_wavelength(self):
        """
        Checks whether all workspaces have the same wavelength.
        Raises error if not,
        sets self.wavelength otherwise
        """
        wls = []
        for wsname in self.workspace_names:
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            if run.hasProperty('wavelength'):
                wls.append(round(run.getProperty('wavelength').value, 3))
            else:
                message = " Workspace " + wks.getName() + " does not have property wavelength! Cannot merge."
                self.log().error(message)
                raise RuntimeError(message)
        wlength = wls[0]
        if wls.count(wlength) == len(wls):
            self.wavelength = wlength
            self.log().information("The wavelength is " + str(self.wavelength) + " Angstrom.")
            return True
        else:
            message = "Cannot merge workspaces with different wavelength!"
            self.log().error(message)
            raise RuntimeError(message)

    def _are_normalized(self):
        """
        Checks whether the given workspaces are normalized
            @ returns True if all given workspaces are normalized
            @ returns False if all given workspaces are not normalized
            raises exception if mixed workspaces are given
            or if no 'normalized' sample log has been found
        """
        norms = []
        for wsname in self.workspace_names:
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            if run.hasProperty('normalized'):
                norms.append(run.getProperty('normalized').value)
            else:
                message = " Workspace " + wks.getName() + " does not have property normalized! Cannot merge."
                self.log().error(message)
                raise RuntimeError(message)

        is_normalized = norms[0]
        if norms.count(is_normalized) == len(norms):
            if is_normalized == 'yes':
                self.is_normalized = True
                self.log().information("Merge normalized workspaces")
                return True
            else:
                self.is_normalized = False
                self.log().information("Merge not normalized workspaces")
                return False
        else:
            message = "Cannot merge workspaces with different wavelength!"
            self.log().error(message)
            raise RuntimeError(message)

        return True

    def _merge_workspaces(self, norm=False):
        """
        merges given workspaces into one
            @param norm If True, normalization workspaces will be merged
        """
        arr = []
        beamDirection = V3D(0, 0, 1)
        if norm:
            suffix = '_NORM'
        else:
            suffix = ''
        # merge workspaces, existance has been checked by _can_merge function
        for ws_name in self.workspace_names:
            wks = api.AnalysisDataService.retrieve(ws_name + suffix)
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
        if self.xaxis == "2theta":
            data[:, 0] = np.round(np.degrees(data[:, 0]), 2)
            unitx = "Degrees"
        elif self.xaxis == "|Q|":
            data[:, 0] = np.fabs(4.0*np.pi*np.sin(0.5*data[:, 0])/self.wavelength)
            unitx = "MomentumTransfer"
        elif self.xaxis == "d-Spacing":
            data[:, 0] = np.fabs(0.5*self.wavelength/np.sin(0.5*data[:, 0]))
            unitx = "dSpacing"
        else:
            message = "The value for X axis " + self.xaxis + " is invalid! Cannot merge."
            self.log().error(message)
            raise RuntimeError(message)

        data_sorted = data[np.argsort(data[:, 0])]
        api.CreateWorkspace(dataX=data_sorted[:, 0], dataY=data_sorted[:, 1], dataE=data_sorted[:, 2],
                            UnitX=unitx, OutputWorkspace=self.outws_name + suffix)
        outws = api.AnalysisDataService.retrieve(self.outws_name + suffix)
        # assume that all input workspaces have the same YUnits and YUnitLabel
        wks = api.AnalysisDataService.retrieve(self.workspace_names[0] + suffix)
        outws.setYUnit(wks.YUnit())
        outws.setYUnitLabel(wks.YUnitLabel())

        return

    def PyExec(self):
        # Input
        normalize = self.getProperty("Normalize").value
        self.workspace_names = self.getProperty("WorkspaceNames").value
        self.outws_name = self.getProperty("OutputWorkspace").valueAsStr
        self.xaxis = self.getProperty("HorizontalAxis").value

        self.log().information("Workspaces to merge: %i" % (len(self.workspace_names)))
        # check whether given workspaces can be merged
        self._can_merge()

        if self.is_normalized and normalize:
            message = "Invalid setting for Normalize property. The given workspaces are already normalized."
            self.log().error(message)
            raise RuntimeError(message)

        self._merge_workspaces()
        if not self.is_normalized:
            self._merge_workspaces(norm=True)

        outws = api.AnalysisDataService.retrieve(self.outws_name)
        api.CopyLogs(self.workspace_names[0], outws)
        api.RemoveLogs(outws, self.sample_logs)

        if normalize:
            normws = api.AnalysisDataService.retrieve(self.outws_name + '_NORM')
            run = normws.getRun()
            if run.hasProperty('normalization'):
                norm = run.getProperty('normalization').value
                if norm == 'duration':
                    yunit = "Counts per second"
                elif norm == 'monitor':
                    yunit = "Counts per monitor"
                else:
                    yunit = ""
            else:
                yunit = ""
            ylabel = "Normalized Intensity"
            api.Divide(LHSWorkspace=outws, RHSWorkspace=normws, OutputWorkspace=self.outws_name)
            api.DeleteWorkspace(normws)
            outws.setYUnit(yunit)
            outws.setYUnitLabel(ylabel)

        self.setProperty("OutputWorkspace", outws)
        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSMergeRuns)
