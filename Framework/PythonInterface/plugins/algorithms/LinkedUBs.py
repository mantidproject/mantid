# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name
from mantid.api import DataProcessorAlgorithm, mtd, AlgorithmFactory, WorkspaceProperty, PropertyMode, ITableWorkspaceProperty
from mantid.simpleapi import CalculateUMatrix, PredictPeaks, FilterPeaks, DeleteWorkspace, CreatePeaksWorkspace
from mantid.kernel import Direction, FloatBoundedValidator, IntBoundedValidator, StringMandatoryValidator, StringListValidator, V3D
import numpy as np


class LinkedUBs(DataProcessorAlgorithm):
    _qtol = None
    _qdecrement = None
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
        return "Diffraction\\Reduction;Crystal\\UBMatrix"

    def seeAlso(self):
        return ["SetGoniometer", "CalculateUMatrix"]

    def name(self):
        return "LinkedUBs"

    def summary(self):
        return "Links the indexing of a given run to the UB of a reference\
         run. PredictedPeaks should be calculated via goniometer rotation of\
          the reference UB. Use of this algorithm will result in a seperate\
           (linked) UB for each goniometer setting considered."

    def PyInit(self):
        # Refinement parameters
        self.declareProperty(
            name="QTolerance",
            defaultValue=0.5,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Radius of isotropic q envelope to search within.",
        )
        self.declareProperty(
            name="QDecrement",
            defaultValue=0.95,
            validator=FloatBoundedValidator(lower=0.0, upper=1.0),
            direction=Direction.Input,
            doc="Multiplicative factor by which to decrement q envelope\
             on each iteration.",
        )
        self.declareProperty(
            name="DTolerance",
            defaultValue=0.01,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Observed peak is linked if\
             abs(dSpacing) < dPredicted + dTolerance.",
        )
        self.declareProperty(
            name="NumPeaks",
            defaultValue=15,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=0),
            doc="Number of peaks, ordered from highest to lowest \
            dSpacing to consider.",
        )
        self.declareProperty(
            name="PeakIncrement",
            defaultValue=10,
            validator=IntBoundedValidator(lower=0),
            direction=Direction.Input,
            doc="Number of peaks to add to numPeaks on each iteration.",
        )
        self.declareProperty(
            name="Iterations",
            defaultValue=10,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="Number of cycles of refinement.",
        )

        # lattice
        self.declareProperty(
            name="a", defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0), direction=Direction.Input, doc="Lattice parameter a."
        )
        self.declareProperty(
            name="b", defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0), direction=Direction.Input, doc="Lattice parameter b."
        )
        self.declareProperty(
            name="c", defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0), direction=Direction.Input, doc="Lattice parameter c."
        )
        self.declareProperty(
            name="alpha",
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter alpha.",
        )
        self.declareProperty(
            name="beta",
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter beta.",
        )
        self.declareProperty(
            name="gamma",
            defaultValue=90.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Lattice parameter gamma.",
        )

        # linked predicted peaks parameters
        self.declareProperty(
            name="MinWavelength",
            defaultValue=0.8,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Minimum wavelength for LinkedPredictedPeaks.",
        )
        self.declareProperty(
            name="MaxWavelength",
            defaultValue=9.3,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Maximum wavelength for LinkedPredictedPeaks.",
        )
        self.declareProperty(
            name="MinDSpacing",
            defaultValue=0.6,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Minimum dSpacing for LinkedPredictedPeaks.",
        )
        self.declareProperty(
            name="MaxDSpacing",
            defaultValue=20.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc="Maximum dSpacing for LinkedPredictedPeaks.",
        )
        self.declareProperty(
            name="ReflectionCondition",
            defaultValue="Primitive",
            direction=Direction.Input,
            validator=StringListValidator(
                [
                    "Primitive",
                    "C-face centred",
                    "A-face centred",
                    "B-face centred",
                    "Body centred",
                    "All-face centred",
                    "Rhombohedrally centred, obverse",
                    "Rhombohedrally centred, reverse",
                    "Hexagonally centred, reverse",
                ]
            ),
            doc="Reflection condition \
                                    for LinkedPredictedPeaks.",
        )

        # input workspaces
        self.declareProperty(
            WorkspaceProperty(name="Workspace", defaultValue="", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Instrument workspace on which observed peaks are defined.",
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="ObservedPeaks", defaultValue="", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="FindPeaks table to which PredictedPeaks are compared.",
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="PredictedPeaks", defaultValue="", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="PredictedPeaks table to which ObservedPeaks are compared.",
        )

        # output workspaces
        self.declareProperty(
            ITableWorkspaceProperty(name="LinkedPeaks", defaultValue="", validator=StringMandatoryValidator(), direction=Direction.Output),
            doc="Linked peaks: UB matrix consistent with that of \
            PredictedPeaks.",
        )
        self.declareProperty(
            ITableWorkspaceProperty(
                name="LinkedPredictedPeaks", defaultValue="", validator=StringMandatoryValidator(), direction=Direction.Output
            ),
            doc="LinkedPredictedPeaks: UB matrix consistent with \
            PredictedPeaks.",
        )
        self.declareProperty(
            "DeleteWorkspace", defaultValue=False, direction=Direction.Input, doc="Delete workspace after execution for memory management."
        )

        # groupings
        self.setPropertyGroup("QTolerance", "Refinement parameters")
        self.setPropertyGroup("QDecrement", "Refinement parameters")
        self.setPropertyGroup("DTolerance", "Refinement parameters")
        self.setPropertyGroup("NumPeaks", "Refinement parameters")
        self.setPropertyGroup("PeakIncrement", "Refinement parameters")
        self.setPropertyGroup("Iterations", "Refinement parameters")
        self.setPropertyGroup("a", "Lattice")
        self.setPropertyGroup("b", "Lattice")
        self.setPropertyGroup("c", "Lattice")
        self.setPropertyGroup("alpha", "Lattice")
        self.setPropertyGroup("beta", "Lattice")
        self.setPropertyGroup("gamma", "Lattice")
        self.setPropertyGroup("MinWavelength", "PredictPeaksParameters")
        self.setPropertyGroup("MaxWavelength", "PredictPeaksParameters")
        self.setPropertyGroup("MinDSpacing", "PredictPeaksParameters")
        self.setPropertyGroup("MaxDSpacing", "PredictPeaksParameters")
        self.setPropertyGroup("ReflectionCondition", "PredictPeaksParameters")
        self.setPropertyGroup("Workspace", "Input")
        self.setPropertyGroup("ObservedPeaks", "Input")
        self.setPropertyGroup("PredictedPeaks", "Input")
        self.setPropertyGroup("LinkedPeaks", "Output")
        self.setPropertyGroup("LinkedPredictedPeaks", "Output")
        self.setPropertyGroup("DeleteWorkspace", "Output")

    def validateInputs(self):
        self._get_properties()
        issues = dict()
        return issues

    def _get_properties(self):
        self._qtol = self.getProperty("QTolerance").value
        self._qdecrement = self.getProperty("QDecrement").value
        self._dtol = self.getProperty("DTolerance").value
        self._num_peaks = self.getProperty("NumPeaks").value
        self._peak_increment = self.getProperty("PeakIncrement").value
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
        # create peaks workspace to store linked peaks
        linked_peaks = CreatePeaksWorkspace(InstrumentWorkspace=self._workspace, NumberOfPeaks=0, StoreInADS=False)

        # create peaks table to store linked predicted peaks
        linked_peaks_predicted = CreatePeaksWorkspace(InstrumentWorkspace=self._workspace, NumberOfPeaks=0, StoreInADS=False)

        for m in range(0, self._iterations):
            if m == 0:
                predictor = self._predicted_peaks
            if m > 0:
                predictor = linked_peaks_predicted

            qtol_var = self._qtol * self._qdecrement**m
            num_peaks_var = self._num_peaks + self._peak_increment * m

            # add q_lab and dpsacing values of found peaks to a list
            qlabs_observed = np.array(self._observed_peaks.column("QLab"))
            dspacings_observed = np.array(self._observed_peaks.column("DSpacing"))

            # sort the predicted peaks from largest to smallest dspacing
            qlabs_predicted = np.array(predictor.column("QLab"))
            dspacings_predicted = np.array(predictor.column("DSpacing"))

            # get the indexing list that sorts dspacing from largest to
            # smallest
            hkls = np.array([[p.getH(), p.getK(), p.getL()] for p in predictor])
            idx = dspacings_predicted.argsort()[::-1]
            HKL_predicted = hkls[idx, :]

            # sort q, d and h, k, l by this indexing
            qlabs_predicted = qlabs_predicted[idx]
            dspacings_predicted = dspacings_predicted[idx]

            q_ordered = qlabs_predicted[:num_peaks_var]
            d_ordered = dspacings_predicted[:num_peaks_var]
            HKL_ordered = HKL_predicted[:num_peaks_var]

            # loop through the ordered find peaks, compare q and d to each
            # predicted peak if the q and d values of a found peak match a
            # predicted peak within tolerance, the found peak inherits
            # the HKL of the predicted peak
            for i in range(len(qlabs_observed)):
                qx_obs, qy_obs, qz_obs = qlabs_observed[i]
                q_obs = V3D(qx_obs, qy_obs, qz_obs)
                p_obs = linked_peaks.createPeak(q_obs)
                d_obs = dspacings_observed[i]

                for j in range(len(q_ordered)):
                    qx_pred, qy_pred, qz_pred = q_ordered[j]
                    d_pred = d_ordered[j]

                    if (
                        qx_pred - qtol_var <= qx_obs <= qx_pred + qtol_var
                        and qy_pred - qtol_var <= qy_obs <= qy_pred + qtol_var
                        and qz_pred - qtol_var <= qz_obs <= qz_pred + qtol_var
                        and d_pred - self._dtol <= d_obs <= d_pred + self._dtol
                    ):
                        h, k, l = HKL_ordered[j]
                        p_obs.setHKL(h, k, l)
                        p_obs.setIntHKL(p_obs.getHKL())
                        linked_peaks.addPeak(p_obs)

            # Clean up peaks where H == K == L == 0
            linked_peaks = FilterPeaks(linked_peaks, FilterVariable="h^2+k^2+l^2", Operator="!=", FilterValue="0")

            # force UB on linked_peaks using known lattice parameters
            CalculateUMatrix(
                PeaksWorkspace=linked_peaks,
                a=self._a,
                b=self._b,
                c=self._c,
                alpha=self._alpha,
                beta=self._beta,
                gamma=self._gamma,
                StoreInADS=False,
            )

            # new linked predicted peaks
            linked_peaks_predicted = PredictPeaks(
                InputWorkspace=linked_peaks,
                WavelengthMin=self._wavelength_min,
                WavelengthMax=self._wavelength_max,
                MinDSpacing=self._min_dspacing,
                MaxDSpacing=self._max_dspacing,
                ReflectionCondition=self._reflection_condition,
                StoreInADS=False,
            )

        # clean up
        self.setProperty("LinkedPeaks", linked_peaks)
        self.setProperty("LinkedPredictedPeaks", linked_peaks_predicted)
        if mtd.doesExist("linked_peaks"):
            DeleteWorkspace(linked_peaks)
        if mtd.doesExist("linked_peaks_predicted"):
            DeleteWorkspace(linked_peaks_predicted)
        if self._delete_ws:
            DeleteWorkspace(self._workspace)


# register algorithm with mantid
AlgorithmFactory.subscribe(LinkedUBs)
