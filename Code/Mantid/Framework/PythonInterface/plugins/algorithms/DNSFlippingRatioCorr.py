# pylint: disable=too-many-locals
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator
import numpy as np

import mlzutils


class DNSFlippingRatioCorr(PythonAlgorithm):
    """
    Peforms flipping ratio correction of the given dataset
    using the NiCr data
    This algorithm is written for the DNS @ MLZ,
    but can be adjusted for other instruments if needed.
    """
    properties_to_compare = ['deterota', 'wavelength', 'slit_i_left_blade_position',
                             'slit_i_right_blade_position', 'slit_i_lower_blade_position',
                             'slit_i_upper_blade_position']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.input_workspaces = {}
        self.sf_outws_name = None
        self.nsf_outws_name = None
        self.dfr = 0.05

    def category(self):
        """
        Returns category
        """
        return 'PythonAlgorithms\\MLZ\\DNS;CorrectionFunctions'

    def name(self):
        """
        Returns name
        """
        return "DNSFlippingRatioCorr"

    def summary(self):
        return "Peforms flipping ratio correction on a given dataset using the NiCr data."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("SFDataWorkspace", "", direction=Direction.Input),
                             doc="A workspace with spin-flip experimental data from sample.")
        self.declareProperty(MatrixWorkspaceProperty("NSFDataWorkspace", "", direction=Direction.Input),
                             doc="A workspace with non spin-flip experimental data from sample.")
        self.declareProperty(MatrixWorkspaceProperty("SFNiCrWorkspace", "", direction=Direction.Input),
                             doc="A workspace with spin-flip NiCr data.")
        self.declareProperty(MatrixWorkspaceProperty("NSFNiCrWorkspace", "", direction=Direction.Input),
                             doc="A workspace with non spin-flip NiCr data.")
        self.declareProperty(MatrixWorkspaceProperty("SFBkgrWorkspace", "", direction=Direction.Input),
                             doc="A workspace with spin-flip background for NiCr.")
        self.declareProperty(MatrixWorkspaceProperty("NSFBkgrWorkspace", "", direction=Direction.Input),
                             doc="A workspace with non spin-flip background for NiCr.")
        self.declareProperty(MatrixWorkspaceProperty("SFOutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace to save the corrected spin-flip data.")
        self.declareProperty(MatrixWorkspaceProperty("NSFOutputWorkspace", "", direction=Direction.Output),
                             doc="A workspace to save the corrected non spin-flip data.")
        self.declareProperty("DoubleSpinFlipScatteringProbability", 0.05, FloatBoundedValidator(lower=0, upper=1.0),
                             doc="Probability of the double spin-flip scattering. Number between 0 and 1.")

        return

    def _same_polarisation(self):
        """
        Checks whether all workspaces have the same polarisation.
        Raises error if not.
        """
        pols = []
        for wsname in self.input_workspaces.values():
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            if run.hasProperty('polarisation'):
                pols.append(run.getProperty('polarisation').value)
            else:
                message = \
                    " Workspace " + wks.getName() + " does not have property polarisation! Cannot correct flipping ratio."
                self.log().error(message)
                raise RuntimeError(message)
        polarisation = pols[0]
        if pols.count(polarisation) == len(pols):
            self.log().information("The polarisation is " + polarisation)
            return True
        else:
            message = "Cannot apply correction to workspaces with different polarisation!"
            self.log().error(message)
            raise RuntimeError(message)

    def _flipper_valid(self):
        """
        checks whether SF workspaces have flipper on
        and NSF workspaces have flipper OFF
        """
        # sort workspaces for sf and nsf
        nsf = []
        for key in self.input_workspaces.keys():
            if 'NSF' in key:
                nsf.append(key)
        for key in self.input_workspaces.keys():
            wks = api.AnalysisDataService.retrieve(self.input_workspaces[key])
            run = wks.getRun()
            if not run.hasProperty('flipper'):
                message = "Workspace " + wks.getName() + " does not have flipper sample log!"
                self.log().error(message)
                raise RuntimeError(message)
            flipper = run.getProperty('flipper').value
            if key in nsf:
                needed = 'OFF'
            else:
                needed = 'ON'
            if flipper != needed:
                message = "Workspace " + wks.getName() + " must have flipper " + needed + ", but it is " + flipper
                self.log().error(message)
                raise RuntimeError(message)

        return True

    def _can_correct(self):
        """
        checks whether FR correction is possible with the given list of workspaces
        """
        # list of workspaces must not be empty
        if not self.input_workspaces:
            message = "No workspace names has been specified! Nothing for FR correction."
            self.log().error(message)
            raise RuntimeError(message)
        # workspaces and normalization workspaces must exist
        wslist = self.input_workspaces.values()
        # for data workspaces normalization is not mandatory
        dataws_names = [self.input_workspaces['SF_Data'], self.input_workspaces['NSF_Data']]
        for wsname in self.input_workspaces.values():
            if wsname not in dataws_names:
                wslist.append(wsname + '_NORM')

        mlzutils.ws_exist(wslist, self.log())

        # they must have the same dimensions
        mlzutils.same_dimensions(self.input_workspaces.values())

        # and the same polarisation
        self._same_polarisation()

        # check validity of flipper status
        self._flipper_valid()

        # algorithm must warn if some properties_to_compare are different
        ws1 = api.AnalysisDataService.retrieve(self.input_workspaces.values()[0])
        run1 = ws1.getRun()
        for wsname in self.input_workspaces.values()[1:]:
            wks = api.AnalysisDataService.retrieve(wsname)
            run = wks.getRun()
            mlzutils.compare_properties(run1, run, self.properties_to_compare, self.log())
        return True

    def _fr_correction(self):
        """
        applies flipping ratio correction
        creates the corrected workspaces
        """
        wslist = []
        # 1. normalize NiCr and Background
        sf_nicr_normws = api.AnalysisDataService.retrieve(self.input_workspaces['SF_NiCr'] + '_NORM')
        sf_nicr = api.AnalysisDataService.retrieve(self.input_workspaces['SF_NiCr'])
        _sf_nicr_norm_ = api.Divide(sf_nicr, sf_nicr_normws)
        wslist.append(_sf_nicr_norm_.getName())

        nsf_nicr_normws = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_NiCr'] + '_NORM')
        nsf_nicr = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_NiCr'])
        _nsf_nicr_norm_ = api.Divide(nsf_nicr, nsf_nicr_normws)
        wslist.append(_nsf_nicr_norm_.getName())

        sf_bkgr_normws = api.AnalysisDataService.retrieve(self.input_workspaces['SF_Background'] + '_NORM')
        sf_bkgr = api.AnalysisDataService.retrieve(self.input_workspaces['SF_Background'])
        _sf_bkgr_norm_ = api.Divide(sf_bkgr, sf_bkgr_normws)
        wslist.append(_sf_bkgr_norm_.getName())

        nsf_bkgr_normws = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_Background'] + '_NORM')
        nsf_bkgr = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_Background'])
        _nsf_bkgr_norm_ = api.Divide(nsf_bkgr, nsf_bkgr_normws)
        wslist.append(_nsf_bkgr_norm_.getName())

        # 2. subtract background from NiCr
        _sf_nicr_bg_ = _sf_nicr_norm_ - _sf_bkgr_norm_
        wslist.append(_sf_nicr_bg_.getName())
        _nsf_nicr_bg_ = _nsf_nicr_norm_ - _nsf_bkgr_norm_
        wslist.append(_nsf_nicr_bg_.getName())
        # check negative values, throw exception
        sf_arr = np.array(_sf_nicr_bg_.extractY()).flatten()
        nsf_arr = np.array(_nsf_nicr_bg_.extractY()).flatten()
        sf_neg_values = np.where(sf_arr < 0)[0]
        nsf_neg_values = np.where(nsf_arr < 0)[0]
        if len(sf_neg_values) or len(nsf_neg_values):
            mlzutils.cleanup(wslist)
            message = "Background is higher than NiCr signal!"
            self.log().error(message)
            raise RuntimeError(message)

        # 3. calculate correction coefficients
        _coef_ws_ = api.Divide(LHSWorkspace=_nsf_nicr_bg_, RHSWorkspace=_sf_nicr_bg_, WarnOnZeroDivide=True)
        wslist.append(_coef_ws_.getName())
        # 4. apply correction raw data (not normalized!)
        sf_data_ws = api.AnalysisDataService.retrieve(self.input_workspaces['SF_Data'])
        nsf_data_ws = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_Data'])
        # NSF_corr[i] = NSF[i] - SF[i]/c[i]
        _tmp_ws_ = api.Divide(LHSWorkspace=sf_data_ws, RHSWorkspace=_coef_ws_, WarnOnZeroDivide=True)
        _tmp_ws_.setYUnit(nsf_data_ws.YUnit())
        api.Minus(LHSWorkspace=nsf_data_ws, RHSWorkspace=_tmp_ws_, OutputWorkspace=self.nsf_outws_name)
        nsf_outws = api.AnalysisDataService.retrieve(self.nsf_outws_name)
        api.DeleteWorkspace(_tmp_ws_)
        # SF_corr[i] = SF[i] - NSF[i]/c[i]
        _tmp_ws_ = api.Divide(LHSWorkspace=nsf_data_ws, RHSWorkspace=_coef_ws_, WarnOnZeroDivide=True)
        _tmp_ws_.setYUnit(sf_data_ws.YUnit())
        api.Minus(LHSWorkspace=sf_data_ws, RHSWorkspace=_tmp_ws_, OutputWorkspace=self.sf_outws_name)
        sf_outws = api.AnalysisDataService.retrieve(self.sf_outws_name)
        api.DeleteWorkspace(_tmp_ws_)

        # 5. Apply correction for a double spin-flip scattering
        if self.dfr > 1e-7:
            _tmp_ws_ = sf_outws * self.dfr
            _tmp_ws_.setYUnit(nsf_outws.YUnit())
            wslist.append(_tmp_ws_.getName())
            # NSF_corr[i] = NSF_prev_corr[i] - SF_prev_corr*dfr, SF_corr = SF_prev_corr
            api.Minus(LHSWorkspace=nsf_outws, RHSWorkspace=_tmp_ws_, OutputWorkspace=self.nsf_outws_name)

        # cleanup
        mlzutils.cleanup(wslist)
        return

    def PyExec(self):
        # Input
        self.input_workspaces['SF_Data'] = self.getPropertyValue("SFDataWorkspace")
        self.input_workspaces['NSF_Data'] = self.getPropertyValue("NSFDataWorkspace")
        self.input_workspaces['SF_NiCr'] = self.getPropertyValue("SFNiCrWorkspace")
        self.input_workspaces['NSF_NiCr'] = self.getPropertyValue("NSFNiCrWorkspace")
        self.input_workspaces['SF_Background'] = self.getPropertyValue("SFBkgrWorkspace")
        self.input_workspaces['NSF_Background'] = self.getPropertyValue("NSFBkgrWorkspace")
        self.sf_outws_name = self.getPropertyValue("SFOutputWorkspace")
        self.nsf_outws_name = self.getPropertyValue("NSFOutputWorkspace")
        self.dfr = float(self.getPropertyValue("DoubleSpinFlipScatteringProbability"))

        # check if possible to apply correction
        self._can_correct()

        # apply flipping ratio correction, retrieve the result
        self._fr_correction()
        nsf_outws = api.AnalysisDataService.retrieve(self.nsf_outws_name)
        sf_outws = api.AnalysisDataService.retrieve(self.sf_outws_name)

        # copy sample logs from data workspace to the output workspace
        api.CopyLogs(InputWorkspace=self.input_workspaces['SF_Data'], OutputWorkspace=self.sf_outws_name,
                     MergeStrategy='MergeReplaceExisting')
        api.CopyLogs(InputWorkspace=self.input_workspaces['NSF_Data'], OutputWorkspace=self.nsf_outws_name,
                     MergeStrategy='MergeReplaceExisting')
        self.setProperty("SFOutputWorkspace", sf_outws)
        self.setProperty("NSFOutputWorkspace", nsf_outws)

        # clone the normalization workspaces
        if api.AnalysisDataService.doesExist(self.input_workspaces['SF_Data'] + '_NORM'):
            api.CloneWorkspace(InputWorkspace=self.input_workspaces['SF_Data'] + '_NORM',
                               OutputWorkspace=self.sf_outws_name + '_NORM')
        if api.AnalysisDataService.doesExist(self.input_workspaces['NSF_Data'] + '_NORM'):
            api.CloneWorkspace(InputWorkspace=self.input_workspaces['NSF_Data'] + '_NORM',
                               OutputWorkspace=self.nsf_outws_name + '_NORM')

        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSFlippingRatioCorr)
