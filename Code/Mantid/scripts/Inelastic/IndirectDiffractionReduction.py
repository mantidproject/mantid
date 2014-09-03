import mantid
from msg_reducer import MSGReducer
import inelastic_indirect_reduction_steps as steps

class MSGDiffractionReducer(MSGReducer):
    """Reducer for Diffraction on IRIS and TOSCA.
    """

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

        step = steps.Naming()
        self.append_step(step)


def getStringProperty(workspace, property):
    """This function is used in the interface.
    """
    inst = mantid.AnalysisDataService[workspace].getInstrument()
    try:
        prop = inst.getStringParameter(property)[0]
    except IndexError: return ""
    return prop
