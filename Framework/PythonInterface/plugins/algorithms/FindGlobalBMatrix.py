# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, Progress, ADSValidator, IPeaksWorkspace
from mantid.simpleapi import AnalysisDataService, logger
from mantid.kernel import Direction, FloatBoundedValidator, StringArrayProperty
import numpy as np
from scipy.optimize import leastsq
from plugins.algorithms.FindGoniometerFromUB import getSignMaxAbsValInCol

_MIN_NUM_PEAKS = 6  # minimum required to use FindUBUsingLatticeParameters assuming all linearly indep.
_MIN_NUM_INDEXED_PEAKS = 2  # minimum indexed peaks required for CalculateUMatrix
_MIN_NUM_VALID_WS = 2  # minimum number of workspaces with satisfactory peak tables


class FindGlobalBMatrix(DataProcessorAlgorithm):
    def name(self):
        return "FindGlobalBMatrix"

    def category(self):
        return "Diffraction\\Reduction;Crystal\\UBMatrix"

    def seeAlso(self):
        return ["FindUBUsingLatticeParameters", "CalculateUMatrix"]

    def summary(self):
        return "Takes multiple peak tables from different runs and refines common lattice parameters and angles."

    def PyInit(self):
        # Input
        self.declareProperty(
            StringArrayProperty(name="PeakWorkspaces", direction=Direction.Input, validator=ADSValidator()),
            doc="List of peak workspaces to use (must be more than" " two peaks workspaces and each must contain at least 6 peaks.",
        )
        positiveFloatValidator = FloatBoundedValidator(lower=0.0)
        angleValidator = FloatBoundedValidator(lower=0.0, upper=180.0)
        self.declareProperty(
            name="a", defaultValue=-1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter a"
        )
        self.declareProperty(
            name="b", defaultValue=-1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter b"
        )
        self.declareProperty(
            name="c", defaultValue=-1.0, direction=Direction.Input, validator=positiveFloatValidator, doc="Lattice parameter c"
        )
        self.declareProperty(
            name="alpha", defaultValue=-1.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle alpha"
        )
        self.declareProperty(name="beta", defaultValue=-1.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle beta")
        self.declareProperty(
            name="gamma", defaultValue=-1.0, direction=Direction.Input, validator=angleValidator, doc="Lattice angle gamma"
        )
        self.declareProperty(
            name="Tolerance",
            defaultValue=0.15,
            direction=Direction.Input,
            validator=positiveFloatValidator,
            doc="Tolerance to index peaks in in H,K and L",
        )

    def validateInputs(self):
        issues = dict()
        n_valid_ws = 0
        ws_list = self.getProperty("PeakWorkspaces").value
        for wsname in ws_list:
            ws = AnalysisDataService.retrieve(wsname)
            if not isinstance(ws, IPeaksWorkspace):
                issues["PeakWorkspaces"] = f"{wsname} is not a PeaksWorkspace."
                break
            if ws.sample().hasOrientedLattice():
                nindexed, *_ = self.exec_child_alg("IndexPeaks", PeaksWorkspace=wsname, RoundHKLs=True, CommonUBForAll=True)
                if nindexed < _MIN_NUM_INDEXED_PEAKS:
                    issues["PeakWorkspaces"] = (
                        f"{wsname} has a UB set, therefore it must contain at least " f"{_MIN_NUM_INDEXED_PEAKS} peaks that can be indexed."
                    )
                else:
                    # if it has UB and doesn't have too few peaks it is valid
                    n_valid_ws += 1
            elif ws.getNumberPeaks() < _MIN_NUM_PEAKS:
                issues["PeakWorkspaces"] = (
                    f"{wsname} does not have a UB set, therefore it must " f"contain at least {_MIN_NUM_PEAKS} peaks."
                )
            else:
                # if it doesn't have a UB but does have more than the min num peaks it is also valid
                n_valid_ws += 1
        if n_valid_ws < _MIN_NUM_VALID_WS:
            issues["PeakWorkspaces"] = (
                f"Fewer than the desired number of {_MIN_NUM_VALID_WS} workspaces "
                f"have valid peak tables. "
                f"Currently {n_valid_ws}/{len(ws_list)} of provided workspaces are valid"
            )

        return issues

    def PyExec(self):
        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=3)
        # Get input
        ws_list = self.getProperty("PeakWorkspaces").value
        a = self.getProperty("a").value
        b = self.getProperty("b").value
        c = self.getProperty("c").value
        alpha = self.getProperty("alpha").value
        beta = self.getProperty("beta").value
        gamma = self.getProperty("gamma").value
        self.tol = self.getProperty("Tolerance").value

        # Find initial UB and use to index peaks in all runs
        prog_reporter.report(1, "Find initial UB for peak indexing")
        self.find_initial_indexing(a, b, c, alpha, beta, gamma, ws_list)  # removes runs from ws_list if can't index

        # optimize the lattice parameters across runs (i.e. B matrix)
        prog_reporter.report(2, "Optimize B")

        def fobj(x):
            return self.calcResiduals(x, ws_list)

        alatt0 = [a, b, c, alpha, beta, gamma]
        alatt, cov, info, msg, ier = leastsq(fobj, x0=alatt0, full_output=True)
        # eval the fobj at optimal solution to set UB (leastsq iteration stops at a next sub-optimal solution)
        fobj(alatt)
        success = ier in [1, 2, 3, 4] and cov is not None  # cov is None when matrix is singular
        if success:
            # calculate errors
            n_peaks = [self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws, RoundHKLs=True, CommonUBForAll=False)[0] for ws in ws_list]
            dof = sum(n_peaks) - len(alatt0)
            err = np.sqrt(abs(np.diag(cov)) * (info["fvec"] ** 2).sum() / dof)
            for iws, wsname in enumerate(ws_list):
                ws = AnalysisDataService.retrieve(wsname)
                ws.sample().getOrientedLattice().setError(*err)
                if n_peaks[iws] < _MIN_NUM_INDEXED_PEAKS:
                    logger.warning(
                        f"Workspace: {wsname}, has only {n_peaks[iws]} indexed peaks, " f"fewer than the desired {_MIN_NUM_INDEXED_PEAKS}"
                    )
            logger.notice(
                f"Lattice parameters successfully refined for workspaces: {ws_list}\n"
                f"Lattice Parameters: {np.array2string(alatt, precision=6)}\n"
                f"Parameter Errors  : {np.array2string(err, precision=6)}"
            )
        else:
            logger.warning(f"Error in optimization of lattice parameters: {msg}")
        # complete progress
        prog_reporter.report(3, "Done")

    def find_initial_indexing(self, a, b, c, alpha, beta, gamma, ws_list):
        # check if a UB exists on any run and if so whether it indexes a sufficient number of peaks
        foundUB = False

        # find which of the ws have a ub and how many peaks are indexed
        iws_have_ub = np.array([iws for iws, ws in enumerate(ws_list) if AnalysisDataService.retrieve(ws).sample().hasOrientedLattice()])
        nindexed = np.array(
            [self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws_list[iws], RoundHKLs=True, CommonUBForAll=False)[0] for iws in iws_have_ub]
        )

        # find which of the ws have a sufficient number of indexed peaks to be the reference
        ref_mask = np.flatnonzero(nindexed > _MIN_NUM_INDEXED_PEAKS)
        iws_potential_ref_ub = iws_have_ub[ref_mask]
        nindexed_ref = nindexed[ref_mask]
        foundUB = len(iws_potential_ref_ub) > 0

        # if none of the ws with UBs index enough peaks try calculating new UB for all from Lattice Params
        if not foundUB:
            # loop over all ws and try to find a UB
            for iws, ws in enumerate(ws_list):
                try:
                    self.exec_child_alg(
                        "FindUBUsingLatticeParameters",
                        PeaksWorkspace=ws,
                        a=a,
                        b=b,
                        c=c,
                        alpha=alpha,
                        beta=beta,
                        gamma=gamma,
                        FixParameters=False,
                    )
                    nindexed = self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws, RoundHKLs=True, CommonUBForAll=False)[0]
                    foundUB = nindexed >= _MIN_NUM_INDEXED_PEAKS
                except ValueError:
                    pass
                if foundUB:
                    # to reach this point this current UB must be the only potential reference we have
                    # still need to evaluate_best_ref to get n_peaks indexed by this UB
                    # evaluate_best_ref requires lists of ref options
                    iws_potential_ref_ub = [iws]
                    nindexed_ref = [nindexed]
                    break  # stop once found one UB found as FindUBUsingLatticeParameter takes a long time
        if not foundUB:
            raise RuntimeError("An initial UB could not be found with the provided lattice parameters")

        # we'll evaluate this in a separate function to make it easier to swap out for a different metric
        ref_ub, n_indexed_by_ref, iref = self.evaluate_best_ref_UB(iws_potential_ref_ub, nindexed_ref, ws_list)

        # set this UB and re-index the ws
        for iws, cws in enumerate(ws_list):
            self.exec_child_alg("SetUB", Workspace=cws, UB=ref_ub)

        # for the ws with fewer peaks than threshold, try and find some more by adjusting U slightly
        for iws in np.where(n_indexed_by_ref < _MIN_NUM_INDEXED_PEAKS)[0]:
            self.exec_child_alg(
                "FindUBUsingLatticeParameters",
                PeaksWorkspace=ws_list[iws],
                a=a,
                b=b,
                c=c,
                alpha=alpha,
                beta=beta,
                gamma=gamma,
                FixParameters=False,
            )
            self.make_UB_consistent(ws_list[iref], ws_list[iws])
            nindexed = self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws_list[iws], RoundHKLs=True, CommonUBForAll=False)[0]

            # if still too few, warn user and remove
            if nindexed < _MIN_NUM_INDEXED_PEAKS:
                logger.warning(f"Fewer than the desired {_MIN_NUM_INDEXED_PEAKS} peaks were indexed for Workspace {iws}")
                ws_list.pop(iws)
                logger.warning(f"Workspace {iws} removed")

    def evaluate_best_ref_UB(self, iws_potential_ref_ub, nindexed_ref, ws_list):
        indexed_peaks = np.zeros((len(iws_potential_ref_ub), len(ws_list)))

        # need to get the UBs first as we are setting them in-place
        pot_ubs = [
            AnalysisDataService.retrieve(ws_list[iref]).sample().getOrientedLattice().getUB().copy() for iref in iws_potential_ref_ub
        ]

        # iterate over all the potential reference workspaces and find how many peaks each UB indexes
        for iub, iref in enumerate(iws_potential_ref_ub):
            ref_ub = pot_ubs[iub]

            for iws, cws in enumerate(ws_list):
                if iws == iref:
                    indexed_peaks[iub, iws] = nindexed_ref[iub]
                else:
                    self.exec_child_alg("SetUB", Workspace=cws, UB=ref_ub)
                    indexed_peaks[iub, iws] = self.exec_child_alg("IndexPeaks", PeaksWorkspace=cws, RoundHKLs=True, CommonUBForAll=True)[0]

        # find, for each UB, the number of ws that have n_peaks over the threshold and the total sum of these
        n_ws_over_thresh = np.sum(indexed_peaks >= _MIN_NUM_INDEXED_PEAKS, axis=1)
        total_indexed_peaks = np.sum(indexed_peaks, axis=1)

        # find which UBs give the most over threshold and then, of these, which fit the most peaks
        max_over_thresh = np.argwhere(n_ws_over_thresh == np.max(n_ws_over_thresh))
        best_iub = max_over_thresh[np.argmax(total_indexed_peaks[max_over_thresh])][0]

        return pot_ubs[best_iub], indexed_peaks[best_iub], iws_potential_ref_ub[best_iub]

    def make_UB_consistent(self, ws_ref, ws):
        # compare U matrix to perform TransformHKL to preserve indexing
        U_ref = AnalysisDataService.retrieve(ws_ref).sample().getOrientedLattice().getU()
        U = AnalysisDataService.retrieve(ws).sample().getOrientedLattice().getU()
        # find transform required  ( U_ref = U T^-1) - see TransformHKL docs for details
        transform = np.linalg.inv(getSignMaxAbsValInCol(np.linalg.inv(U) @ U_ref))
        self.exec_child_alg("TransformHKL", PeaksWorkspace=ws, HKLTransform=transform, FindError=False)

    def calcResiduals(self, x0, ws_list):
        """
        Calculates average of square magnitude of difference between qsample and q of integer HKL
        :param x0: lattice parameters [a, b, c, alpha, beta, gamma]
        :param ws_list: list of peak workspaces
        :return: sqrt of average square residuals (required by scipy.leastsq optimiser - default settings behave better)
        """
        residsq = np.zeros(sum([AnalysisDataService.retrieve(wsname).getNumberPeaks() for wsname in ws_list]))
        ipk = 0  # normalise by n peaks indexed so no penalty in indexing more peaks
        for wsname in ws_list:
            # index peaks with CommonUBForAll=False (optimises a temp. UB when indexing - helps for bad guesses)
            nindexed = self.exec_child_alg("IndexPeaks", PeaksWorkspace=wsname, RoundHKLs=True, CommonUBForAll=False)[0]
            if nindexed >= _MIN_NUM_INDEXED_PEAKS:
                try:
                    self.exec_child_alg(
                        "CalculateUMatrix",
                        PeaksWorkspace=wsname,
                        a=x0[0],
                        b=x0[1],
                        c=x0[2],
                        alpha=x0[3],
                        beta=x0[4],
                        gamma=x0[5],
                    )
                except ValueError:
                    logger.error(f"Cannot calculate U Matrix for {wsname} - this workspace should be removed.")
                    return None
                # don't index with optimisation after this point and don't round HKL (to calc resids)
                self.exec_child_alg("IndexPeaks", PeaksWorkspace=wsname, RoundHKLs=True, CommonUBForAll=True)
                ws = AnalysisDataService.retrieve(wsname)
                UB = 2 * np.pi * ws.sample().getOrientedLattice().getUB()
                for ii in range(ws.getNumberPeaks()):
                    pk = ws.getPeak(ii)
                    if pk.getHKL().norm2() > 1e-6:
                        residsq[ipk] = np.sum((UB @ pk.getIntHKL() - pk.getQSampleFrame()) ** 2)
                        ipk += 1
        residsq = residsq[np.isfinite(residsq)]
        return np.sqrt(residsq / (ipk + 1))

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props


# register algorithm with mantid
AlgorithmFactory.subscribe(FindGlobalBMatrix)
