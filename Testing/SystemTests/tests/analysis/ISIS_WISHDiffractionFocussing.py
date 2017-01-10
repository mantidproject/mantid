from mantid.simpleapi import *
import stresstesting
import os.path


class WISHDiffractionFocussingTest(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["WISH00035991.raw", "35922_h00_RW.cal"]

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        for ws in self._focused_workspaces:
            try:
                path = config.getString("defaultsave.directory")
                os.remove(os.path.join(path, ws + ".dat"))
                os.remove(os.path.join(path, ws + ".nxs"))
            except:
                continue

    def runTest(self):
        output_path = config.getString("defaultsave.directory")
        grouping_filename = '35922_h00_RW.cal'
        focused_suffix = '-foc-h00'

        monitor_id = 4
        crop_limits = {"XMin": 6000, "XMax": 99900 }

        run_numbers = [35991]

        min_run = min(map(lambda x: x if type(x) == int else min(x), run_numbers))
        max_run = max(map(lambda x: x if type(x) == int else max(x), run_numbers))
        group_name = str(min_run) + '-' + str(max_run) + focused_suffix

        self._focused_workspaces = []
        for runno in run_numbers:
            run_number = str(runno)
            raw_file = "WISH000" + run_number +'.raw'

            LoadRaw(Filename=raw_file, OutputWorkspace=run_number)
            CropWorkspace(InputWorkspace=run_number, OutputWorkspace=run_number, **crop_limits)
            NormaliseByCurrent(InputWorkspace=run_number, OutputWorkspace=run_number)
            ConvertUnits(InputWorkspace=run_number, OutputWorkspace=run_number, Target='Wavelength')
            NormaliseToMonitor(InputWorkspace=run_number, OutputWorkspace=run_number, MonitorID=monitor_id)
            ConvertUnits(InputWorkspace=run_number, OutputWorkspace=run_number, Target='dSpacing')

            focused = run_number + focused_suffix
            DiffractionFocussing(InputWorkspace=run_number, OutputWorkspace=focused,
                                 GroupingFileName=grouping_filename, PreserveEvents=False)

            focused_xye = os.path.join(output_path, focused +'.dat')
            focused_nxs = os.path.join(output_path, focused +'.nxs')
            SaveFocusedXYE(focused, focused_xye, SplitFiles=False)
            SaveNexusProcessed(InputWorkspace=focused, Filename=focused_nxs)

            self._focused_workspaces.append(focused)
            DeleteWorkspace(run_number)

        GroupWorkspaces(self._focused_workspaces, OutputWorkspace=group_name)

    def validate(self):
        self.assertEqual(len(self._focused_workspaces), 1)

        for ws in self._focused_workspaces:
            path = config.getString("defaultsave.directory")
            self.assertTrue(os.path.exists(os.path.join(path, ws + ".dat")))
            self.assertTrue(os.path.exists(os.path.join(path, ws + ".nxs")))
