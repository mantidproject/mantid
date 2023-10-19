# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, IMDHistoWorkspaceProperty, PythonAlgorithm
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
            if not PointGroupFactory.isSubscribedSymbol(ptgr_sym):
                issues["Spacegroup"] = "Not a valid spacegroup symbol."
        else:
            issues["Spacegroup"] = "Please only provide one of Spacegroup or Pointgroup."
        # check workspace has same extent and binning along all axes
        ws = self.getProperty("InputWorkspace").value
        lo, hi, nbins = _get_dim_extents_and_nbins(ws, 0)
        if not np.isclose(lo, -hi):
            issues["InputWorkspace"] = "Workspace must have have dimensions centered on 0 (i.e. min = -max)."
        for idim in range(1, ws.getNumDims()):
            if not all(np.isclose(_get_dim_extents_and_nbins(ws, idim), [lo, hi, nbins])):
                issues["InputWorkspace"] = "Workspace must have same binning along all dimensions."

        return issues

    def PyExec(self):
        """Execute the algorithm"""
        ws = self.getProperty("InputWorkspace").value
        spgr_sym = self.getProperty("Spacegroup").value
        ptgr_sym = self.getProperty("Pointgroup").value

        # get point group of Laue class
        if spgr_sym:
            spgr = SpaceGroupFactory.createSpaceGroup(spgr_sym)
            ptgr = PointGroupFactory.createPointGroupFromSpaceGroup(spgr)
            laue_ptgr = PointGroupFactory.createPointGroup(ptgr.getLauePointGroupSymbol())
        else:
            ptgr = PointGroupFactory.createPointGroup(ptgr_sym)
            laue_ptgr = PointGroupFactory.createPointGroup(ptgr.getLauePointGroupSymbol())

        # symmetrise data
        signal = ws.getSignalArray().copy()
        npix = np.zeros(signal.shape, dtype=int)
        inonzero = abs(signal) > 1e-10
        npix[inonzero] = 1

        for sym_op in laue_ptgr.getSymmetryOperations():
            transformed = sym_op.transformHKL([1, 2, 3])
            ws_out = self.child_alg("PermuteMD", InputWorkspace=ws, Axes=[int(abs(iax)) - 1 for iax in transformed])
            # could call TransformMD with Scaling=np.sign(transformed) but it's actually slower!
            signs = np.sign(transformed)
            signal_tmp = ws_out.getSignalArray().copy()[:: int(signs[0]), :: int(signs[1]), :: int(signs[2])]
            npix_tmp = abs(signal_tmp) > 1e-10
            signal += signal_tmp
            npix += npix_tmp

        # set symmetrised signal in ws
        inonzero = abs(npix) > 0
        signal[inonzero] = signal[inonzero] / npix[inonzero]

        ws_out.setSignalArray(signal)

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
