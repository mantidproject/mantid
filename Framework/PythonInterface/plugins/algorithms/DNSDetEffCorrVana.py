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

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.dataws = None
        self.outws_name = None
        self.vanaws = None
        self.bkgws = None
        self.vana_mean_name = None

    def category(self):
        """
        Returns category
        """
        return 'PythonAlgorithms\\MLZ\\DNS;CorrectionFunctions\\EfficiencyCorrections'

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

    def _vana_mean(self, vanaws):
        """
        checks whether the workspace with mean counts for Vanadium exists
        creates one if not.
            @param vanaws A MatrixWorkspace with Vanadium counts. Will be used to create VanadiumMean workspace if needed.
            @returns  A MatrixWorkspace with Vanadium mean counts.
        """
        if api.mtd.doesExist(self.vana_mean_name):
            ndims = self.dataws.getNumDims()
            vmean = api.mtd[self.vana_mean_name]
            if vmean.getNumDims() != ndims:
                message = "Specified VanadiumMean Workspace has wrong dimensions! Must be 2."
                self.log().error(message)
                return None
            if vmean.getNumberHistograms() != 1:
                message = "Specified VanadiumMean Workspace has wrong number of histograms! Must be 1."
                self.log().error(message)
                return None
            if vmean.blocksize() != 1:
                message = "Specified VanadiumMean Workspace has wrong number of bins! Must be 1."
                self.log().error(message)
                return None

        else:
            nhists = self.vanaws.getNumberHistograms()
            vmean = api.SumSpectra(vanaws, IncludeMonitors=False) / nhists

        return vmean

    def cleanup(self, wslist):
        """
        deletes workspaces from list
        @param wslist List of names of workspaces to delete.
        """
        for wsname in wslist:
            if api.AnalysisDataService.doesExist(wsname):
                delete = self.createChildAlgorithm('DeleteWorkspace')
                delete.setProperty("Workspace", wsname)
                delete.execute()
        return

    def _vana_correct(self):
        """
        creates the corrected workspace
        """
        wslist = []
        # 1. normalize Vanadium and Background
        vana_normws = api.mtd[self.vanaws.getName() + '_NORM']
        bkg_normws = api.mtd[self.bkgws.getName() + '_NORM']
        _vana_norm_ = api.Divide(self.vanaws, vana_normws)
        wslist.append(_vana_norm_.getName())
        _bkg_norm_ = api.Divide(self.bkgws, bkg_normws)
        wslist.append(_bkg_norm_.getName())
        # 2. substract background from Vanadium
        _vana_bg_ = _vana_norm_ - _bkg_norm_
        wslist.append(_vana_bg_.getName())
        # check negative values, throw exception
        arr = np.array(_vana_bg_.extractY()).flatten()
        neg_values = np.where(arr < 0)[0]
        if len(neg_values):
            self.cleanup(wslist)
            message = "Background " + self.bkgws.getName() + " is higher than Vanadium " + \
                self.vanaws.getName() + " signal!"
            self.log().error(message)
            raise RuntimeError(message)

        # 3. calculate correction coefficients
        _vana_mean_ws_ = self._vana_mean(_vana_bg_)
        if not _vana_mean_ws_:
            self.cleanup(wslist)
            return None
        if not self.vana_mean_name:
            wslist.append(_vana_mean_ws_.getName())
        _coef_ws_ = api.Divide(LHSWorkspace=_vana_bg_, RHSWorkspace=_vana_mean_ws_, WarnOnZeroDivide=True)
        wslist.append(_coef_ws_.getName())
        # 4. correct raw data (not normalized!)
        api.Divide(LHSWorkspace=self.dataws, RHSWorkspace=_coef_ws_, WarnOnZeroDivide=True,
                   OutputWorkspace=self.outws_name)
        outws = api.mtd[self.outws_name]
        # cleanup
        self.cleanup(wslist)
        return outws

    def validateInputs(self):
        issues = dict()

        dataws = self.getProperty("InputWorkspace").value
        vanaws = self.getProperty("VanaWorkspace").value
        bkgws = self.getProperty("BkgWorkspace").value

        # dimensions must match
        ndims = dataws.getNumDims()
        nhists = dataws.getNumberHistograms()
        nblocks = dataws.blocksize()
        if vanaws.getNumDims() != ndims:
            issues["VanaWorkspace"] = "Number of dimensions does not match to the data workspace."
        if bkgws.getNumDims() != ndims:
            issues["BkgWorkspace"] = "Number of dimensions does not match to the data workspace."
        if vanaws.getNumberHistograms() != nhists:
            issues["VanaWorkspace"] = "Number of histograms does not match to the data workspace."
        if bkgws.getNumberHistograms() != nhists:
            issues["BkgWorkspace"] = "Number of histograms does not match to the data workspace."
        if vanaws.blocksize() != nblocks:
            issues["VanaWorkspace"] = "Number of blocks does not match to the data workspace."
        if bkgws.blocksize() != nblocks:
            issues["BkgWorkspace"] = "Number of blocks does not match to the data workspace."

        return issues

    def PyExec(self):
        # Input
        self.dataws = api.mtd[self.getPropertyValue("InputWorkspace")]
        self.outws_name = self.getPropertyValue("OutputWorkspace")
        self.vanaws = api.mtd[self.getPropertyValue("VanaWorkspace")]
        self.bkgws = api.mtd[self.getPropertyValue("BkgWorkspace")]
        self.vana_mean_name = self.getPropertyValue("VanadiumMean")

        # check if the _NORM workspaces exist
        wslist_norm = [self.vanaws.getName() + '_NORM', self.bkgws.getName() + '_NORM']
        for wsname in wslist_norm:
            if not api.AnalysisDataService.doesExist(wsname):
                message = "Workspace " + wsname + " does not exist!"
                raise RuntimeError(message)

        # check sample logs, produce warnings if different
        wslist = [self.dataws.getName(), self.vanaws.getName(), self.bkgws.getName()]
        result = api.CompareSampleLogs(wslist, self.properties_to_compare, 5e-3)
        if len(result) > 0:
            self.log().warning("Sample logs " + result + " do not match!")

        # apply correction
        outws = self._vana_correct()
        if not outws:
            raise RuntimeError("Correction failed. Invalid VanadiumMean workspace dimensions.")

        # copy sample logs from data workspace to the output workspace
        api.CopyLogs(InputWorkspace=self.dataws.getName(), OutputWorkspace=outws.getName(),
                     MergeStrategy='MergeReplaceExisting')
        self.setProperty("OutputWorkspace", outws)

        # clone the normalization workspace for data if it exists
        if api.mtd.doesExist(self.dataws.getName()+'_NORM'):
            api.CloneWorkspace(InputWorkspace=self.dataws.getName()+'_NORM', OutputWorkspace=self.outws_name+'_NORM')
        self.log().debug('DNSDetEffCorrVana: OK. The result has been saved to ' + self.outws_name)

        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSDetEffCorrVana)
