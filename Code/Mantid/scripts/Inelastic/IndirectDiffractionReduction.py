import mantid
from msg_reducer import MSGReducer
import inelastic_indirect_reduction_steps as steps
from  mantid.simpleapi import *

class MSGDiffractionReducer(MSGReducer):
    """Reducer for Diffraction on IRIS and TOSCA.
    """

    _container_workspace = None
    _container_scale_factor = None

    def __init__(self):
        super(MSGDiffractionReducer, self).__init__()
    
    def _setup_steps(self):
        self.append_step(steps.IdentifyBadDetectors(
            MultipleFrames=self._multiple_frames))
        self.append_step(steps.HandleMonitor(
            MultipleFrames=self._multiple_frames))
        self.append_step(steps.CorrectByMonitor(
            MultipleFrames=self._multiple_frames, EMode="Elastic"))

        if self._multiple_frames:
            if self._fold_multiple_frames:
                self.append_step(steps.FoldData())
            else:
                return
        
        step = mantid.AlgorithmManager.create("ConvertUnits")
        step.setPropertyValue("Target", "dSpacing")
        step.setPropertyValue("EMode", "Elastic")
        self.append_step(step)
        
        if self._rebin_string is not None:
            step = mantid.AlgorithmManager.create("Rebin")
            step.setPropertyValue("Params", self._rebin_string)
            self.append_step(step)
        else:
            self.append_step(steps.RebinToFirstSpectrum())
        
        step = steps.Grouping()
        step.set_grouping_policy("All")
        self.append_step(step)
        
        # The "SaveItem" step saves the files in the requested formats.
        if (len(self._save_formats) > 0):
            step = steps.SaveItem()
            step.set_formats(self._save_formats)
            self.append_step(step)

    def post_process(self):
        #if we're using a can, normalise by beam current and subtact from the sample
        if self._container_workspace is not None:
            NormaliseByCurrent(self._container_workspace, OutputWorkspace=self._container_workspace)

            #scale if required
            if self._container_scale_factor is not None:
                Scale(self._container_workspace, Factor=self._container_scale_factor, 
                      OutputWorkspace=self._container_workspace)

            workspaces = self.get_result_workspaces()
            for ws in workspaces:
                if ws != self._container_workspace:
                    NormaliseByCurrent(ws, OutputWorkspace=ws)
                    Minus(ws, self._container_workspace, OutputWorkspace=ws)

    def set_container(self, container):
        self._container_workspace = container

    def set_container_scale_factor(self, factor):
        self._container_scale_factor = factor


def getStringProperty(workspace, property):
    """This function is used in the interface.
    """
    inst = mantid.AnalysisDataService[workspace].getInstrument()
    try:
        prop = inst.getStringParameter(property)[0]
    except IndexError: return ""
    return prop
