#pylint: disable=no-init
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.find_data import find_file
import os

class EQSANSNormalise(PythonAlgorithm):
    """
        Normalise detector counts by accelerator current and beam spectrum.
    """

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "EQSANSNormalise"

    def summary(self):
        return "Normalise detector counts by accelerator current and beam spectrum"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty("NormaliseToBeam", True,
                             "If true, the data will also be normalise by the beam profile")
        self.declareProperty(FileProperty("BeamSpectrumFile", "", action=FileAction.OptionalLoad),
                             "Beam spectrum to be used for normalisation [takes precedence over default]")
        self.declareProperty("NormaliseToMonitor", False,
                             "If true, the algorithm will look for a monitor workspace to use")
        self.declareProperty("ReductionProperties", "__sans_reduction_properties",
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction=Direction.Output))
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        # If we need to normalise by monitor, skip all other options
        if self.getProperty("NormaliseToMonitor").value:
            return self._normalise_to_monitor()

        workspace = self.getProperty("InputWorkspace").value
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        flux_data_path = None

        if self.getProperty("NormaliseToBeam").value:
            # If a spectrum file was supplied, check if it's a valid file path
            beam_spectrum_file = self.getPropertyValue("BeamSpectrumFile").strip()
            if len(beam_spectrum_file):
                if os.path.isfile(beam_spectrum_file):
                    flux_data_path = beam_spectrum_file
                else:
                    Logger("EQSANSNormalise").error("%s is not a file" % beam_spectrum_file)
            else:
                flux_files = find_file(filename="bl6_flux_at_sample", data_dir=None)
                if len(flux_files)>0:
                    flux_data_path = flux_files[0]
                    Logger("EQSANSNormalise").notice("Using beam flux file: %s" % flux_data_path)
                else:
                    Logger("EQSANSNormalise").notice("Could not find beam flux file!")

            if flux_data_path is not None:
                beam_flux_ws_name = "__beam_flux"
                alg = AlgorithmManager.create("LoadAscii")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("Filename", flux_data_path)
                alg.setProperty("OutputWorkspace", beam_flux_ws_name)
                alg.setProperty("Separator", "Tab")
                alg.setProperty("Unit", "Wavelength")
                alg.execute()
                beam_flux_ws = alg.getProperty("OutputWorkspace").value

                alg = AlgorithmManager.create("ConvertToHistogram")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("InputWorkspace", beam_flux_ws)
                alg.setProperty("OutputWorkspace", beam_flux_ws_name)
                alg.execute()
                beam_flux_ws = alg.getProperty("OutputWorkspace").value

                alg = AlgorithmManager.create("RebinToWorkspace")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("WorkspaceToRebin", beam_flux_ws)
                alg.setProperty("WorkspaceToMatch", workspace)
                alg.setProperty("OutputWorkspace", beam_flux_ws_name)
                alg.execute()
                beam_flux_ws = alg.getProperty("OutputWorkspace").value

                alg = AlgorithmManager.create("NormaliseToUnity")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("InputWorkspace", beam_flux_ws)
                alg.setProperty("OutputWorkspace", beam_flux_ws_name)
                alg.execute()
                beam_flux_ws = alg.getProperty("OutputWorkspace").value

                alg = AlgorithmManager.create("Divide")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("LHSWorkspace", workspace)
                alg.setProperty("RHSWorkspace", beam_flux_ws)
                alg.setProperty("OutputWorkspace", beam_flux_ws_name)
                alg.execute()
                workspace = alg.getProperty("OutputWorkspace").value

                workspace.getRun().addProperty("beam_flux_ws", beam_flux_ws_name, True)
            else:
                flux_data_path = "Could not find beam flux file!"

        alg = AlgorithmManager.create("NormaliseByCurrent")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", workspace)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.execute()
        workspace = alg.getProperty("OutputWorkspace").value
        workspace_name = alg.getPropertyValue("OutputWorkspace")
        self.setProperty("OutputMessage", "Data [%s] normalized to accelerator current\n   Beam flux file: %s" % (workspace_name, str(flux_data_path)))
        self.setProperty("OutputWorkspace", workspace)

    def _normalise_to_monitor(self):
        """
            Normalize
        """
        input_ws_name = self.getPropertyValue("InputWorkspace")
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        prop_mng = self.getPropertyValue("ReductionProperties")
        reference_flux = self.getPropertyValue("BeamSpectrumFile").strip()

        monitor_ws_name = input_ws_name+'_monitors'
        if AnalysisDataService.doesExist(monitor_ws_name):

            alg = AlgorithmManager.create("EQSANSMonitorTOF")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", monitor_ws_name)
            alg.setProperty("OutputWorkspace", monitor_ws_name+'_tof')
            alg.execute()
            monitor_ws = alg.getProperty("OutputWorkspace").value

            alg = AlgorithmManager.create("ConvertUnits")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", monitor_ws)
            alg.setProperty("OutputWorkspace", monitor_ws_name+'_wl')
            alg.setProperty("Target", "Wavelength")
            alg.execute()
            monitor_ws = alg.getProperty("OutputWorkspace").value

            alg = AlgorithmManager.create("SANSBeamFluxCorrection")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", input_ws_name)
            alg.setProperty("InputMonitorWorkspace", monitor_ws)
            alg.setProperty("ReferenceFluxFilename", reference_flux)
            alg.setProperty("ReductionProperties", prop_mng)
            alg.setProperty("OutputWorkspace", output_ws_name)
            alg.execute()
            output_msg = alg.getPropertyValue("OutputMessage")
            output_ws = alg.getProperty("OutputWorkspace").value

            self.setProperty("OutputWorkspace", output_ws)
            self.setProperty("OutputMessage",
                             "Data [%s] normalized to monitor\n  %s" % (input_ws_name, output_msg))
        else:
            self.setProperty("OutputMessage", "Monitor not available. Data [%s] NOT normalized to monitor" % (input_ws_name))
#############################################################################################

AlgorithmFactory.subscribe(EQSANSNormalise)
