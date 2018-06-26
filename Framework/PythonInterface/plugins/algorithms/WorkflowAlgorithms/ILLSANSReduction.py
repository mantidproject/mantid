#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import Algorithm, PythonAlgorithm, MultipleFileProperty, FileAction, MatrixWorkspaceProperty
from mantid.kernel import StringMandatoryValidator, Direction
from mantid.simpleapi import *
import os


class ILLSANSReduction(PythonAlgorithm):

    _message = ''
    _output_ws = ''
    _property_list = []
    _property_manager_name = ''
    _property_manager = None
    _filename = None

    def category(self):
        return 'Workflow\\SANS\\UsesPropertyManager;ILL\\SANS'

    def name(self):
        return 'ILLSANSReduction'

    def summary(self):
        return "Performs ILL SANS reduction"

    def validateInputs(self):
        issues = dict()
        property_manager_name = self.getPropertyValue("ReductionProperties")
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        if property_manager is None:
            issues['ReductionProperties'] = 'Property manager does not exist in ADS'
        elif not property_manager.existsProperty('LoadAlgorithm'):
            issues['ReductionProperties'] = 'LoadAlgorithm not specified in the property manager'
        if property_manager.existsProperty('TransmissionAlgorithm'):
            trans_alg = property_manager.getPropertyValue("TransmissionAlgorithm")
            alg = Algorithm.fromString(trans_alg)
            if alg.name() == "SANSDirectBeamTransmission" and not self.getPropertyValue("TransmissionFilename"):
                issues['TransmissionFilename'] = 'Sample transmission file is required for direct beam method.'
        return issues

    def _get_numor(self, path):
        return os.path.basename(path).split('.')[0]

    def _setup(self):
        self._filename = self.getPropertyValue("Filename")
        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._property_manager_name = self.getPropertyValue("ReductionProperties")
        self._property_manager = PropertyManagerDataService.retrieve(self._property_manager_name)
        self._property_list = [p.name for p in self._property_manager.getProperties()]

    def _load(self, filename, output_ws):
        loader = self._property_manager.getPropertyValue('LoadAlgorithm')
        alg = Algorithm.fromString(loader)
        alg.setProperty('Filename', filename)
        alg.setProperty('OutputWorkspace', output_ws)
        if alg.existsProperty('ReductionProperties'):
            alg.setProperty('ReductionProperties', self._property_manager_name)
        alg.execute()
        msg = 'Loaded run %s\n' % filename
        if alg.existsProperty('OutputMessage'):
            msg += alg.getPropertyValue('OutputMessage')
        return msg

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('Filename', extensions=['nxs']),
                             doc='Run number(s) of sample run(s).')
        self.declareProperty(MultipleFileProperty('TransmissionFilename', extensions=['nxs'], action=FileAction.OptionalLoad),
                             doc='Run number(s) of sample transmission run; use if TransmissionMethod is DirectBeam')
        self.declareProperty('TransmissionValue', 1., doc='Sample transmission value; use if TransmissionMethod is Value')
        self.declareProperty('TransmissionError', 0., doc='Sample transmission value error; use if TransmissionMethod is Value')
        self.declareProperty('SampleThickness', 0., doc='Sample thickness [cm]; leave default, to use the value from configuration')
        self.declareProperty('ReductionProperties', '__sans_reduction_properties', validator=StringMandatoryValidator(),
                             doc='Property manager name for the reduction configuration')
        self.declareProperty('OutputMessage', '', direction=Direction.Output, doc='Output message report')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Reduced workspace')

    def PyExec(self): #noqa C901, we know it's complex

        self._setup()
        # PROCESS SAMPLE DATA
        # Step 1. Find the beam center
        if "SANSBeamFinderAlgorithm" in self._property_list:
            beam_finder_name = self._property_manager.getPropertyValue("SANSBeamFinderAlgorithm")
            alg = Algorithm.fromString(beam_finder_name)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                self._message += alg.getPropertyValue("OutputMessage") + '\n'

        # Step 2. Load the sample data
        sample_ws = "__sample_%s" % self._output_ws
        self._message += self._load(self._filename, sample_ws)

        # Step 3. Perform dark current subtraction
        self._message += self._simple_execution("DarkCurrentAlgorithm", sample_ws)

        # Step 4. Normalize to time or monitor
        self._message += self._simple_execution("NormaliseAlgorithm", sample_ws)

        # Step 5. Mask detectors
        self._message += self._simple_execution("MaskAlgorithm", sample_ws)

        # Step 6. Perform solid angle correction
        self._message += self._simple_execution("SANSSolidAngleCorrection", sample_ws)

        # Step 7. Calculate and apply sample transmission correction
        beam_center_x = None
        beam_center_y = None
        if "TransmissionBeamCenterAlgorithm" in self._property_list:
            # Execute the beam finding algorithm and set the beam
            # center for the transmission calculation
            p=self._property_manager.getPropertyValue("TransmissionBeamCenterAlgorithm")
            alg=Algorithm.fromString(p)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            beam_center_x = alg.getProperty("FoundBeamCenterX").value
            beam_center_y = alg.getProperty("FoundBeamCenterY").value

        if "TransmissionAlgorithm" in self._property_list:
            # Calculate and apply sample transmission correction
            trans_alg = self._property_manager.getPropertyValue("TransmissionAlgorithm")
            alg = Algorithm.fromString(trans_alg)
            alg.setProperty("InputWorkspace", sample_ws)
            alg.setProperty("OutputWorkspace", sample_ws)

            if alg.name() == "ApplyTransmissionCorrection":
                alg.setProperty("TransmissionValue", self.getProperty("TransmissionValue").value)
                alg.setProperty("TransmissionError", self.getProperty("TransmissionError").value)
            elif alg.name() == "SANSDirectBeamTransmission":
                alg.setPropertyValue("SampleDataFilename", self.getPropertyValue("TransmissionFilename"))

            if alg.existsProperty("BeamCenterX") \
                    and alg.existsProperty("BeamCenterY") \
                    and beam_center_x is not None \
                    and beam_center_y is not None:
                alg.setProperty("BeamCenterX", beam_center_x)
                alg.setProperty("BeamCenterY", beam_center_y)

            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()

            if alg.existsProperty("MeasuredTransmission"):
                meas_trans = alg.getProperty("MeasuredTransmission").value
                if self._property_manager.existsProperty("MeasuredTransmissionValue"):
                    self._property_manager.setProperty("MeasuredTransmissionValue", meas_trans)
                else:
                    self._property_manager.declareProperty("MeasuredTransmissionValue", meas_trans)
            if alg.existsProperty("MeasuredError"):
                meas_err = alg.getProperty("MeasuredError").value
                if self._property_manager.existsProperty("MeasuredTransmissionError"):
                    self._property_manager.setProperty("MeasuredTransmissionError", meas_err)
                else:
                    self._property_manager.declareProperty("MeasuredTransmissionError", meas_err)

            if alg.existsProperty("OutputMessage"):
                self._message += alg.getProperty("OutputMessage").value+'\n'

        # PROCESS BACKGROUND DATA
        if "BackgroundFiles" in self._property_list:
            background = self._property_manager.getPropertyValue("BackgroundFiles")
            background_ws = "__bckg_%s" % self._output_ws

            # Perform steps 2-7 for background file
            # Step 2. Load the background file
            self._message += self._load(background, background_ws)

            # Step 3. Perform dark current subtraction
            self._message += self._simple_execution("DarkCurrentAlgorithm", background_ws)

            # Step 4. Normalize to time or monitor
            self._message += self._simple_execution("NormaliseAlgorithm", background_ws)

            # Step 5. Mask detectors
            self._message += self._simple_execution("MaskAlgorithm", background_ws)

            # Step 6. Perform solid angle correction
            self._message += self._simple_execution("SANSSolidAngleCorrection", background_ws)

            # Step 7. Calculate and apply background transmission correction
            trans_beam_center_x = None
            trans_beam_center_y = None
            if "BckTransmissionBeamCenterAlgorithm" in self._property_list:
                # Execute the beam finding algorithm and set the beam
                # center for the background transmission calculation
                p=self._property_manager.getProperty("BckTransmissionBeamCenterAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", self._property_manager_name)
                alg.execute()
                trans_beam_center_x = alg.getProperty("FoundBeamCenterX").value
                trans_beam_center_y = alg.getProperty("FoundBeamCenterY").value

            if "BckTransmissionAlgorithm" in self._property_list:
                # Calculate and apply background transmission correction
                p=self._property_manager.getProperty("BckTransmissionAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                alg.setProperty("InputWorkspace", background_ws)
                alg.setProperty("OutputWorkspace", background_ws)

                if alg.existsProperty("BeamCenterX") \
                        and alg.existsProperty("BeamCenterY") \
                        and trans_beam_center_x is not None \
                        and trans_beam_center_y is not None:
                    alg.setProperty("BeamCenterX", trans_beam_center_x)
                    alg.setProperty("BeamCenterY", trans_beam_center_y)

                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", self._property_manager_name)
                alg.execute()

                if alg.existsProperty("MeasuredTransmission"):
                    meas_trans = alg.getProperty("MeasuredTransmission").value
                    if self._property_manager.existsProperty("MeasuredBckTransmissionValue"):
                        self._property_manager.setProperty("MeasuredBckTransmissionValue", meas_trans)
                    else:
                        self._property_manager.declareProperty("MeasuredBckTransmissionValue", meas_trans)
                if alg.existsProperty("MeasuredError"):
                    meas_err = alg.getProperty("MeasuredError").value
                    if self._property_manager.existsProperty("MeasuredBckTransmissionError"):
                        self._property_manager.setProperty("MeasuredBckTransmissionError", meas_err)
                    else:
                        self._property_manager.declareProperty("MeasuredBckTransmissionError", meas_err)

                if alg.existsProperty("OutputMessage"):
                    self._message += alg.getProperty("OutputMessage").value+'\n'
                else:
                    self._message += "Transmission correction applied\n"

            # Step 8: Subtract the background from the sample
            RebinToWorkspace(WorkspaceToRebin=background_ws,
                             WorkspaceToMatch=sample_ws,
                             OutputWorkspace=background_ws)
            Minus(LHSWorkspace=sample_ws,
                  RHSWorkspace=background_ws,
                  OutputWorkspace=sample_ws)

            self._message += "Subtracted background %s from sample %s\n   " % (background_ws, sample_ws)

        # Step 9: Apply sensitivity correction
        self._message += self._apply_sensitivity(sample_ws)

        # Step 10: Geometry correction (normalise by thickness)
        self._message += self._normalise_by_thickness(sample_ws)

        # Step 11: Absolute scale correction (flux normalisation method)
        self._message += self._simple_execution("AbsoluteScaleAlgorithm", sample_ws)

        # Step 12: Calculate the automatic Q range
        CalculateQMinMax(Workspace = sample_ws)

        # Compute I(q)
        if "IQAlgorithm" in self._property_list:
            iq_output = self.getPropertyValue("OutputWorkspace") + '_iq'
            p=self._property_manager.getProperty("IQAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", sample_ws)
            alg.setProperty("OutputWorkspace", iq_output)
            alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                self._message += alg.getProperty("OutputMessage").value+'\n'

        # Compute I(qx,qy)
        if "IQXYAlgorithm" in self._property_list:
            iq_output_name = self.getPropertyValue("OutputWorkspace")+'_iqxy'
            p=self._property_manager.getProperty("IQXYAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", sample_ws)
            alg.setProperty("OutputWorkspace", iq_output_name)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                self._message += alg.getProperty("OutputMessage").value+'\n'

        self.setProperty("OutputMessage", self._message)
        RenameWorkspace(InputWorkspace=sample_ws, OutputWorkspace=self._output_ws)
        self.setProperty("OutputWorkspace", self._output_ws)

    def _normalise_by_thickness(self, workspace):

        output_msg = ""
        if "GeometryAlgorithm" in self._property_list:
            norm_alg = self._property_manager.getPropertyValue("GeometryAlgorithm")
            alg = Algorithm.fromString(norm_alg)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            thickness = self.getProperty("SampleThickness").value
            if thickness != 0.:
                # overwrite the setting from the configuration
                alg.setProperty("SampleThickness", thickness)
            alg.execute()
            output_msg = "Normalising by sample thickness\n"
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getPropertyValue("OutputMessage") + "\n"
        return output_msg

    def _apply_sensitivity(self, workspace):

        output_msg = ""
        # Sensitivity correction
        if "SensitivityAlgorithm" in self._property_list:
            # Beam center for the sensitivity correction
            beam_center_x = None
            beam_center_y = None
            if "SensitivityBeamCenterAlgorithm" in self._property_list:
                # Execute the beam finding algorithm and set the beam
                # center for the transmission calculation
                p=self._property_manager.getProperty("SensitivityBeamCenterAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", self._property_manager_name)
                alg.execute()
                beam_center_x = alg.getProperty("FoundBeamCenterX").value
                beam_center_y = alg.getProperty("FoundBeamCenterY").value

            p=self._property_manager.getProperty("SensitivityAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)

            if alg.existsProperty("BeamCenterX") \
                    and alg.existsProperty("BeamCenterY") \
                    and beam_center_x is not None \
                    and beam_center_y is not None:
                alg.setProperty("BeamCenterX", beam_center_x)
                alg.setProperty("BeamCenterY", beam_center_y)

            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

            # Store sensitivity beam center so that we can access it later
            if beam_center_x is not None and beam_center_y is not None:
                if self._property_manager.existsProperty("SensitivityBeamCenterXUsed"):
                    self._property_manager.setProperty("SensitivityBeamCenterXUsed", beam_center_x)
                else:
                    self._property_manager.declareProperty("SensitivityBeamCenterXUsed", beam_center_y)
                if property_manager.existsProperty("SensitivityBeamCenterYUsed"):
                    self._property_manager.setProperty("SensitivityBeamCenterYUsed", beam_center_y)
                else:
                    self._property_manager.declareProperty("SensitivityBeamCenterYUsed", beam_center_y)

        return output_msg

    def _simple_execution(self, algorithm_name, workspace, output_workspace = None):
        """
            Simple execution of an algorithm on the given workspace
            @param algorithm_name : the name of the algorithm
            @param workspace : the input workspace
            @param output_workspace :  the output workspace if different from input
            @returns : the output message of the execution
        """
        output_msg = ""
        if output_workspace is None:
            output_workspace = workspace
        if self._property_manager.existsProperty(algorithm_name):
            p = self._property_manager.getProperty(algorithm_name)
            alg = Algorithm.fromString(p.valueAsStr)
            if alg.existsProperty("InputWorkspace"):
                alg.setProperty("InputWorkspace", workspace)
                if alg.existsProperty("OutputWorkspace"):
                    alg.setProperty("OutputWorkspace", output_workspace)
            else:
                alg.setProperty("Workspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", self._property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg = alg.getPropertyValue("OutputMessage")+'\n'
        return output_msg

# Subscribe the algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSReduction)
