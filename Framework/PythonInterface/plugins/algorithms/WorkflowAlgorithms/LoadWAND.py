# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MultipleFileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, Property, IntArrayProperty, StringListValidator
from mantid.simpleapi import (
    mtd,
    SetGoniometer,
    AddSampleLog,
    MaskBTP,
    RenameWorkspace,
    GroupWorkspaces,
    CreateWorkspace,
    LoadNexusLogs,
    LoadInstrument,
)
import numpy as np
import h5py


class LoadWAND(DataProcessorAlgorithm):
    def name(self):
        return "LoadWAND"

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return "Loads Event Nexus file, integrates events, sets wavelength, mask, and goniometer, and sets proton charge to monitor counts"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]), "Files to Load")
        self.declareProperty("IPTS", Property.EMPTY_INT, "IPTS number to load from")
        self.declareProperty(IntArrayProperty("RunNumbers", []), "Run numbers to load")
        self.declareProperty("ApplyMask", True, "If True standard masking will be applied to the workspace")
        self.declareProperty("Grouping", "None", StringListValidator(["None", "2x2", "4x4"]), "Group pixels")
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def validateInputs(self):
        issues = dict()

        if not self.getProperty("Filename").value:
            if (self.getProperty("IPTS").value == Property.EMPTY_INT) or len(self.getProperty("RunNumbers").value) == 0:
                issues["Filename"] = "Must specify either Filename or IPTS AND RunNumbers"

        return issues

    def PyExec(self):
        runs = self.getProperty("Filename").value

        if not runs:
            ipts = self.getProperty("IPTS").value
            runs = ["/HFIR/HB2C/IPTS-{}/nexus/HB2C_{}.nxs.h5".format(ipts, run) for run in self.getProperty("RunNumbers").value]

        outWS = self.getPropertyValue("OutputWorkspace")
        group_names = []

        grouping = self.getProperty("Grouping").value
        if grouping == "None":
            grouping = 1
        else:
            grouping = 2 if grouping == "2x2" else 4

        for i, run in enumerate(runs):
            data = np.zeros((512 * 480 * 8), dtype=np.int64)
            with h5py.File(run, "r") as f:
                monitor_count = f["/entry/monitor1/total_counts"][0]
                duration = f["/entry/duration"][0]
                run_number = f["/entry/run_number"][0]
                for b in range(8):
                    data += np.bincount(f["/entry/bank" + str(b + 1) + "_events/event_id"][()], minlength=512 * 480 * 8)
            data = data.reshape((480 * 8, 512))
            if grouping == 2:
                data = data[::2, ::2] + data[1::2, ::2] + data[::2, 1::2] + data[1::2, 1::2]
            elif grouping == 4:
                data = (
                    data[::4, ::4]
                    + data[1::4, ::4]
                    + data[2::4, ::4]
                    + data[3::4, ::4]
                    + data[::4, 1::4]
                    + data[1::4, 1::4]
                    + data[2::4, 1::4]
                    + data[3::4, 1::4]
                    + data[::4, 2::4]
                    + data[1::4, 2::4]
                    + data[2::4, 2::4]
                    + data[3::4, 2::4]
                    + data[::4, 3::4]
                    + data[1::4, 3::4]
                    + data[2::4, 3::4]
                    + data[3::4, 3::4]
                )

            CreateWorkspace(
                DataX=[0, 1],
                DataY=data,
                DataE=np.sqrt(data),
                UnitX="Empty",  # this axis is ignored by the subsequent reduction alg, WANDPowderReduction
                YUnitLabel="Counts",
                NSpec=1966080 // grouping**2,
                OutputWorkspace="__tmp_load",
                EnableLogging=False,
            )
            LoadNexusLogs("__tmp_load", Filename=run, EnableLogging=False)
            AddSampleLog(
                "__tmp_load",
                LogName="monitor_count",
                LogType="Number",
                NumberType="Double",
                LogText=str(monitor_count),
                EnableLogging=False,
            )
            AddSampleLog(
                "__tmp_load", LogName="gd_prtn_chrg", LogType="Number", NumberType="Double", LogText=str(monitor_count), EnableLogging=False
            )
            AddSampleLog("__tmp_load", LogName="run_number", LogText=run_number, EnableLogging=False)
            AddSampleLog(
                "__tmp_load", LogName="duration", LogType="Number", LogText=str(duration), NumberType="Double", EnableLogging=False
            )

            if grouping > 1:  # Fix detector IDs per spectrum before loading instrument
                __tmp_load = mtd["__tmp_load"]
                for n in range(__tmp_load.getNumberHistograms()):
                    s = __tmp_load.getSpectrum(n)
                    for i in range(grouping):
                        for j in range(grouping):
                            s.addDetectorID(int(n * grouping % 512 + n // (512 / grouping) * 512 * grouping + j + i * 512))

                LoadInstrument("__tmp_load", InstrumentName="WAND", RewriteSpectraMap=False, EnableLogging=False)
            else:
                LoadInstrument("__tmp_load", InstrumentName="WAND", RewriteSpectraMap=True, EnableLogging=False)

            SetGoniometer("__tmp_load", Axis0="HB2C:Mot:s1,0,1,0,1", EnableLogging=False)

            if self.getProperty("ApplyMask").value:
                MaskBTP("__tmp_load", Pixel="1,2,511,512", EnableLogging=False)
                if mtd["__tmp_load"].getRunNumber() > 26600:  # They changed pixel mapping and bank name order here
                    MaskBTP("__tmp_load", Bank="1", Tube="479-480", EnableLogging=False)
                    MaskBTP("__tmp_load", Bank="8", Tube="1-2", EnableLogging=False)
                else:
                    MaskBTP("__tmp_load", Bank="8", Tube="475-480", EnableLogging=False)

            if len(runs) == 1:
                RenameWorkspace("__tmp_load", outWS, EnableLogging=False)
            else:
                try:
                    temp_val = mtd["__tmp_load"].run().getTimeAveragedValue("HB2C:SE:SampleTemp")
                except RuntimeError:
                    temp_val = 300.0

                if temp_val == 0.0 or np.isnan(temp_val):
                    temp_val = 300.0
                temp_val = "{:.1F}".format(temp_val).replace(".", "p")
                outName = outWS + "_" + str(mtd["__tmp_load"].getRunNumber()) + f"_T{temp_val}K"
                group_names.append(outName)
                RenameWorkspace("__tmp_load", outName, EnableLogging=False)

        if len(runs) > 1:
            GroupWorkspaces(group_names, OutputWorkspace=outWS, EnableLogging=False)

        self.setProperty("OutputWorkspace", outWS)


AlgorithmFactory.subscribe(LoadWAND)
