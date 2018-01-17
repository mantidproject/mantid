import SANSadd2
from mantid.kernel import *
from mantid.api import *
from ui.sans_isis.work_handler import WorkHandler


class SANSAddFiles(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(StringArrayProperty("RunSelectionPaths"),
                             doc="The .nxs files to add.")
        self.declareProperty("BinningParameters",
                             "",
                             direction=Direction.Input,
                             doc="The binning to use for the added runs.")
        self.declareProperty("Instrument",
                             "",
                             direction=Direction.Input,
                             doc="The instrument in use.")
        self.declareProperty("AdditionalTimeShifts",
                             "",
                             direction=Direction.Input,
                             doc="The time shifts to offset the added data by.")
        self.declareProperty("OverlayEventWorkspaces",
                             True,
                             direction=Direction.Input,
                             doc="Whether or not the event data from the files should be shifted"
                                 " to fit on top of eachother.")
        self.declareProperty("SaveAsEventData",
                             False,
                             direction=Direction.Input,
                             doc="If this option is chosen, the output file will contain event"
                             " data. The output is not an event workspace but rather a group"
                             " workspace, which contains two child event workspaces, one for the"
                             " added event data and one for the added monitor data.")
        self.declareProperty("OutputFilename",
                             "add.nxs",
                             direction=Direction.Input,
                             doc="The name of the file to write the added data to.")
        self.declareProperty("MonitorsOutputFilename",
                             "add_monitors.nxs",
                             direction=Direction.Input,
                             doc="The name of the file to write the added monitors data to.")

    def category(self):
        return 'Workflow'

    def PyExec(self):
        SANSadd2.add_runs(
            tuple(self.getProperty("RunSelectionPaths").value),
            self.getProperty("Instrument").value,
            lowMem=True,
            binning=self.getProperty("BinningParameters").value,
            isOverlay=self.getProperty("OverlayEventWorkspaces").value,
            saveAsEvent=self.getProperty("SaveAsEventData").value,
            time_shifts=self.getProperty("AdditionalTimeShifts").value,
            outFile=self.getProperty("OutputFilename").value,
            outFile_monitors=self.getProperty("MonitorsOutputFilename").value)


AlgorithmFactory.subscribe(SANSAddFiles)


class RunSummation(object):
    def __init__(self, work_handler):
        self._work_handler = work_handler

    class Listener(WorkHandler.WorkListener):
        def on_processing_finished(self, result):
            pass

        def on_processing_error(self, error):
            pass

    def __call__(self, run_selection, settings, base_file_name):
        self._work_handler.process(RunSummation.Listener(), self.run, run_selection, settings, base_file_name)

    def run(self, run_selection, settings, base_file_name):
        run_selection = self._run_selection_as_path_list(run_selection)
        binning = self._bin_settings_or_monitors(settings)
        additional_time_shifts = self._time_shifts_or_empty_string(settings)
        overlay_event_workspaces = self._is_overlay_event_workspaces_enabled(settings)
        save_as_event = self._should_save_as_event_workspaces(settings)

        file_name = base_file_name + '.nxs'
        monitors_file_name = base_file_name + '_monitors.nxs'

        add_files = AlgorithmManager.create("SANSAddFiles")
        add_files.initialize()
        add_files.setProperty("RunSelectionPaths", run_selection)
        add_files.setProperty("BinningParameters", binning)
        add_files.setProperty("Instrument", settings.instrument())
        add_files.setProperty("AdditionalTimeShifts", additional_time_shifts)
        add_files.setProperty("OverlayEventWorkspaces", overlay_event_workspaces)
        add_files.setProperty("SaveAsEventData", save_as_event)
        add_files.setProperty("OutputFilename", file_name)
        add_files.setProperty("MonitorsOutputFilename", monitors_file_name)
        add_files.execute()

    def _run_selection_as_path_list(self, run_selection):
        return [run.file_path() for run in run_selection]

    def _bin_settings_or_monitors(self, settings):
        return settings.bin_settings if settings.has_bin_settings() \
            else 'Monitors'

    def _time_shifts_or_empty_string(self, settings):
        return settings.additional_time_shifts if settings.has_additional_time_shifts() \
            else ""

    def _is_overlay_event_workspaces_enabled(self, settings):
        return settings.is_overlay_event_workspaces_enabled() \
            if settings.has_overlay_event_workspaces() \
            else False

    def _should_save_as_event_workspaces(self, settings):
        return settings.should_save_as_event_workspaces()
