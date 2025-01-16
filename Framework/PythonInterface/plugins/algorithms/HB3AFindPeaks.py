# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    WorkspaceProperty,
    PythonAlgorithm,
    PropertyMode,
    ADSValidator,
    WorkspaceGroup,
    MultipleExperimentInfos,
)
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatPropertyWithValue,
    PropertyCriterion,
    StringListValidator,
    StringArrayProperty,
)
from mantid.simpleapi import (
    DeleteWorkspace,
    FindPeaksMD,
    FindUBUsingFFT,
    FindUBUsingLatticeParameters,
    IndexPeaks,
    ShowPossibleCells,
    SelectCellOfType,
    OptimizeLatticeForCellType,
    AnalysisDataService,
    CreatePeaksWorkspace,
    CombinePeaksWorkspaces,
    CopySample,
    GroupWorkspaces,
)
from mantid.dataobjects import MDHistoWorkspace
import numpy as np


class HB3AFindPeaks(PythonAlgorithm):
    def category(self):
        return "Crystal\\Peaks;Crystal\\UBMatrix"

    def seeAlso(self):
        return [
            "FindPeaksMD",
            "OptimizeLatticeForCellType",
            "FindUBUsingFFT",
            "SelectCellOfType",
            "IndexPeaks",
            "HB3AAdjustSampleNorm",
            "HB3APredictPeaks",
            "HB3AIntegratePeaks",
        ]

    def name(self):
        return "HB3AFindPeaks"

    def summary(self):
        return (
            "Given a MD workspace in Q-space, calculates the UB matrix and finds peaks; the UB matrix can be "
            "optionally refined using lattice parameters."
        )

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspace", direction=Direction.Input, validator=ADSValidator()),
            doc="Input MD workspace (in Q-space) to use for peak finding",
        )
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

        # Some of the options to pass through to FindPeaksMD (options like CalculateGoniometerForCW, FlipX, etc are
        # assumed to be True)
        self.declareProperty("MaxPeaks", 1000, doc="Maximum number of peaks to find.")
        self.declareProperty(
            "PeakDistanceThreshold", 0.25, doc="Threshold distance for rejecting peaks that are found to be too close from each other"
        )
        # Having this number too low will likely cause FindUBUsingFFT to fail since not enough peaks will be found
        self.declareProperty(
            "DensityThresholdFactor",
            2000.0,
            doc="Scaling factor which the overall signal density will be multiplied by to determine a threshold for determining peaks.",
        )

        self.declareProperty(
            "Wavelength", FloatPropertyWithValue.EMPTY_DBL, doc="Wavelength value to use only if one was not found in the sample log"
        )

        # Lattice parameter validators from same as FindUBUsingLatticeParameters
        self.declareProperty(
            "UseLattice", False, direction=Direction.Input, doc="Whether to refine UB matrix based on given lattice parameters"
        )

        self.declareProperty("LatticeA", FloatPropertyWithValue.EMPTY_DBL, doc="The a value of the lattice")
        self.declareProperty("LatticeB", FloatPropertyWithValue.EMPTY_DBL, doc="The b value of the lattice")
        self.declareProperty("LatticeC", FloatPropertyWithValue.EMPTY_DBL, doc="The c value of the lattice")
        self.declareProperty("LatticeAlpha", FloatPropertyWithValue.EMPTY_DBL, doc="The alpha value of the lattice")
        self.declareProperty("LatticeBeta", FloatPropertyWithValue.EMPTY_DBL, doc="The beta value of the lattice")
        self.declareProperty("LatticeGamma", FloatPropertyWithValue.EMPTY_DBL, doc="The gamma value of the lattice")

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output, optional=PropertyMode.Mandatory),
            doc="Output peaks workspace",
        )

        lattice_params = ["LatticeA", "LatticeB", "LatticeC", "LatticeAlpha", "LatticeBeta", "LatticeGamma"]
        for param in lattice_params:
            self.setPropertyGroup(param, "Lattice Settings")
            self.setPropertySettings(param, EnabledWhenProperty("UseLattice", PropertyCriterion.IsNotDefault))

    def validateInputs(self):
        issues = dict()

        lattice_params = ["LatticeA", "LatticeB", "LatticeC", "LatticeAlpha", "LatticeBeta", "LatticeGamma"]
        set_count = 0
        for param in lattice_params:
            prop = self.getProperty(param)
            if not prop.isDefault:
                if prop.value <= 0.0:
                    issues[param] = "Lattice parameter must be greater than 0"
                if param == "LatticeAlpha" or param == "LatticeBeta" or param == "LatticeGamma":
                    if prop.value < 5.0 or prop.value > 175.0:
                        issues[param] = "Lattice parameter must have an angle between 5 and 175."
                set_count += 1

        if 0 <= set_count < 6 and not self.getProperty("UseLattice").isDefault:
            for param in lattice_params:
                issues[param] = "Must set all or none of the optional lattice parameters."

        input_workspaces = self._expand_groups()
        for input_ws in input_workspaces:
            if not (
                isinstance(AnalysisDataService[input_ws], MultipleExperimentInfos)
                or isinstance(AnalysisDataService[input_ws], MDHistoWorkspace)
            ):
                issues["InputWorkspace"] = "Workspace need to be a MDWorkspace"
            elif AnalysisDataService[input_ws].getSpecialCoordinateSystem().name != "QSample":
                issues["InputWorkspace"] = "Input workspace expected to be in QSample, workspace is in '{}'".format(
                    AnalysisDataService[input_ws].getSpecialCoordinateSystem().name
                )
            elif AnalysisDataService[input_ws].getNumDims() != 3:
                issues["InputWorkspace"] = "Workspace has the wrong number of dimensions"

        wavelength = self.getProperty("Wavelength")
        if not wavelength.isDefault:
            if wavelength.value <= 0.0:
                issues["Wavelength"] = "Wavelength should be greater than zero"

        return issues

    def PyExec(self):
        input_workspaces = self._expand_groups()
        output_workspace_name = self.getPropertyValue("OutputWorkspace")

        cell_type = self.getProperty("CellType").value
        centering = self.getProperty("Centering").value

        npeaks = self.getProperty("MaxPeaks").value
        dist_thresh = self.getProperty("PeakDistanceThreshold").value
        density_thresh = self.getProperty("DensityThresholdFactor").value

        lattice = self.getProperty("UseLattice").value
        if lattice:
            a = self.getProperty("LatticeA").value
            b = self.getProperty("LatticeB").value
            c = self.getProperty("LatticeC").value
            alpha = self.getProperty("LatticeAlpha").value
            beta = self.getProperty("LatticeBeta").value
            gamma = self.getProperty("LatticeGamma").value

        # Whether to use the inner goniometer depending on omega and phi in sample logs
        use_inner = False

        # Initially set the back-up wavelength to use. This is overwritten if found in sample logs.
        wavelength = None
        if not self.getProperty("Wavelength").isDefault:
            wavelength = self.getProperty("Wavelength").value

        multi_ws = len(input_workspaces) > 1
        output_workspaces = []
        for input_ws in input_workspaces:
            if multi_ws:
                peaks_ws_name = input_ws + "_" + output_workspace_name
                output_workspaces.append(peaks_ws_name)
            else:
                peaks_ws_name = output_workspace_name
            ws = AnalysisDataService[input_ws]
            if ws.getNumExperimentInfo() == 0:
                # Warn if we could extract a wavelength from the workspace
                raise RuntimeWarning("No experiment info was found in input '{}'".format(ws.getName()))
            else:
                exp_info = ws.getExperimentInfo(0)
                if exp_info.run().hasProperty("wavelength"):
                    wavelength = exp_info.run().getProperty("wavelength").value
                if exp_info.run().hasProperty("omega"):
                    if np.isclose(exp_info.run().getTimeAveragedStd("omega"), 0.0):
                        use_inner = True
                if exp_info.run().hasProperty("phi"):
                    if np.isclose(exp_info.run().getTimeAveragedStd("phi"), 0.0):
                        use_inner = False
                self.log().information("Using inner goniometer: {}".format(use_inner))

            FindPeaksMD(
                InputWorkspace=input_ws,
                PeakDistanceThreshold=dist_thresh,
                DensityThresholdFactor=density_thresh,
                CalculateGoniometerForCW=True,
                Wavelength=wavelength,
                FlipX=True,
                InnerGoniometer=use_inner,
                MaxPeaks=npeaks,
                OutputWorkspace=peaks_ws_name,
            )

        if multi_ws:
            peaks_ws_name = "__tmp_peaks_ws"
            CreatePeaksWorkspace(InstrumentWorkspace=input_workspaces[0], NumberOfPeaks=0, OutputWorkspace=peaks_ws_name)
            for peak_ws in output_workspaces:
                CombinePeaksWorkspaces(peaks_ws_name, peak_ws, OutputWorkspace=peaks_ws_name)

        if lattice:
            FindUBUsingLatticeParameters(PeaksWorkspace=peaks_ws_name, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        else:
            FindUBUsingFFT(PeaksWorkspace=peaks_ws_name, MinD=3, MaxD=20)

        ShowPossibleCells(PeaksWorkspace=peaks_ws_name)
        SelectCellOfType(PeaksWorkspace=peaks_ws_name, CellType=cell_type, Centering=centering, Apply=True)
        OptimizeLatticeForCellType(PeaksWorkspace=peaks_ws_name, CellType=cell_type, Apply=True)

        if multi_ws:
            for out_ws in output_workspaces:
                CopySample(
                    InputWorkspace=peaks_ws_name,
                    OutputWorkspace=out_ws,
                    CopyName=False,
                    CopyMaterial=False,
                    CopyEnvironment=False,
                    CopyShape=False,
                    CopyLattice=True,
                )
                IndexPeaks(PeaksWorkspace=out_ws)

            GroupWorkspaces(output_workspaces, OutputWorkspace=output_workspace_name)
            DeleteWorkspace(peaks_ws_name)

        self.setProperty("OutputWorkspace", AnalysisDataService[output_workspace_name])

    def _expand_groups(self):
        """expand workspace groups"""
        workspaces = self.getProperty("InputWorkspace").value
        input_workspaces = []
        for wsname in workspaces:
            wks = AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                input_workspaces.extend(wks.getNames())
            else:
                input_workspaces.append(wsname)

        return input_workspaces


AlgorithmFactory.subscribe(HB3AFindPeaks)
