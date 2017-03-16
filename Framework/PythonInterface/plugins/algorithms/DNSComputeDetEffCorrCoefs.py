from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, WorkspaceGroup
from mantid.kernel import Direction, StringArrayProperty, StringArrayLengthValidator, FloatBoundedValidator
import numpy as np


class DNSComputeDetEffCorrCoefs(PythonAlgorithm):
    """
    Computes the detector efficiency correction coefficients
    using the Vanadium data.
    This algorithm is written for the DNS @ MLZ,
    but can be used for other instruments if needed.
    """
    properties_to_compare = ['wavelength', 'slit_i_left_blade_position',
                             'slit_i_right_blade_position', 'slit_i_lower_blade_position',
                             'slit_i_upper_blade_position', 'polarisation', 'polarisation_comment']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.toremove = []

    def category(self):
        """
        Returns category
        """
        return 'Workflow\\MLZ\\DNS;CorrectionFunctions\\SpecialCorrections'

    def name(self):
        """
        Returns name
        """
        return "DNSComputeDetEffCorrCoefs"

    def summary(self):
        return "Computes detector efficiency correction coefficients using the Vanadium data."

    def PyInit(self):
        validator = StringArrayLengthValidator()
        validator.setLengthMin(2)       # even for workspacegroups at least 2 (SF, NSF) must be given

        self.declareProperty(StringArrayProperty(name="VanadiumWorkspaces", direction=Direction.Input, validator=validator),
                             doc="Comma separated list of Vanadium workspaces or groups of workspaces.")
        self.declareProperty(StringArrayProperty(name="BackgroundWorkspaces", direction=Direction.Input, validator=validator),
                             doc="Comma separated list of Background workspaces or groups of workspaces.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name of the workspace or group of workspaces that will contain computed coefficients.")
        self.declareProperty("TwoThetaTolerance", 0.05, FloatBoundedValidator(lower=0, upper=0.1),
                             doc="Tolerance for 2theta comparison (degrees). Number between 0 and 0.1.")

        return

    def validateInputs(self):
        issues = dict()
        # workspaces or groups must exist
        vana_workspaces = self.getProperty("VanadiumWorkspaces").value
        for wsname in vana_workspaces:
            if not api.AnalysisDataService.doesExist(wsname):
                issues["VanadiumWorkspaces"] = "Workspace " + wsname + " does not exist!"
        bg_workspaces = self.getProperty("BackgroundWorkspaces").value
        for wsname in bg_workspaces:
            if not api.AnalysisDataService.doesExist(wsname):
                issues["BackgroundWorkspaces"] = "Workspace " + wsname + " does not exist!"

        workspace_names = self._expand_groups(vana_workspaces)
        workspace_names.extend(self._expand_groups(bg_workspaces))

        # workspaces must have the same normalization
        result = api.CompareSampleLogs(workspace_names, 'normalized', 0.01)
        if len(result) > 0:
            issues["WorkspaceNames"] = "Workspaces must have the same normalization."

        return issues

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
        return workspaces

    def cleanup(self, wslist):
        """
        deletes workspaces from list
        @param wslist List of names of workspaces to delete.
        """
        if len(wslist) < 1:
            return
        for wsname in wslist:
            if api.AnalysisDataService.doesExist(wsname):
                delete = self.createChildAlgorithm('DeleteWorkspace')
                delete.setProperty("Workspace", wsname)
                delete.execute()
        return

    def _get_detector_positions(self, wslist):
        """
        get list of detector positions from the given list of workspaces
        """
        deterota = []
        for wsname in wslist:
            run = api.AnalysisDataService.retrieve(wsname).getRun()
            angle = run.getProperty('deterota').value
            if angle not in deterota:
                deterota.append(angle)
        return deterota

    def _sort_workspaces(self, wslist, deterota):
        """
        sorts workspaces according to their flipper status and detector positions
        """
        tol = self.getProperty("TwoThetaTolerance").value
        sfdata = dict.fromkeys(deterota)
        nsfdata = dict.fromkeys(deterota)
        for wsname in wslist:
            run = api.AnalysisDataService.retrieve(wsname).getRun()
            wsangle = run.getProperty('deterota').value
            flipper = run.getProperty('flipper').value
            for key in deterota:
                if np.fabs(wsangle - key) < tol:
                    angle = key
            if flipper == 'ON':
                sfdata[angle] = wsname
            else:
                nsfdata[angle] = wsname

        return sfdata, nsfdata

    def _is_negative(self, wsname):
        """
        checks for negative values in the given workspace
        """
        wks = api.AnalysisDataService.retrieve(wsname)
        arr = np.array(wks.extractY()).flatten()
        neg_values = np.where(arr < 0)[0]
        if len(neg_values):
            return True
        return False

    def _subtract_background(self, datalist, bglist, deterota):
        """
        subracts background
        """
        result = dict.fromkeys(deterota)
        for angle in deterota:
            wsname = datalist[angle] + "-bg"
            api.Minus(datalist[angle], bglist[angle], OutputWorkspace=wsname)
            self.toremove.append(wsname)
            if self._is_negative(wsname):
                message = "Background " + bglist[angle] + " is higher than Vanadium " + datalist[angle] + " signal!"
                self.cleanup(self.toremove)
                raise RuntimeError(message)
            result[angle] = wsname

        return result

    def _get_notmasked_detectors_number(self, workspace):
        """
        returns number of not masked detectors
        """
        num = 0
        spectrumInfo = workspace.spectrumInfo()
        for idx in range(workspace.getNumberHistograms()):
            if not spectrumInfo.isMasked(idx):
                num += 1
        return num

    def _sum_signal(self, datasf, datansf, deterota):
        result = dict.fromkeys(deterota)
        for angle in deterota:
            wsname = 'sum' + str(angle)
            api.Plus(datasf[angle], datansf[angle], OutputWorkspace=wsname)
            self.toremove.append(wsname)
            result[angle] = wsname
        return result

    def PyExec(self):
        # Input
        vana_input = self.getProperty("VanadiumWorkspaces").value
        bg_input = self.getProperty("BackgroundWorkspaces").value
        vana_workspaces = self._expand_groups(vana_input)
        bg_workspaces = self._expand_groups(bg_input)
        self.log().notice("Input Vanadium workspaces: " + str(vana_workspaces))
        self.log().notice("Input Background workspaces: " + str(bg_workspaces))

        # number of vanadium and background workspaces must match
        if len(vana_workspaces) != len(bg_workspaces):
            raise RuntimeError("Number of Vanadium and background workspaces doe not match!")

        # compare optional sample logs, throw warnings
        result = api.CompareSampleLogs(vana_workspaces+bg_workspaces, self.properties_to_compare, 5e-3)
        if result:
            self.log().warning("Following properties do not match: " + result)

        # split input workspaces to groups SF/NSF and detector angles
        deterota = self._get_detector_positions(vana_workspaces)
        sfvana, nsfvana = self._sort_workspaces(vana_workspaces, deterota)
        sfbg, nsfbg = self._sort_workspaces(bg_workspaces, deterota)

        # subract background
        sfv = self._subtract_background(sfvana, sfbg, deterota)
        nsfv = self._subtract_background(nsfvana, nsfbg, deterota)
        total = self._sum_signal(sfv, nsfv, deterota)

        # compute vmean
        _mean_ws_ = api.Mean(",".join(list(total.values())))     # Mean takes string
        self.toremove.append(_mean_ws_.name())
        num =  self._get_notmasked_detectors_number(_mean_ws_)
        if num == 0:
            self.cleanup(self.toremove)
            raise RuntimeError("All detectors are masked! Cannot compute coefficients.")
        _vana_mean_ = api.SumSpectra(_mean_ws_)/num
        self.toremove.append(_vana_mean_.name())

        # compute coefficients k_i = (VSF_i + VNSF_i)/Vmean
        outws_name = self.getPropertyValue("OutputWorkspace")
        # for only one detector position only one workspace will be created
        if len(deterota) == 1:
            api.Divide(list(total.values())[0], _vana_mean_, OutputWorkspace=outws_name)
        else:
            # for many detector positions group of workspaces will be created
            results = []
            for angle in deterota:
                wsname = outws_name + '_2theta' + str(angle)
                api.Divide(total[angle], _vana_mean_, OutputWorkspace=wsname)
                results.append(wsname)

            api.GroupWorkspaces(results, OutputWorkspace=outws_name)

        self.cleanup(self.toremove)
        outws = api.AnalysisDataService.retrieve(outws_name)
        self.setProperty("OutputWorkspace", outws)

        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSComputeDetEffCorrCoefs)
