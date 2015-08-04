# pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction
import numpy as np


class DNSDetEffCorrVana(PythonAlgorithm):
    """
    Peforms detector efficiency correction on a given data workspace
    using the Vanadium data.
    This algorithm is written for the DNS @ MLZ,
    but can be used for other instruments if needed.
    """
    properties_to_compare = ['deterota', 'wavelength', 'slit_i_left_blade_position',
                             'slit_i_right_blade_position', 'slit_i_lower_blade_position',
                             'slit_i_upper_blade_position', 'polarisation', 'flipper']

    def category(self):
        """
        Returns category
        """
        return 'PythonAlgorithms\\MLZ\\DNS\\CorrectionFunctions\\EfficiencyCorrections'

    def name(self):
        """
        Returns name
        """
        return "DNSDetEffCorrVana"

    def summary(self):
        return " Peforms detector efficiency correction \
                on a given data workspace using the Vanadium data."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
                             doc="A workspace with experimental data from sample.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace to save the corrected data.")
        self.declareProperty(MatrixWorkspaceProperty("VanaWorkspace", "", direction=Direction.Input),
                             doc="A workspace with Vanadium data.")
        self.declareProperty(MatrixWorkspaceProperty("BkgWorkspace", "", direction=Direction.Input),
                             doc="A workspace with background for Vanadium data.")
        self.declareProperty("VanadiumMean", "", direction=Direction.Input,
                             doc="A 2x1 matrix workspace with mean Vanadium counts (optional).")

        return

    def _dimensions_valid(self, data_ws, vana_ws, bkgr_ws):
        """
        Checks validity of the workspace dimensions:
        all given workspaces must have the same number of dimensions
        and the same number of histograms
        and the same number of bins
        """
        nd = data_ws.getNumDims()
        nh = data_ws.getNumberHistograms()
        nb = data_ws.blocksize()

        if vana_ws.getNumDims() != nd or vana_ws.getNumberHistograms() != nh or vana_ws.blocksize() != nb:
            self.log().error("The dimensions of Vanadium workspace are not valid.")
            return False

        if bkgr_ws.getNumDims() != nd or bkgr_ws.getNumberHistograms() != nh or bkgr_ws.blocksize() != nb:
            self.log().error("The dimensions of Background workspace are not valid.")
            return False

        return True

    def _norm_ws_exist(self, vana_ws, bkgr_ws):
        """
        Checks whether the normalization workspaces exist
        normalization workspaces are created by the loader,
        but could be ocasionally deleted by the user
        """
        if not api.mtd.doesExist(bkgr_ws.getName() + '_NORM'):
            self.log().error("Normalization workspace for " + bkgr_ws.getName() + " is not found!")
            return False
        if not api.mtd.doesExist(vana_ws.getName() + '_NORM'):
            self.log().error("Normalization workspace for " + vana_ws.getName() + " is not found!")
            return False

        return True

    def _vana_mean(self, vana_ws, vana_mean_ws_name, nd=2):
        """
        checks whether the workspace with mean counts for Vanadium exists
        creates one if not
        """
        if api.mtd.doesExist(vana_mean_ws_name):
            vana_mean = api.mtd[vana_mean_ws_name]
            if vana_mean.getNumDims() != nd:
                message = "Specified VanadiumMean Workspace has wrong dimensions! Must be 2."
                self.log().error(message)
                return None
            if vana_mean.getNumberHistograms() != 1:
                message = "Specified VanadiumMean Workspace has wrong number of histograms! Must be 1."
                self.log().error(message)
                return None
            if vana_mean.blocksize() != 1:
                message = "Specified VanadiumMean Workspace has wrong number of bins! Must be 1."
                self.log().error(message)
                return None

        else:
            nh = vana_ws.getNumberHistograms()
            vana_mean = api.SumSpectra(vana_ws, IncludeMonitors=False) / nh

        return vana_mean

    def _cleanup(self, wslist):
        """
        deletes workspaces from list
        """
        for w in wslist:
            api.DeleteWorkspace(w)
        return

    def _vana_correct(self, data_ws, vana_ws, bkgr_ws, vana_mean_ws_name, outws_name):
        """
        creates the corrected workspace
        """
        wslist = []
        # 1. normalize Vanadium and Background
        vana_normws = api.mtd[vana_ws.getName() + '_NORM']
        bkg_normws = api.mtd[bkgr_ws.getName() + '_NORM']
        _vana_norm_ = api.Divide(vana_ws, vana_normws)
        wslist.append(_vana_norm_.getName())
        _bkg_norm_ = api.Divide(bkgr_ws, bkg_normws)
        wslist.append(_bkg_norm_.getName())
        # 2. substract background from Vanadium
        _vana_bg_ = _vana_norm_ - _bkg_norm_
        wslist.append(_vana_bg_.getName())
        # check negative values, throw exception
        arr = np.array(_vana_bg_.extractY()).flatten()
        neg_values = np.where(arr < 0)[0]
        if len(neg_values):
            self._cleanup(wslist)
            message = "Background " + bkgr_ws.getName() + " is higher than Vanadium " + \
                vana_ws.getName() + " signal!"
            self.log().error(message)
            raise RuntimeError(message)

        # 3. calculate correction coefficients
        _vana_mean_ws_ = self._vana_mean(_vana_bg_, vana_mean_ws_name, data_ws.getNumDims())
        if not _vana_mean_ws_:
            self._cleanup(wslist)
            return None
        if not vana_mean_ws_name:
            wslist.append(_vana_mean_ws_)
        _coef_ws_ = api.Divide(LHSWorkspace=_vana_bg_, RHSWorkspace=_vana_mean_ws_, WarnOnZeroDivide=True)
        wslist.append(_coef_ws_)
        # 4. correct raw data (not normalized!)
        api.Divide(LHSWorkspace=data_ws, RHSWorkspace=_coef_ws_, WarnOnZeroDivide=True,
                   OutputWorkspace=outws_name)
        outws = api.mtd[outws_name]
        # cleanup
        self._cleanup(wslist)
        return outws

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

    def PyExec(self):
        # Input
        dataws = api.mtd[self.getPropertyValue("InputWorkspace")]
        outws_name = self.getPropertyValue("OutputWorkspace")
        vanaws = api.mtd[self.getPropertyValue("VanaWorkspace")]
        bkgws = api.mtd[self.getPropertyValue("BkgWorkspace")]
        vana_mean = self.getPropertyValue("VanadiumMean")

        if not self._dimensions_valid(dataws, vanaws, bkgws):
            raise RuntimeError("Error: all input workspaces must have the same dimensions!.")
        # check if the _NORM workspaces exist
        if not self._norm_ws_exist(vanaws, bkgws):
            message = "Normalization workspace is not found!"
            raise RuntimeError(message)
        # check sample logs, produce warnings if different
        drun = dataws.getRun()
        vrun = vanaws.getRun()
        brun = bkgws.getRun()
        self._check_properties(drun, vrun)
        self._check_properties(vrun, brun)
        # apply correction
        outws = self._vana_correct(dataws, vanaws, bkgws, vana_mean, outws_name)
        if not outws:
            raise RuntimeError("Correction failed. Invalid VanadiumMean workspace dimensions.")

        # copy sample logs from data workspace to the output workspace
        api.CopyLogs(InputWorkspace=dataws.getName(), OutputWorkspace=outws.getName(),
                     MergeStrategy='MergeReplaceExisting')
        self.setProperty("OutputWorkspace", outws)

        # clone the normalization workspace for data if it exists
        if api.mtd.doesExist(dataws.getName()+'_NORM'):
            api.CloneWorkspace(InputWorkspace=dataws.getName()+'_NORM', OutputWorkspace=outws_name+'_NORM')
        self.log().debug('DNSDetEffCorrVana: OK. The result has been saved to ' + outws_name)

        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSDetEffCorrVana)
