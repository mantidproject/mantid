#pylint: disable=no-init
import math
import numpy as np
from scipy.stats import chisquare
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, \
    ITableWorkspaceProperty, FileAction, FileProperty, WorkspaceProperty, InstrumentValidator, Progress
from mantid.kernel import Direction, FloatBoundedValidator, PropertyCriterion, EnabledWhenProperty, logger, Quat, V3D, StringArrayProperty
import mantid.simpleapi as api

class AlignComponents(PythonAlgorithm):
    """
    Class to align components
    """

    _optionsList = ["Xposition", "Yposition", "Zposition", "Xrotation", "Yrotation", "Zrotation"]
    _optionsDict = {}
    _initialPos = None
    _move = False
    _rotate = False
    _masking = False

    def category(self):
        """
        Mantid required
        """
        return "PythonAlgorithms;Diffraction"

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

        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               validator=InstrumentValidator(),
                                               optional=PropertyMode.Optional,
                                               direction=Direction.InOut),
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
        condition = EnabledWhenProperty("Xposition", PropertyCriterion.IsDefault)
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
        condition = EnabledWhenProperty("Yposition", PropertyCriterion.IsDefault)
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
        condition = EnabledWhenProperty("Zposition", PropertyCriterion.IsDefault)
        self.declareProperty(name="MinZposition", defaultValue=-0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Minimum relative Z bound (m)")
        self.setPropertySettings("MinZposition", condition)
        self.declareProperty(name="MaxZposition", defaultValue=0.1,
                             validator=FloatBoundedValidator(-10.0, 10.0),
                             doc="Maximum relative Z bound (m)")
        self.setPropertySettings("MaxZposition", condition)

        # X rotation
        self.declareProperty(name="Xrotation", defaultValue=False,
                             doc="Refinerotation around X")
        condition = EnabledWhenProperty("Xrotation", PropertyCriterion.IsDefault)
        self.declareProperty(name="MinXrotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative Xrotation (deg)")
        self.setPropertySettings("MinXrotation", condition)
        self.declareProperty(name="MaxXrotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative Xrotation (deg)")
        self.setPropertySettings("MaxXrotation", condition)

        # Y rotation
        self.declareProperty(name="Yrotation", defaultValue=False,
                             doc="Refinerotation around Y")
        condition = EnabledWhenProperty("Yrotation", PropertyCriterion.IsDefault)
        self.declareProperty(name="MinYrotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative Yrotation (deg)")
        self.setPropertySettings("MinYrotation", condition)
        self.declareProperty(name="MaxYrotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative Yrotation (deg)")
        self.setPropertySettings("MaxYrotation", condition)

        # Z rotation
        self.declareProperty(name="Zrotation", defaultValue=False,
                             doc="Refinerotation around Z")
        condition = EnabledWhenProperty("Zrotation", PropertyCriterion.IsDefault)
        self.declareProperty(name="MinZrotation", defaultValue=-10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Minimum relative Zrotation (deg)")
        self.setPropertySettings("MinZrotation", condition)
        self.declareProperty(name="MaxZrotation", defaultValue=10.0,
                             validator=FloatBoundedValidator(-90, 90),
                             doc="Maximum relative Zrotation (deg)")
        self.setPropertySettings("MaxZrotation", condition)

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
        if self.getProperty("InputWorkspace").value is not None:
            wks_name = self.getProperty("InputWorkspace").value.getName()
        else:
            inputFilename = self.getProperty("InstrumentFilename").value
            if inputFilename == "":
                issues["InputWorkspace"] = "A InputWorkspace or InstrumentFilename must be defined"
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
        if not (self.getProperty("Xposition").value or self.getProperty("Yposition").value or self.getProperty("Zposition").value or
                self.getProperty("Xrotation").value or self.getProperty("Yrotation").value or self.getProperty("Zrotation").value or
                self.getProperty("FitSourcePosition").value or self.getProperty("FitSamplePosition").value):
            issues["Xposition"] = "You must calibrate at least one property"

        return issues

    #pylint: disable=too-many-branches
    def PyExec(self):
        calWS = self.getProperty('CalibrationTable').value
        calWS = api.SortTableWorkspace(calWS, Columns='detid')
        maskWS = self.getProperty("MaskWorkspace").value

        if maskWS != None:
            self._masking = True
            mask = maskWS.extractY().flatten()

        difc = calWS.column('difc')
        if self._masking:
            difc = np.ma.masked_array(difc, mask)

        detID = calWS.column('detid')

        wks = self.getProperty("InputWorkspace").value
        if wks is None:
            wks = api.LoadEmptyInstrument(Filename=self.getProperty("InstrumentFilename").value,
                                          OutputWorkspace="alignedWorkspace")
        wks_name=wks.getName()

        # First fit L1 if selected for Source and/or Sample
        for component in "Source", "Sample":
            if self.getProperty("Fit"+component+"Position").value:
                if component == "Sample":
                    componentName = api.mtd[wks_name].getInstrument().getSample().getFullName()
                    componentZ = api.mtd[wks_name].getInstrument().getSample().getPos().getZ()
                else:
                    componentName = api.mtd[wks_name].getInstrument().getSource().getFullName()
                    componentZ = api.mtd[wks_name].getInstrument().getSource().getPos().getZ()
                logger.notice("Working on " + componentName +
                              " Starting position is " + str(componentZ))
                firstIndex = 0
                lastIndex = len(difc)
                if self._masking:
                    mask_out = mask[firstIndex:lastIndex + 1]
                else:
                    mask_out = None
                newZ = minimize(self._minimisation_func_L1, x0=componentZ,
                                args=(wks_name,
                                      componentName,
                                      firstIndex,
                                      lastIndex,
                                      difc[firstIndex:lastIndex + 1],
                                      mask_out),
                                bounds=[(componentZ-1,componentZ+1)])
                api.MoveInstrumentComponent(wks_name, componentName, Z=newZ.x[0],
                                            RelativePosition=False)
                logger.notice("Finished " + componentName +
                              " Final position is " + str(newZ.x[0]))

        # Now fit all the components if any
        components = self.getProperty("ComponentList").value

        for opt in self._optionsList:
            self._optionsDict[opt] = self.getProperty(opt).value

        if self._optionsDict["Xposition"] or self._optionsDict["Yposition"] or self._optionsDict["Zposition"]:
            self._move = True

        if self._optionsDict["Xrotation"] or self._optionsDict["Yrotation"] or self._optionsDict["Zrotation"]:
            self._rotate = True

        prog = Progress(self, start=0, end=1, nreports=len(components))
        for component in components:
            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)
            firstDetID = self._getFirstDetID(comp)
            firstIndex = detID.index(firstDetID)
            lastDetID = self._getLastDetID(comp)
            lastIndex = detID.index(lastDetID)
            if lastDetID - firstDetID != lastIndex - firstIndex:
                raise RuntimeError("Calibration detid doesn't match instrument")

            eulerAngles = comp.getRotation().getEulerAngles()

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

            if self._optionsDict["Xposition"]:
                x0List.append(self._initialPos[0])
                boundsList.append((self._initialPos[0] + self.getProperty("MinXposition").value,
                                   self._initialPos[0] + self.getProperty("MaxXposition").value))
            if self._optionsDict["Yposition"]:
                x0List.append(self._initialPos[1])
                boundsList.append((self._initialPos[1] + self.getProperty("MinYposition").value,
                                   self._initialPos[1] + self.getProperty("MaxYposition").value))
            if self._optionsDict["Zposition"]:
                x0List.append(self._initialPos[2])
                boundsList.append((self._initialPos[2] + self.getProperty("MinZposition").value,
                                   self._initialPos[2] + self.getProperty("MaxZposition").value))
            if self._optionsDict["Xrotation"]:
                x0List.append(self._initialPos[3])
                boundsList.append((self._initialPos[3] + self.getProperty("MinXrotation").value,
                                   self._initialPos[3] + self.getProperty("MaxXrotation").value))
            if self._optionsDict["Yrotation"]:
                x0List.append(self._initialPos[4])
                boundsList.append((self._initialPos[4] + self.getProperty("MinYrotation").value,
                                   self._initialPos[4] + self.getProperty("MaxYrotation").value))
            if self._optionsDict["Zrotation"]:
                x0List.append(self._initialPos[5])
                boundsList.append((self._initialPos[5] + self.getProperty("MinZrotation").value,
                                   self._initialPos[5] + self.getProperty("MaxZrotation").value))

            results = minimize(self._minimisation_func, x0=x0List,
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
                (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5])
                api.RotateInstrumentComponent(wks_name, component, X=rotx, Y=roty, Z=rotz, Angle=rotw,
                                              RelativeRotation=False)

            # Need to grab the component again, as things have changed
            comp = api.mtd[wks_name].getInstrument().getComponentByName(component)
            logger.notice("Finshed " + comp.getFullName() +
                          " Final position is " + str(comp.getPos()) +
                          " Final rotation is " + str(comp.getRotation().getEulerAngles()))

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
            (rotw, rotx, roty, rotz) = self._eulerToAngleAxis(xmap[3], xmap[4], xmap[5])
            api.RotateInstrumentComponent(wks_name, component, X=rotx, Y=roty, Z=rotz, Angle=rotw,
                                          RelativeRotation=False)

        api.CalculateDIFC(InputWorkspace=wks_name, OutputWorkspace=wks_name)

        difc_new = api.mtd[wks_name].extractY().flatten()[firstIndex:lastIndex + 1]

        if self._masking:
            difc_new = np.ma.masked_array(difc_new, mask)

        return chisquare(f_obs=difc, f_exp=difc_new)[0]

    def _minimisation_func_L1(self, x_0, wks_name, component, firstIndex, lastIndex, difc, mask):
        """
        Minimization function for moving component along Z only.
        """
        api.MoveInstrumentComponent(wks_name, component, Z=x_0[0], RelativePosition=False)
        wks_new = api.CalculateDIFC(InputWorkspace=wks_name, OutputWorkspace=wks_name)

        difc_new = wks_new.extractY().flatten()[firstIndex:lastIndex + 1]

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

    def _eulerToQuat(self, alpha, beta, gamma, convention="YZX"):
        """
        Convert Euler angles to a quaternion
        """
        getV3D = {'X': V3D(1, 0, 0), 'Y': V3D(0, 1, 0), 'Z': V3D(0, 0, 1)}
        return (Quat(alpha, getV3D[convention[0]]) * Quat(beta, getV3D[convention[1]]) *
                Quat(gamma, getV3D[convention[2]]))

    def _eulerToAngleAxis(self, alpha, beta, gamma, convention="YZX"):
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


#pylint: disable=wrong-import-position, wrong-import-order
try:
    from scipy.optimize import minimize
    AlgorithmFactory.subscribe(AlignComponents)
except ImportError:
    logger.debug('Failed to subscribe algorithm AlignComponets; cannot import minimize from scipy.optimize')
