# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init, no-name-in-module
import math
import numpy as np
from scipy.stats import chisquare
from typing import List

from mantid.api import (AlgorithmFactory, FileAction, FileProperty, InstrumentValidator, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm, WorkspaceProperty)
from mantid.dataobjects import MaskWorkspace, TableWorkspace
from mantid.kernel import (Direction, EnabledWhenProperty, FloatBoundedValidator, logger, PropertyCriterion,
                           Quat, StringArrayProperty, StringListValidator, V3D)
import mantid.simpleapi as api


class AlignComponents(PythonAlgorithm):
    """
    Class to align components
    """

    _optionsList = ["Xposition", "Yposition", "Zposition", "AlphaRotation", "BetaRotation", "GammaRotation"]
    _optionsDict = {}
    _initialPos = None
    _move = False
    _rotate = False
    _masking = False
    _eulerConvention = None

    def category(self):
        """
        Mantid required
        """
        return "Diffraction"

    def seeAlso(self):
        return ["GetDetOffsetsMultiPeaks", "CalibrateRectangularDetectors"]

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

    #pylint: disable=too-many-locals
    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty("CalibrationTable", "",
                                                     optional=PropertyMode.Mandatory,
                                                     direction=Direction.Input),
                             doc="Calibration table, currently only uses difc")

        self.declareProperty(MatrixWorkspaceProperty("MaskWorkspace", "",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc="Mask workspace")

        self.declareProperty(FileProperty(name="InstrumentFilename",
                                          defaultValue="",
                                          action=FileAction.OptionalLoad,
                                          extensions=[".xml"]),
                             doc="Instrument filename")

        self.declareProperty(WorkspaceProperty("Workspace", "",
                                               validator=InstrumentValidator(),
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Input),
                             doc="Workspace containing the instrument to be calibrated.")

        # Source
        self.declareProperty(name="FitSourcePosition", defaultValue=False,
                             doc="Fit the source position, changes L1 (source to sample) distance."
                             "Uses entire instrument. Occurs before Components are Aligned.")

        # Sample
        self.declareProperty(name="FitSamplePosition", defaultValue=False,
                             doc="Fit the sample position, changes L1 (source to sample) and L2 (sample to detector) distance."
                             "Uses entire instrument. Occurs before Components are Aligned.")

        # List of components
        self.declareProperty(StringArrayProperty("ComponentList",
                                                 direction=Direction.Input),
                             doc="Comma separated list on instrument components to refine.")

        # X position
        self.declareProperty(name="Xposition", defaultValue=False,
                             doc="Refine Xposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Xposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinXposition", defaultValue=-0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Minimum relative X bound (m)")
        self.setPropertySettings("MinXposition", condition)
        self.declareProperty(name="MaxXposition", defaultValue=0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Maximum relative X bound (m)")
        self.setPropertySettings("MaxXposition", condition)

        # Y position
        self.declareProperty(name="Yposition", defaultValue=False,
                             doc="Refine Yposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Yposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinYposition", defaultValue=-0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Minimum relative Y bound (m)")
        self.setPropertySettings("MinYposition", condition)
        self.declareProperty(name="MaxYposition", defaultValue=0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Maximum relative Y bound (m)")
        self.setPropertySettings("MaxYposition", condition)

        # Z position
        self.declareProperty(name="Zposition", defaultValue=False,
                             doc="Refine Zposition of source and/or sample and/or components")
        condition = EnabledWhenProperty("Zposition", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinZposition", defaultValue=-0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Minimum relative Z bound (m)")
        self.setPropertySettings("MinZposition", condition)
        self.declareProperty(name="MaxZposition", defaultValue=0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Maximum relative Z bound (m)")
        self.setPropertySettings("MaxZposition", condition)

        # euler angles convention
        eulerConventions = ["ZXZ", "XYX", "YZY", "ZYZ", "XZX", "YXY", "XYZ", "YZX", "ZXY", "XZY", "ZYX", "YXZ"]
        self.declareProperty(name="EulerConvention", defaultValue="YZX",
                             validator=StringListValidator(eulerConventions),
                             doc="Euler angles convention used when calculating and displaying angles,"
                             "eg XYZ corresponding to alpha beta gamma.")

        # alpha rotation
        self.declareProperty(name="AlphaRotation", defaultValue=False,
                             doc="Refine rotation around first axis, alpha, for the components")
        condition = EnabledWhenProperty("AlphaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinAlphaRotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative alpha rotation (deg)")
        self.setPropertySettings("MinAlphaRotation", condition)
        self.declareProperty(name="MaxAlphaRotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative alpha rotation (deg)")
        self.setPropertySettings("MaxAlphaRotation", condition)

        # beta rotation
        self.declareProperty(name="BetaRotation", defaultValue=False,
                             doc="Refine rotation around seconds axis, beta, for the components")
        condition = EnabledWhenProperty("BetaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinBetaRotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative beta rotation (deg)")
        self.setPropertySettings("MinBetaRotation", condition)
        self.declareProperty(name="MaxBetaRotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative beta rotation (deg)")
        self.setPropertySettings("MaxBetaRotation", condition)

        # gamma rotation
        self.declareProperty(name="GammaRotation", defaultValue=False,
                             doc="Refine rotation around third axis, gamma, for the components")
        condition = EnabledWhenProperty("GammaRotation", PropertyCriterion.IsNotDefault)
        self.declareProperty(name="MinGammaRotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative gamma rotation (deg)")
        self.setPropertySettings("MinGammaRotation", condition)
        self.declareProperty(name="MaxGammaRotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative gamma rotation (deg)")
        self.setPropertySettings("MaxGammaRotation", condition)

        # Translation
        self.setPropertyGroup("Xposition", "Translation")
        self.setPropertyGroup("MinXposition", "Translation")
        self.setPropertyGroup("MaxXposition", "Translation")
        self.setPropertyGroup("Yposition", "Translation")
        self.setPropertyGroup("MinYposition", "Translation")
        self.setPropertyGroup("MaxYposition", "Translation")
        self.setPropertyGroup("Zposition", "Translation")
        self.setPropertyGroup("MinZposition", "Translation")
        self.setPropertyGroup("MaxZposition", "Translation")

        # Rotation
        self.setPropertyGroup("EulerConvention", "Rotation")
        self.setPropertyGroup("AlphaRotation", "Rotation")
        self.setPropertyGroup("MinAlphaRotation", "Rotation")
        self.setPropertyGroup("MaxAlphaRotation", "Rotation")
        self.setPropertyGroup("BetaRotation", "Rotation")
        self.setPropertyGroup("MinBetaRotation", "Rotation")
        self.setPropertyGroup("MaxBetaRotation", "Rotation")
        self.setPropertyGroup("GammaRotation", "Rotation")
        self.setPropertyGroup("MinGammaRotation", "Rotation")
        self.setPropertyGroup("MaxGammaRotation", "Rotation")

    def validateInputs(self):
        """
        Does basic validation for inputs
        """
        issues = dict()

        calWS: TableWorkspace = self.getProperty('CalibrationTable').value

        if 'difc' not in calWS.getColumnNames() or 'detid' not in calWS.getColumnNames():
            issues['CalibrationTable'] = "Calibration table requires detid and difc"

        maskWS: MaskWorkspace = self.getProperty("MaskWorkspace").value
        if maskWS is not None:
            if maskWS.id() != 'MaskWorkspace':
                issues['MaskWorkspace'] = "MaskWorkspace must be empty or of type \"MaskWorkspace\""
            # The mask workspace should contain as many spectra as rows in the calibration table
            if maskWS.getNumberHistograms() != calWS.rowCount():
                error_message = 'The mask workspace must contain as many spectra as rows in the calibration table'
                issues['MaskWorkspace'] = error_message

        # Need to get instrument in order to check components are valid
        if self.getProperty("Workspace").value is not None:
            wks_name = self.getProperty("Workspace").value.name()
        else:
            inputFilename = self.getProperty("InstrumentFilename").value
            if inputFilename == "":
                issues["Workspace"] = "A Workspace or InstrumentFilename must be defined"
                return issues
            else:
                api.LoadEmptyInstrument(Filename=inputFilename,
                                        OutputWorkspace="alignedWorkspace")
                wks_name = "alignedWorkspace"

        # Check if each component listed is defined in the instrument
        components = self.getProperty("ComponentList").value
        if len(components) <= 0 and not self.getProperty("FitSourcePosition").value and not self.getProperty("FitSamplePosition").value:
            issues['ComponentList'] = "Must supply components"
        else:
            get_component = api.mtd[wks_name].getInstrument().getComponentByName
            components = [component for component in components if get_component(component) is None]
            if len(components) > 0:
                issues['ComponentList'] = "Instrument has no component \"" \
                                       + ','.join(components) + "\""

        # This checks that something will actually be refined,
        if not (self.getProperty("Xposition").value
                or self.getProperty("Yposition").value
                or self.getProperty("Zposition").value
                or self.getProperty("AlphaRotation").value
                or self.getProperty("BetaRotation").value
                or self.getProperty("GammaRotation").value):
            issues["Xposition"] = "You must calibrate at least one position or rotation parameter."

        # Check that a position refinement is selected for sample/source
        if ((self.getProperty("FitSourcePosition").value
             or self.getProperty("FitSamplePosition").value)
                and not (self.getProperty("Xposition").value
                         or self.getProperty("Yposition").value
                         or self.getProperty("Zposition").value)):
            issues["Xposition"] = "If fitting source or sample, you must calibrate at least one position parameter."

        return issues

    #pylint: disable=too-many-branches
    def PyExec(self):
        self._eulerConvention=self.getProperty('EulerConvention').value
        calWS = self.getProperty('CalibrationTable').value
        calWS = api.SortTableWorkspace(calWS, Columns='detid')
        maskWS = self.getProperty("MaskWorkspace").value

        difc = calWS.column('difc')
        if maskWS is not None:
            self._masking = True
            mask = maskWS.extractY().flatten()
            difc = np.ma.masked_array(difc, mask)

        detID = calWS.column('detid')

        if self.getProperty("Workspace").value is not None:
            wks_name = self.getProperty("Workspace").value.name()
        else:
            wks_name = "alignedWorkspace"
            api.LoadEmptyInstrument(Filename=self.getProperty("InstrumentFilename").value,
                                    OutputWorkspace=wks_name)

        # Make a dictionary of what options are being refined for sample/source. No rotation.
        for translation_option in self._optionsList[:3]:
            self._optionsDict[translation_option] = self.getProperty(translation_option).value
        for rotation_option in self._optionsList[3:]:
            self._optionsDict[rotation_option] = False

        # First fit L1 if selected for Source and/or Sample
        for component in "Source", "Sample":  # fit first the source position, then the sample position
            if self.getProperty("Fit"+component+"Position").value:
                self._move = True
                if component == "Sample":
                    comp = api.mtd[wks_name].getInstrument().getSample()
                else:
                    comp = api.mtd[wks_name].getInstrument().getSource()
                componentName = comp.getFullName()
                logger.notice("Working on " + componentName + " Starting position is " + str(comp.getPos()))
                firstIndex = 0
                lastIndex = len(difc)
                if self._masking:
                    mask_out = mask[firstIndex:lastIndex + 1]
                else:
                    mask_out = None

                self._initialPos = [comp.getPos().getX(),
                                    comp.getPos().getY(),
                                    comp.getPos().getZ(),
                                    0, 0, 0]  # no rotation

                # Set up x0 and bounds lists
                x0List = []  # initial X, Y, Z coordinates
                boundsList = []  # [(minX, maxX), (minZ, maxZ), (minZ, maxZ)]
                for iopt, translation_option in enumerate(self._optionsList[:3]):  # iterate over X, Y, and Z
                    if self._optionsDict[translation_option]:
                        x0List.append(self._initialPos[iopt])
                        # default range for X is (x0 - 0.1m, x0 + 0.1m), same for Y and Z
                        boundsList.append((self._initialPos[iopt] + self.getProperty("Min"+translation_option).value,
                                           self._initialPos[iopt] + self.getProperty("Max"+translation_option).value))

                # scipy.opimize.minimize with the L-BFGS-B algorithm
                results: OptimizeResult = minimize(self._minimisation_func, x0=x0List,
                                                   method='L-BFGS-B',
                                                   args=(wks_name,
                                                         componentName,
                                                         firstIndex,
                                                         lastIndex,
                                                         difc[firstIndex:lastIndex + 1],
                                                         mask_out),
                                                   bounds=boundsList)

                # Apply the results to the output workspace
                xmap = self._mapOptions(results.x)

                # Need to grab the component again, as things have changed
                api.MoveInstrumentComponent(wks_name, componentName,
                                            X=xmap[0],
                                            Y=xmap[1],
                                            Z=xmap[2],
                                            RelativePosition=False)
                comp = api.mtd[wks_name].getInstrument().getComponentByName(componentName)
                logger.notice("Finished " + componentName + " Final position is " + str(comp.getPos()))
                self._move = False

        # Now fit all the remaining components, if any
        components = self.getProperty("ComponentList").value

        # Make a dictionary of what translational and rotational options are being refined.
        for opt in self._optionsList:
            self._optionsDict[opt] = self.getProperty(opt).value

        self._move = any([self._optionsDict[t] for t in ('Xposition', 'Yposition', 'Zposition')])
        self._rotate = any([self._optionsDict[r] for r in ('AlphaRotation', 'BetaRotation', 'GammaRotation')])

        prog = Progress(self, start=0, end=1, nreports=len(components))
        get_component = api.mtd[wks_name].getInstrument().getComponentByName  # shortcut
        for component in components:
            comp = get_component(component)
            firstDetID = self._getFirstDetID(comp)
            firstIndex = detID.index(firstDetID)  # a row index in the input calibration table
            lastDetID = self._getLastDetID(comp)
            lastIndex = detID.index(lastDetID)  # a row index in the input calibration table
            if lastDetID - firstDetID != lastIndex - firstIndex:
                raise RuntimeError("Calibration detid doesn't match instrument")

            eulerAngles: List[float] = comp.getRotation().getEulerAngles(self._eulerConvention)

            logger.notice("Working on " + comp.getFullName() + " Starting position is " + str(comp.getPos())
                          + " Starting rotation is " + str(eulerAngles))

            x0List = []
            self._initialPos = [comp.getPos().getX(), comp.getPos().getY(), comp.getPos().getZ(),
                                eulerAngles[0], eulerAngles[1], eulerAngles[2]]

            boundsList = []

            if self._masking:
                mask_out = mask[firstIndex:lastIndex + 1]
                if mask_out.sum() == mask_out.size:
                    self.log().warning("All pixels in '%s' are masked. Skipping calibration." % component)
                    continue
            else:
                mask_out = None

            for iopt, opt in enumerate(self._optionsList):
                if self._optionsDict[opt]:
                    x0List.append(self._initialPos[iopt])
                    boundsList.append((self._initialPos[iopt] + self.getProperty("Min"+opt).value,
                                       self._initialPos[iopt] + self.getProperty("Max"+opt).value))

            # scipy.opimize.minimize with the L-BFGS-B algorithm
            results: OptimizeResult = minimize(self._minimisation_func, x0=x0List,
                                               method='L-BFGS-B',
                                               args=(wks_name,
                                                     component,
                                                     firstIndex,
                                                     lastIndex,
                                                     difc[firstIndex:lastIndex + 1],
                                                     mask_out),
                                               bounds=boundsList)

            # Apply the results to the output workspace
            xmap = self._mapOptions(results.x)

            if self._move:
                api.MoveInstrumentComponent(wks_name, component, X=xmap[0], Y=xmap[1], Z=xmap[2],
                                            RelativePosition=False)

            if self._rotate:
                (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5], self._eulerConvention)
                api.RotateInstrumentComponent(wks_name, component, X=rotx, Y=roty, Z=rotz, Angle=rotw,
                                              RelativeRotation=False)

            # Need to grab the component object again, as things have changed
            comp = get_component(component)
            logger.notice("Finished " + comp.getFullName() + " Final position is " + str(comp.getPos())
                          + " Final rotation is " + str(comp.getRotation().getEulerAngles(self._eulerConvention)))

            prog.report()
        logger.notice("Results applied to workspace "+wks_name)

    #pylint: disable=too-many-arguments
    def _minimisation_func(self, x_0, wks_name, component, firstIndex, lastIndex, difc, mask):
        """
        Basic minimization function used. Returns the chisquared difference between the expected
        difc and the new difc after the component has been moved or rotated.

        There's an implicit one-to-correspondence between array index of `difc` and workspace index of `wks_name`,
        that is, between row index of the input calibration table and workspace index of `wks_name`.

        @param x_0 :: list of length 3 (new XYZ coordinates of the component) or length 6 (XYZ and rotation coords)
        @param wks_name :: name of a workspace with an embedded instrument. The instrument will be adjusted according to
            the new coordinates `x_0` for instrument component `component`. It's pixel spectra will contain the new DIFC
        @param component :: name of the instrument component to be optimized
        @param firstIndex :: workspace index of first index of `difc` array to be considered when comparing old
            and new DIFC values. When fitting the source or sample, this is the first spectrum index.
        @param lastIndex ::  workspace index of last index of `difc` array to be considered when comparing old
            and new DIFC values. When fitting the source or sample, this is the last row number of the input
            calibration table.
        @param mask :: mask array indicating which spectra should be considered when calculating the Chi-square value

        @return Chi-square value between old and new DIFC values for the unmasked spectra
        """
        xmap = self._mapOptions(x_0)  # pad null rotations when x_0 contains only translations

        if self._move:
            api.MoveInstrumentComponent(wks_name, component, X=xmap[0], Y=xmap[1], Z=xmap[2], RelativePosition=False)

        if self._rotate:
            (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5], self._eulerConvention)  # YZX
            api.RotateInstrumentComponent(wks_name, component, X=rotx, Y=roty, Z=rotz, Angle=rotw,
                                          RelativeRotation=False)

        api.CalculateDIFC(InputWorkspace=wks_name, OutputWorkspace=wks_name)

        difc_new = api.mtd[wks_name].extractY().flatten()[firstIndex:lastIndex + 1]

        if self._masking:
            difc_new = np.ma.masked_array(difc_new, mask)

        return chisquare(f_obs=difc, f_exp=difc_new)[0]

    def _getFirstDetID(self, component):
        """
        recursive search to find first detID of a component

        @param component :: reference to a detector component object

        @returns detector ID (`int`) of the first detector in the component
        """
        if component.type() == 'DetectorComponent' or component.type() == 'GridDetectorPixel':
            return component.getID()
        else:
            return self._getFirstDetID(component[0])

    def _getLastDetID(self, component):
        """
        recursive search to find last detID of a component

        @param component :: reference to a detector component object

        @returns detector ID (`int`) of the last detector in the component        """
        if component.type() == 'DetectorComponent' or component.type() == 'GridDetectorPixel':
            return component.getID()
        else:
            return self._getLastDetID(component[component.nelements() - 1])

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
        getV3D = {'X': V3D(1, 0, 0), 'Y': V3D(0, 1, 0), 'Z': V3D(0, 0, 1)}
        return (Quat(alpha, getV3D[convention[0]]) * Quat(beta, getV3D[convention[1]])
                * Quat(gamma, getV3D[convention[2]]))

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
    from scipy.optimize import minimize, OptimizeResult
    AlgorithmFactory.subscribe(AlignComponents)
except ImportError:
    logger.debug('Failed to subscribe algorithm AlignComponets; cannot import minimize from scipy.optimize')
