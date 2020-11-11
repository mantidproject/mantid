# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, IMDWorkspaceProperty, IPeaksWorkspaceProperty, PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion, StringListValidator
from mantid.simpleapi import DeleteWorkspace, FindPeaksMD, FindUBUsingIndexedPeaks, \
    FindUBUsingLatticeParameters, IndexPeaks, ShowPossibleCells, SelectCellOfType, OptimizeLatticeForCellType


class SCDFindPeaks(PythonAlgorithm):

    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["FindPeaksMD"]

    def name(self):
        return "SCDFindPeaks"

    def summary(self):
        return 'Given a MD workspace in Q-space, calculates the UB matrix and finds peaks; the UB matrix can be ' \
               'optionally refined using lattice parameters.'

    def PyInit(self):
        self.declareProperty(IMDWorkspaceProperty("InputWorkspace", defaultValue="", optional=PropertyMode.Mandatory,
                                                  direction=Direction.Input),
                             doc="Input MD workspace (in Q-space) to use for peak finding")

        cell_type = StringListValidator()
        cell_type.addAllowedValue("Cubic")
        cell_type.addAllowedValue("Hexagonal")
        cell_type.addAllowedValue("Rhombohedral")
        cell_type.addAllowedValue("Tetragonal")
        cell_type.addAllowedValue("Orthorhombic")
        cell_type.addAllowedValue("Monoclinic")
        cell_type.addAllowedValue("Triclinic")
        self.declareProperty("CellType", "", cell_type, doc="Cell type to use for UB refining")

        centering_type = StringListValidator()
        centering_type.addAllowedValue("F")
        centering_type.addAllowedValue("I")
        centering_type.addAllowedValue("C")
        centering_type.addAllowedValue("P")
        centering_type.addAllowedValue("R")
        self.declareProperty("Centering", "P", centering_type, doc="Centering to use for selecting cells")

        # Lattice parameter validators from same as FindUBUsingLatticeParameters
        self.declareProperty("UseLattice", False, direction=Direction.Input,
                             doc="Whether to refine UB matrix based on given lattice parameters")

        self.declareProperty("a", "", doc="The a value of the lattice")
        self.declareProperty("b", "", doc="The b value of the lattice")
        self.declareProperty("c", "", doc="The c value of the lattice")
        self.declareProperty("alpha", "", doc="The alpha value of the lattice")
        self.declareProperty("beta", "", doc="The beta value of the lattice")
        self.declareProperty("gamma", "", doc="The gamma value of the lattice")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output,
                                                     optional=PropertyMode.Mandatory), doc="Output peaks workspace")

        lattice_params = ["a", "b", "c", "alpha", "beta", "gamma"]
        for param in lattice_params:
            self.setPropertyGroup(param, "Lattice Settings")
            self.setPropertySettings(param, EnabledWhenProperty("UseLattice", PropertyCriterion.IsNotDefault))

    def validateInputs(self):
        issues = dict()

        lattice_params = ["a", "b", "c", "alpha", "beta", "gamma"]
        set_count = 0
        for param in lattice_params:
            prop = self.getProperty(param)
            if not prop.isDefault:
                if prop.value <= 0.0:
                    issues[param] = "Lattice parameter must be greater than 0"
                if param == "alpha" or param == "beta" or param == "gamma":
                    if prop.value < 5.0 or prop.value > 175.0:
                        issues[param] = "Lattice parameter must have an angle between 5 and 175."
                set_count += 1

        if 0 <= set_count < 6 and not self.getProperty("UseLattice").isDefault:
            for param in lattice_params:
                issues[param] = "Must set all or none of the optional lattice parameters."

        if self.getProperty("InputWorkspace").value.getNumDims() != 3:
            issues["InputWorkspace"] = "Workspace has the wrong number of dimensions"

        return issues

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        cell_type = self.getProperty("CellType").value
        centering = self.getProperty("Centering").value

        lattice = self.getProperty("UseLattice").value
        if lattice:
            a = self.getProperty("a").value
            b = self.getProperty("b").value
            c = self.getProperty("c").value
            alpha = self.getProperty("alpha").value
            beta = self.getProperty("beta").value
            gamma = self.getProperty("gamma").value

        peak_ws = self.getProperty("OutputWorkspace")

        wavelength = 1.008
        exp_info = input_ws.getExperimentInfo(0)
        if exp_info.run().hasProperty("wavelength"):
            wavelength = exp_info.run().getProperty("wavelength").value

        peak_ws = FindPeaksMD(InputWorkspace=input_ws, PeakDistanceThreshold=0.25, CalculateGoniometerForCW=True,
                              Wavelength=wavelength, FlipX=True, InnerGoniometer=True)

        IndexPeaks(PeaksWorkspace=peak_ws)

        if lattice:
            FindUBUsingLatticeParameters(PeaksWorkspace=peak_ws, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        else:
            FindUBUsingIndexedPeaks(PeaksWorkspace=peak_ws)

        ShowPossibleCells(PeaksWorkspace=peak_ws)
        SelectCellOfType(PeaksWorkspace=peak_ws, CellType=cell_type, Centering=centering, Apply=True)
        OptimizeLatticeForCellType(PeaksWorkspace=peak_ws, CellType=cell_type, Apply=True)

        self.setProperty("OutputWorkspace", peak_ws)

        DeleteWorkspace(peak_ws)


AlgorithmFactory.subscribe(SCDFindPeaks)
