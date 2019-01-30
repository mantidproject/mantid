from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
import numpy as np 

class LinkedUBs(DataProcessorAlgorithm):
    _qtol = None 
    _q_decrement = None 
    _dtol = None 
    _num_peaks = None 
    _peak_increment = None
    _iterations = None 
    _a = None 
    _b = None 
    _c = None 
    _alpha = None 
    _beta = None 
    _gamma = None 
    _wavelength_min = None 
    _wavelength_max = None 
    _min_dspacing = None 
    _max_dspacing = None 
    _reflection_condition = None 
    _workspace = None 
    _observed_peaks = None 
    _predicted_peaks = None 
    _linked_peaks = None 
    _linked_predicted_peaks = None 
    _delete_ws = None

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["SetGoniometer","CalculateUMatrix"]

    def name(self):
        return "LinkedUBs"

    def summary(self):
        return "Links the indexing of a given run to the UB of a reference run. PredictedPeaks should be calculated via goniometer rotation of the reference UB. Use of this algorithm will result in a seperate (linked) UB for each goniometer setting considered."

    def PyInit(self):
        # Refinement parameters
        self.declareProperty(name="qTolerance", 
            defaultValue=0.5,
            direction=Direction.Input, 
            validator=FloatBoundedValidator(lower=0.0),
            doc="Radius of isotropic q envelope to search within.")
        self.declareProperty(name="qDecrement", 
            defaultValue=0.95,
            validator=FloatBoundedValidator(lower=0.0, upper=1.0), 
            direction=Direction.Input, 
            doc="Multiplicative factor by which to decrement q envelope on each iteration.")
        self.declareProperty(name="dTolerance", 
            defaultValue=0.01,
            direction=Direction.Input, 
            validator=FloatBoundedValidator(lower=0.0),
            doc="Observed peak is linked if |dSpacing| < dPredicted + dTolerance.")
        self.declareProperty(name="numPeaks", 
            defaultValue=15,
            direction=Direction.Input, 
            validator=IntBoundedValidator(lower=0),
            doc="Number of peaks, ordered from highest to lowest dSpacing to consider.")
        self.declareProperty(name="peakIncrement", 
            defaultValue=10, 
            validator=IntBoundedValidator(lower=0),
            direction=Direction.Input,
            doc="Number of peaks to add to numPeaks on each iteration.")
        self.declareProperty(name="Iterations", 
            defaultValue=10,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="Number of cycles of refinement.")

        # lattice 
        self.declareProperty(name="a", 
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter a.")
        self.declareProperty(name="b", 
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter b.")
        self.declareProperty(name="c", 
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter c.")
        self.declareProperty(name="alpha", 
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter alpha.")
        self.declareProperty(name="beta", 
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter beta.")
        self.declareProperty(name="gamma", 
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter gamma.")

        # linked predicted peaks parameters 
        self.declareProperty(name="MinWavelength", 
            defaultValue=0.8,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Minimum wavelength for LinkedPredictedPeaks.")
        self.declareProperty(name="MaxWavelength", 
            defaultValue=9.3,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Maximum wavelength for LinkedPredictedPeaks.")
        self.declareProperty(name="MinDSpacing", 
            defaultValue=0.6,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Minimum dSpacing for LinkedPredictedPeaks.")
        self.declareProperty(name="MaxDSpacing", 
            defaultValue=20.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Maximum dSpacing for LinkedPredictedPeaks.")
        self.declareProperty(name="ReflectionCondition", 
            defaultValue="Primitive",
            direction=Direction.Input,
            validator=StringListValidator(["Primitive", "C-face centred" , "A-face centred",\
                "B-face centred", "Body centred", "All-face centred",\
                "Rhombohedrally centred, obverse", "Rhombohedrally centred, reverse",\
                "Hexagonally centred, reverse"]),
            doc="Reflection condition for LinkedPredictedPeaks.")

        # input workspaces
        self.declareProperty(WorkspaceProperty(name="Workspace", 
            defaultValue="",
            optional=PropertyMode.Mandatory, 
            direction=Direction.Input),
            doc="Instrument workspace on which observed peaks are defined.")
        self.declareProperty(ITableWorkspaceProperty(name="ObservedPeaks", 
            defaultValue="",
            optional=PropertyMode.Mandatory, 
            direction=Direction.Input),
            doc="FindPeaks table to which PredictedPeaks are compared.")
        self.declareProperty(ITableWorkspaceProperty(name="PredictedPeaks", 
            defaultValue="",
            optional=PropertyMode.Mandatory, 
            direction=Direction.Input),
            doc="PredictedPeaks table to which ObservedPeaks are compared.")

        # output workspaces 
        self.declareProperty(ITableWorkspaceProperty(name="LinkedPeaks",
            defaultValue="",
            validator=StringMandatoryValidator(),
            direction=Direction.Output),
            doc="Linked peaks: UB matrix consistent with that of PredictedPeaks.")
        self.declareProperty(ITableWorkspaceProperty(name="LinkedPredictedPeaks",
            defaultValue="", 
            validator=StringMandatoryValidator(),
            direction=Direction.Output),
            doc="LinkedPredictedPeaks: UB matrix consistent with PredictedPeaks.")
        self.declareProperty("DeleteWorkspace", 
            defaultValue=False,
            direction=Direction.Input, 
            doc="Delete workspace after execution for memory management.")

        # groupings
        self.setPropertyGroup("qTolerance", "Refinement parameters")
        self.setPropertyGroup("qDecrement", "Refinement parameters")
        self.setPropertyGroup("dTolerance", "Refinement parameters")
        self.setPropertyGroup("numPeaks", "Refinement parameters")
        self.setPropertyGroup("peakIncrement", "Refinement parameters")
        self.setPropertyGroup("Iterations", "Refinement parameters")
        self.setPropertyGroup("a", "Lattice")
        self.setPropertyGroup("b", "Lattice")
        self.setPropertyGroup("c", "Lattice")
        self.setPropertyGroup("alpha", "Lattice")
        self.setPropertyGroup("beta", "Lattice")
        self.setPropertyGroup("gamma", "Lattice")
        self.setPropertyGroup("MinWavelength", "LinkedPredictedPeaks")
        self.setPropertyGroup("MaxWavelength", "LinkedPredictedPeaks")
        self.setPropertyGroup("MinDSpacing", "LinkedPredictedPeaks")
        self.setPropertyGroup("MaxDSpacing", "LinkedPredictedPeaks")
        self.setPropertyGroup("ReflectionCondition", "LinkedPredictedPeaks")
        self.setPropertyGroup("Workspace", "Input")
        self.setPropertyGroup("ObservedPeaks", "Input")
        self.setPropertyGroup("PredictedPeaks", "Input")
        self.setPropertyGroup("LinkedPeaks", "Output")
        self.setPropertyGroup("LinkedPredictedPeaks", "Output")
        self.setPropertyGroup("DeleteWorkspace","Output")

    def validateInputs(self):
    	self._get_properties()
    	issues = dict()
    	return issues

    def _get_properties(self):
        self._qtol = self.getProperty("qTolerance").value
        self._q_decrement = self.getProperty("qDecrement").value
        self._dtol = self.getProperty("dTolerance").value
        self._num_peaks = self.getProperty("numPeaks").value
        self._peak_increment = self.getProperty("peakIncrement").value
        self._iterations = self.getProperty("Iterations").value
        self._a = self.getProperty("a").value
        self._b = self.getProperty("b").value
        self._c = self.getProperty("c").value
        self._alpha = self.getProperty("alpha").value
        self._beta = self.getProperty("beta").value
        self._gamma = self.getProperty("gamma").value
        self._wavelength_min = self.getProperty("MinWavelength").value
        self._wavelength_max = self.getProperty("MaxWavelength").value
        self._min_dspacing = self.getProperty("MinDSpacing").value
        self._max_dspacing = self.getProperty("MaxDSpacing").value
        self._reflection_condition = self.getProperty("ReflectionCondition").value
        self._workspace = self.getProperty("Workspace").value
        self._observed_peaks = self.getProperty("ObservedPeaks").value 
        self._predicted_peaks = self.getProperty("PredictedPeaks").value 
        self._linked_peaks = self.getPropertyValue("LinkedPeaks")
        self._linked_predicted_peaks = self.getPropertyValue("LinkedPredictedPeaks")
        self._delete_ws = self.getProperty("DeleteWorkspace").value

    def PyExec(self):
        for iter in range(0, self._iterations):
            if iter == 0:
                _predictor = self._predicted_peaks
            if iter > 0:
                _predictor = _LinkedPeaksPredicted

            _qtol_var = self._qtol * (self._q_decrement)**iter 
            _num_peaks_var = self._num_peaks + (self._peak_increment)*iter

            # create peaks workspace to store linked peaks 
            _LinkedPeaks = CreatePeaksWorkspace(InstrumentWorkspace=self._workspace, 
                                                NumberOfPeaks=0, 
                                                StoreInADS=False)

            # add q_lab and dpsacing values of found peaks to a list
            q_find = []
            d_find = []

            for i in range(len(self._observed_peaks.column(1))):
                _q_find = self._observed_peaks.cell(i,15)
                _d_find = self._observed_peaks.cell(i,8)

                q_find.append(_q_find)
                d_find.append(_d_find)

            # recast these lists as numpy arrays
            q_find = np.array(q_find)
            d_find = np.array(d_find)

            # sort the predicted peaks from largest to smallest dspacing
            q_predicted = []
            d_predicted = []
            ind_h = []
            ind_k = []
            ind_l = []

            for i in range(len(_predictor.column(1))):
                _q_predicted = _predictor.cell(i,15)
                _d_predicted = _predictor.cell(i,8)
                _h_predicted = _predictor.cell(i,2)
                _k_predicted = _predictor.cell(i,3)
                _l_predicted = _predictor.cell(i,4)

                q_predicted.append(_q_predicted)
                d_predicted.append(_d_predicted)
                ind_h.append(_h_predicted)
                ind_k.append(_k_predicted)
                ind_l.append(_l_predicted)

            # recast these lists as numpy arrays
            q_predicted = np.array(q_predicted)
            d_predicted = np.array(d_predicted)

            ind_h = np.array(ind_h)
            ind_k = np.array(ind_k)
            ind_l = np.array(ind_l)

            # get the indexing list that sorts dspacing from largest to smallest
            idx = d_predicted.argsort()[::-1]

            # sort q, d and h, k, l by this indexing
            q_predicted = q_predicted[idx]
            d_predicted = d_predicted[idx]
            ind_h = ind_h[idx]
            ind_k = ind_k[idx]
            ind_l = ind_l[idx]

            HKL_predicted = []
            for i in range(len(ind_h)):
                _HKL = (ind_h[i], ind_k[i], ind_l[i])
                HKL_predicted.append(_HKL)

            # recast h,k,l list as numpy array
            HKL_predicted = np.array(HKL_predicted)

            # create the list of predicted peaks to be considered in linking procedure,
            # with peak list partitioned by num_peaks parameter
            q_ordered = []
            d_ordered = []
            HKL_ordered = []

            for i in range(0, _num_peaks_var):
                _q_ordered = q_predicted[i]
                _d_ordered = d_predicted[i]
                _HKL_ordered = HKL_predicted[i]

                q_ordered.append(_q_ordered)
                d_ordered.append(_d_ordered)
                HKL_ordered.append(_HKL_ordered)

            # recast these ordered lists as numpy arrays
            q_ordered = np.array(q_ordered)
            d_ordered = np.array(d_ordered)
            HKL_ordered = np.array(HKL_ordered)

            # loop through the ordered find peaks, compare q and d to each predicted peak
            # if the q and d values of a found peak match a predicted peak within tolerance,
            # the found peak inherits the HKL of the predicted peak
            for i in range(len(q_find)):
                _qx_obs, _qy_obs, _qz_obs = q_find[i]
                _q_obs = V3D(_qx_obs, _qy_obs, _qz_obs)
                _p_obs = _LinkedPeaks.createPeak(_q_obs)
                _d_obs = d_find[i]

                for j in range(len(q_ordered)):
                    _qx_pred, _qy_pred, _qz_pred = q_ordered[j]
                    _q_pred = V3D(_qx_pred, _qy_pred, _qz_pred)
                    _d_pred = d_ordered[j]

                    if (_qx_pred - _qtol_var <= _qx_obs <= _qx_pred + _qtol_var
                    and _qy_pred - _qtol_var <= _qy_obs <= _qy_pred + _qtol_var
                    and _qz_pred - _qtol_var <= _qz_obs <= _qz_pred + _qtol_var
                    and _d_pred  - self._dtol <= _d_obs <=  _d_pred  + self._dtol):
                        _h, _k, _l = HKL_ordered[j]
                        _p_obs.setHKL(_h, _k, _l)
                        _LinkedPeaks.addPeak(_p_obs)

            # Clean up peaks where H == K == L == 0
            _LinkedPeaks = FilterPeaks(_LinkedPeaks, 
                                        FilterVariable="h^2+k^2+l^2", 
                                        Operator="!=", 
                                        FilterValue="0")

            # force UB on LinkedPeaks using known lattice parameters 
            CalculateUMatrix(PeaksWorkspace=_LinkedPeaks,
                             a=self._a,
                             b=self._b,
                             c=self._c,
                             alpha=self._alpha,
                             beta=self._beta,
                             gamma=self._gamma,
                             StoreInADS=False)
            
            # new linked predicted peaks
            _LinkedPeaksPredicted = PredictPeaks(InputWorkspace=_LinkedPeaks,
                                                 WavelengthMin=self._wavelength_min,
                                                 WavelengthMax=self._wavelength_max,
                                                 MinDSpacing=self._min_dspacing,
                                                 MaxDSpacing=self._max_dspacing,
                                                 ReflectionCondition=self._reflection_condition,
                                                 StoreInADS=False)

        # clean up
        self.setProperty("LinkedPeaks", _LinkedPeaks)
        self.setProperty("LinkedPredictedPeaks", _LinkedPeaksPredicted)
        if mtd.doesExist("_LinkedPeaks") == True:
            DeleteWorkspace(_LinkedPeaks)
        if mtd.doesExist("_LinkedPeaksPredicted") == True:
            DeleteWorkspace(_LinkedPeaksPredicted)
        if self._delete_ws == True:
            DeleteWorkspace(self._workspace)

# register algorithm with mantid
AlgorithmFactory.subscribe(LinkedUBs)
