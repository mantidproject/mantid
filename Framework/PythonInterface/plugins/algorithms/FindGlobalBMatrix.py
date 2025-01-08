# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, Progress, ADSValidator, IPeaksWorkspace
from mantid.simpleapi import AnalysisDataService, logger, SetUB
from mantid.kernel import Direction, FloatBoundedValidator, StringArrayProperty
import numpy as np
from scipy.optimize import leastsq
from plugins.algorithms.FindGoniometerFromUB import getSignMaxAbsValInCol

_MIN_NUM_PEAKS = 6  # minimum required to use FindUBUsingLatticeParameters assuming all linearly indep.
_MIN_NUM_INDEXED_PEAKS = 3  # minimum indexed peaks required for CalculateUMatrix


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
        ws_list = self.getProperty("PeakWorkspaces").value
        n_valid_ws = 0
        for wsname in ws_list:
            ws = AnalysisDataService.retrieve(wsname)
            if isinstance(ws, IPeaksWorkspace) and ws.getNumberPeaks() >= _MIN_NUM_PEAKS:
                n_valid_ws += 1
        if n_valid_ws < 2 or n_valid_ws < len(ws_list):
            issues["PeakWorkspaces"] = (
                f"Accept only peaks workspace with more than {_MIN_NUM_PEAKS} peaks - "
                "there must be at least two peak tables provided in total."
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
            dof = sum(
                [self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws, RoundHKLs=True, CommonUBForAll=False)[0] for ws in ws_list]
            ) - len(alatt0)
            err = np.sqrt(abs(np.diag(cov)) * (info["fvec"] ** 2).sum() / dof)
            for wsname in ws_list:
                ws = AnalysisDataService.retrieve(wsname)
                ws.sample().getOrientedLattice().setError(*err)
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
        ws_with_UB = [
            (iws, self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws, RoundHKLs=True, CommonUBForAll=False)[0])
            for iws, ws in enumerate(ws_list)
            if AnalysisDataService.retrieve(ws).sample().hasOrientedLattice()
        ]  # [(iws, n_peaks_indexed),...]

        # find which of the ws have a sufficient number of indexed peaks to be the reference
        potential_ref_UB_iws = [(iws, n_peaks) for iws, n_peaks in ws_with_UB if n_peaks >= _MIN_NUM_INDEXED_PEAKS]
        foundUB = len(potential_ref_UB_iws) > 0

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
                    potential_ref_UB_iws = [(iws, nindexed)]
                    break  # stop once found one UB found as FindUBUsingLatticeParameter takes a long time
        if not foundUB:
            raise RuntimeError("An initial UB could not be found with the provided lattice parameters")

        # iterate over the potential reference UBs to find the best one
        potential_ref_UBs = [
            AnalysisDataService.retrieve(ws_list[iws]).sample().getOrientedLattice().getUB().copy()
            for (iws, n_peaks) in potential_ref_UB_iws
        ]

        # we'll evaluate this in a separate function to make it easier to swap out for a different metric
        best_iub, n_indexed_by_ref = self.evaluate_best_ref_UB(potential_ref_UB_iws, potential_ref_UBs, ws_list)
        ref_ub = potential_ref_UBs[best_iub]

        # set this UB and re-index the ws
        for iws, cws in enumerate(ws_list):
            SetUB(cws, UB=ref_ub)
            self.exec_child_alg("IndexPeaks", PeaksWorkspace=cws, RoundHKLs=True, CommonUBForAll=False)

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
            self.make_UB_consistent(ws_list[best_iub], ws_list[iws])
            nindexed = self.exec_child_alg("IndexPeaks", PeaksWorkspace=ws_list[iws], RoundHKLs=True, CommonUBForAll=False)[0]

            # if still too few, warn user and remove
            if nindexed < _MIN_NUM_INDEXED_PEAKS:
                logger.warning(f"Fewer than the desired {_MIN_NUM_INDEXED_PEAKS} peaks were indexed for Workspace {iws}")
                ws_list.pop(iws)
                logger.warning(f"Workspace {iws} removed")

    def evaluate_best_ref_UB(self, potential_ref_UB_iws, potential_ref_UBs, ws_list):
        # create an array to save n indexed peaks for each ref option
        indexed_peaks = np.zeros((len(potential_ref_UBs), len(ws_list)))

        # iterate over all the potential reference workspaces and find how many peaks each UB indexes
        for iub, (iref, n_peaks) in enumerate(potential_ref_UB_iws):
            ref_ub = potential_ref_UBs[iub]

            for iws, cws in enumerate(ws_list):
                if iws == iref:
                    indexed_peaks[iub, iws] = n_peaks
                else:
                    SetUB(cws, UB=ref_ub)
                    indexed_peaks[iub, iws] = self.exec_child_alg("IndexPeaks", PeaksWorkspace=cws, RoundHKLs=True, CommonUBForAll=True)[0]
        # find, for each UB, the number of ws that have n_peaks over the threshold and the total sum of these
        n_ws_over_thresh = np.sum(indexed_peaks >= _MIN_NUM_INDEXED_PEAKS, axis=1)
        total_indexed_peaks = np.sum(indexed_peaks, axis=1)

        # find which UBs give the most over threshold and then, of these, which fit the most peaks
        max_over_thresh = np.argwhere(n_ws_over_thresh == np.max(n_ws_over_thresh))
        best_iub = max_over_thresh[np.argmax(total_indexed_peaks[max_over_thresh])][0]

        return best_iub, indexed_peaks[best_iub]

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
