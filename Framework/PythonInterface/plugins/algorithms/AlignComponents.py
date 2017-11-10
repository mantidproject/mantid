#pylint: disable=no-init, no-name-in-module
from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
from scipy.stats import chisquare
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, \
    ITableWorkspaceProperty, FileAction, FileProperty, WorkspaceProperty, InstrumentValidator, Progress
from mantid.kernel import Direction, FloatBoundedValidator, PropertyCriterion, EnabledWhenProperty, \
    logger, Quat, V3D, StringArrayProperty, StringListValidator
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
                             doc="Refine Xposition")
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
                             doc="Refine Yposition")
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
                             doc="Refine Zposition")
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
                             doc="Refine rotation around first axis, alpha")
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
                             doc="Refine rotation around seconds axis, beta")
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
                             doc="Refine rotation around third axis, gamma")
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
        self.setPropertyGroup("Xposition","Translation")
        self.setPropertyGroup("MinXposition","Translation")
        self.setPropertyGroup("MaxXposition","Translation")
        self.setPropertyGroup("Yposition","Translation")
        self.setPropertyGroup("MinYposition","Translation")
        self.setPropertyGroup("MaxYposition","Translation")
        self.setPropertyGroup("Zposition","Translation")
        self.setPropertyGroup("MinZposition","Translation")
        self.setPropertyGroup("MaxZposition","Translation")

        # Rotation
        self.setPropertyGroup("EulerConvention","Rotation")
        self.setPropertyGroup("AlphaRotation","Rotation")
        self.setPropertyGroup("MinAlphaRotation","Rotation")
        self.setPropertyGroup("MaxAlphaRotation","Rotation")
        self.setPropertyGroup("BetaRotation","Rotation")
        self.setPropertyGroup("MinBetaRotation","Rotation")
        self.setPropertyGroup("MaxBetaRotation","Rotation")
        self.setPropertyGroup("GammaRotation","Rotation")
        self.setPropertyGroup("MinGammaRotation","Rotation")
        self.setPropertyGroup("MaxGammaRotation","Rotation")

    def validateInputs(self):
        """
        Does basic validation for inputs
        """
        issues = dict()

        calWS = self.getProperty('CalibrationTable').value

        if 'difc' not in calWS.getColumnNames() or 'detid' not in calWS.getColumnNames():
            issues['CalibrationTable'] = "Calibration table requires detid and difc"

        maskWS = self.getProperty("MaskWorkspace").value
        if maskWS is not None and maskWS.id() != 'MaskWorkspace':
            issues['MaskWorkspace'] = "MaskWorkspace must be empty or of type \"MaskWorkspace\""

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
            components = [component for component in components
                          if api.mtd[wks_name].getInstrument().getComponentByName(component) is None]
            if len(components) > 0:
                issues['ComponentList'] = "Instrument has no component \"" \
                                       + ','.join(components) + "\""

        # This checks that something will actually be refined,
        if not (self.getProperty("Xposition").value or
                self.getProperty("Yposition").value or
                self.getProperty("Zposition").value or
                self.getProperty("AlphaRotation").value or
                self.getProperty("BetaRotation").value or
                self.getProperty("GammaRotation").value):
            issues["Xposition"] = "You must calibrate at least one parameter."

        # Check that a position refinement is selected for sample/source
        if ((self.getProperty("FitSourcePosition").value or
             self.getProperty("FitSamplePosition").value) and
                not (self.getProperty("Xposition").value or
                     self.getProperty("Yposition").value or
                     self.getProperty("Zposition").value)):
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
        for opt in self._optionsList[:3]:
            self._optionsDict[opt] = self.getProperty(opt).value
        for opt in self._optionsList[3:]:
            self._optionsDict[opt] = False

        # First fit L1 if selected for Source and/or Sample
        for component in "Source", "Sample":
            if self.getProperty("Fit"+component+"Position").value:
                self._move = True
                if component == "Sample":
                    comp = api.mtd[wks_name].getInstrument().getSample()
                else:
                    comp = api.mtd[wks_name].getInstrument().getSource()
                componentName = comp.getFullName()
                logger.notice("Working on " + componentName +
                              " Starting position is " + str(comp.getPos()))
                firstIndex = 0
                lastIndex = len(difc)
                if self._masking:
                    mask_out = mask[firstIndex:lastIndex + 1]
                else:
                    mask_out = None

                self._initialPos = [comp.getPos().getX(),
                                    comp.getPos().getY(),
                                    comp.getPos().getZ(),
                                    0, 0, 0]

                # Set up x0 and bounds lists
                x0List = []
                boundsList = []
                for iopt,opt in enumerate(self._optionsList[:3]):
                    if self._optionsDict[opt]:
                        x0List.append(self._initialPos[iopt])
                        boundsList.append((self._initialPos[iopt] + self.getProperty("Min"+opt).value,
                                           self._initialPos[iopt] + self.getProperty("Max"+opt).value))

                results = minimize(self._minimisation_func, x0=x0List,
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
                logger.notice("Finished " + componentName +
                              " Final position is " + str(comp.getPos()))
                self._move = False

        # Now fit all the components if any
        components = self.getProperty("ComponentList").value

        # Make a dictionary of what options are being refined.
        for opt in self._optionsList:
            self._optionsDict[opt] = self.getProperty(opt).value

        self._move = (self._optionsDict["Xposition"] or self._optionsDict["Yposition"] or self._optionsDict["Zposition"])

        self._rotate = (self._optionsDict["AlphaRotation"] or self._optionsDict["BetaRotation"] or self._optionsDict["GammaRotation"])

        prog = Progress(self, start=0, end=1, nreports=len(components))
        for component in components:
            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)
            firstDetID = self._getFirstDetID(comp)
            firstIndex = detID.index(firstDetID)
            lastDetID = self._getLastDetID(comp)
            lastIndex = detID.index(lastDetID)
            if lastDetID - firstDetID != lastIndex - firstIndex:
                raise RuntimeError("Calibration detid doesn't match instrument")

            eulerAngles = comp.getRotation().getEulerAngles(self._eulerConvention)

            logger.notice("Working on " + comp.getFullName() +
                          " Starting position is " + str(comp.getPos()) +
                          " Starting rotation is " + str(eulerAngles))

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

            for iopt,opt in enumerate(self._optionsList):
                if self._optionsDict[opt]:
                    x0List.append(self._initialPos[iopt])
                    boundsList.append((self._initialPos[iopt] + self.getProperty("Min"+opt).value,
                                       self._initialPos[iopt] + self.getProperty("Max"+opt).value))

            results = minimize(self._minimisation_func, x0=x0List,
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

            # Need to grab the component again, as things have changed
            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)
            logger.notice("Finshed " + comp.getFullName() +
                          " Final position is " + str(comp.getPos()) +
                          " Final rotation is " + str(comp.getRotation().getEulerAngles(self._eulerConvention)))

            prog.report()
        logger.notice("Results applied to workspace "+wks_name)

    #pylint: disable=too-many-arguments
    def _minimisation_func(self, x_0, wks_name, component, firstIndex, lastIndex, difc, mask):
        """
        Basic minimization function used. Returns the chisquared difference between the expected
        difc and the new difc after the component has been moved or rotated.
        """
        xmap = self._mapOptions(x_0)

        if self._move:
            api.MoveInstrumentComponent(wks_name, component, X=xmap[0], Y=xmap[1], Z=xmap[2], RelativePosition=False)

        if self._rotate:
            (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5], self._eulerConvention) # YZX
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
        """
        if component.type() == 'DetectorComponent' or component.type() == 'RectangularDetectorPixel':
            return component.getID()
        else:
            return self._getFirstDetID(component[0])

    def _getLastDetID(self, component):
        """
        recursive search to find last detID of a component
        """
        if component.type() == 'DetectorComponent' or component.type() == 'RectangularDetectorPixel':
            return component.getID()
        else:
            return self._getLastDetID(component[component.nelements() - 1])

    def _mapOptions(self, inX):
        """
        Creates an array combining the refining and constant variables
        This is required because scipy.optimise.minimise expect a constant
        number of variable, so need to be able to maps any number of
        inputs to six outputs.

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
        return (Quat(alpha, getV3D[convention[0]]) * Quat(beta, getV3D[convention[1]]) *
                Quat(gamma, getV3D[convention[2]]))

    def _eulerToAngleAxis(self, alpha, beta, gamma, convention):
        """
        Convert Euler angles to a angle rotation around an axis
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
    from scipy.optimize import minimize
    AlgorithmFactory.subscribe(AlignComponents)
except ImportError:
    logger.debug('Failed to subscribe algorithm AlignComponets; cannot import minimize from scipy.optimize')
