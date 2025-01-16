# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init, no-name-in-module
import math
import numpy as np
from typing import List

from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    InstrumentValidator,
    ITableWorkspaceProperty,
    MatrixWorkspaceProperty,
    Progress,
    PropertyMode,
    PythonAlgorithm,
    WorkspaceProperty,
)
from mantid.dataobjects import MaskWorkspace, TableWorkspace
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatArrayProperty,
    FloatBoundedValidator,
    logger,
    PropertyCriterion,
    Quat,
    StringArrayProperty,
    StringListValidator,
    V3D,
)
import mantid.simpleapi as api


class AlignComponents(PythonAlgorithm):
    """
    Class to align components
    """

    _optionsList = ["Xposition", "Yposition", "Zposition", "AlphaRotation", "BetaRotation", "GammaRotation"]
    adjustment_items = [
        "ComponentName",
        "Xposition",
        "Yposition",
        "Zposition",
        "XdirectionCosine",
        "YdirectionCosine",
        "ZdirectionCosine",
        "RotationAngle",
    ]
    r"""Items featuring the changes in position and orientation for each bank
    - DeltaR: change in distance from Component to Sample (in mili-meter)
    - DeltaX: change in X-coordinate of Component (in mili-meter)
    - DeltaY: change in Y-coordinate of Component (in mili-meter)
    - DeltaZ: change in Z-coordinate of Component (in mili-meter)
    Changes in Euler Angles are understood once a Euler convention is selected. If `YXZ` is selected, then:
    - DeltaAlpha: change in rotation around the Y-axis (in degrees)
    - DeltaBeta: change in rotation around the X-axis (in degrees)
    - DeltaGamma: change in rotation around the Z-axis (in degrees)
    """
    displacement_items = ["ComponentName", "DeltaR", "DeltaX", "DeltaY", "DeltaZ", "DeltaAlpha", "DeltaBeta", "DeltaGamma"]
    _optionsDict = {}
    _initialPos = None
    _move = False
    _rotate = False
    _eulerConvention = None

    def category(self):
        """
        Mantid required
        """
        return "Diffraction"

    def seeAlso(self):
        return []

    def name(self):
        """
        Mantid required
        """
        return "AlignComponents"

    def summary(self):
        """
        Mantid required
        """
        return "Align a component by minimising difference to an offset workspace"

    # pylint: disable=too-many-locals
    def PyInit(self):
        #
        # Reference and input data
        self.declareProperty(
            ITableWorkspaceProperty("PeakCentersTofTable", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Table of found peak centers, in TOF units",
        )

        self.declareProperty(
            FloatArrayProperty("PeakPositions", values=[], direction=Direction.Input),
            doc="Comma separated list of reference peak center d-spacings, sorted by increasing value.",
        )

        properties = ["PeakCentersTofTable", "PeakPositions"]
        [self.setPropertyGroup(name, "Reference and Input Data") for name in properties]

        #
        # Output Tables
        self.declareProperty(
            "AdjustmentsTable",
            "",
            direction=Direction.Input,
            doc="Name of output table containing optimized locations and orientations for each component",
        )

        self.declareProperty(
            "DisplacementsTable",
            "",
            direction=Direction.Input,
            doc="Name of output table containing changes in position and euler angles for each bank component",
        )

        properties = ["AdjustmentsTable", "DisplacementsTable"]
        [self.setPropertyGroup(name, "Output Tables") for name in properties]

        #
        # Selection of the instrument and mask
        self.declareProperty(
            MatrixWorkspaceProperty("MaskWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input), doc="Mask workspace"
        )

        self.declareProperty(
            FileProperty(name="InstrumentFilename", defaultValue="", action=FileAction.OptionalLoad, extensions=[".xml"]),
            doc="Instrument filename",
        )

        self.declareProperty(
            WorkspaceProperty(
                "InputWorkspace", "", validator=InstrumentValidator(), optional=PropertyMode.Optional, direction=Direction.Input
            ),
            doc="Workspace containing the instrument to be calibrated",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Workspace containing the calibrated instrument",
        )

        properties = ["MaskWorkspace", "InstrumentFilename", "InputWorkspace", "OutputWorkspace"]
        [self.setPropertyGroup(name, "Instrument Options") for name in properties]

        #
        # Components
        self.declareProperty(
            name="FitSourcePosition",
            defaultValue=False,
            doc="Fit the source position, changes L1 (source to sample) distance. "
            "Uses entire instrument. Occurs before Components are Aligned.",
        )

        self.declareProperty(
            name="FitSamplePosition",
            defaultValue=False,
            doc="Fit the sample position, changes L1 (source to sample) and L2 (sample to detector) distance."
            "Uses entire instrument. Occurs before Components are Aligned.",
        )

        self.declareProperty(
            StringArrayProperty("ComponentList", direction=Direction.Input), doc="Comma separated list on instrument components to refine."
        )

        properties = ["FitSourcePosition", "FitSamplePosition", "ComponentList"]
        [self.setPropertyGroup(name, "Declaration of Components") for name in properties]

        #
        # Translation Properties
        # X position
        self.declareProperty(name="Xposition", defaultValue=False, doc="Refine Xposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Xposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinXposition", defaultValue=-0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Minimum relative X bound (m)"
        )
        self.setPropertySettings("MinXposition", condition)
        self.declareProperty(
            name="MaxXposition", defaultValue=0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Maximum relative X bound (m)"
        )
        self.setPropertySettings("MaxXposition", condition)

        self.declareProperty(name="Yposition", defaultValue=False, doc="Refine Yposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Yposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinYposition", defaultValue=-0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Minimum relative Y bound (m)"
        )
        self.setPropertySettings("MinYposition", condition)
        self.declareProperty(
            name="MaxYposition", defaultValue=0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Maximum relative Y bound (m)"
        )
        self.setPropertySettings("MaxYposition", condition)

        # Z position
        self.declareProperty(name="Zposition", defaultValue=False, doc="Refine Zposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Zposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinZposition", defaultValue=-0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Minimum relative Z bound (m)"
        )
        self.setPropertySettings("MinZposition", condition)
        self.declareProperty(
            name="MaxZposition", defaultValue=0.1, validator=FloatBoundedValidator(-10.0, 10.0), doc="Maximum relative Z bound (m)"
        )
        self.setPropertySettings("MaxZposition", condition)

        properties = [
            "Xposition",
            "MinXposition",
            "MaxXposition",
            "Yposition",
            "MinYposition",
            "MaxYposition",
            "Zposition",
            "MinZposition",
            "MaxZposition",
        ]
        [self.setPropertyGroup(name, "Translation") for name in properties]

        #
        # Rotation Properties
        eulerConventions = ["ZXZ", "XYX", "YZY", "ZYZ", "XZX", "YXY", "XYZ", "YZX", "ZXY", "XZY", "ZYX", "YXZ"]
        self.declareProperty(
            name="EulerConvention",
            defaultValue="YZX",
            validator=StringListValidator(eulerConventions),
            doc="Euler angles convention used when calculating and displaying angles,eg XYZ corresponding to alpha beta gamma.",
        )

        # alpha rotation
        self.declareProperty(name="AlphaRotation", defaultValue=False, doc="Refine rotation around first axis, alpha, for the components")
        condition = EnabledWhenProperty("AlphaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinAlphaRotation",
            defaultValue=-10.0,
            validator=FloatBoundedValidator(-90, 90),
            doc="Minimum relative alpha rotation (deg)",
        )
        self.setPropertySettings("MinAlphaRotation", condition)
        self.declareProperty(
            name="MaxAlphaRotation",
            defaultValue=10.0,
            validator=FloatBoundedValidator(-90, 90),
            doc="Maximum relative alpha rotation (deg)",
        )
        self.setPropertySettings("MaxAlphaRotation", condition)

        # beta rotation
        self.declareProperty(name="BetaRotation", defaultValue=False, doc="Refine rotation around seconds axis, beta, for the components")
        condition = EnabledWhenProperty("BetaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinBetaRotation", defaultValue=-10.0, validator=FloatBoundedValidator(-90, 90), doc="Minimum relative beta rotation (deg)"
        )
        self.setPropertySettings("MinBetaRotation", condition)
        self.declareProperty(
            name="MaxBetaRotation", defaultValue=10.0, validator=FloatBoundedValidator(-90, 90), doc="Maximum relative beta rotation (deg)"
        )
        self.setPropertySettings("MaxBetaRotation", condition)

        # gamma rotation
        self.declareProperty(name="GammaRotation", defaultValue=False, doc="Refine rotation around third axis, gamma, for the components")
        condition = EnabledWhenProperty("GammaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            name="MinGammaRotation",
            defaultValue=-10.0,
            validator=FloatBoundedValidator(-90, 90),
            doc="Minimum relative gamma rotation (deg)",
        )
        self.setPropertySettings("MinGammaRotation", condition)
        self.declareProperty(
            name="MaxGammaRotation",
            defaultValue=10.0,
            validator=FloatBoundedValidator(-90, 90),
            doc="Maximum relative gamma rotation (deg)",
        )
        self.setPropertySettings("MaxGammaRotation", condition)

        properties = [
            "EulerConvention",
            "AlphaRotation",
            "MinAlphaRotation",
            "MaxAlphaRotation",
            "BetaRotation",
            "MinBetaRotation",
            "MaxBetaRotation",
            "GammaRotation",
            "MinGammaRotation",
            "MaxGammaRotation",
        ]
        [self.setPropertyGroup(name, "Rotation") for name in properties]

        #
        # Minimization Properties
        self.declareProperty(
            name="Minimizer",
            defaultValue="L-BFGS-B",
            direction=Direction.Input,
            validator=StringListValidator(["L-BFGS-B", "differential_evolution"]),
            doc="Minimizer to Use",
        )
        self.declareProperty(
            name="MaxIterations",
            defaultValue=100,
            direction=Direction.Input,
            doc="Maximum number of iterations for minimizer differential_evolution",
        )

        properties = ["Minimizer", "MaxIterations"]
        [self.setPropertyGroup(name, "Minimization") for name in properties]

    def validateInputs(self):
        """
        Does basic validation for inputs
        """
        issues = dict()

        peak_positions = self.getProperty("PeakPositions").value

        table_tof: TableWorkspace = self.getProperty("PeakCentersTofTable").value

        if "detid" not in table_tof.getColumnNames():
            issues["PeakCentersTofTable"] = 'PeakCentersTofTable is missing column "detid"'

        # The titles for table columns storing the TOF peak-center positions start with '@'
        column_names = [name for name in table_tof.getColumnNames() if name[0] == "@"]
        peak_count = len(peak_positions)  # number of reference peak-center values
        if len(column_names) != peak_count:
            error_message = (
                f"The number of table columns containing the peak center positions"
                f" {len(column_names)} is different than the number of peak positions {peak_count}"
            )
            issues["PeakCentersTofTable"] = error_message

        # The titles for table columns storing the TOF peak-center positions do contain the values
        # of the reference peak-center positions in d-spacing units, up to a precision of 5
        def with_precision(the_number, precision):
            r"""Analog of C++'s std::setprecision"""
            return round(the_number, precision - len(str(int(the_number))))

        for column_name, peak_position in zip(column_names, sorted(peak_positions)):
            if (float(column_name[1:]) - with_precision(peak_position, 5)) > 1.0e-3:
                issues["PeakCentersTofTable"] = f"{column_name} and {peak_position} differ up to precision 5"

        maskWS: MaskWorkspace = self.getProperty("MaskWorkspace").value
        if maskWS is not None:
            if maskWS.id() != "MaskWorkspace":
                issues["MaskWorkspace"] = 'MaskWorkspace must be empty or of type "MaskWorkspace"'
            # The mask workspace should contain as many spectra as rows in the TOFS table
            if maskWS.getNumberHistograms() != table_tof.rowCount():
                error_message = "The mask workspace must contain as many spectra as rows in the TOFS table"
                issues["MaskWorkspace"] = error_message

        # Need to get instrument in order to check if components are valid
        input_workspace = self.getProperty("InputWorkspace").value
        if bool(input_workspace) is True:
            wks_name = input_workspace.name()
        else:
            inputFilename = self.getProperty("InstrumentFilename").value
            if inputFilename == "":
                issues["InputWorkspace"] = "A Workspace or InstrumentFilename must be defined"
                return issues
            else:
                wks_name = "__alignedWorkspace"  # a temporary workspace
                api.LoadEmptyInstrument(Filename=inputFilename, OutputWorkspace=wks_name)

        # Check if each component listed is defined in the instrument
        components = self.getProperty("ComponentList").value
        source_or_sample = self.getProperty("FitSourcePosition").value or self.getProperty("FitSamplePosition").value
        if len(components) <= 0 and not source_or_sample:
            issues["ComponentList"] = "Must supply components"
        else:
            get_component = api.mtd[wks_name].getInstrument().getComponentByName
            components = [component for component in components if get_component(component) is None]
            if len(components) > 0:
                issues["ComponentList"] = 'Instrument has no component "' + ",".join(components) + '"'
        if wks_name == "__alignedWorkspace":
            api.DeleteWorkspace("__alignedWorkspace")  # delete temporary workspace

        # This checks that something will actually be refined,
        if not (
            self.getProperty("Xposition").value
            or self.getProperty("Yposition").value
            or self.getProperty("Zposition").value
            or self.getProperty("AlphaRotation").value
            or self.getProperty("BetaRotation").value
            or self.getProperty("GammaRotation").value
        ):
            issues["Xposition"] = "You must calibrate at least one position or rotation parameter."

        # Check that a position refinement is selected for sample/source
        if (self.getProperty("FitSourcePosition").value or self.getProperty("FitSamplePosition").value) and not (
            self.getProperty("Xposition").value or self.getProperty("Yposition").value or self.getProperty("Zposition").value
        ):
            issues["Xposition"] = "If fitting source or sample, you must calibrate at least one position parameter."

        return issues

    # flake8: noqa: C901
    def PyExec(self):
        table_tof = self.getProperty("PeakCentersTofTable").value
        self.peaks_tof = self._extract_tofs(table_tof)
        detector_count, peak_count = self.peaks_tof.shape
        table_tof = api.SortTableWorkspace(table_tof, Columns="detid")
        detID = table_tof.column("detid")
        peaks_ref = np.sort(self.getProperty("PeakPositions").value)  # sort by increasing value
        self.peaks_ref = peaks_ref[np.newaxis, :]  # shape = (1, peak_count)

        # Process input mask
        maskWS = self.getProperty("MaskWorkspace").value
        if maskWS is not None:
            mask = maskWS.extractY().flatten()  # shape=(detector_count,)
            peaks_mask = np.tile(mask[:, np.newaxis], peak_count)  # shape=(detector_count, peak_count)
        else:
            peaks_mask = np.zeros((detector_count, peak_count))  # no detectors are masked
        peaks_mask[np.isnan(self.peaks_tof)] = True
        # mask the defective detectors and missing peaks
        self.peaks_tof = np.ma.masked_array(self.peaks_tof, peaks_mask)

        input_workspace = self.getProperty("InputWorkspace").value

        # Table containing the optimized absolute locations and orientations for each component
        adjustments_table_name = self.getProperty("AdjustmentsTable").value
        if len(adjustments_table_name) > 0:
            adjustments_table = self._initialize_adjustments_table(adjustments_table_name)
            saving_adjustments = True
        else:
            saving_adjustments = False

        # Table containing the relative changes in position and euler angles for each bank component
        displacements_table_name = self.getProperty("DisplacementsTable").value
        if len(displacements_table_name) > 0:
            displacements_table = self._initialize_displacements_table(displacements_table_name)
            saving_displacements = True
        else:
            saving_displacements = False

        self._eulerConvention = self.getProperty("EulerConvention").value

        output_workspace = self.getPropertyValue("OutputWorkspace")
        wks_name = "__alignedworkspace"  # workspace whose counts will be DIFC values
        if bool(input_workspace) is True:
            api.CloneWorkspace(InputWorkspace=input_workspace, OutputWorkspace=wks_name)
            if output_workspace != str(input_workspace):
                api.CloneWorkspace(InputWorkspace=input_workspace, OutputWorkspace=output_workspace)
        else:
            api.LoadEmptyInstrument(Filename=self.getProperty("InstrumentFilename").value, OutputWorkspace=wks_name)

        # mapping from component-info index (or detector-info index) to detector-ID
        self.ci2id = api.mtd[wks_name].detectorInfo().detectorIDs()

        # Make a dictionary of what options are being refined for sample/source. No rotation.
        for translation_option in self._optionsList[:3]:
            self._optionsDict[translation_option] = self.getProperty(translation_option).value
        for rotation_option in self._optionsList[3:]:
            self._optionsDict[rotation_option] = False

        # First fit L1 if selected for Source and/or Sample
        sample_position_begin = api.mtd[wks_name].getInstrument().getSample().getPos()
        for component in "Source", "Sample":  # fit first the source position, then the sample position
            if self.getProperty("Fit" + component + "Position").value:
                self._move = True
                if component == "Sample":
                    comp = api.mtd[wks_name].getInstrument().getSample()
                else:
                    comp = api.mtd[wks_name].getInstrument().getSource()
                componentName = comp.getFullName()
                logger.notice("Working on " + componentName + " Starting position is " + str(comp.getPos()))
                firstIndex = 0
                lastIndex = detector_count - 1

                self._initialPos = [comp.getPos().getX(), comp.getPos().getY(), comp.getPos().getZ(), 0, 0, 0]  # no rotation

                # Set up x0 and bounds lists
                x0List = []  # initial X, Y, Z coordinates
                boundsList = []  # [(minX, maxX), (minZ, maxZ), (minZ, maxZ)]
                for iopt, translation_option in enumerate(self._optionsList[:3]):  # iterate over X, Y, and Z
                    if self._optionsDict[translation_option]:
                        x0List.append(self._initialPos[iopt])
                        # default range for X is (x0 - 0.1m, x0 + 0.1m), same for Y and Z
                        boundsList.append(
                            (
                                self._initialPos[iopt] + self.getProperty("Min" + translation_option).value,
                                self._initialPos[iopt] + self.getProperty("Max" + translation_option).value,
                            )
                        )

                # scipy.opimize.minimize with the L-BFGS-B algorithm
                results: OptimizeResult = minimize(
                    self._minimisation_func,
                    x0=x0List,
                    method="L-BFGS-B",
                    args=(wks_name, componentName, firstIndex, lastIndex),
                    bounds=boundsList,
                )

                # Apply the results to the output workspace
                xmap = self._mapOptions(results.x)

                # Save translation and rotations, if requested
                if saving_adjustments:
                    instrument = api.mtd[wks_name].getInstrument()
                    name_finder = {"Source": instrument.getSource().getName(), "Sample": instrument.getSample().getName()}
                    component_adjustments = [name_finder[component]] + xmap[:3] + [0.0] * 4  # no rotations
                    adjustments_table.addRow(component_adjustments)

                # Need to grab the component again, as things have changed
                kwargs = dict(X=xmap[0], Y=xmap[1], Z=xmap[2], RelativePosition=False, EnableLogging=False)
                api.MoveInstrumentComponent(wks_name, componentName, **kwargs)  # adjust workspace
                api.MoveInstrumentComponent(output_workspace, componentName, **kwargs)  # adjust workspace
                comp = api.mtd[wks_name].getInstrument().getComponentByName(componentName)
                logger.notice("Finished " + componentName + " Final position is " + str(comp.getPos()))
                self._move = False
        sample_position_end = api.mtd[wks_name].getInstrument().getSample().getPos()

        # Now fit all the remaining components, if any
        components = self.getProperty("ComponentList").value

        # Make a dictionary of what translational and rotational options are being refined.
        for opt in self._optionsList:
            self._optionsDict[opt] = self.getProperty(opt).value

        self._move = any([self._optionsDict[t] for t in ("Xposition", "Yposition", "Zposition")])
        self._rotate = any([self._optionsDict[r] for r in ("AlphaRotation", "BetaRotation", "GammaRotation")])

        prog = Progress(self, start=0, end=1, nreports=len(components))
        for component in components:
            firstDetID, lastDetID = self._firstAndLastDetID(component, api.mtd[wks_name].componentInfo())

            firstIndex = detID.index(firstDetID)  # a row index in the input TOFS table
            lastIndex = detID.index(lastDetID)  # a row index in the input TOFS table
            if lastDetID - firstDetID != lastIndex - firstIndex:
                raise RuntimeError("TOFS detid doesn't match instrument")

            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)
            eulerAngles: List[float] = comp.getRotation().getEulerAngles(self._eulerConvention)

            logger.notice(
                "Working on "
                + comp.getFullName()
                + " Starting position is "
                + str(comp.getPos())
                + " Starting rotation is "
                + str(eulerAngles)
            )

            x0List = []
            self._initialPos = [
                comp.getPos().getX(),
                comp.getPos().getY(),
                comp.getPos().getZ(),
                eulerAngles[0],
                eulerAngles[1],
                eulerAngles[2],
            ]

            # Distance between the original position of the sample and the original position of the component
            comp_sample_distance_begin = (comp.getPos() - sample_position_begin).norm()

            boundsList = []

            if np.all(peaks_mask[firstIndex : lastIndex + 1].astype(bool)):
                self.log().warning("All pixels in '%s' are masked. Skipping calibration." % component)
                continue

            for iopt, opt in enumerate(self._optionsList):
                if self._optionsDict[opt]:
                    x0List.append(self._initialPos[iopt])
                    boundsList.append(
                        (
                            self._initialPos[iopt] + self.getProperty("Min" + opt).value,
                            self._initialPos[iopt] + self.getProperty("Max" + opt).value,
                        )
                    )

            minimizer_selection = self.getProperty("Minimizer").value
            if minimizer_selection == "L-BFGS-B":
                # scipy.opimize.minimize with the L-BFGS-B algorithm
                results: OptimizeResult = minimize(
                    self._minimisation_func,
                    x0=x0List,
                    method="L-BFGS-B",
                    args=(wks_name, component, firstIndex, lastIndex),
                    bounds=boundsList,
                )
            elif minimizer_selection == "differential_evolution":
                results: OptimizeResult = differential_evolution(
                    self._minimisation_func,
                    bounds=boundsList,
                    args=(wks_name, component, firstIndex, lastIndex),
                    maxiter=self.getProperty("MaxIterations").value,
                )
            # Apply the results to the output workspace
            xmap = self._mapOptions(results.x)

            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)  # adjusted component
            # Distance between the adjusted position of the sample and the adjusted position of the component
            comp_sample_distance_end = (comp.getPos() - sample_position_end).norm()

            component_adjustments = [0.0] * 7  # 3 for translation, 3 for rotation axis, 1 for rotation angle
            component_displacements = [0.0] * 7  # 1 for distnace, 3 for translation, 3 for Euler angles
            component_displacements[0] = 1000 * (comp_sample_distance_end - comp_sample_distance_begin)  # in mili-meters

            if self._move:
                kwargs = dict(X=xmap[0], Y=xmap[1], Z=xmap[2], RelativePosition=False, EnableLogging=False)
                api.MoveInstrumentComponent(wks_name, component, **kwargs)  # adjust workspace
                api.MoveInstrumentComponent(output_workspace, component, **kwargs)  # adjust workspace
                component_adjustments[:3] = xmap[:3]
                for i in range(3):
                    component_displacements[i + 1] = 1000 * (xmap[i] - self._initialPos[i])  # in mili-meters

            if self._rotate:
                (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5], self._eulerConvention)
                kwargs = dict(X=rotx, Y=roty, Z=rotz, Angle=rotw, RelativeRotation=False, EnableLogging=False)
                api.RotateInstrumentComponent(wks_name, component, **kwargs)  # adjust workspace
                api.RotateInstrumentComponent(output_workspace, component, **kwargs)  # adjust workspace
                component_adjustments[3:] = [rotx, roty, rotz, rotw]
                for i in range(3, 6):
                    component_displacements[i + 1] = xmap[i] - self._initialPos[i]  # in degrees

            if saving_adjustments and (self._move or self._rotate):
                adjustments_table.addRow([component] + component_adjustments)

            if saving_displacements and (self._move or self._rotate):
                displacements_table.addRow([component] + component_displacements)

            # Need to grab the component object again, as things have changed
            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)  # adjusted component
            logger.notice(
                "Finished "
                + comp.getFullName()
                + " Final position is "
                + str(comp.getPos())
                + " Final rotation is "
                + str(comp.getRotation().getEulerAngles(self._eulerConvention))
            )

            prog.report()
        api.DeleteWorkspace(wks_name)
        self.setProperty("OutputWorkspace", output_workspace)
        logger.notice("Results applied to workspace " + wks_name)

    def _initialize_adjustments_table(self, table_name):
        r"""Create a table with appropriate column names for saving the adjustments to each component"""
        table = api.CreateEmptyTableWorkspace(OutputWorkspace=table_name)
        item_types = [
            "str",  # component name
            "double",
            "double",
            "double",  # cartesian coordinates
            "double",
            "double",
            "double",  # direction cosines of axis of rotation
            "double",
        ]  # angle of rotation
        for column_name, column_type in zip(self.adjustment_items, item_types):
            table.addColumn(name=column_name, type=column_type)
        return table

    def _initialize_displacements_table(self, table_name):
        r"""Create a table with appropriate column names for saving the relative displacements to each component"""
        table = api.CreateEmptyTableWorkspace(OutputWorkspace=table_name)
        item_types = [
            "str",  # component name
            "double",  # change in the distance between the component and the sample
            "double",
            "double",
            "double",  # relative displacement in cartesian coordinates
            "double",
            "double",
            "double",
        ]  # relative displacement in Euler angles
        for column_name, column_type in zip(self.displacement_items, item_types):
            table.addColumn(name=column_name, type=column_type)
        return table

    def _extract_tofs(self, table_tofs: TableWorkspace) -> np.ndarray:
        r"""
        Extract the columns of the input table containing the peak centers, sorted by increasing value
        of the peak center in d-spacing units

        :param table_tofs: table of peak centers, in TOF units
        :return array of shape (detector_count, peak_count)
        """
        # the title for the columns containing the peak centers begin with '@'
        indexes_and_titles = [(index, title) for index, title in enumerate(table_tofs.getColumnNames()) if "@" in title]
        column_indexes, titles = list(zip(*indexes_and_titles))
        peak_tofs = np.array([table_tofs.column(i) for i in column_indexes])  # shape = (peak_count, detector_count)
        peak_centers = np.array([float(title.replace("@", "")) for title in titles])
        permutation = np.argsort(peak_centers)  # reorder of indices guarantee increase in d-spacing
        peak_tofs = peak_tofs[permutation]  # sort by increasing d-spacing
        return np.transpose(peak_tofs)  # shape = (detector_count, peak_count)

    def _minimisation_func(self, x_0, wks_name, component, firstIndex, lastIndex):
        """
        Basic minimization function used. Returns the sum of the absolute values for the fractional peak
        deviations:

        .. math::

            \\sum_i^{N_d}\\sum_j^{N_p} (1 - m_{i,j}) \\frac{|d_{i,j} - d_j^*|}{d_j^*}

        where :math:`N_d` is the number of detectors in the bank, :math:`N_p` is the number of reference peaks, and
        :math:`m_{i,j}` is the mask for peak :math:`j` and detector :math:`i`. The mask evaluates to 1 if the
        detector is defective or the peak is missing in the detector, otherwise the mask evaluates to zero.

        There's an implicit one-to-correspondence between array index of ``difc`` and workspace index of ``wks_name``,
        that is, between row index of the input TOFS table and workspace index of ``wks_name``.

        @param x_0 :: list of length 3 (new XYZ coordinates of the component) or length 6 (XYZ and rotation coords)
        @param wks_name :: name of a workspace with an embedded instrument. The instrument will be adjusted according to
            the new coordinates ``x_0`` for instrument component ``component``. It's pixel spectra will contain the new DIFC
        @param component :: name of the instrument component to be optimized
        @param firstIndex :: workspace index of first index of ``difc`` array to be considered when comparing old
            and new DIFC values. When fitting the source or sample, this is the first spectrum index.
        @param lastIndex ::  workspace index of last index of ``difc`` array to be considered when comparing old
            and new DIFC values. When fitting the source or sample, this is the last row number of the input
            TOFS table.

        @return Chi-square value between old and new DIFC values for the unmasked spectra
        """
        xmap = self._mapOptions(x_0)  # pad null rotations when x_0 contains only translations

        if self._move:
            api.MoveInstrumentComponent(wks_name, component, X=xmap[0], Y=xmap[1], Z=xmap[2], RelativePosition=False, EnableLogging=False)

        if self._rotate:
            (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5], self._eulerConvention)  # YZX
            api.RotateInstrumentComponent(
                wks_name, component, X=rotx, Y=roty, Z=rotz, Angle=rotw, RelativeRotation=False, EnableLogging=False
            )

        api.CalculateDIFC(InputWorkspace=wks_name, OutputWorkspace=wks_name, EnableLogging=False)
        difc = api.mtd[wks_name].extractY().flatten()[firstIndex : lastIndex + 1]
        peaks_d = self.peaks_tof[firstIndex : lastIndex + 1] / difc[:, np.newaxis]  # peak centers in d-spacing units

        # calculate the fractional peak center deviations, then sum their absolute values
        return np.sum(np.abs((peaks_d - self.peaks_ref) / self.peaks_ref))

    def _unique_name(self, component, component_info):
        r"""Given the full name (or part of the full name) of a component, find the part
        of the name that is unique in the whole instrument.

        Example: the unique name for 'CORELLI/A row/bank1/sixteenpack' is 'bank1'. There's only
        one 'bank1' in the whole instrument

        @param str component: (partial) full name of the component assembly
        @param mantid.geometry.componentInfo component_info: object holding information for the instrument components
        @return str: the unique name
        """
        name_parts = sorted(component.split("/"), reverse=True)
        for part in name_parts:
            if component_info.uniqueName(part):
                return part
        raise RuntimeError("Could not find a unique name for {component}")

    def _firstAndLastDetID(self, component, component_info):
        r"""first and last detector ID's in a component
        @param str component: name of the component assembly
        @param mantid.geometry.componentInfo component_info: object holding information for the instrument components
        @return tuple: firt and last detector ID's
        """
        unique_name = self._unique_name(component, component_info)
        index = component_info.indexOfAny(unique_name)  # component-info index
        # component-info indexes for all detector pixels in this components
        index_all = sorted(component_info.detectorsInSubtree(index))
        # find the detector-ID for the first and last component-info indexes
        first, last = self.ci2id[index_all[0]], self.ci2id[index_all[-1]]
        logger.debug(f"First and last detectorID for {component} are {first}, {last}")
        return first, last

    def _mapOptions(self, inX):
        """
        Creates an array combining the refining and constant variables
        This is required because scipy.optimise.minimize expects a constant
        number of variables, so need to be able to maps any number of
        inputs to six outputs.

        @param inX :: list of length 3 or 6

        @return list of length 6
        """
        x0_index = 0
        out = []
        for opt in self._optionsList:
            if self._optionsDict[opt]:
                out.append(inX[x0_index])
                x0_index += 1
            else:
                out.append(self._initialPos[self._optionsList.index(opt)])
        return out

    def _eulerToQuat(self, alpha, beta, gamma, convention):
        """
        Convert Euler angles to a quaternion
        """
        getV3D = {"X": V3D(1, 0, 0), "Y": V3D(0, 1, 0), "Z": V3D(0, 0, 1)}
        return Quat(alpha, getV3D[convention[0]]) * Quat(beta, getV3D[convention[1]]) * Quat(gamma, getV3D[convention[2]])

    def _eulerToAngleAxis(self, alpha, beta, gamma, convention):
        """
        Find the Euler axis and Euler angle

        @param alpha :: rotation angle, in degrees, around the first axis of `convention`
        @param beta :: rotation angle, in degrees, around the second axis of `convention`
        @param gamma :: rotation angle, in degrees, around the third axis of `convention`
        @param convention :: string, e.g. 'YZX'

        @return Euler angle, and three direct cosines defining the Euler axis
        """
        quat = self._eulerToQuat(alpha, beta, gamma, convention)
        if quat[0] == 1:
            return 0, 0, 0, 1
        deg = math.acos(quat[0])
        scale = math.sin(deg)
        deg *= 360.0 / math.pi
        ax0 = quat[1] / scale
        ax1 = quat[2] / scale
        ax2 = quat[3] / scale
        return deg, ax0, ax1, ax2


try:
    from scipy.optimize import minimize, differential_evolution, OptimizeResult

    AlgorithmFactory.subscribe(AlignComponents)
except ImportError:
    logger.debug("Failed to subscribe algorithm AlignComponets; cannot import minimize from scipy.optimize")
