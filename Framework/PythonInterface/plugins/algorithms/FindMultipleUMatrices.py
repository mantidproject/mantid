# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    Progress,
    WorkspaceGroupProperty,
    IPeaksWorkspaceProperty,
    AnalysisDataService as ADS,
)
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator, EnabledWhenProperty, PropertyCriterion, logger
from mantid.geometry import SpaceGroupFactory, PointGroupFactory, CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from itertools import combinations, product
from mantid.kernel import V3D
import numpy as np


_MIN_NUM_INDEXED_PEAKS = 2  # minimum indexed peaks required for CalculateUMatrix


class FindMultipleUMatrices(DataProcessorAlgorithm):
    def name(self):
        return "FindMultipleUBUsingLatticeParameters"

    def category(self):
        return "Diffraction\\Reduction;Crystal\\UBMatrix"

    def seeAlso(self):
        return ["FindUBUsingLatticeParameters", "CalculateUMatrix"]

    def summary(self):
        return (
            "Finds multiple UB matrices using lattice parameters - can be used to find UBs in the presence of "
            "mutiple domains, crystallites or in the presence of spurious peaks."
        )

    def PyInit(self):
        # Input
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input),
            doc="A PeaksWorkspace containing the peaks to integrate.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output peak workspaces with UBs set - 1 per UB requested.",
        )
        self.declareProperty(
            name="NumberOfUBs",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Number of UB matrices to find.",
        )
        positiveFloatValidator = FloatBoundedValidator(lower=0.0)
        angleValidator = FloatBoundedValidator(lower=0.0, upper=180.0)
        self.declareProperty(
            name="a", defaultValue=1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter a"
        )
        self.declareProperty(
            name="b", defaultValue=1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter b"
        )
        self.declareProperty(
            name="c", defaultValue=1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter c"
        )
        self.declareProperty(
            name="alpha", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle alpha"
        )
        self.declareProperty(name="beta", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle beta")
        self.declareProperty(
            name="gamma", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle gamma"
        )
        self.declareProperty(
            name="HKLTolerance",
            defaultValue=0.15,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Tolerance to index peaks in in H,K and L - see IndexPeaks for details.",
        )
        self.declareProperty(
            name="DSpacingTolerance",
            defaultValue=0.05,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Tolerance in d-spacing used to try and index peaks",
        )
        self.declareProperty(
            name="AngleTolerance",
            defaultValue=1.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Tolerance (in degrees) of angle between peaks in QLab when identifying possible pairs of HKL.",
        )
        self.declareProperty(
            name="MinAngle",
            defaultValue=2.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Minimum angle in QLab (in degrees) between pairs of peaks in when calculating U matrix.",
        )
        self.declareProperty(
            name="MinDSpacing",
            defaultValue=0.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Min d-spacing of reflections to consider.",
        )
        self.declareProperty(
            name="MaxDSpacing",
            defaultValue=0.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Max d-spacing of reflections to consider.",
        )
        self.declareProperty(
            name="Spacegroup",
            defaultValue="",
            direction=Direction.Input,
            doc="Spacegroup Hermann–Mauguin symbol used to determine the point group of the Laue class.",
        )
        self.declareProperty(
            name="MaxOrder", defaultValue=0, direction=Direction.Input, doc="Maximum order to apply ModVectors. Default = 0"
        )
        enable_modvecs = EnabledWhenProperty("MaxOrder", PropertyCriterion.IsNotDefault)
        for ivec in range(1, 4):
            prop_name = f"ModVector{ivec}"
            self.declareProperty(
                name=prop_name, defaultValue="0.0,0.0,0.0", direction=Direction.Input, doc="Offsets for h, k, l directions"
            )
            self.setPropertySettings(prop_name, enable_modvecs)
        self.declareProperty(
            name="OptimiseFoundUBs",
            defaultValue=True,
            direction=Direction.Input,
            doc="Optimise final UBs using all peaks indexed best by that UB.",
        )

    def validateInputs(self):
        issues = dict()
        # check enough peaks
        if self.getProperty("PeaksWorkspace").value.getNumberPeaks() < _MIN_NUM_INDEXED_PEAKS * self.getProperty("NumberOfUBs").value:
            issues["PeaksWorkspace"] = f"There must be at least {_MIN_NUM_INDEXED_PEAKS} peaks for each UB requested."
        # check point group of laue class can be retrieved
        spgr_sym = self.getProperty("Spacegroup").value
        if not SpaceGroupFactory.isSubscribedSymbol(spgr_sym):
            issues["Spacegroup"] = "Not a valid spacegroup symbol."
        # check min < max
        if self.getProperty("MinDSpacing").value >= self.getProperty("MaxDSpacing").value:
            issues["MinDSpacing"] = "Minimum d-spacing must be less than maximum."
        return issues

    def PyExec(self):
        # Get input
        peaks = self.getProperty("PeaksWorkspace").value
        num_ubs = self.getProperty("NumberOfUBs").value
        hkl_tol = self.getProperty("HKLTolerance").value
        dmax = self.getProperty("MaxDSpacing").value
        dmin = self.getProperty("MinDSpacing").value
        dtol = self.getProperty("DSpacingTolerance").value
        angle_tol = np.radians(self.getProperty("AngleTolerance").value)
        min_angle = np.radians(self.getProperty("MinAngle").value)
        optimise_ubs = self.getProperty("OptimiseFoundUBs").value
        spgr_sym = self.getProperty("Spacegroup").value
        spgr = SpaceGroupFactory.createSpaceGroup(spgr_sym)
        ptgr = PointGroupFactory.createPointGroupFromSpaceGroup(spgr)
        # put properties in kwargs dict for index peaks
        self.index_peaks_kwargs = {"Tolerance": hkl_tol, "CommonUBForAll": True}
        max_order = self.getProperty("MaxOrder").value
        if max_order > 0:
            self.index_peaks_kwargs.update({f"ModVector{ivec}": self.getProperty(f"ModVector{ivec}").value for ivec in range(1, 4)})
            self.index_peaks_kwargs["MaxOrder"] = max_order
            self.index_peaks_kwargs["ToleranceForSatellite"] = hkl_tol  # same as main reflections
            self.index_peaks_kwargs["SaveModulationInfo"] = True
        # put properties in kwargs dict for CalculateUMatrix
        self.calc_u_kwargs = {prop: self.getProperty(prop).value for prop in ("a", "b", "c", "alpha", "beta", "gamma")}

        # generate reflections and calc angles between them
        hkls, dhkls, latt = self.calculate_reflections(spgr_sym, dmin, dmax, dtol, **self.calc_u_kwargs)
        bmat = latt.getB()
        bmat_inv = latt.getBinv()  # used later when comparing U matrices
        angles = self.calculate_angles_between_reflections(hkls, bmat)

        # get all peaks between d-spacing limits
        peaks_filtered = self.exec_child_alg(
            "FilterPeaks", InputWorkspace=peaks, FilterVariable="DSpacing", FilterValue=dmin - dtol, Operator=">="
        )
        peaks_filtered = self.exec_child_alg(
            "FilterPeaks", InputWorkspace=peaks_filtered, FilterVariable="DSpacing", FilterValue=dmax + dtol, Operator="<="
        )

        # print info about reflections considered to log
        logger.information("H, K, L, d-spac (Ang)")
        logger.information(f"{np.c_[hkls, np.round(dhkls, 3)]}")
        logger.information(f"Observed peak d-spacings within limits: {dmin} < d < {dmax}")
        logger.information(f"{np.round(peaks_filtered.column('DSpacing'), 3)}")

        # loop over every pair of peaks
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=len(dhkls))
        ipairs = list(combinations(range(peaks_filtered.getNumberPeaks()), 2))
        ubs = []
        hkl_ers = []
        for ipair, ipks in enumerate(ipairs):
            prog_reporter.report()
            pks = [peaks_filtered.getPeak(ii) for ii in ipks]
            angle_observed = pks[0].getQLabFrame().angle(pks[1].getQLabFrame())  # between observed peaks
            if angle_observed > min_angle:
                # this means don't have to worry about reflections being along same direction
                # get index of reflections with consistent d-spacing
                ihkls = [np.flatnonzero(abs(dhkls - pks[ii].getDSpacing()) < dtol) for ii in range(2)]
                ihkl_pairs_used = []
                if all(irefl.size > 0 for irefl in ihkls):
                    # peaks are both consistent with d-spacing of at least one reflection
                    # loop over every pair of consistent reflections
                    ihkl_pairs = list(product(*ihkls))
                    for ihkl_pair in ihkl_pairs:
                        angle_hkl = angles[ihkl_pair]  # angle between reflections
                        if angle_hkl > min_angle and abs(angle_hkl - angle_observed) < angle_tol:
                            # angle between observed peaks is consistent with reflections
                            # check pair of reflections are not equivalent to any pair previously used
                            if not self.any_equivalent_pair_reflections(ptgr, angles, hkls, ihkl_pair, ihkl_pairs_used):
                                ihkl_pairs_used.append(ihkl_pair)
                                # set hkl of peaks
                                [pk.setHKL(0, 0, 0) for pk in peaks_filtered]  # reset HKL of all peaks
                                [pk.setHKL(*hkls[ihkl_pair[ipk]]) for ipk, pk in enumerate(pks)]
                                try:
                                    self.exec_child_alg("CalculateUMatrix", PeaksWorkspace=peaks_filtered, **self.calc_u_kwargs)
                                except ValueError:
                                    continue  # skip this pair
                                # SetUB in all found peaks and calc hkl_ers
                                this_ub = peaks_filtered.sample().getOrientedLattice().getUB().copy()
                                self.exec_child_alg("SetUB", Workspace=peaks, UB=this_ub)
                                nindexed, *_ = self.exec_child_alg(
                                    "IndexPeaks", PeaksWorkspace=peaks, RoundHKLs=False, **self.index_peaks_kwargs
                                )
                                if nindexed >= _MIN_NUM_INDEXED_PEAKS:
                                    # calculate hkl er in indexing
                                    mod_hkl = peaks.sample().getOrientedLattice().getModHKL()
                                    this_hkl_ers = np.array(
                                        [
                                            (
                                                np.sum((pk.getIntHKL() + (mod_hkl @ pk.getIntMNP()) - pk.getHKL()) ** 2)
                                                if np.any(pk.getHKL())
                                                else np.inf
                                            )
                                            for pk in peaks
                                        ]
                                    )

                                    # see if similar UBs have already been calculated
                                    this_u = this_ub @ bmat_inv
                                    iub_similar = np.array(
                                        [
                                            iub
                                            for iub in range(len(ubs))
                                            if self.calculate_angle_between_rotation_matrices(ubs[iub] @ bmat_inv, this_u) < angle_tol
                                        ]
                                    )
                                    if len(iub_similar) == 0:
                                        ubs.append(this_ub)
                                        hkl_ers.append(this_hkl_ers)
                                    else:
                                        idel = self.find_similar_ubs_that_index_less_accurately(hkl_ers, iub_similar, this_hkl_ers)
                                        if idel:
                                            # delete in reverse order so as to preserve indexing
                                            for iub in idel[::-1]:
                                                ubs.pop(iub)
                                                hkl_ers.pop(iub)
                                            ubs.append(this_ub)
                                            hkl_ers.append(this_hkl_ers)
        # categorise peaks belonging to each UB requested
        if not ubs:
            raise RuntimeError(f"No valid UBs found using {peaks_filtered.getNumberPeaks()} peaks in d-spacing range.")
        peaks_out_list = self.get_peak_tables_for_each_ub(peaks, ubs, hkl_ers, num_ubs, optimise_ubs) if ubs else []
        ws_out = self.exec_child_alg("GroupWorkspaces", InputWorkspaces=peaks_out_list)
        self.setProperty("OutputWorkspace", ws_out)

    def exec_child_alg(self, alg_name, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def get_peak_tables_for_each_ub(self, peaks, ubs, hkl_ers, num_ubs, optimise_ubs):
        # see which ubs index the most peaks most accurately (with minimum error)
        iub_min_er = np.argmin(hkl_ers, axis=0)
        # exclude reflections that are not indexed by any UB
        i_indexed = np.any(np.isfinite(hkl_ers), axis=0)
        iub, nindex = np.unique(iub_min_er[i_indexed], return_counts=True)
        # sort by ubs that index most peaks
        isort = np.argsort(-nindex)  # descending
        iub = iub[isort]
        # return peak tables with peaks best indexed by top nub
        nub = min(num_ubs, len(iub))
        peaks_out_list = []
        for itable in range(nub):
            # get peaks indexed best by this UB
            ipks = np.flatnonzero(np.logical_and(iub_min_er == iub[itable], i_indexed))
            peaks_out = self.exec_child_alg("CreatePeaksWorkspace", InstrumentWorkspace=peaks, NumberOfPeaks=0)
            ADS.addOrReplace(f"{peaks.name()}_ub{itable}", peaks_out)
            [peaks_out.addPeak(peaks.getPeak(int(ipk))) for ipk in ipks]
            self.exec_child_alg("SetUB", Workspace=peaks_out, UB=ubs[iub[itable]])
            self.exec_child_alg("IndexPeaks", PeaksWorkspace=peaks_out, RoundHKLs=True, **self.index_peaks_kwargs)
            if optimise_ubs and peaks_out.getNumberPeaks() > _MIN_NUM_INDEXED_PEAKS:
                try:
                    self.exec_child_alg("CalculateUMatrix", PeaksWorkspace=peaks_out, **self.calc_u_kwargs)
                except ValueError:
                    # reset UB
                    self.exec_child_alg("SetUB", Workspace=peaks_out, UB=ubs[iub[itable]])
            peaks_out_list.append(peaks_out)
        return peaks_out_list

    @staticmethod
    def calculate_reflections(spgr_sym, dmin, dmax, dtol, a, b, c, alpha, beta, gamma):
        xtal = CrystalStructure(f"{a} {b} {c} {alpha} {beta} {gamma}", spgr_sym, "")  # no basis required
        generator = ReflectionGenerator(xtal, ReflectionConditionFilter.Centering)
        hkls = generator.getHKLs(dmin - dtol, dmax + dtol)
        dhkls = np.array(generator.getDValues(hkls))
        isort = np.argsort(-dhkls)
        dhkls = dhkls[isort]
        hkls = [hkls[ihkl] for ihkl in isort]
        return hkls, dhkls, xtal.getUnitCell()

    @staticmethod
    def calculate_angles_between_reflections(hkls, bmat):
        angles = np.zeros(2 * [len(hkls)])
        for ihkl_1 in range(1, angles.shape[0]):
            qlab1 = 2 * np.pi * bmat @ hkls[ihkl_1]
            for ihkl_2 in range(0, ihkl_1):
                qlab2 = 2 * np.pi * bmat @ hkls[ihkl_2]
                angles[ihkl_1, ihkl_2] = V3D(*qlab1).angle(V3D(*qlab2))
                angles[ihkl_2, ihkl_1] = angles[ihkl_1, ihkl_2]
        return angles

    @staticmethod
    def any_equivalent_pair_reflections(ptgr, angles, hkls, ihkl_pair, ihkl_pairs_used):
        any_equiv = False
        hkl_pair = [hkls[irefl] * (1 / hkls[irefl].norm()) for irefl in ihkl_pair]
        for ihkl_pair_prev in ihkl_pairs_used:
            # check is not equivalent to any previous one
            same_angle = np.isclose(angles[ihkl_pair_prev], angles[ihkl_pair])
            # check hkls unit vectors are not equivalent
            hkl_pair_prev = [hkls[irefl] * (1 / hkls[irefl].norm()) for irefl in ihkl_pair_prev]
            equiv_hkl = all([ptgr.isEquivalent(hkl_pair[irefl], hkl_pair_prev[irefl]) for irefl in range(len(hkl_pair))])
            if same_angle and equiv_hkl:
                any_equiv = True
                break
        return any_equiv

    @staticmethod
    def calculate_angle_between_rotation_matrices(R1, R2):
        R = np.dot(R1, R2.T)
        cos_theta = np.clip((np.trace(R) - 1) / 2, -1, 1)  # in case outside range due to machine precision
        return np.arccos(abs(cos_theta))

    @staticmethod
    def find_similar_ubs_that_index_less_accurately(hkl_ers, iub_similar, this_hkl_ers):
        idel = []
        for iub in iub_similar:
            prev_hkl_ers = np.asarray(hkl_ers[iub])
            irefl_indexed = np.logical_or(np.isfinite(this_hkl_ers), np.isfinite(prev_hkl_ers))
            nrefl_indexed = irefl_indexed.sum()
            nrefl_indexed_better = np.sum(this_hkl_ers[irefl_indexed] < prev_hkl_ers[irefl_indexed])
            if nrefl_indexed_better > nrefl_indexed / 2:
                idel.append(iub)
        return idel


# register algorithm with mantid
AlgorithmFactory.subscribe(FindMultipleUMatrices)
