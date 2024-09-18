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
        try:
            alatt, cov, info, msg, ier = leastsq(fobj, x0=alatt0, full_output=True)
            # eval the fobj at optimal solution to set UB (leastsq iteration stops at a next sub-optimal solution)
            fobj(alatt)
        except ValueError:
            logger.error("CalculateUMatrix failed - check initial lattice parameters and tolerance provided.")
            return

        success = ier in [1, 2, 3, 4] and cov is not None  # cov is None when matrix is singular
        if success:
            # calculate errors
            dof = sum([self.child_IndexPeaks(ws, RoundHKLs=True) for ws in ws_list]) - len(alatt0)
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
            (iws, self.child_IndexPeaks(ws))
            for iws, ws in enumerate(ws_list)
            if AnalysisDataService.retrieve(ws).sample().hasOrientedLattice()
        ]  # [(iws, n_peaks_indexed),...]
        if ws_with_UB:
            iref, nindexed = max(ws_with_UB, key=lambda x: x[1])  # get UB which indexes most peaks
            foundUB = nindexed >= _MIN_NUM_INDEXED_PEAKS
        if not foundUB:
            # loop over all ws and try to find a UB
            for iws, ws in enumerate(ws_list):
                try:
                    self.child_FindUBUsingLatticeParameters(
                        PeaksWorkspace=ws, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma, FixParameters=False
                    )
                    nindexed = self.child_IndexPeaks(PeaksWorkspace=ws, RoundHKLs=True)
                    foundUB = nindexed >= _MIN_NUM_INDEXED_PEAKS
                except ValueError:
                    pass
                if foundUB:
                    iref = iws
                    break  # stop once found one UB found as FindUBUsingLatticeParameter takes a long time
        if not foundUB:
            raise RuntimeError("An initial UB could not be found with the provided lattice parameters")

        # index other runs consistent with reference UB
        iws_with_valid_UB = [iw for iw, npks in ws_with_UB if npks >= _MIN_NUM_INDEXED_PEAKS]
        iws_unindexed = list(range(len(ws_list)))
        iws_unindexed.pop(iref)
        # loop over copy of iws_unindexed so can remove item from it when indexed a ws
        for iws in iws_unindexed[:]:
            if iws in iws_with_valid_UB:
                self.make_UB_consistent(ws_list[iref], ws_list[iws])
                foundUB = True  # already know npks >= _MIN_NUM_INDEXED_PEAKS
            else:
                # copy orientation from sample so as to ensure consistency of indexing
                self.child_CopySample(InputWorkspace=ws_list[iref], OutputWorkspace=ws_list[iws])
                nindexed = self.child_IndexPeaks(PeaksWorkspace=ws_list[iws], RoundHKLs=True)
                if nindexed < _MIN_NUM_INDEXED_PEAKS:
                    # if gonio matrix is inaccurate we have to find the UB from scratch and transform to correct HKL
                    self.child_FindUBUsingLatticeParameters(
                        PeaksWorkspace=ws_list[iws], a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma, FixParameters=False
                    )
                    self.make_UB_consistent(ws_list[iref], ws_list[iws])
                    nindexed = self.child_IndexPeaks(PeaksWorkspace=ws_list[iws], RoundHKLs=True)
                    foundUB = nindexed >= _MIN_NUM_INDEXED_PEAKS
            if foundUB:
                iws_unindexed.remove(iws)

        # remove unindexed runs from workspaces
        for iws in iws_unindexed:
            ws = ws_list.pop(iws)
            logger.warning(f"Consistent UB not found for {ws} - this workspace will be ignored.")

    def make_UB_consistent(self, ws_ref, ws):
        # compare U matrix to perform TransformHKL to preserve indexing
        U_ref = AnalysisDataService.retrieve(ws_ref).sample().getOrientedLattice().getU()
        U = AnalysisDataService.retrieve(ws).sample().getOrientedLattice().getU()
        # find transform required  ( U_ref = U T^-1) - see TransformHKL docs for details
        transform = np.linalg.inv(getSignMaxAbsValInCol(np.linalg.inv(U) @ U_ref))
        self.child_TransformHKL(PeaksWorkspace=ws, HKLTransform=transform, FindError=False)

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
            nindexed = self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=True, CommonUBForAll=False)
            if nindexed >= _MIN_NUM_INDEXED_PEAKS:
                self.child_CalculateUMatrix(wsname, *x0)
                # don't index with optimisation after this point and don't round HKL (to calc resids)
                self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=False, CommonUBForAll=True)
                ws = AnalysisDataService.retrieve(wsname)
                UB = 2 * np.pi * ws.sample().getOrientedLattice().getUB()
                for ii in range(ws.getNumberPeaks()):
                    pk = ws.getPeak(ii)
                    if pk.getHKL().norm2() > 1e-6:
                        residsq[ipk] = np.sum((UB @ pk.getIntHKL() - pk.getQSampleFrame()) ** 2)
                        ipk += 1
        return np.sqrt(residsq / (ipk + 1))

    def child_IndexPeaks(self, PeaksWorkspace, RoundHKLs=True, CommonUBForAll=False):
        alg = self.createChildAlgorithm("IndexPeaks", enableLogging=False)
        alg.setProperty("PeaksWorkspace", PeaksWorkspace)
        alg.setProperty("Tolerance", self.tol)
        alg.setProperty("RoundHKLs", RoundHKLs)
        alg.setProperty("CommonUBForAll", CommonUBForAll)  # if False then optimises a temp UB before indexing
        alg.execute()
        return alg.getProperty("NumIndexed").value

    def child_FindUBUsingLatticeParameters(self, PeaksWorkspace, a, b, c, alpha, beta, gamma, FixParameters=False):
        alg = self.createChildAlgorithm("FindUBUsingLatticeParameters", enableLogging=False)
        alg.setProperty("PeaksWorkspace", PeaksWorkspace)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha)
        alg.setProperty("beta", beta)
        alg.setProperty("gamma", gamma)
        alg.setProperty("FixParameters", FixParameters)
        alg.execute()

    def child_CalculateUMatrix(self, PeaksWorkspace, a, b, c, alpha, beta, gamma):
        alg = self.createChildAlgorithm("CalculateUMatrix", enableLogging=False)
        alg.setProperty("PeaksWorkspace", PeaksWorkspace)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha)
        alg.setProperty("beta", beta)
        alg.setProperty("gamma", gamma)
        alg.execute()

    def child_TransformHKL(self, PeaksWorkspace, HKLTransform, FindError=False):
        alg = self.createChildAlgorithm("TransformHKL", enableLogging=False)
        alg.setProperty("PeaksWorkspace", PeaksWorkspace)
        alg.setProperty("HKLTransform", HKLTransform)
        alg.setProperty("FindError", FindError)
        alg.execute()

    def child_CopySample(
        self,
        InputWorkspace,
        OutputWorkspace,
        CopyName=False,
        CopyMaterial=False,
        CopyEnvironment=False,
        CopyShape=False,
        CopyOrientationOnly=True,
    ):
        alg = self.createChildAlgorithm("CopySample", enableLogging=False)
        alg.setProperty("InputWorkspace", InputWorkspace)
        alg.setProperty("OutputWorkspace", OutputWorkspace)
        alg.setProperty("CopyName", CopyName)
        alg.setProperty("CopyMaterial", CopyMaterial)
        alg.setProperty("CopyEnvironment", CopyEnvironment)
        alg.setProperty("CopyShape", CopyShape)
        alg.setProperty("CopyOrientationOnly", CopyOrientationOnly)
        alg.execute()


# register algorithm with mantid
AlgorithmFactory.subscribe(FindGlobalBMatrix)
