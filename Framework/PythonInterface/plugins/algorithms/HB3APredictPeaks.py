# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, IMDWorkspaceProperty, IPeaksWorkspaceProperty, PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, FloatPropertyWithValue, StringListValidator
from mantid.simpleapi import PredictPeaks, CloneMDWorkspace, CopySample, DeleteWorkspace, mtd
import numpy as np


class HB3APredictPeaks(PythonAlgorithm):

    def category(self):
        return "Crystal\\Peaks;Crystal\\UBMatrix"

    def seeAlso(self):
        return ["PredictPeaks", "HB3AFindPeaks", "HB3AIntegratePeaks", "HB3AAdjustSampleNorm"]

    def name(self):
        return "HB3APredictPeaks"

    def summary(self):
        return 'Given a MD workspace in Q-space, and an optional workspace for UB, predict the peaks covering for that data'

    def PyInit(self):
        self.declareProperty(IMDWorkspaceProperty("InputWorkspace", defaultValue="", optional=PropertyMode.Mandatory,
                                                  direction=Direction.Input),
                             doc="Input MD workspace (in Q-space) to use for peak prediction")

        self.declareProperty(IPeaksWorkspaceProperty("UBWorkspace", defaultValue="", optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc="PeaksWorkspace with UB matrix to use, if non is provided the UB from the InputWorkspace is used")

        self.declareProperty(name="ReflectionCondition",
                             defaultValue="Primitive",
                             direction=Direction.Input,
                             validator=StringListValidator(
                                 ["Primitive",
                                  "C-face centred",
                                  "A-face centred",
                                  "B-face centred",
                                  "Body centred",
                                  "All-face centred",
                                  "Rhombohedrally centred, obverse",
                                  "Rhombohedrally centred, reverse",
                                  "Hexagonally centred, reverse"]),
                             doc="Reflection condition for Predicted Peaks.")

        self.declareProperty("Wavelength", FloatPropertyWithValue.EMPTY_DBL,
                             doc="Wavelength value to use only if one was not found in the sample log")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output,
                                                     optional=PropertyMode.Mandatory), doc="Output peaks workspace")

    def validateInputs(self):
        issues = dict()

        input_ws = self.getProperty("InputWorkspace").value
        if input_ws.getSpecialCoordinateSystem().name != "QSample":
            issues["InputWorkspace"] = "Input workspace expected to be in QSample, " \
                "workspace is in '{}'".format(input_ws.getSpecialCoordinateSystem().name)
        elif input_ws.getNumDims() != 3:
            issues["InputWorkspace"] = "Workspace has the wrong number of dimensions"

        wavelength = self.getProperty("Wavelength")
        if not wavelength.isDefault:
            if wavelength.value <= 0.0:
                issues['Wavelength'] = "Wavelength should be greater than zero"

        return issues

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        ub_ws = self.getProperty("UBWorkspace").value
        output_ws = self.getProperty("OutputWorkspace").valueAsStr
        reflection_condition = self.getProperty("ReflectionCondition").value

        # Whether to use the inner goniometer depending on omega and phi in sample logs
        use_inner = False
        min_angle = None
        max_angle = None

        wavelength = 0.0

        if input_ws.getNumExperimentInfo() == 0:
            # Warn if we could extract a wavelength from the workspace
            raise RuntimeWarning("No experiment info was found in input '{}'".format(input_ws.getName()))
        else:
            exp_info = input_ws.getExperimentInfo(0)
            if exp_info.run().hasProperty("wavelength"):
                wavelength = exp_info.run().getProperty("wavelength").value

            if exp_info.run().hasProperty("omega") and exp_info.run().hasProperty("phi"):
                gon = exp_info.run().getGoniometer().getEulerAngles('YZY')
                if np.isclose(exp_info.run().getTimeAveragedStd("omega"), 0.0):
                    use_inner = True
                    min_angle = -exp_info.run().getLogData('phi').value.max()
                    max_angle = -exp_info.run().getLogData('phi').value.min()
                    # Sometimes you get the 180 degrees off what is expected from the log
                    phi_log = -exp_info.run().getLogData('phi').value[0]
                    if np.isclose(phi_log + 180, gon[2]):
                        min_angle += 180
                        max_angle += 180
                    elif np.isclose(phi_log - 180, gon[2]):
                        min_angle -= 180
                        max_angle -= 180
                elif np.isclose(exp_info.run().getTimeAveragedStd("phi"), 0.0):
                    use_inner = False
                    min_angle = -exp_info.run().getLogData('omega').value.max()
                    max_angle = -exp_info.run().getLogData('omega').value.min()
                    # Sometimes you get the 180 degrees off what is expected from the log
                    omega_log = -exp_info.run().getLogData('omega').value[0]
                    if np.isclose(omega_log + 180, gon[0]):
                        min_angle += 180
                        max_angle += 180
                    elif np.isclose(omega_log - 180, gon[0]):
                        min_angle -= 180
                        max_angle -= 180
                else:
                    self.log().warning("No appropriate goniometer rotation found, try anyway")

            self.log().information("Using inner goniometer: {}".format(use_inner))

        if not self.getProperty("Wavelength").isDefault:
            wavelength = self.getProperty("Wavelength").value
        elif wavelength == 0:
            raise RuntimeWarning("No wavelength found, you need to provide one")

        # temporary set UB on workspace if one is provided by UBWorkspace
        tmp_ws_name = '__HB3APredictPeaks_UB_tmp'
        if ub_ws is not None:
            input_ws = CloneMDWorkspace(InputWorkspace=input_ws,
                                        OutputWorkspace=tmp_ws_name)
            CopySample(InputWorkspace=ub_ws,
                       OutputWorkspace=tmp_ws_name,
                       CopyName=False,
                       CopyMaterial=False,
                       CopyEnvironment=False,
                       CopyShape=False,
                       CopyLattice=True)

        peaks = PredictPeaks(InputWorkspace=input_ws,
                             ReflectionCondition=reflection_condition,
                             CalculateGoniometerForCW=True,
                             Wavelength=wavelength,
                             FlipX=True,
                             InnerGoniometer=use_inner,
                             MinAngle=min_angle,
                             MaxAngle=max_angle,
                             OutputWorkspace=output_ws)

        # delete tmp workspace
        if mtd.doesExist(tmp_ws_name):
            DeleteWorkspace(tmp_ws_name)

        self.setProperty("OutputWorkspace", peaks)


AlgorithmFactory.subscribe(HB3APredictPeaks)
