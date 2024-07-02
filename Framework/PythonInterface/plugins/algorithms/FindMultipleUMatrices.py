# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
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
from mantid.dataobjects import PeaksWorkspace
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator, EnabledWhenProperty, PropertyCriterion, logger
from mantid.geometry import (
    SpaceGroupFactory,
    PointGroupFactory,
    CrystalStructure,
    ReflectionGenerator,
    ReflectionConditionFilter,
    SpaceGroup,
    PointGroup,
    UnitCell,
)
from itertools import combinations, product
import numpy as np
from typing import Sequence


_MIN_NUM_INDEXED_PEAKS = 3  # one more peak indexed than required for CalculateUMatrix


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
            name="alpha", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle alpha (in degrees)"
        )
        self.declareProperty(
            name="beta", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle beta (in degrees)"
        )
        self.declareProperty(
            name="gamma", defaultValue=90.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle gamma (in degrees)"
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
            name="MinAngleBetweenPeaks",
            defaultValue=2.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Minimum angle in QLab (in degrees) between pairs of peaks in when calculating U matrix.",
        )
        self.declareProperty(
            name="MinAngleBetweenUB",
            defaultValue=2.0,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Minimum angle in degrees between u and v vectors (converted from HKL to QLab) of different UB.",
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
        min_angle = np.radians(self.getProperty("MinAngleBetweenPeaks").value)
        min_angle_ub = np.radians(self.getProperty("MinAngleBetweenUB").value)
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
        hkls, dhkls, unit_cell = self.calculate_reflections(spgr_sym, dmin, dmax, dtol, **self.calc_u_kwargs)

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
                if all(irefl.size > 0 for irefl in ihkls):
                    # peaks are both consistent with d-spacing of at least one reflection
                    # loop over every pair of consistent reflections
                    ihkl_pairs = list(product(*ihkls))
                    for ihkl_pair in ihkl_pairs:
                        # generate angles between equivalent hkls for these reflections
                        hkls1 = ptgr.getEquivalents(hkls[ihkl_pair[0]])
                        hkls2 = ptgr.getEquivalents(hkls[ihkl_pair[1]])
                        angles = self.calculate_angles_between_reflections(hkls1, hkls2, unit_cell)
                        # get a single pair of HKL consistent with observed angle
                        ihkl1, ihkl2 = np.unravel_index(np.argmin(abs(angles - angle_observed)), angles.shape)
                        angle_hkl = angles[ihkl1, ihkl2]
                        if angle_hkl > min_angle and abs(angle_hkl - angle_observed) < angle_tol:
                            # set hkl of peaks
                            [pk.setHKL(0, 0, 0) for pk in peaks_filtered]  # reset HKL of all peaks
                            pks[0].setHKL(*hkls1[ihkl1])
                            pks[1].setHKL(*hkls2[ihkl2])
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
                                ubs.append(this_ub)
                                hkl_ers.append(self.calculate_hkl_ers(peaks, spgr))
        # categorise peaks belonging to each UB requested
        if not ubs:
            raise RuntimeError(f"No valid UBs found using {peaks_filtered.getNumberPeaks()} peaks in d-spacing range.")
        # filter out similar UBs
        hkl_ers = np.array(hkl_ers)
        hkl_ers, ubs = self.remove_equiv_ubs_by_accuracy(hkl_ers, ubs, ptgr, unit_cell, min_angle_ub)
        # get nub most accurate
        peaks_out_list = self.get_peak_tables_for_each_ub(peaks, ubs, hkl_ers, num_ubs, optimise_ubs) if ubs else []
        ws_out = self.exec_child_alg("GroupWorkspaces", InputWorkspaces=peaks_out_list)
        self.setProperty("OutputWorkspace", ws_out)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def remove_equiv_ubs_by_accuracy(
        self, hkl_ers: np.ndarray, ubs: Sequence[np.ndarray], pointgroup: PointGroup, unit_cell: UnitCell, min_angle_ub: float
    ):
        iub_keep = set()
        for iub, ub in enumerate(ubs):
            if iub in iub_keep:
                continue
            iub_similar = [
                int(iub_other)
                for iub_other in range(len(ubs))
                if self.are_ubs_equivalent_within_tolerance(ub, ubs[iub_other], pointgroup, unit_cell, min_angle_ub) and iub_other != iub
            ]
            if iub_similar:
                ibest, *_ = self.get_indices_of_nub_most_accurate_ubs(hkl_ers[tuple(iub_similar), :], num_ubs=1)
                iub_keep.add(iub_similar[next(iter(ibest))])
            else:
                iub_keep.add(iub)
        iub_keep = list(iub_keep)
        return hkl_ers[iub_keep], [ubs[ii] for ii in iub_keep]

    @staticmethod
    def are_ubs_equivalent_within_tolerance(
        ub1: np.ndarray, ub2: np.ndarray, pointgroup: PointGroup, unit_cell: UnitCell, angle_tol: float
    ):
        inv_ub1 = np.linalg.inv(ub1)
        inv_ub2 = np.linalg.inv(ub2)
        axes_equiv = 2 * [False]
        # loop over hkl vectors along 2 of QLab cartesian axes
        for iqax in range(2):
            # loop over all equivalent HKL
            for hkl1 in pointgroup.getEquivalents(inv_ub1[iqax, :]):
                if abs(unit_cell.recAngle(*hkl1, *inv_ub2[iqax, :], 1)) < angle_tol:
                    axes_equiv[iqax] = True
                    break
            if not axes_equiv[iqax]:
                break
        return all(axes_equiv)

    def get_indices_of_nub_most_accurate_ubs(self, hkl_ers: np.ndarray, num_ubs: int = 1):
        # see which ubs index the most peaks most accurately (with minimum error)
        iub_min_er = np.argmin(hkl_ers, axis=0)
        # exclude reflections that are not indexed by any UB
        hkl_ers_indexed = np.isfinite(hkl_ers)
        ipks_indexed = np.any(hkl_ers_indexed, axis=0)
        iub, nindex_best = np.unique(iub_min_er[ipks_indexed], return_counts=True)
        # remove ubs that don't index any peaks most accurately
        ikeep = nindex_best > 0
        iub = iub[ikeep]
        nindex_best = nindex_best[ikeep]
        # sort by ubs that index peaks most accurately
        # if same num peaks indexed most accurately then additionally by total number peaks indexed
        nindex_total = hkl_ers_indexed[iub, :].sum(axis=1)  # total num peaks indexed by each UB within tol
        isort = np.lexsort((-nindex_total, -nindex_best))
        iub = iub[isort].astype(int)
        if len(iub) <= num_ubs:
            return iub, iub_min_er, ipks_indexed
        else:
            hkl_ers[iub[-1], :] = np.inf  # overwrite values as if didn't index any peaks
            return self.get_indices_of_nub_most_accurate_ubs(hkl_ers, num_ubs)

    def get_peak_tables_for_each_ub(
        self, peaks: PeaksWorkspace, ubs: Sequence[np.ndarray], hkl_ers: np.ndarray, num_ubs: int, optimise_ubs: bool
    ):
        iub, iub_min_er, ipks_indexed = self.get_indices_of_nub_most_accurate_ubs(hkl_ers, num_ubs)
        peaks_out_list = []
        for itable in range(min(num_ubs, len(iub))):
            # get peaks indexed best by this UB
            ipks = np.flatnonzero(np.logical_and(iub_min_er == iub[itable], ipks_indexed))
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
    def calculate_reflections(
        spgr_sym: str, dmin: float, dmax: float, dtol: float, a: float, b: float, c: float, alpha: float, beta: float, gamma: float
    ):
        xtal = CrystalStructure(f"{a} {b} {c} {alpha} {beta} {gamma}", spgr_sym, "")  # no basis required
        generator = ReflectionGenerator(xtal, ReflectionConditionFilter.Centering)
        hkls = generator.getUniqueHKLs(dmin - dtol, dmax + dtol)  # getUnique
        dhkls = np.array(generator.getDValues(hkls))
        isort = np.argsort(-dhkls)
        dhkls = dhkls[isort]
        hkls = [hkls[ihkl] for ihkl in isort]
        return hkls, dhkls, xtal.getUnitCell()

    @staticmethod
    def calculate_angles_between_reflections(hkls1: Sequence[float], hkls2: Sequence[float], unit_cell):
        angles = np.zeros((len(hkls1), len(hkls2)))
        for ihkl_1, hkl1 in enumerate(hkls1):
            for ihkl_2, hkl2 in enumerate(hkls2):
                angles[ihkl_1, ihkl_2] = unit_cell.recAngle(*hkl1, *hkl2, 1)  # final arg specifies radians
        return angles

    @staticmethod
    def calculate_hkl_ers(peaks: PeaksWorkspace, spacegroup: SpaceGroup):
        mod_hkl = peaks.sample().getOrientedLattice().getModHKL()
        return np.array(
            [
                (
                    np.sum((pk.getIntHKL() + (mod_hkl @ pk.getIntMNP()) - pk.getHKL()) ** 2)
                    if not np.allclose(pk.getHKL(), 0.0) and spacegroup.isAllowedReflection(pk.getIntHKL())
                    else np.inf
                )
                for pk in peaks
            ]
        )


# register algorithm with mantid
AlgorithmFactory.subscribe(FindMultipleUMatrices)
