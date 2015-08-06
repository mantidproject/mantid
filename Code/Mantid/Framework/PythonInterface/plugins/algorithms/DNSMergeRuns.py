# pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty, StringListValidator, V3D
import numpy as np
from math import pi


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
    workspace_names = []
    wavelength = 0

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
        return "Merges runs made at different detector bank positions into one matrix workspace."

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="WorkspaceNames",
                                                 direction=Direction.Input),
                             doc="List of Workspace names to merge.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace name to save the merged data.")
        H_AXIS = ["2theta", "|Q|", "d-Spacing"]
        self.declareProperty("HorizontalAxis", "2theta", StringListValidator(H_AXIS),
                             doc="X axis in the merged workspace")
        self.declareProperty("Normalize", False, "If checked, the merged data will be normalized,"
                             "otherwise the separate normalization workspace will be produced.")
        return

    def _dimensions_valid(self):
        """
        Checks validity of the workspace dimensions:
        all given workspaces must have the same number of dimensions
        and the same number of histograms
        and the same number of bins
        """
        ndims = []
        nhists = []
        nbins = []
        for wsname in self.workspace_names:
            ws = api.AnalysisDataService.retrieve(wsname)
            ndims.append(ws.getNumDims())
            nhists.append(ws.getNumberHistograms())
            nbins.append(ws.blocksize())

        nd = ndims[0]
        nh = nhists[0]
        nb = nbins[0]
        if ndims.count(nd) == len(ndims) and nhists.count(nh) == len(nhists) and nbins.count(nb) == len(nbins):
            return True
        else:
            message = "Cannot merge workspaces with different dimensions."
            self.log().error(message)
            return False

    def _ws_exist(self):
        """
        Checks whether the workspace and its normalization workspaces exist
        """
        for wsname in self.workspace_names:
            if not api.AnalysisDataService.doesExist(wsname):
                message = "Workspace " + wsname + " does not exist! Cannot merge."
                self.log().error(message)
                return False
            if not api.AnalysisDataService.doesExist(wsname + '_NORM'):
                message = "Workspace " + wsname + "_NORM does not exist! Cannot merge."
                self.log().error(message)
                return False

        return True

    def _can_merge(self):
        """
        checks whether it is possible to merge the given list of workspaces
        """
        # list of workspaces must not be empty
        if not self.workspace_names:
            message = "No workspace names has been specified! Nothing to merge."
            self.log().error(message)
            return False
        # workspaces and normalization workspaces must exist
        if not self._ws_exist():
            return False
        # they must have the same dimensions
        if not self._dimensions_valid():
            return False
        # and the same wavelength
        if not self._same_wavelength():
            return False
        # algorithm must warn if some properties_to_compare are different
        ws1 = api.AnalysisDataService.retrieve(self.workspace_names[0])
        run1 = ws1.getRun()
        for wsname in self.workspace_names[1:]:
            ws = api.AnalysisDataService.retrieve(wsname)
            run = ws.getRun()
            self._check_properties(run1, run)
        return True

    def _cleanup(self, wslist):
        """
        deletes workspaces from list
        """
        for w in wslist:
            api.DeleteWorkspace(w)
        return

    def _check_properties(self, lhs_run, rhs_run):
        """
        checks whether properties match
        """
        lhs_title = ""
        rhs_title = ""
        if lhs_run.hasProperty('run_title'):
            lhs_title = lhs_run.getProperty('run_title').value
        if rhs_run.hasProperty('run_title'):
            rhs_title = rhs_run.getProperty('run_title').value

        for property_name in self.properties_to_compare:
            if lhs_run.hasProperty(property_name) and rhs_run.hasProperty(property_name):
                lhs_property = lhs_run.getProperty(property_name)
                rhs_property = rhs_run.getProperty(property_name)
                if lhs_property.type == rhs_property.type:
                    if lhs_property.type == 'string':
                        if lhs_property.value != rhs_property.value:
                            message = "Property " + property_name + " does not match! " + \
                                lhs_title + ": " + lhs_property.value + ", but " + \
                                rhs_title + ": " + rhs_property.value
                            self.log().warning(message)
                    if lhs_property.type == 'number':
                        if abs(lhs_property.value - rhs_property.value) > 5e-3:
                            message = "Property " + property_name + " does not match! " + \
                                lhs_title + ": " + str(lhs_property.value) + ", but " + \
                                rhs_title + ": " + str(rhs_property.value)
                            self.log().warning(message)
                else:
                    message = "Property " + property_name + " does not match! " + \
                        lhs_title + ": " + str(lhs_property.value) + ", but " + \
                        rhs_title + ": " + str(rhs_property.value)
                    self.log().warning(message)
            else:
                message = "Property ' + property_name + ' is not present in " +\
                    lhs_title + " or " + rhs_title + " - skipping comparison."
                self.log().warning(message)
        return True

    def _same_wavelength(self):
        """
        Checks whether all workspaces have the same wavelength.
        Raises error if not,
        sets self.wavelength otherwise
        """
        wls = []
        for wsname in self.workspace_names:
            ws = api.AnalysisDataService.retrieve(wsname)
            run = ws.getRun()
            if run.hasProperty('wavelength'):
                wls.append(round(run.getProperty('wavelength').value, 3))
            else:
                message = " Workspace " + ws.getName() + " does not have property wavelength! Cannot merge."
                self.log().error(message)
                return False
        wl = wls[0]
        if wls.count(wl) == len(wls):
            self.wavelength = wl
            return True
        else:
            message = "Cannot merge workspaces with different wavelength!"
            self.log().error(message)
            return False

    def PyExec(self):
        # Input
        normalize = self.getProperty("Normalize").value
        self.workspace_names = self.getProperty("WorkspaceNames").value
        outws_name = self.getProperty("OutputWorkspace").valueAsStr
        xaxis = self.getProperty("HorizontalAxis").value

        self.log().information("Workspaces to merge: %i" % (len(self.workspace_names)))
        if not self._can_merge():
            message = "Impossible to merge given workspaces."
            raise RuntimeError(message)

        arr = []
        arr_norm = []
        beamDirection = V3D(0, 0, 1)
        wl = self.wavelength

        for wsName in self.workspace_names:
            if not api.AnalysisDataService.doesExist(wsName):
                message = "Workspace " + wsName + " does not exist! Cannot merge."
                self.log().error(message)
                raise KeyError(message)
            wsnormName = wsName + '_NORM'
            if not api.AnalysisDataService.doesExist(wsnormName):
                message = "Workspace " + wsnormName + " does not exist! Cannot merge."
                self.log().error(message)
                raise KeyError(message)

            ws = api.AnalysisDataService.retrieve(wsName)
            wsnorm = api.AnalysisDataService.retrieve(wsnormName)
            samplePos = ws.getInstrument().getSample().getPos()
            n_hists = ws.getNumberHistograms()
            two_theta = np.array([ws.getDetector(i).getTwoTheta(samplePos, beamDirection) for i in range(0, n_hists)])
            dataY = np.rot90(ws.extractY())[0]
            dataYnorm = np.rot90(wsnorm.extractY())[0]
            dataE = np.rot90(ws.extractE())[0]
            dataEnorm = np.rot90(wsnorm.extractE())[0]
            for i in range(n_hists):
                arr.append([two_theta[i], dataY[i], dataE[i]])
                arr_norm.append([two_theta[i], dataYnorm[i], dataEnorm[i]])
        data = np.array(arr)
        norm = np.array(arr_norm)
        if xaxis == "2theta":
            data[:, 0] = np.degrees(data[:, 0])
            unitx = "Degrees"
        elif xaxis == "|Q|":
            data[:, 0] = np.fabs(4.0*pi*np.sin(data[:, 0])/wl)
            unitx = "MomentumTransfer"
        elif xaxis == "d-Spacing":
            data[:, 0] = np.fabs(0.5*wl/np.sin(data[:, 0]))
            unitx = "dSpacing"
        else:
            message = "The value for X axis " + xaxis + " is invalid! Cannot merge."
            self.log().error(message)
            raise RuntimeError(message)

        data_sorted = data[np.argsort(data[:, 0])]
        api.CreateWorkspace(dataX=data_sorted[:, 0], dataY=data_sorted[:, 1], dataE=data_sorted[:, 2],
                            UnitX=unitx, OutputWorkspace=outws_name)
        norm[:, 0] = data[:, 0]
        norm_sorted = norm[np.argsort(norm[:, 0])]
        api.CreateWorkspace(dataX=norm_sorted[:, 0], dataY=norm_sorted[:, 1], dataE=norm_sorted[:, 2],
                            UnitX=unitx, OutputWorkspace=outws_name + '_NORM')

        outws = api.AnalysisDataService.retrieve(outws_name)
        api.CopyLogs(self.workspace_names[0], outws)
        api.RemoveLogs(outws, self.sample_logs)

        if normalize:
            normws = api.AnalysisDataService.retrieve(outws_name + '_NORM')
            api.Divide(LHSWorkspace=outws, RHSWorkspace=normws, OutputWorkspace=outws_name)
            api.DeleteWorkspace(normws)

        self.setProperty("OutputWorkspace", outws)
        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSMergeRuns)
