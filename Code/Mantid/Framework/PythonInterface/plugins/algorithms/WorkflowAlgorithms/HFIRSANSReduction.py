#pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.find_data import find_data
import os

class HFIRSANSReduction(PythonAlgorithm):

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "HFIRSANSReduction"

    def summary(self):
        return "HFIR SANS reduction workflow."

    def PyInit(self):
        self.declareProperty('Filename', '', doc='List of input file paths')
        self.declareProperty('ReductionProperties', '__sans_reduction_properties', validator=StringMandatoryValidator(), doc='Property manager name for the reduction')
        self.declareProperty('OutputWorkspace', '', doc='Reduced workspace')
        self.declareProperty('OutputMessage', '', direction=Direction.Output, doc='Output message')

    def _multiple_load(self, data_file, workspace,
                       property_manager, property_manager_name):
        # Check whether we have a list of files that need merging
        #   Make sure we process a list of files written as a string
        def _load_data(filename, output_ws):
            if not property_manager.existsProperty("LoadAlgorithm"):
                raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
            p=property_manager.getProperty("LoadAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("Filename", filename)
            alg.setProperty("OutputWorkspace", output_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            msg = "Loaded %s\n" % filename
            if alg.existsProperty("OutputMessage"):
                msg = alg.getProperty("OutputMessage").value
            return msg

        # Get instrument to use with FileFinder
        instrument = ''
        if property_manager.existsProperty("InstrumentName"):
            instrument = property_manager.getProperty("InstrumentName").value

        output_str = ''
        if type(data_file)==str:
            data_file = find_data(data_file, instrument=instrument, allow_multiple=True)
        if type(data_file)==list:
            monitor = 0.0
            timer = 0.0
            for i in range(len(data_file)):
                if i==0:
                    output_str += _load_data(data_file[i], workspace)
                    # Use the first file location as the default output directory
                    head, tail = os.path.split(data_file[0])
                    if os.path.isdir(head):
                        self.default_output_dir = head
                else:
                    output_str += _load_data(data_file[i], '__tmp_wksp')
                    api.Plus(LHSWorkspace=workspace,\
                         RHSWorkspace='__tmp_wksp',\
                         OutputWorkspace=workspace)
                    # Get the monitor and timer values
                    ws = AnalysisDataService.retrieve('__tmp_wksp')
                    monitor += ws.getRun().getProperty("monitor").value
                    timer += ws.getRun().getProperty("timer").value

            # Get the monitor and timer of the first file, which haven't yet
            # been added to the total
            ws = AnalysisDataService.retrieve(workspace)
            monitor += ws.getRun().getProperty("monitor").value
            timer += ws.getRun().getProperty("timer").value

            # Update the timer and monitor
            ws.getRun().addProperty("monitor", monitor, True)
            ws.getRun().addProperty("timer", timer, True)

            if AnalysisDataService.doesExist('__tmp_wksp'):
                AnalysisDataService.remove('__tmp_wksp')
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data(data_file, workspace)
            head, tail = os.path.split(data_file)
            if os.path.isdir(head):
                self.default_output_dir = head
        return output_str

    def PyExec(self):
        filename = self.getProperty("Filename").value
        output_ws = self.getPropertyValue("OutputWorkspace")
        #output_ws = '__'+output_ws+'_reduced'
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        property_list = [p.name for p in property_manager.getProperties()]

        # Keep track of best output directory guess in case it wasn't supplied
        self.default_output_dir = os.path.expanduser('~')

        output_msg = ""
        # Find the beam center
        if "SANSBeamFinderAlgorithm" in property_list:
            p=property_manager.getProperty("SANSBeamFinderAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Load the sample data
        msg = self._multiple_load(filename, output_ws,
                                  property_manager, property_manager_name)
        output_msg += "Loaded %s\n" % filename
        output_msg += msg

        # Perform the main corrections on the sample data
        output_msg += self.process_data_file(output_ws)

        # Sample data transmission correction
        beam_center_x = None
        beam_center_y = None
        if "TransmissionBeamCenterAlgorithm" in property_list:
            # Execute the beam finding algorithm and set the beam
            # center for the transmission calculation
            p=property_manager.getProperty("TransmissionBeamCenterAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            beam_center_x = alg.getProperty("FoundBeamCenterX").value
            beam_center_y = alg.getProperty("FoundBeamCenterY").value

        if "TransmissionAlgorithm" in property_list:
            p=property_manager.getProperty("TransmissionAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("OutputWorkspace", output_ws)

            if alg.existsProperty("BeamCenterX") \
                and alg.existsProperty("BeamCenterY") \
                and beam_center_x is not None \
                and beam_center_y is not None:
                alg.setProperty("BeamCenterX", beam_center_x)
                alg.setProperty("BeamCenterY", beam_center_y)

            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()

            if alg.existsProperty("MeasuredTransmission"):
                meas_trans = alg.getProperty("MeasuredTransmission").value
                if property_manager.existsProperty("MeasuredTransmissionValue"):
                    property_manager.setProperty("MeasuredTransmissionValue", meas_trans)
                else:
                    property_manager.declareProperty("MeasuredTransmissionValue", meas_trans)
            if alg.existsProperty("MeasuredError"):
                meas_err = alg.getProperty("MeasuredError").value
                if property_manager.existsProperty("MeasuredTransmissionError"):
                    property_manager.setProperty("MeasuredTransmissionError", meas_err)
                else:
                    property_manager.declareProperty("MeasuredTransmissionError", meas_err)

            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Process background data
        if "BackgroundFiles" in property_list:
            background = property_manager.getProperty("BackgroundFiles").value
            background_ws = "__background_%s" % output_ws
            msg = self._multiple_load(background, background_ws,\
                                property_manager, property_manager_name)
            bck_msg = "Loaded background %s\n" % background
            bck_msg += msg

            # Process background like we processed the sample data
            bck_msg += self.process_data_file(background_ws)

            trans_beam_center_x = None
            trans_beam_center_y = None
            if "BckTransmissionBeamCenterAlgorithm" in property_list:
                # Execute the beam finding algorithm and set the beam
                # center for the transmission calculation
                p=property_manager.getProperty("BckTransmissionBeamCenterAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                trans_beam_center_x = alg.getProperty("FoundBeamCenterX").value
                trans_beam_center_y = alg.getProperty("FoundBeamCenterY").value

            # Background transmission correction
            if "BckTransmissionAlgorithm" in property_list:
                p=property_manager.getProperty("BckTransmissionAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                alg.setProperty("InputWorkspace", background_ws)
                alg.setProperty("OutputWorkspace", '__'+background_ws+"_reduced")

                if alg.existsProperty("BeamCenterX") \
                    and alg.existsProperty("BeamCenterY") \
                    and trans_beam_center_x is not None \
                    and trans_beam_center_y is not None:
                    alg.setProperty("BeamCenterX", trans_beam_center_x)
                    alg.setProperty("BeamCenterY", trans_beam_center_y)

                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()

                if alg.existsProperty("MeasuredTransmission"):
                    meas_trans = alg.getProperty("MeasuredTransmission").value
                    if property_manager.existsProperty("MeasuredBckTransmissionValue"):
                        property_manager.setProperty("MeasuredBckTransmissionValue", meas_trans)
                    else:
                        property_manager.declareProperty("MeasuredBckTransmissionValue", meas_trans)
                if alg.existsProperty("MeasuredError"):
                    meas_err = alg.getProperty("MeasuredError").value
                    if property_manager.existsProperty("MeasuredBckTransmissionError"):
                        property_manager.setProperty("MeasuredBckTransmissionError", meas_err)
                    else:
                        property_manager.declareProperty("MeasuredBckTransmissionError", meas_err)

                if alg.existsProperty("OutputMessage"):
                    output_msg += alg.getProperty("OutputMessage").value+'\n'
                background_ws = '__'+background_ws+'_reduced'

            # Subtract background
            api.RebinToWorkspace(WorkspaceToRebin=background_ws,
                                 WorkspaceToMatch=output_ws,
                                 OutputWorkspace=background_ws+'_rebin',
                                 PreserveEvents=False)
            api.Minus(LHSWorkspace=output_ws,\
                         RHSWorkspace=background_ws,\
                         OutputWorkspace=output_ws)

            bck_msg = bck_msg.replace('\n','\n   |')
            output_msg += "Background subtracted [%s]%s\n" % (background_ws, bck_msg)

        # Absolute scale correction
        output_msg += self._simple_execution("AbsoluteScaleAlgorithm", output_ws)

        # Geometry correction
        output_msg += self._simple_execution("GeometryAlgorithm", output_ws)

        # Compute I(q)
        iq_output = None
        if "IQAlgorithm" in property_list:
            iq_output = self.getPropertyValue("OutputWorkspace")
            iq_output = iq_output+'_Iq'
            p=property_manager.getProperty("IQAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("OutputWorkspace", iq_output)
            alg.setProperty("ReductionProperties", property_manager_name)
            if alg.existsProperty("WedgeWorkspace"):
                alg.setProperty("WedgeWorkspace", iq_output+'_wedges')
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Compute I(qx,qy)
        iqxy_output = None
        if "IQXYAlgorithm" in property_list:
            iq_output_name = self.getPropertyValue("OutputWorkspace")
            iqxy_output = iq_output_name+'_Iqxy'
            p=property_manager.getProperty("IQXYAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("OutputWorkspace", iq_output_name)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Verify output directory and save data
        output_dir = ''
        if "OutputDirectory" in property_list:
            output_dir = property_manager.getProperty("OutputDirectory").value
        if len(output_dir)==0:
            output_dir = self.default_output_dir

        if os.path.isdir(output_dir):
            output_msg += self._save_output(iq_output, iqxy_output,
                                            output_dir, property_manager)
            Logger("HFIRSANSReduction").notice("Output saved in %s" % output_dir)
        elif len(output_dir)>0:
            msg = "Output directory doesn't exist: %s\n" % output_dir
            Logger("HFIRSANSReduction").error(msg)

        self.setProperty("OutputMessage", output_msg)

    def process_data_file(self, workspace):
        output_msg = ""
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        property_list = [p.name for p in property_manager.getProperties()]

        # Dark current subtraction
        output_msg += self._simple_execution("DarkCurrentAlgorithm", workspace)

        # Normalize
        output_msg += self._simple_execution("NormaliseAlgorithm", workspace)

        # Mask
        output_msg += self._simple_execution("MaskAlgorithm", workspace)

        # Solid angle correction
        output_msg += self._simple_execution("SANSSolidAngleCorrection", workspace)

        # Sensitivity correction
        if "SensitivityAlgorithm" in property_list:
            # Beam center for the sensitivity correction
            beam_center_x = None
            beam_center_y = None
            if "SensitivityBeamCenterAlgorithm" in property_list:
                # Execute the beam finding algorithm and set the beam
                # center for the transmission calculation
                p=property_manager.getProperty("SensitivityBeamCenterAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                beam_center_x = alg.getProperty("FoundBeamCenterX").value
                beam_center_y = alg.getProperty("FoundBeamCenterY").value

            p=property_manager.getProperty("SensitivityAlgorithm")
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
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

            # Store sensitivity beam center so that we can access it later
            if beam_center_x is not None and beam_center_y is not None:
                if property_manager.existsProperty("SensitivityBeamCenterXUsed"):
                    property_manager.setProperty("SensitivityBeamCenterXUsed", beam_center_x)
                else:
                    property_manager.declareProperty("SensitivityBeamCenterXUsed", beam_center_y)
                if property_manager.existsProperty("SensitivityBeamCenterYUsed"):
                    property_manager.setProperty("SensitivityBeamCenterYUsed", beam_center_y)
                else:
                    property_manager.declareProperty("SensitivityBeamCenterYUsed", beam_center_y)

        return output_msg

    def _simple_execution(self, algorithm_name, workspace, output_workspace=None):
        """
            Simple execution of an algorithm on the given workspace
        """
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        output_msg = ""
        if output_workspace is None:
            output_workspace = workspace

        if property_manager.existsProperty(algorithm_name):
            p=property_manager.getProperty(algorithm_name)
            alg=Algorithm.fromString(p.valueAsStr)
            if alg.existsProperty("InputWorkspace"):
                alg.setProperty("InputWorkspace", workspace)
                if alg.existsProperty("OutputWorkspace"):
                    alg.setProperty("OutputWorkspace", output_workspace)
            else:
                alg.setProperty("Workspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg = alg.getProperty("OutputMessage").value+'\n'
        return output_msg

    def _save_output(self, iq_output, iqxy_output, output_dir, property_manager):
        """
            Save the I(Q) and I(QxQy) output to file.

            @param iq_output: name of the I(Q) workspace
            @param iqxy_output: name of the I(QxQy) workspace
            @param output_dir: output director path
            @param property_manager: property manager object
        """
        output_msg = ""

        def _save_ws(iq_ws):
            if AnalysisDataService.doesExist(iq_ws):
                proc_xml = ""
                if property_manager.existsProperty("ProcessInfo"):
                    process_file = property_manager.getProperty("ProcessInfo").value
                    if os.path.isfile(process_file):
                        proc = open(process_file, 'r')
                        proc_xml = proc.read()
                    elif len(process_file)>0:
                        Logger("HFIRSANSReduction").error("Could not read %s\n" % process_file)
                if property_manager.existsProperty("SetupAlgorithm"):
                    setup_info = property_manager.getProperty("SetupAlgorithm").value
                    proc_xml += "\n<Reduction>\n"
                        # The instrument name refers to the UI, which is named BIOSANS for all HFIR SANS
                    proc_xml += "  <instrument_name>BIOSANS</instrument_name>\n"
                    proc_xml += "  <SetupInfo>%s</SetupInfo>\n" % setup_info
                    filename = self.getProperty("Filename").value
                    proc_xml += "  <Filename>%s</Filename>\n" % filename
                    proc_xml += "</Reduction>\n"

                filename = os.path.join(output_dir, iq_ws+'.txt')

                alg = AlgorithmManager.create("SaveAscii")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("Filename", filename)
                alg.setProperty("InputWorkspace", iq_ws)
                alg.setProperty("Separator", "Tab")
                alg.setProperty("CommentIndicator", "# ")
                alg.setProperty("WriteXError", True)
                alg.setProperty("WriteSpectrumID", False)
                alg.execute()

                filename = os.path.join(output_dir, iq_ws+'.xml')
                alg = AlgorithmManager.create("SaveCanSAS1D")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("Filename", filename)
                alg.setProperty("InputWorkspace", iq_ws)
                alg.setProperty("Process", proc_xml)
                alg.execute()
                return filename
            else:
                Logger("HFIRSANSReduction").error("No I(Q) output found for %s" % iq_ws)

        # Save I(Q), including all wedges
        ws_list = AnalysisDataService.getObjectNames()
        for item in ws_list:
            if iq_output is not None and item.startswith(iq_output) and not item.startswith(iqxy_output):
                filename = _save_ws(item)
                if filename is not None:
                    output_msg += "I(Q) saved in %s\n" % (filename)

        # Save I(Qx,Qy)
        if iqxy_output is not None:
            if AnalysisDataService.doesExist(iqxy_output):
                filename = os.path.join(output_dir, iqxy_output+'.dat')
                alg = AlgorithmManager.create("SaveNISTDAT")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("Filename", filename)
                alg.setProperty("InputWorkspace", iqxy_output)
                alg.execute()
                #api.SaveNISTDAT(InputWorkspace=iqxy_output, Filename=filename)
                output_msg += "I(Qx,Qy) saved in %s\n" % (filename)
            else:
                Logger("HFIRSANSReduction").error("No I(Qx,Qy) output found")

        return output_msg

#############################################################################################

AlgorithmFactory.subscribe(HFIRSANSReduction)
