import os
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.instruments.sans import hfir_instrument
from reduction_workflow.find_data import find_data

class SANSAbsoluteScale(PythonAlgorithm):
    """
        Normalise detector counts by the sample thickness
    """

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSAbsoluteScale"

    def summary(self):
        return "Calculate and apply absolute scale correction for SANS data"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output))

        methods = [ "Value", "ReferenceData"]
        self.declareProperty("Method", "Value",
                             StringListValidator(methods),
                             "Scaling method - either a simple scaling by value or using a reference data set")

        self.declareProperty("ScalingFactor", 1.0, "Scaling factor to use with the Value method")

        self.declareProperty(FileProperty("ReferenceDataFilename", "",
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml', 'nxs', 'nxs.h5']),
                             "Reference data file to compute the scaling factor")
        self.declareProperty("BeamstopDiameter", 0.0, "Diameter of the beam on the detector, in mm")
        self.declareProperty("AttenuatorTransmission", 1.0,
                             "Attenuator transmission used in the measurement")
        self.declareProperty("ApplySensitivity", False,
                             "If True, the sensitivity correction will be applied to the reference data set")

        self.declareProperty("ReductionProperties", "__sans_reduction_properties",
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")

        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        # Get instrument to use with FileFinder
        self.instrument = ''
        if property_manager.existsProperty("InstrumentName"):
            self.instrument = property_manager.getProperty("InstrumentName").value

        method = self.getPropertyValue("Method")
        if method=="Value":
            input_ws = self.getProperty("InputWorkspace").value
            output_ws_name = self.getPropertyValue("OutputWorkspace")
            scaling_factor = self.getProperty("ScalingFactor").value

            alg = AlgorithmManager.create("Scale")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", input_ws)
            alg.setProperty("OutputWorkspace", output_ws_name)
            alg.setProperty("Factor", scaling_factor)
            alg.setPropertyValue("Operation", "Multiply")
            alg.execute()
            output_ws = alg.getProperty("OutputWorkspace").value

            self.setProperty("OutputWorkspace", output_ws)
            self.setProperty("OutputMessage", "Applied scaling factor %g" % scaling_factor)

        elif self.instrument.lower() in ['biosans', 'gpsans', 'hfirsans']:
            self._hfir_scaling(property_manager)
        else:
            msg = "Absolute scale calculation with a reference is only available for HFIR"
            Logger("SANSAbsoluteScale").error(msg)
            self.setProperty("OutputMessage", msg)
            return

    def _hfir_scaling(self, property_manager):
        property_manager_name = self.getProperty("ReductionProperties").value
        input_ws = self.getProperty("InputWorkspace").value
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        output_msg = ""

        # Load data file
        data_file = self.getProperty("ReferenceDataFilename").value
        filepath = find_data(data_file, instrument=self.instrument)

        ref_basename = os.path.basename(filepath)
        ref_ws_name = "__abs_scale_%s" % ref_basename

        def _load_data(filename, output_ws):
            if not property_manager.existsProperty("LoadAlgorithm"):
                Logger("SANSDirectBeamTransmission").error("SANS reduction not set up properly: missing load algorithm")
                raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
            p=property_manager.getProperty("LoadAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setChild(True)
            alg.setProperty("Filename", filename)
            alg.setProperty("OutputWorkspace", output_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            msg = ''
            if alg.existsProperty("OutputMessage"):
                msg = alg.getProperty("OutputMessage").value
            ws = alg.getProperty("OutputWorkspace").value
            return ws, msg

        ref_ws, msg = _load_data(filepath, ref_ws_name)
        output_msg += msg+'\n'

        # Get monitor value:
        # This call is left unprotected because it should fail if that property
        # doesn't exist. It's the responsibility of the parent algorithm to
        # catch that error.
        monitor_prop = property_manager.getProperty("NormaliseAlgorithm")
        alg=Algorithm.fromString(monitor_prop.valueAsStr)
        monitor_id = alg.getPropertyValue("NormalisationType").lower()

        monitor_value = ref_ws.getRun().getProperty(monitor_id.lower()).value
        # HFIR-specific: If we count for monitor we need to multiply by 1e8
        # Need to be consistent with the Normalization step
        if monitor_id == "monitor":
            monitor_value /= 1.0e8

        # Get sample-detector distance
        sdd = ref_ws.getRun().getProperty("sample_detector_distance").value

        # Get the beamstop diameter
        beam_diameter = self.getProperty("BeamstopDiameter").value
        if beam_diameter <= 0:
            if ref_ws.getRun().hasProperty("beam-diameter"):
                beam_diameter = ref_ws.getRun().getProperty("beam-diameter").value
                Logger("SANSAbsoluteScale").debug("Found beamstop diameter: %g" % beam_diameter)
            else:
                raise RuntimeError, "AbsoluteScale could not read the beam radius and none was provided"

        # Apply sensitivity correction
        apply_sensitivity = self.getProperty("ApplySensitivity").value
        if apply_sensitivity and property_manager.existsProperty("SensitivityAlgorithm"):
            p=property_manager.getProperty("SensitivityAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setChild(True)
            alg.setProperty("InputWorkspace", ref_ws)
            alg.setProperty("OutputWorkspace", ref_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Get the reference count
        Logger("SANSAbsoluteScale").information("Using beamstop diameter: %g" % beam_diameter)
        det_count = 1
        cylXML = '<infinite-cylinder id="asbsolute_scale">' + \
                   '<centre x="0.0" y="0.0" z="0.0" />' + \
                   '<axis x="0.0" y="0.0" z="1.0" />' + \
                   '<radius val="%12.10f" />' % (beam_diameter/2000.0) + \
                 '</infinite-cylinder>\n'

        alg = AlgorithmManager.create("FindDetectorsInShape")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("Workspace", ref_ws)
        alg.setPropertyValue("ShapeXML", cylXML)
        alg.execute()
        det_list = alg.getProperty("DetectorList").value
        det_list_str = alg.getPropertyValue("DetectorList")

        det_count_ws_name = "__absolute_scale"
        alg = AlgorithmManager.create("GroupDetectors")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", ref_ws)
        alg.setProperty("OutputWorkspace", det_count_ws_name)
        alg.setPropertyValue("KeepUngroupedSpectra", "0")
        alg.setPropertyValue("DetectorList", det_list_str)
        alg.execute()
        det_count_ws = alg.getProperty("OutputWorkspace").value
        det_count = det_count_ws.readY(0)[0]
        Logger("SANSAbsoluteScale").information("Reference detector counts: %g" % det_count)
        if det_count <= 0:
            Logger("SANSAbsoluteScale").error("Bad reference detector count: check your beam parameters")

        # Pixel size, in mm
        pixel_size_param = ref_ws.getInstrument().getNumberParameter("x-pixel-size")
        if pixel_size_param is not None:
            pixel_size = pixel_size_param[0]
        else:
            raise RuntimeError, "AbsoluteScale could not read the pixel size"

        attenuator_trans = self.getProperty("AttenuatorTransmission").value
        # (detector count rate)/(attenuator transmission)/(monitor rate)*(pixel size/SDD)**2
        scaling_factor = 1.0/(det_count/attenuator_trans/(monitor_value)*(pixel_size/sdd)*(pixel_size/sdd))

        # Apply the scaling factor
        alg = AlgorithmManager.create("Scale")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", input_ws)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.setProperty("Factor", scaling_factor)
        alg.setPropertyValue("Operation", "Multiply")
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value
        Logger("SANSAbsoluteScale").notice( "Applied scaling factor %15.15f" % scaling_factor)

        output_msg = output_msg.replace('\n','\n   |')
        output_msg = "Applied scaling factor %g\n%s" % (scaling_factor, output_msg)

        self.setProperty("OutputWorkspace", output_ws)
        self.setProperty("OutputMessage", output_msg)

AlgorithmFactory.subscribe(SANSAbsoluteScale())
