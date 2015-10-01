import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction
import numpy as np

import mlzutils


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
            mlzutils.cleanup(wslist)
            message = "Background " + self.bkgws.getName() + " is higher than Vanadium " + \
                self.vanaws.getName() + " signal!"
            self.log().error(message)
            raise RuntimeError(message)

        # 3. calculate correction coefficients
        _vana_mean_ws_ = self._vana_mean(_vana_bg_)
        if not _vana_mean_ws_:
            mlzutils.cleanup(wslist)
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
        mlzutils.cleanup(wslist)
        return outws

    def PyExec(self):
        # Input
        self.dataws = api.mtd[self.getPropertyValue("InputWorkspace")]
        self.outws_name = self.getPropertyValue("OutputWorkspace")
        self.vanaws = api.mtd[self.getPropertyValue("VanaWorkspace")]
        self.bkgws = api.mtd[self.getPropertyValue("BkgWorkspace")]
        self.vana_mean_name = self.getPropertyValue("VanadiumMean")

        # check dimensions
        wslist = [self.dataws.getName(), self.vanaws.getName(), self.bkgws.getName()]
        mlzutils.same_dimensions(wslist)
        # check if the _NORM workspaces exist
        wslist = [self.vanaws.getName() + '_NORM', self.bkgws.getName() + '_NORM']
        mlzutils.ws_exist(wslist, self.log())

        # check sample logs, produce warnings if different
        drun = self.dataws.getRun()
        vrun = self.vanaws.getRun()
        brun = self.bkgws.getRun()
        mlzutils.compare_properties(drun, vrun, self.properties_to_compare, self.log())
        mlzutils.compare_properties(vrun, brun, self.properties_to_compare, self.log())
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
