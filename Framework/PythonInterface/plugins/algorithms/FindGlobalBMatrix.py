# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, Progress, ADSValidator, IPeaksWorkspace)
from mantid.simpleapi import (AnalysisDataService, logger)
from mantid.kernel import (Direction, FloatBoundedValidator, StringArrayProperty)
import numpy as np
from scipy.optimize import leastsq
from FindGoniometerFromUB import getSignMaxAbsValInCol

_MIN_NUM_PEAKS = 6  # minimum required to use FindUBUsingLatticeParameters assuming all linearly indep.
_MIN_NUM_INDEXED_PEAKS = 2  # minimum indexed peaks required for CalculateUMatrix


class FindGlobalBMatrix(DataProcessorAlgorithm):

    def name(self):
        return "FindGlobalBMatrix"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["FindUBUsingLatticeParameters", "CalculateUMatrix"]

    def summary(self):
        return "Takes multiple peak tables from different runs and refines common lattice parameters and angles."

    def PyInit(self):
        # Input
        self.declareProperty(
            StringArrayProperty(name="PeakWorkspaces",
                                direction=Direction.Input, validator=ADSValidator()),
            doc='List of peak workspaces to use.')
        positiveFloatValidator = FloatBoundedValidator(lower=0.0)
        angleValidator = FloatBoundedValidator(lower=0.0, upper=180.0)
        self.declareProperty(name="a", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=positiveFloatValidator,
                             doc="Lattice parameter a")
        self.declareProperty(name="b", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=positiveFloatValidator,
                             doc="Lattice parameter b")
        self.declareProperty(name="c", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=positiveFloatValidator,
                             doc="Lattice parameter c")
        self.declareProperty(name="alpha", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=angleValidator,
                             doc="Lattice angle alpha")
        self.declareProperty(name="beta", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=angleValidator,
                             doc="Lattice angle beta")
        self.declareProperty(name="gamma", defaultValue=-1.0,
                             direction=Direction.Input,
                             validator=angleValidator,
                             doc="Lattice angle gamma")

    def validateInputs(self):
        issues = dict()
        ws_list = self.getProperty("PeakWorkspaces").value
        n_valid_ws = 0
        for wsname in ws_list:
            ws = AnalysisDataService.retrieve(wsname)
            if isinstance(ws, IPeaksWorkspace) and ws.getNumberPeaks() > _MIN_NUM_PEAKS:
                n_valid_ws += 1
        if n_valid_ws < 2 or n_valid_ws < len(ws_list):
            issues["PeakWorkspaces"] = "Accept only peaks workspace with more than 6 peaks - " \
                                       "there must be at least two peak tables provided in total."
        return issues

    def PyExec(self):

        def fobj(alatt):
            return self.fobj_lstsq(alatt, valid_ws_list)

        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=3)
        # Get input
        ws_list = self.getProperty("PeakWorkspaces").value
        a = self.getProperty('a').value
        b = self.getProperty('b').value
        c = self.getProperty('c').value
        alpha = self.getProperty('alpha').value
        beta = self.getProperty('beta').value
        gamma = self.getProperty('gamma').value

        # Find initial UB and use to index peaks in all runs
        prog_reporter.report(1, "Find initial UB for peak indexing")
        w_ref = AnalysisDataService.retrieve(ws_list[0])
        nindexed = 0
        if w_ref.sample().hasOrientedLattice():
            # try use existing UB to index peaks
            nindexed = self.child_IndexPeaks(PeaksWorkspace=ws_list[0], RoundHKLs=True)
        if nindexed < _MIN_NUM_INDEXED_PEAKS:
            # find a better intitial UB
            self.child_FindUBUsingLatticeParameters(PeaksWorkspace=ws_list[0], a=a, b=b, c=c,
                                                    alpha=alpha, beta=beta, gamma=gamma, FixParameters=False)
            nindexed = self.child_IndexPeaks(PeaksWorkspace=ws_list[0], RoundHKLs=True)
        if nindexed < _MIN_NUM_INDEXED_PEAKS:
            # can't get good initial UB
            logger.error(f"Could not refine an initla UB from workspace {ws_list[0]}")
            return
        # what if indexes less than 2 peaks - it won't
        valid_ws_list = [ws_list[0]]
        for iws, wsname in enumerate(ws_list[1:]):
            # copy UB and index
            self.child_CopySample(InputWorkspace=ws_list[0], OutputWorkspace=wsname)
            nindexed = self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=True)
            if nindexed < _MIN_NUM_INDEXED_PEAKS:
                # assume gonio matrix is bad - find UB from scratch and transform to preserve indexing
                self.child_FindUBUsingLatticeParameters(PeaksWorkspace=wsname, a=a, b=b, c=c,
                                                        alpha=alpha, beta=beta, gamma=gamma, FixParameters=False)
                # compare U matrix to perform TransformHKL to preserve indexing
                w = AnalysisDataService.retrieve(wsname)
                U_ref = w_ref.sample().getOrientedLattice().getU()
                U = w.sample().getOrientedLattice().getU()
                # find transform required ( U_ref = T U)
                transform = getSignMaxAbsValInCol(np.linalg.inv(U) @ U_ref)
                self.child_TransformHKL(PeaksWorkspace=wsname, HKLTransform=transform, FindError=False)
                nindexed = self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=True)
            if nindexed < _MIN_NUM_INDEXED_PEAKS:
                logger.warning(f"Consistent UB not found for {wsname} - this workspace will be ignored.")
            else:
                valid_ws_list.append(wsname)

        # optimize the lattice parameters (i.e. B matrix)
        prog_reporter.report(2, "Optimize B")
        alatt0 = [a, b, c, alpha, beta, gamma]  # should get this from ws_list[0] instead
        alatt, *_, msg, ier = leastsq(fobj, x0=alatt0, full_output=True)

        success = ier in [1, 2, 3, 4]
        if success:
            logger.notice(f"Lattice parameters successfully refined for workspaces: {valid_ws_list}\n"
                          f"Lattice parameters [a, b, c, alpha, beta, gamma] = {alatt}")
        else:
            logger.warning(f"Error in optimization of lattice parameters: {msg}")
        # complete progress
        prog_reporter.report(3, "Done")

    def fobj_lstsq(self, x0, ws_list):
        """
        Calulates sum of square magnitude of difference between qsample and q of integer HKL
        x0 = [a, b, c, alpha, beta, gamma]
        """
        residsq = np.zeros(sum([AnalysisDataService.retrieve(wsname).getNumberPeaks() for wsname in ws_list]))
        ipk = 0  # normalise by n peaks indexed so no penalty in indexing more peaks
        for wsname in ws_list:
            nindexed = self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=True)
            if nindexed > _MIN_NUM_INDEXED_PEAKS:
                self.child_CalculateUMatrix(wsname, *x0)
                self.child_IndexPeaks(PeaksWorkspace=wsname, RoundHKLs=False)
                ws = AnalysisDataService.retrieve(wsname)
                UB = 2 * np.pi * ws.sample().getOrientedLattice().getUB()
                for ii in range(ws.getNumberPeaks()):
                    pk = ws.getPeak(ii)
                    if pk.getHKL().norm2() > 1e-6:
                        residsq[ipk] += (np.sum((UB @ pk.getIntHKL() - pk.getQSampleFrame()) ** 2))
                        ipk += 1
        return np.sqrt(residsq / (ipk + 1))

    def child_IndexPeaks(self, PeaksWorkspace, RoundHKLs):
        alg = self.createChildAlgorithm("IndexPeaks", enableLogging=False)
        alg.setProperty("PeaksWorkspace", PeaksWorkspace)
        alg.setProperty("RoundHKLs", RoundHKLs)
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

    def child_CopySample(self, InputWorkspace, OutputWorkspace, CopyName=False, CopyMaterial=False,
                         CopyEnvironment=False, CopyShape=False, CopyOrientationOnly=True):
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
