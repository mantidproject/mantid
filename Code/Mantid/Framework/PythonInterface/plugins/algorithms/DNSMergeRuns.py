# pylint: disable=too-many-locals
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty, StringListValidator, V3D
import numpy as np


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
            wks = api.AnalysisDataService.retrieve(wsname)
            ndims.append(wks.getNumDims())
            nhists.append(wks.getNumberHistograms())
            nbins.append(wks.blocksize())

        ndi = ndims[0]
        nhi = nhists[0]
        nbi = nbins[0]
        if ndims.count(ndi) == len(ndims) and nhists.count(nhi) == len(nhists) and nbins.count(nbi) == len(nbins):
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
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            self._check_properties(run1, run)
        return True

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
                message = "Property " + property_name + " is not present in " +\
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
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            if run.hasProperty('wavelength'):
                wls.append(round(run.getProperty('wavelength').value, 3))
            else:
                message = " Workspace " + wks.getName() + " does not have property wavelength! Cannot merge."
                self.log().error(message)
                return False
        wlength = wls[0]
        if wls.count(wlength) == len(wls):
            self.wavelength = wlength
            self.log().information("The wavelength is " + str(self.wavelength) + " Angstrom.")
            return True
        else:
            message = "Cannot merge workspaces with different wavelength!"
            self.log().error(message)
            return False

    def _merge_workspaces(self):
        """
        merges given workspaces into one,
        corresponding normalization workspaces are also merged
        """
        arr = []
        arr_norm = []
        beamDirection = V3D(0, 0, 1)
        # merge workspaces, existance has been checked by _can_merge function
        for ws_name in self.workspace_names:
            wks = api.AnalysisDataService.retrieve(ws_name)
            wsnorm = api.AnalysisDataService.retrieve(ws_name + '_NORM')
            samplePos = wks.getInstrument().getSample().getPos()
            n_hists = wks.getNumberHistograms()
            two_theta = np.array([wks.getDetector(i).getTwoTheta(samplePos, beamDirection) for i in range(0, n_hists)])
            # round to approximate hardware accuracy 0.05 degree ~ 1 mrad
            two_theta = np.round(two_theta, 4)
            dataY = np.rot90(wks.extractY())[0]
            dataYnorm = np.rot90(wsnorm.extractY())[0]
            dataE = np.rot90(wks.extractE())[0]
            dataEnorm = np.rot90(wsnorm.extractE())[0]
            for i in range(n_hists):
                arr.append([two_theta[i], dataY[i], dataE[i]])
                arr_norm.append([two_theta[i], dataYnorm[i], dataEnorm[i]])
        data = np.array(arr)
        norm = np.array(arr_norm)
        # sum up intensities for dublicated angles
        data_sorted = data[np.argsort(data[:, 0])]
        norm_sorted = norm[np.argsort(norm[:, 0])]
        # unique values
        unX = np.unique(data_sorted[:, 0])
        if len(data_sorted[:, 0]) - len(unX) > 0:
            arr = []
            arr_norm = []
            self.log().information("There are dublicated 2Theta angles in the dataset. Sum up the intensities.")
            # we must sum up the values
            for i in range(len(unX)):
                idx = np.where(np.fabs(data_sorted[:, 0] - unX[i]) < 1e-4)
                new_y = sum(data_sorted[idx][:, 1])
                new_y_norm = sum(norm_sorted[idx][:, 1])
                err = data_sorted[idx][:, 2]
                err_norm = norm_sorted[idx][:, 2]
                new_e = np.sqrt(np.dot(err, err))
                new_e_norm = np.sqrt(np.dot(err_norm, err_norm))
                arr.append([unX[i], new_y, new_e])
                arr_norm.append([unX[i], new_y_norm, new_e_norm])
            data = np.array(arr)
            norm = np.array(arr_norm)

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
                            UnitX=unitx, OutputWorkspace=self.outws_name)
        norm[:, 0] = data[:, 0]
        norm_sorted = norm[np.argsort(norm[:, 0])]
        api.CreateWorkspace(dataX=norm_sorted[:, 0], dataY=norm_sorted[:, 1], dataE=norm_sorted[:, 2],
                            UnitX=unitx, OutputWorkspace=self.outws_name + '_NORM')
        return

    def PyExec(self):
        # Input
        normalize = self.getProperty("Normalize").value
        self.workspace_names = self.getProperty("WorkspaceNames").value
        self.outws_name = self.getProperty("OutputWorkspace").valueAsStr
        self.xaxis = self.getProperty("HorizontalAxis").value

        self.log().information("Workspaces to merge: %i" % (len(self.workspace_names)))
        # check whether given workspaces can be merged
        if not self._can_merge():
            message = "Impossible to merge given workspaces."
            raise RuntimeError(message)

        self._merge_workspaces()
        outws = api.AnalysisDataService.retrieve(self.outws_name)
        api.CopyLogs(self.workspace_names[0], outws)
        api.RemoveLogs(outws, self.sample_logs)
        yunit = 'Counts'
        ylabel = 'Intensity'

        if normalize:
            normws = api.AnalysisDataService.retrieve(self.outws_name + '_NORM')
            api.Divide(LHSWorkspace=outws, RHSWorkspace=normws, OutputWorkspace=self.outws_name)
            api.DeleteWorkspace(normws)
            yunit = ""
            ylabel = "Normalized Intensity"

        outws.setYUnit(yunit)
        outws.setYUnitLabel(ylabel)

        self.setProperty("OutputWorkspace", outws)
        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSMergeRuns)
