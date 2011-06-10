from mantidsimple import *
from msg_reducer import MSGReducer
import inelastic_indirect_reduction_steps as steps

class MSGDiffractionReducer(MSGReducer):
    """Reducer for Diffraction on IRIS and TOSCA.
    """

    def __init__(self):
        super(MSGDiffractionReducer, self).__init__()
    
    def _setup_steps(self):
        self.append_step(steps.HandleMonitor(
            MultipleFrames=self._multiple_frames))
        self.append_step(steps.CorrectByMonitor(
            MultipleFrames=self._multiple_frames, EMode="Elastic"))
        if self._multiple_frames:
            self.append_step(steps.FoldData())
        
        step = mtd.createAlgorithm("ConvertUnits")
        step.setPropertyValue("Target", "dSpacing")
        step.setPropertyValue("EMode", "Elastic")
        self.append_step(step)


def getStringProperty(workspace, property):
    """This function is used in the interface.
    """
    inst = mtd[workspace].getInstrument()
    try:
        prop = inst.getStringParameter(property)[0]
    except IndexError: return ""
    return prop
