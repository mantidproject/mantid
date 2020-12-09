# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, IMDEventWorkspaceProperty, IMDHistoWorkspaceProperty, IPeaksWorkspaceProperty,\
    PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, EnabledWhenProperty, FloatArrayProperty, PropertyCriterion, IntArrayProperty
from mantid.simpleapi import BinMD, DeleteWorkspace, SetMDFrame, TransformHKL, mtd


class ConvertQtoHKLHisto(PythonAlgorithm):

    _lattice = None

    def category(self):
        return "Crystal\\Creation"

    def seeAlso(self):
        return ["HB3AFindPeaks", "IntegratePeaksMD"]

    def name(self):
        return "ConvertQtoHKLHisto"

    def summary(self):
        return 'Converts a workspace from Q sample to a MDHisto in HKL; the UB matrix can be used from the input, ' \
               'or calculated from peaks found from the input.'

    def PyInit(self):
        self.declareProperty(IMDEventWorkspaceProperty("InputWorkspace", defaultValue="",
                                                       optional=PropertyMode.Mandatory,
                                                       direction=Direction.Input),
                             doc="Input MDEvent workspace to convert to a MDHisto in HKL")

        self.declareProperty("FindUBFromPeaks", True,
                             doc="Whether to find peaks and use them to compute the UB matrix.")

        self.declareProperty(IPeaksWorkspaceProperty("PeaksWorkspace", defaultValue="", optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc="Peaks workspace containing peaks to integrate")

        self.declareProperty(FloatArrayProperty("Transformation", [],
                                                direction=Direction.Input),
                             "Optional HKL transformation matrix to apply to the peaks workspace, each value of the "
                             "matrix should be comma separated.")

        ub_settings = EnabledWhenProperty('FindUBFromPeaks', PropertyCriterion.IsDefault)
        self.setPropertySettings("PeaksWorkspace", ub_settings)
        self.setPropertySettings("Transformation", ub_settings)

        self.declareProperty(FloatArrayProperty("Extents", [-6.02, 6.02, -6.02, 6.02, -6.02, 6.02],
                                                direction=Direction.Input),
                             "Binning parameters for each dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,'.")

        self.declareProperty(IntArrayProperty("Bins", [301, 301, 301],
                                              direction=Direction.Input),
                             "Number of bins to use for each dimension, Enter it as a"
                             "comma-separated list of integers.")

        self.declareProperty(IMDHistoWorkspaceProperty("OutputWorkspace", "",
                                                       optional=PropertyMode.Mandatory,
                                                       direction=Direction.Output),
                             doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.")

    def validateInputs(self):
        issues = dict()

        input_ws = self.getProperty("InputWorkspace").value

        if input_ws.getSpecialCoordinateSystem().name != "QSample":
            issues["InputWorkspace"] = "Input workspace expected to be in QSample, " \
                                       "workspace is in '{}'".format(input_ws.getSpecialCoordinateSystem().name)

        ndims = input_ws.getNumDims()
        if ndims != 3:
            issues["InputWorkspace"] = "Input workspace needs 3 dimensions, it has {} dimensions.".format(ndims)

        # Check that extents and bins were provided for every dimension
        extents = self.getProperty("Extents").value
        bins = self.getProperty("Bins").value

        if len(extents) != ndims * 2:
            issues["Extents"] = "Expected a min and max value for each " \
                                "dimension (got {}, expected {}).".format(len(extents), ndims*2)

        if len(bins) != ndims:
            issues["Bins"] = "Expected a number of bins for each dimension."

        if self.getProperty("FindUBFromPeaks").value:
            peak_ws = self.getProperty("PeaksWorkspace")
            if peak_ws.isDefault:
                issues["PeaksWorkspace"] = "A peaks workspace must be supplied."
            elif not mtd.doesExist(self.getPropertyValue("PeaksWorkspace")):
                issues["PeaksWorkspace"] = "Provided peaks workspace does not exist in the ADS."

            tmatrix = self.getProperty("Transformation")
            if not tmatrix.isDefault and len(tmatrix.value) != 9:
                issues["Transformation"] = "Invalid transformation matrix, there should be 9 values comma separated"
        else:
            # Check that the workspace has a UB matrix
            if not (input_ws.getNumExperimentInfo() > 0 and input_ws.getExperimentInfo(0).sample().hasOrientedLattice()):
                issues["InputWorkspace"] = "Could not find a UB matrix in this workspace."

        return issues

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value

        extents = self.getProperty("Extents").value
        bins = self.getProperty("Bins").value

        if self.getProperty("FindUBFromPeaks").value:
            peak_ws = self.getProperty("PeaksWorkspace").value

            if not self.getProperty("Transformation").isDefault:
                tmatrix = self.getProperty("Transformation").value

                # Apply transformation if specified
                TransformHKL(PeaksWorkspace=peak_ws, HKLTransform=tmatrix)

            self._lattice = peak_ws.sample().getOrientedLattice()
        else:
            self._lattice = input_ws.getExperimentInfo(0).sample().getOrientedLattice()

        q1 = self._lattice.qFromHKL([1, 0, 0])
        q2 = self._lattice.qFromHKL([0, 1, 0])
        q3 = self._lattice.qFromHKL([0, 0, 1])

        mdhist = BinMD(InputWorkspace=input_ws, AxisAligned=False, NormalizeBasisVectors=False,
                       BasisVector0='H,A^-1,{},{},{}'.format(q1.X(), q1.Y(), q1.Z()),
                       BasisVector1='K,A^-1,{},{},{}'.format(q2.X(), q2.Y(), q2.Z()),
                       BasisVector2='L,A^-1,{},{},{}'.format(q3.X(), q3.Y(), q3.Z()),
                       OutputExtents=extents,
                       OutputBins=bins)

        SetMDFrame(mdhist, MDFrame='HKL', Axes='0, 1, 2')

        self.setProperty("OutputWorkspace", mdhist)

        DeleteWorkspace(mdhist)


AlgorithmFactory.subscribe(ConvertQtoHKLHisto)
