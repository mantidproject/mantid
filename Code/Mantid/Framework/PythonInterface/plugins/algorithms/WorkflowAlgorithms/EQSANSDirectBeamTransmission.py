from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi as api
import os

class EQSANSDirectBeamTransmission(PythonAlgorithm):

    def category(self):
        return 'Workflow\\SANS\\UsesPropertyManager'

    def name(self):
        return 'EQSANSDirectBeamTransmission'

    def summary(self):
        return "Compute the transmission using the direct beam method on EQSANS"

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
        self.declareProperty("UseSampleDarkCurrent", True,
                             "If true, the sample dark current will be used")
        self.declareProperty("BeamCenterX", 0.0, "Beam center position in X")
        self.declareProperty("BeamCenterY", 0.0, "Beam center position in Y")
        self.declareProperty("FitFramesTogether", False,
                             "If true, the two frames will be fit together")
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
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        workspace = self.getProperty('InputWorkspace').value

        # Perform transmission correction according to whether or not
        # we are in frame-skipping mode
        if self.getProperty('FitFramesTogether').value or \
            not workspace.getRun().hasProperty('is_frame_skipping') or \
            (workspace.getRun().hasProperty('is_frame_skipping') \
            and workspace.getRun().getProperty('is_frame_skipping').value == 0):
            output_ws_name = self.getPropertyValue('OutputWorkspace')
            msg, ws, trans_ws, trans_name, raw_ws, raw_name = self._call_sans_transmission(workspace, output_ws_name)
            self.setPropertyValue("OutputMessage", msg)
            self.setProperty("OutputWorkspace", ws)
            if trans_ws is not None:
                self.setPropertyValue("TransmissionWorkspace", trans_name)
                self.setProperty("TransmissionWorkspace", trans_ws)
            if raw_ws is not None:
                self.setPropertyValue("RawTransmissionWorkspace", raw_name)
                self.setProperty("RawTransmissionWorkspace", raw_ws)

            # Save the transmission to disk
            self._save_transmission(trans_ws, raw_ws)
        else:
            ws = self._with_frame_skipping(workspace)
            self.setProperty("OutputWorkspace", ws)

    def _call_sans_transmission(self, workspace, output_workspace_name):
        """
            Call the generic transmission correction for SANS
            @param workspace: workspace to correct
            @param output_workspace_name: name of the output workspace
        """
        alg = AlgorithmManager.create('SANSDirectBeamTransmission')
        alg.initialize()
        alg.setChild(True)
        alg.setProperty('InputWorkspace', workspace)
        sample_data_file = self.getProperty("SampleDataFilename").value
        alg.setProperty("SampleDataFilename", sample_data_file)
        empty_data_file = self.getProperty("EmptyDataFilename").value
        alg.setProperty("EmptyDataFilename", empty_data_file)
        beam_radius = self.getProperty("BeamRadius").value
        alg.setProperty("BeamRadius", beam_radius)
        theta_dependent = self.getProperty("ThetaDependent").value
        alg.setProperty("ThetaDependent", theta_dependent)
        dark_current_file = self.getProperty("DarkCurrentFilename").value
        alg.setProperty("DarkCurrentFilename", dark_current_file)
        use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
        alg.setProperty("UseSampleDarkCurrent", use_sample_dc)
        center_x = self.getProperty("BeamCenterX").value
        alg.setProperty("BeamCenterX", center_x)
        center_y = self.getProperty("BeamCenterY").value
        alg.setProperty("BeamCenterY", center_y)
        red_props = self.getProperty("ReductionProperties").value
        alg.setProperty("ReductionProperties", red_props)
        alg.setPropertyValue("OutputWorkspace", output_workspace_name)
        alg.execute()
        if alg.existsProperty('OutputMessage'):
            output_msg = alg.getProperty('OutputMessage').value
        else:
            output_msg = None
        output_ws = alg.getProperty('OutputWorkspace').value

        if alg.existsProperty('TransmissionWorkspace'):
            trans_ws = alg.getProperty('TransmissionWorkspace').value
            trans_name = alg.getPropertyValue('TransmissionWorkspace')
        else:
            trans_ws = None
            trans_name = ''

        if alg.existsProperty('RawTransmissionWorkspace'):
            raw_ws = alg.getProperty('RawTransmissionWorkspace').value
            raw_name = alg.getPropertyValue('RawTransmissionWorkspace')
        else:
            raw_ws = None
            raw_name = ''
        return (output_msg, output_ws, trans_ws, trans_name, raw_ws, raw_name)

    def _with_frame_skipping(self, workspace):
        """
            Perform transmission correction assuming frame-skipping
        """
        import TransmissionUtils
        input_ws_name = self.getPropertyValue('InputWorkspace')
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
            trans_ws_name = "__transmission_fit_"+input_ws_name
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID = TransmissionUtils.load_monitors(self, property_manager)

            def _crop_and_compute(wl_min_prop, wl_max_prop, suffix):
                # Get the wavelength band from the run properties
                if workspace.getRun().hasProperty(wl_min_prop):
                    wl_min = workspace.getRun().getProperty(wl_min_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_min_prop

                if workspace.getRun().hasProperty(wl_max_prop):
                    wl_max = workspace.getRun().getProperty(wl_max_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_max_prop

                rebin_params = "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max)
                alg = TransmissionUtils.simple_algorithm("Rebin",
                                                   {"InputWorkspace": sample_mon_ws,
                                                    "OutputWorkspace": "__sample_mon_"+suffix,
                                                    "Params": rebin_params,
                                                    "PreserveEvents": False
                                                    })
                sample_ws = alg.getProperty("OutputWorkspace").value
                alg = TransmissionUtils.simple_algorithm("Rebin",
                                                   {"InputWorkspace": empty_mon_ws,
                                                    "OutputWorkspace": "__empty_mon_"+suffix,
                                                    "Params": rebin_params,
                                                    "PreserveEvents": False
                                                    })
                empty_ws = alg.getProperty("OutputWorkspace").value
                trans_ws, raw_ws = TransmissionUtils.calculate_transmission(self,
                                                                            sample_ws,
                                                                            empty_ws,
                                                                            first_det,
                                                                            "__transmission_"+suffix)
                alg = TransmissionUtils.simple_algorithm("RebinToWorkspace",
                                                   {"WorkspaceToRebin": trans_ws,
                                                    "WorkspaceToMatch": workspace,
                                                    "OutputWorkspace": "__transmission_"+suffix,
                                                    "PreserveEvents": False
                                                    })
                trans_ws = alg.getProperty("OutputWorkspace").value
                alg = TransmissionUtils.simple_algorithm("RebinToWorkspace",
                                                   {"WorkspaceToRebin": raw_ws,
                                                    "WorkspaceToMatch": workspace,
                                                    "OutputWorkspace": "__transmission_unfitted_"+suffix,
                                                    "PreserveEvents": False
                                                    })
                raw_ws = alg.getProperty("OutputWorkspace").value

                return trans_ws, raw_ws

            # First frame
            trans_frame_1, raw_frame_1 = _crop_and_compute("wavelength_min", "wavelength_max", "_frame1")

            # Second frame
            trans_frame_2, raw_frame_2 = _crop_and_compute("wavelength_min_frame2", "wavelength_max_frame2", "_frame2")

            alg = TransmissionUtils.simple_algorithm("Plus",
                                               {"LHSWorkspace": trans_frame_1,
                                                "RHSWorkspace": trans_frame_2,
                                                "OutputWorkspace": "__transmission",
                                                })
            trans_ws = alg.getProperty("OutputWorkspace").value
            self.setPropertyValue("TransmissionWorkspace", trans_ws_name)
            self.setProperty("TransmissionWorkspace", trans_ws)

            alg = TransmissionUtils.simple_algorithm("Plus",
                                               {"LHSWorkspace": raw_frame_1,
                                                "RHSWorkspace": raw_frame_2,
                                                "OutputWorkspace": "__transmission_unfitted",
                                                })
            raw_ws = alg.getProperty("OutputWorkspace").value
            raw_ws_name = "__transmission_raw_%s" % input_ws_name
            self.setPropertyValue("RawTransmissionWorkspace", raw_ws_name)
            self.setProperty("RawTransmissionWorkspace", raw_ws)

            # Save the transmission to disk
            self._save_transmission(trans_ws, raw_ws)

        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        return TransmissionUtils.apply_transmission(self, workspace, trans_ws)

    def _save_transmission(self, trans_ws, raw_ws):
        """
            Save the transmission data and fit to disk.
            @param trans_ws: transmission workspace
            @param raw_ws: transmission fit workspace
        """
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)

        output_ws_name = self.getPropertyValue('OutputWorkspace')
        if property_manager.existsProperty("OutputDirectory"):
            output_dir = property_manager.getProperty("OutputDirectory").value
            if os.path.isdir(output_dir):
                if raw_ws is not None:
                    filename = os.path.join(output_dir, output_ws_name+'_transmission.txt')
                    alg = AlgorithmManager.create("SaveAscii")
                    alg.initialize()
                    alg.setChild(True)
                    alg.setProperty("Filename", filename)
                    alg.setProperty("InputWorkspace", raw_ws)
                    alg.setProperty("Separator", "Tab")
                    alg.setProperty("CommentIndicator", "# ")
                    alg.setProperty("WriteSpectrumID", False)
                    alg.execute()

                if trans_ws is not None:
                    filename = os.path.join(output_dir, output_ws_name+'_transmission_fit.txt')
                    alg = AlgorithmManager.create("SaveAscii")
                    alg.initialize()
                    alg.setChild(True)
                    alg.setProperty("Filename", filename)
                    alg.setProperty("InputWorkspace", trans_ws)
                    alg.setProperty("Separator", "Tab")
                    alg.setProperty("CommentIndicator", "# ")
                    alg.setProperty("WriteSpectrumID", False)
                    alg.execute()
            else:
                msg = "Output directory doesn't exist: %s\n" % output_dir
                Logger(__file__).error(msg)
        else:
            Logger(__file__).error("Could not find output directory")

AlgorithmFactory.subscribe(EQSANSDirectBeamTransmission)
