#pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os
import sys
from reduction_workflow.find_data import find_data

class SANSBeamSpreaderTransmission(PythonAlgorithm):

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSBeamSpreaderTransmission"

    def summary(self):
        return "Compute transmission using the beam spreader method"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(FileProperty("SampleSpreaderFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("DirectSpreaderFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("SampleScatteringFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("DirectScatteringFilename", "",
                                          action=FileAction.Load,
                                          extensions=['xml', 'nxs', 'nxs.h5']))

        self.declareProperty("SpreaderTransmissionValue", 1.0,
                             "Transmission of the beam spreader")
        self.declareProperty("SpreaderTransmissionError", 0.0,
                             "Error on the transmission of the beam spreader")

        self.declareProperty("ThetaDependent", True,
                             "If true, a theta-dependent correction will be applied")
        self.declareProperty(FileProperty("DarkCurrentFilename", "",
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("UseSampleDarkCurrent", False,
                             "If true, the sample dark current will be used")

        self.declareProperty("ReductionProperties", "__sans_reduction_properties",
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output))
        self.declareProperty("MeasuredTransmission", 0.0,
                             direction=Direction.Output)
        self.declareProperty("MeasuredError", 0.0,
                             direction=Direction.Output)
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        # Get the reduction property manager
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        # Build the name we are going to give the transmission workspace
        sample_scatt = self.getPropertyValue("SampleScatteringFilename")
        sample_basename = os.path.basename(sample_scatt)
        entry_name = "TransmissionSpreader%s" % sample_scatt
        trans_ws_name = "__transmission_fit_%s" % sample_basename
        trans_ws = None

        # If we have already computed the transmission, used the
        # previously computed workspace
        if property_manager.existsProperty(entry_name):
            trans_ws_name = property_manager.getProperty(entry_name)
            if AnalysisDataService.doesExist(trans_ws_name):
                trans_ws = AnalysisDataService.retrieve(trans_ws_name)

        # Get instrument to use with FileFinder
        instrument = ''
        if property_manager.existsProperty("InstrumentName"):
            instrument = property_manager.getProperty("InstrumentName").value

        # Get the data loader
        def _load_data(filename, output_ws):
            if not property_manager.existsProperty("LoadAlgorithm"):
                Logger("SANSBeamSpreaderTransmission").error("SANS reduction not set up properly: missing load algorithm")
                raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
            p=property_manager.getProperty("LoadAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("Filename", filename)
            alg.setProperty("OutputWorkspace", output_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            msg = ''
            if alg.existsProperty("OutputMessage"):
                msg = alg.getProperty("OutputMessage").value
            return msg

        # Compute the transmission if we don't already have it
        if trans_ws is None:
            # Load data files
            sample_spreader_ws = "__trans_sample_spreader"
            direct_spreader_ws = "__trans_direct_spreader"
            sample_scatt_ws = "__trans_sample_scatt"
            direct_scatt_ws = "__trans_direct_scatt"

            sample_spread = self.getPropertyValue("SampleSpreaderFilename")
            direct_spread = self.getPropertyValue("DirectSpreaderFilename")
            direct_scatt = self.getPropertyValue("DirectScatteringFilename")

            ws_names = [[sample_spread, sample_spreader_ws],
                        [direct_spread, direct_spreader_ws],
                        [sample_scatt, sample_scatt_ws],
                        [direct_scatt, direct_scatt_ws]]
            dark_current_data = self.getPropertyValue("DarkCurrentFilename")

            for f in ws_names:
                filepath = find_data(f[0], instrument=instrument)
                _load_data(filepath, f[1])
                self._subtract_dark_current(f[1], property_manager)

            # Get normalization for transmission calculation
            monitor_det_ID = None
            if property_manager.existsProperty("TransmissionNormalisation"):
                sample_ws = AnalysisDataService.retrieve(sample_scatt_ws)
                if property_manager.getProperty("TransmissionNormalisation").value=="Monitor":
                    monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-monitor-spectrum")[0])
                else:
                    monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-timer-spectrum")[0])
            elif property_manager.existsProperty("NormaliseAlgorithm"):
                def _normalise(workspace):
                    p=property_manager.getProperty("NormaliseAlgorithm")
                    alg=Algorithm.fromString(p.valueAsStr)
                    alg.setProperty("InputWorkspace", workspace)
                    alg.setProperty("OutputWorkspace", workspace)
                    if alg.existsProperty("ReductionProperties"):
                        alg.setProperty("ReductionProperties", property_manager_name)
                    alg.execute()
                    msg = ''
                    if alg.existsProperty("OutputMessage"):
                        msg += alg.getProperty("OutputMessage").value+'\n'
                    return msg
                for f in ws_names:
                    _normalise(f[1])

            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            spreader_t_value = self.getPropertyValue("SpreaderTransmissionValue")
            spreader_t_error = self.getPropertyValue("SpreaderTransmissionError")

            alg = AlgorithmManager.createUnmanaged('CalculateTransmissionBeamSpreader')
            alg.initialize()
            alg.setProperty("SampleSpreaderRunWorkspace", sample_spreader_ws)
            alg.setProperty("DirectSpreaderRunWorkspace", direct_spreader_ws)
            alg.setProperty("SampleScatterRunWorkspace", sample_scatt_ws)
            alg.setProperty("DirectScatterRunWorkspace", direct_scatt_ws)
            alg.setProperty("IncidentBeamMonitor", monitor_det_ID)
            alg.setProperty("OutputWorkspace",trans_ws_name)
            alg.setProperty("SpreaderTransmissionValue",spreader_t_value)
            alg.setProperty("SpreaderTransmissionError",spreader_t_error)
            alg.execute()

            trans_ws = AnalysisDataService.retrieve(trans_ws_name)

            for f in ws_names:
                if AnalysisDataService.doesExist(f[1]):
                    AnalysisDataService.remove(f[1])

        # 2- Apply correction (Note: Apply2DTransCorr)
        input_ws_name = self.getPropertyValue("InputWorkspace")
        if not AnalysisDataService.doesExist(input_ws_name):
            Logger("SANSBeamSpreaderTransmission").error("Could not find input workspace")
        workspace = AnalysisDataService.retrieve(input_ws_name).getName()

        # Clone workspace to make boost-python happy
        api.CloneWorkspace(InputWorkspace=workspace,
                           OutputWorkspace='__'+workspace)
        workspace = '__'+workspace

        self._apply_transmission(workspace, trans_ws_name)

        trans = trans_ws.dataY(0)[0]
        error = trans_ws.dataE(0)[0]

        output_str = ''
        if len(trans_ws.dataY(0))==1:
            self.setProperty("MeasuredTransmission", trans)
            self.setProperty("MeasuredError", error)
            output_str = "\n%s   T = %6.2g += %6.2g\n" % (output_str, trans, error)
        output_msg = "Transmission correction applied [%s]%s\n" % (trans_ws_name, output_str)

        output_ws = AnalysisDataService.retrieve(workspace)
        self.setProperty("OutputWorkspace", output_ws)
        self.setPropertyValue("OutputMessage", output_msg)

    def _apply_transmission(self, workspace, trans_workspace):
        """
            Apply transmission correction
            @param workspace: workspace to apply correction to
            @param trans_workspace: workspace name for of the transmission
        """
        # Make sure the binning is compatible
        api.RebinToWorkspace(WorkspaceToRebin=trans_workspace,
                             WorkspaceToMatch=workspace,
                             OutputWorkspace=trans_workspace+'_rebin',
                             PreserveEvents=False)
        # Apply angle-dependent transmission correction using the zero-angle transmission
        theta_dependent = self.getProperty("ThetaDependent").value

        api.ApplyTransmissionCorrection(InputWorkspace=workspace,
                                        TransmissionWorkspace=trans_workspace+'_rebin',
                                        OutputWorkspace=workspace,
                                        ThetaDependent=theta_dependent)

        if AnalysisDataService.doesExist(trans_workspace+'_rebin'):
            AnalysisDataService.remove(trans_workspace+'_rebin')

    def _subtract_dark_current(self, workspace_name, property_manager):
        """
            Subtract the dark current
            @param workspace_name: name of the workspace to subtract from
            @param property_manager: property manager object
        """
        # Subtract dark current
        use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
        dark_current_data = self.getPropertyValue("DarkCurrentFilename")
        property_manager_name = self.getProperty("ReductionProperties").value

        dark_current_property = "DefaultDarkCurrentAlgorithm"
        def _dark(workspace, dark_current_property):
            if property_manager.existsProperty(dark_current_property):
                p=property_manager.getProperty(dark_current_property)
                # Dark current subtraction for sample data
                alg=Algorithm.fromString(p.valueAsStr)
                alg.setProperty("InputWorkspace", workspace)
                alg.setProperty("OutputWorkspace", workspace)
                alg.setProperty("Filename", dark_current_data)
                if alg.existsProperty("PersistentCorrection"):
                    alg.setProperty("PersistentCorrection", False)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                msg = "Dark current subtracted"
                if alg.existsProperty("OutputMessage"):
                    msg += alg.getProperty("OutputMessage").value
                return msg

        if use_sample_dc is True:
            _dark(workspace_name, "DarkCurrentAlgorithm")
        elif len(dark_current_data.strip())>0:
            _dark(workspace_name, "DefaultDarkCurrentAlgorithm")

#############################################################################################

AlgorithmFactory.subscribe(SANSBeamSpreaderTransmission)
