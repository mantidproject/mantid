import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

class SANSDirectBeamTransmission(PythonAlgorithm):

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSDirectBeamTransmission"

    def summary(self):
        return "Compute transmission using the direct beam method"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(FileProperty("SampleDataFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("EmptyDataFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("BeamRadius", 3.0, "Beam radius [pixels]")
        self.declareProperty("ThetaDependent", True,
                             "If true, a theta-dependent correction will be applied")
        self.declareProperty(FileProperty("DarkCurrentFilename", "",
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("UseSampleDarkCurrent", False,
                             "If true, the sample dark current will be used")
        self.declareProperty("BeamCenterX", 0.0, "Beam center position in X")
        self.declareProperty("BeamCenterY", 0.0, "Beam center position in Y")
        self.declareProperty("ReductionProperties", "__sans_reduction_properties",
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output),
                             "Workspace containing the data corrected for the transmission.")
        self.declareProperty(MatrixWorkspaceProperty("TransmissionWorkspace", "",
                                                     optional = PropertyMode.Optional,
                                                     direction = Direction.Output),
                             "Workspace containing the fitted transmission distribution.")
        self.declareProperty(MatrixWorkspaceProperty("RawTransmissionWorkspace", "",
                                                     optional = PropertyMode.Optional,
                                                     direction = Direction.Output),
                             "Workspace containing the transmission distribution before fitting.")
        self.declareProperty("MeasuredTransmission", 0.0,
                             direction=Direction.Output)
        self.declareProperty("MeasuredError", 0.0,
                             direction=Direction.Output)
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        import TransmissionUtils
        sample_file = self.getPropertyValue("SampleDataFilename")
        empty_file = self.getPropertyValue("EmptyDataFilename")

        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        # Build the name we are going to give the transmission workspace
        sample_basename = os.path.basename(sample_file)
        empty_basename = os.path.basename(empty_file)
        entry_name = "Transmission%s%s" % (sample_basename, empty_basename)
        trans_ws_name = "__transmission_fit_%s" % sample_basename
        trans_ws = None

        if property_manager.existsProperty(entry_name):
            trans_ws_name = property_manager.getProperty(entry_name)
            if AnalysisDataService.doesExist(trans_ws_name):
                trans_ws = AnalysisDataService.retrieve(trans_ws_name)

        if trans_ws is None:
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID = TransmissionUtils.load_monitors(self, property_manager)
            trans_ws, raw_ws = TransmissionUtils.calculate_transmission(self, sample_mon_ws, empty_mon_ws,
                                                                        first_det, trans_ws_name, monitor_det_ID)

        # 2- Apply correction (Note: Apply2DTransCorr)
        if trans_ws is not None:
            input_ws = self.getProperty("InputWorkspace").value

            output_ws = TransmissionUtils.apply_transmission(self, input_ws, trans_ws)

            trans = trans_ws.dataY(0)[0]
            error = trans_ws.dataE(0)[0]

            if len(trans_ws.dataY(0))==1:
                self.setProperty("MeasuredTransmission", trans)
                self.setProperty("MeasuredError", error)
                output_str = "%s   T = %6.2g += %6.2g\n" % (output_str, trans, error)

            self.setProperty("OutputWorkspace", output_ws)
            input_tr_name = self.getPropertyValue("TransmissionWorkspace")
            if len(input_tr_name.strip())==0:
                self.setPropertyValue("TransmissionWorkspace", trans_ws_name)
            self.setProperty("TransmissionWorkspace", trans_ws)

            if raw_ws is not None:
                raw_ws_name = "__transmission_raw_%s" % sample_basename
                self.setPropertyValue("RawTransmissionWorkspace", raw_ws_name)
                self.setProperty("RawTransmissionWorkspace", raw_ws)
            output_msg = "Transmission correction applied [%s]\n%s\n" % (trans_ws_name, output_str)
        else:
            output_msg = "Transmission correction had errors\n%s\n" % output_str

        self.setPropertyValue("OutputMessage", output_msg)

#############################################################################################

AlgorithmFactory.subscribe(SANSDirectBeamTransmission)
