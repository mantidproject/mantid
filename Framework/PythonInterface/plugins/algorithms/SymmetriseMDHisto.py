# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, IMDHistoWorkspaceProperty, PythonAlgorithm, Progress
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion
from mantid.geometry import SpaceGroupFactory, PointGroupFactory
import numpy as np


class SymmetriseMDHisto(PythonAlgorithm):
    def category(self):
        return "Crystal\\DataHandling"

    def summary(self):
        return "Symmetrise MDHistoWorkspace using symmetry operations of Laue class point group."

    def PyInit(self):
        """Initilize the algorithms properties"""

        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", Direction.Input),
            doc="Input MDHistoWorkspace to symmetrise.",
        )

        self.declareProperty(
            IMDHistoWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            doc="Symmetrised MDHistoWorkspace.",
        )

        self.declareProperty(
            name="Pointgroup",
            defaultValue="",
            direction=Direction.Input,
            doc="Point group Hermann–Mauguin symbol used to determine the point group of the Laue class.",
        )

        self.declareProperty(
            name="Spacegroup",
            defaultValue="",
            direction=Direction.Input,
            doc="Spacegroup Hermann–Mauguin symbol used to determine the point group of the Laue class.",
        )

        self.declareProperty(
            name="WeightedAverage",
            defaultValue=False,
            direction=Direction.Input,
            doc="Perform average weighted by the error squared.",
        )

        enable_spacegroup = EnabledWhenProperty("Pointgroup", PropertyCriterion.IsDefault)
        self.setPropertySettings("Spacegroup", enable_spacegroup)
        enable_pointgroup = EnabledWhenProperty("Spacegroup", PropertyCriterion.IsDefault)
        self.setPropertySettings("Pointgroup", enable_pointgroup)

    def validateInputs(self):
        issues = dict()
        # check point group of laue class can be retrieved
        spgr_sym = self.getProperty("Spacegroup").value
        ptgr_sym = self.getProperty("Pointgroup").value
        if spgr_sym and not ptgr_sym:
            if not SpaceGroupFactory.isSubscribedSymbol(spgr_sym):
                issues["Spacegroup"] = "Not a valid spacegroup symbol."
        elif ptgr_sym and not spgr_sym:
            if not PointGroupFactory.isSubscribed(ptgr_sym):
                issues["Pointgroup"] = "Not a valid spacegroup symbol."
        else:
            issues["Spacegroup"] = "Please only provide one of Spacegroup or Pointgroup."
        # check workspace has same extent and binning along all axes
        ws = self.getProperty("InputWorkspace").value
        # check nDims=3
        ndims = ws.getNumDims()
        if ndims != 3:
            issues["InputWorkspace"] = "Workspace must have 3 dimensions."
        lo, hi, nbins = _get_dim_extents_and_nbins(ws, 0)
        if not np.isclose(lo, -hi):
            issues["InputWorkspace"] = "Workspace must have dimensions centered on 0 (i.e. min = -max)."
        for idim in range(1, ndims):
            if not all(np.isclose(_get_dim_extents_and_nbins(ws, idim), [lo, hi, nbins])):
                issues["InputWorkspace"] = "Workspace must have same binning along all dimensions."
        # check errors exist if WeightedAverage==True (would produce all NaNs in signal)
        if self.getProperty("WeightedAverage").value:
            if not ws.getErrorSquaredArray().any():
                issues["WeightedAverage"] = "Cannot perform weighted average on data with no errors."
        return issues

    def PyExec(self):
        """Execute the algorithm"""
        ws = self.getProperty("InputWorkspace").value
        spgr_sym = self.getProperty("Spacegroup").value
        ptgr_sym = self.getProperty("Pointgroup").value
        weighted_average = self.getProperty("WeightedAverage").value

        # get point group of Laue class
        if spgr_sym:
            spgr = SpaceGroupFactory.createSpaceGroup(spgr_sym)
            ptgr = PointGroupFactory.createPointGroupFromSpaceGroup(spgr)
            laue_ptgr = PointGroupFactory.createPointGroup(ptgr.getLauePointGroupSymbol())
        else:
            ptgr = PointGroupFactory.createPointGroup(ptgr_sym)
            laue_ptgr = PointGroupFactory.createPointGroup(ptgr.getLauePointGroupSymbol())

        # setup data array (note symmetry operations include identity so start sums from 0)
        nbins = ws.getDimension(0).getNBins()
        signal = np.zeros(3 * [nbins])
        weights = np.zeros(signal.shape)
        if not weighted_average:
            error_sq = np.zeros(signal.shape)

        bin_index = np.arange(signal.size).reshape(signal.shape)
        bin_at_zero = nbins % 2  # odd number of bins implies bin center at 0 along each axis
        sym_ops = laue_ptgr.getSymmetryOperations()
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=len(sym_ops))
        for sym_op in sym_ops:
            prog_reporter.report("Symmetrising")
            transformed = sym_op.transformHKL([1, 2, 3])
            axes = [int(abs(iax)) - 1 for iax in transformed]
            iflip = np.flatnonzero(np.asarray(transformed) < 0)
            signal_tmp = np.flip(np.transpose(ws.getSignalArray().copy(), axes=axes), axis=iflip)
            error_sq_tmp = np.flip(np.transpose(ws.getErrorSquaredArray().copy(), axes=axes), axis=iflip)
            if sym_op.getIdentifier() != "x,y,z" and bin_at_zero:
                # zero bins invariant to symmetry operation (if not identity operation)
                transformed_bin_index = np.flip(np.transpose(bin_index, axes=axes), axis=iflip)
                i_invariant = transformed_bin_index == bin_index
                signal_tmp[i_invariant] = 0
                error_sq_tmp[i_invariant] = 0
            if weighted_average:
                with np.errstate(divide="ignore", invalid="ignore"):
                    weights_tmp = 1.0 / error_sq_tmp
                weights_tmp[~np.isfinite(weights_tmp)] = 0
                signal_tmp *= weights_tmp
            else:
                error_sq += error_sq_tmp
                weights_tmp = (abs(signal_tmp) > 1e-10).astype(int)  # number of non-empty bins
            signal += signal_tmp
            weights += weights_tmp

        # normalise signal and errors by weights
        inonzero = abs(weights) > 0
        signal[inonzero] = signal[inonzero] / weights[inonzero]
        if weighted_average:
            # in this case the weights = sum_i[1/error_i^2]
            with np.errstate(divide="ignore", invalid="ignore"):
                error_sq = 1.0 / weights
            error_sq[~np.isfinite(error_sq)] = 0
        else:
            # weights is the number of bins that contributed to each bin in the symmetrised sum - i.e. average is a mean
            error_sq[inonzero] = error_sq[inonzero] / weights[inonzero]

        # set data on workspace
        ws_out = self.child_alg("CloneWorkspace", InputWorkspace=ws)
        ws_out.setSignalArray(signal)
        ws_out.setErrorSquaredArray(error_sq)

        # assign output
        self.setProperty("OutputWorkspace", ws_out)

    def child_alg(self, alg_name, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False, StoreInADS=False)
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


def _get_dim_extents_and_nbins(ws, idim):
    dim = ws.getDimension(idim)
    return dim.getMinimum(), dim.getMaximum(), dim.getNBins()


AlgorithmFactory.subscribe(SymmetriseMDHisto)
