# pylint: disable=too-many-locals
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as api
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction
import numpy as np


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

    def category(self):
        """
        Returns category
        """
        return 'Workflow\\MLZ\\DNS;;CorrectionFunctions\\SpecialCorrections'

    def seeAlso(self):
        return [ "DNSComputeDetEffCorrCoefs" ]

    def name(self):
        """
        Returns name
        """
        return "DNSFlippingRatioCorr"

    def summary(self):
        return "Performs flipping ratio correction on a given dataset using the NiCr data."

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

        return

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
                message = "Workspace " + wks.name() + " does not have flipper sample log!"
                self.log().error(message)
                raise RuntimeError(message)
            flipper = run.getProperty('flipper').value
            if key in nsf:
                needed = 'OFF'
            else:
                needed = 'ON'
            if flipper != needed:
                message = "Workspace " + wks.name() + " must have flipper " + needed + ", but it is " + flipper
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

        # check validity of flipper status
        self._flipper_valid()

        # algorithm must warn if some properties_to_compare are different
        result = api.CompareSampleLogs(list(self.input_workspaces.values()), self.properties_to_compare, 5e-3)
        if len(result) > 0:
            self.log().warning("Sample logs " + result + " do not match!")
        return True

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

    def _fr_correction(self):
        """
        applies flipping ratio correction
        according to J Appl. Cryst. 42, 69-84, 2009
        creates the corrected workspaces
        """
        wslist = []
        # 1. retrieve NiCr and Background
        sf_nicr = api.AnalysisDataService.retrieve(self.input_workspaces['SF_NiCr'])
        nsf_nicr = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_NiCr'])
        sf_bkgr = api.AnalysisDataService.retrieve(self.input_workspaces['SF_Background'])
        nsf_bkgr = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_Background'])

        # 2. subtract background from NiCr
        _sf_nicr_bg_ = sf_nicr - sf_bkgr
        wslist.append(_sf_nicr_bg_.name())
        _nsf_nicr_bg_ = nsf_nicr - nsf_bkgr
        wslist.append(_nsf_nicr_bg_.name())
        # check negative values, throw exception
        sf_arr = np.array(_sf_nicr_bg_.extractY()).flatten()
        nsf_arr = np.array(_nsf_nicr_bg_.extractY()).flatten()
        sf_neg_values = np.where(sf_arr < 0)[0]
        nsf_neg_values = np.where(nsf_arr < 0)[0]
        if len(sf_neg_values) or len(nsf_neg_values):
            self.cleanup(wslist)
            message = "Background is higher than NiCr signal!"
            self.log().error(message)
            raise RuntimeError(message)

        # 3. calculate flipping ratio F - 1 = (NiCr - Bkg)NSF/(NiCr - Bkg)SF - 1
        _coef_ws_ = api.Divide(LHSWorkspace=_nsf_nicr_bg_, RHSWorkspace=_sf_nicr_bg_, WarnOnZeroDivide=True) - 1.0
        wslist.append(_coef_ws_.name())
        # 4. apply correction raw data
        sf_data_ws = api.AnalysisDataService.retrieve(self.input_workspaces['SF_Data'])
        nsf_data_ws = api.AnalysisDataService.retrieve(self.input_workspaces['NSF_Data'])
        # NSF_corr[i] = NSF[i] + (NSF[i] - SF[i])/(F[i] - 1)
        _diff_ws_ = nsf_data_ws - sf_data_ws
        wslist.append(_diff_ws_.name())
        _tmp_ws_ = api.Divide(LHSWorkspace=_diff_ws_, RHSWorkspace=_coef_ws_, WarnOnZeroDivide=True)
        _tmp_ws_.setYUnit(nsf_data_ws.YUnit())
        api.Plus(LHSWorkspace=nsf_data_ws, RHSWorkspace=_tmp_ws_, OutputWorkspace=self.nsf_outws_name)
        # SF_corr[i] = SF[i] - (NSF[i] - SF[i])/(F[i] - 1)
        api.Minus(LHSWorkspace=sf_data_ws, RHSWorkspace=_tmp_ws_, OutputWorkspace=self.sf_outws_name)
        api.DeleteWorkspace(_tmp_ws_)

        # cleanup
        self.cleanup(wslist)
        return

    def validateInputs(self):
        issues = dict()

        workspaces = {"NSFDataWorkspace": None, "SFNiCrWorkspace": None, "NSFNiCrWorkspace": None,
                      "SFBkgrWorkspace": None, "NSFBkgrWorkspace": None}

        for key in workspaces.keys():
            workspaces[key] = self.getProperty(key).value

        # dimensions must match
        datasf = self.getProperty("SFDataWorkspace").value
        ndims = datasf.getNumDims()
        nhists = datasf.getNumberHistograms()
        nblocks = datasf.blocksize()

        for key in workspaces.keys():
            if workspaces[key].getNumDims() != ndims:
                issues[key] = "Number of dimensions does not match to the data workspace."
            if workspaces[key].getNumberHistograms() != nhists:
                issues[key] = "Number of histograms does not match to the data workspace."
            if workspaces[key].blocksize() != nblocks:
                issues[key] = "Number of blocks does not match to the data workspace."

        # normalizations must match and must be either monitor or duration, polarisations must match
        lognames = "normalized,polarisation,polarisation_comment"
        wslist = list(workspaces.values())
        wslist.append(datasf)
        result = api.CompareSampleLogs(wslist, lognames)
        if len(result) > 0:
            issues['SFDataWorkspace'] = "Cannot apply correction to workspaces with different " + result
        else:
            norm = datasf.getRun().getProperty('normalized').value
            if norm == 'no':
                issues['SFDataWorkspace'] = "Data must be normalized before correction."

        return issues

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

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(DNSFlippingRatioCorr)
