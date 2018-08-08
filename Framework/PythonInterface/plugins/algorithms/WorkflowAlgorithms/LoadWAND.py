from __future__ import absolute_import, division, print_function
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MultipleFileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, UnitConversion, Elastic, Property, IntArrayProperty
from mantid.simpleapi import (mtd, SetGoniometer, AddSampleLog, MaskBTP, RenameWorkspace, GroupWorkspaces,
                              CreateWorkspace, LoadNexusLogs, LoadInstrument)
from six.moves import range
import numpy as np
import h5py


class LoadWAND(DataProcessorAlgorithm):
    def name(self):
        return 'LoadWAND'

    def category(self):
        return 'DataHandling\\Nexus'

    def summary(self):
        return 'Loads Event Nexus file, integrates events, sets wavelength, mask, and goniometer, and sets proton charge to monitor counts'

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]), "Files to Load")
        self.declareProperty('IPTS', Property.EMPTY_INT, "IPTS number to load from")
        self.declareProperty(IntArrayProperty("RunNumbers", []), 'Run numbers to load')
        self.declareProperty("Wavelength", 1.488, doc="Wavelength to set the workspace")
        self.declareProperty("ApplyMask", True, "If True standard masking will be applied to the workspace")
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def validateInputs(self):
        issues = dict()

        if not self.getProperty("Filename").value:
            if (self.getProperty("IPTS").value == Property.EMPTY_INT) or len(self.getProperty("RunNumbers").value) is 0:
                issues["Filename"] = 'Must specify either Filename or IPTS AND RunNumbers'

        return issues

    def PyExec(self):
        runs = self.getProperty("Filename").value

        if not runs:
            ipts = self.getProperty("IPTS").value
            runs = ['/HFIR/HB2C/IPTS-{}/nexus/HB2C_{}.nxs.h5'.format(ipts, run) for run in self.getProperty("RunNumbers").value]

        wavelength = self.getProperty("wavelength").value
        outWS = self.getPropertyValue("OutputWorkspace")
        group_names = []

        for i, run in enumerate(runs):
            data = np.zeros((512*480*8),dtype=np.int64)
            with h5py.File(run, 'r') as f:
                monitor_count = f['/entry/monitor1/total_counts'].value[0]
                run_number = f['/entry/run_number'].value[0]
                for b in range(8):
                    data += np.bincount(f['/entry/bank'+str(b+1)+'_events/event_id'].value,minlength=512*480*8)
            data = data.reshape((480*8, 512))

            CreateWorkspace(DataX=[wavelength-0.001, wavelength+0.001],
                            DataY=data,
                            DataE=np.sqrt(data),
                            UnitX='Wavelength',
                            YUnitLabel='Counts',
                            NSpec=1966080,
                            OutputWorkspace='__tmp_load', EnableLogging=False)
            LoadNexusLogs('__tmp_load', Filename=run, EnableLogging=False)
            LoadInstrument('__tmp_load', InstrumentName='WAND', RewriteSpectraMap=True, EnableLogging=False)

            if self.getProperty("ApplyMask").value:
                MaskBTP('__tmp_load', Pixel='1,2,511,512', EnableLogging=False)
                if mtd['__tmp_load'].getRunNumber() > 26600: # They changed pixel mapping and bank name order here
                    MaskBTP('__tmp_load', Bank='1', Tube='479-480', EnableLogging=False)
                    MaskBTP('__tmp_load', Bank='8', Tube='1-2', EnableLogging=False)
                else:
                    MaskBTP('__tmp_load', Bank='8', Tube='475-480', EnableLogging=False)

            SetGoniometer('__tmp_load', Axis0="HB2C:Mot:s1,0,1,0,1", EnableLogging=False)
            AddSampleLog('__tmp_load', LogName="monitor_count", LogType='Number', NumberType='Double',
                         LogText=str(monitor_count), EnableLogging=False)
            AddSampleLog('__tmp_load', LogName="gd_prtn_chrg", LogType='Number', NumberType='Double',
                         LogText=str(monitor_count), EnableLogging=False)
            AddSampleLog('__tmp_load', LogName="Wavelength", LogType='Number', NumberType='Double',
                         LogText=str(wavelength), EnableLogging=False)
            AddSampleLog('__tmp_load', LogName="Ei", LogType='Number', NumberType='Double',
                         LogText=str(UnitConversion.run('Wavelength', 'Energy', wavelength, 0, 0, 0, Elastic, 0)), EnableLogging=False)
            AddSampleLog('__tmp_load', LogName="run_number", LogText=run_number, EnableLogging=False)
            if len(runs) == 1:
                RenameWorkspace('__tmp_load', outWS, EnableLogging=False)
            else:
                outName = outWS+"_"+str(mtd['__tmp_load'].getRunNumber())
                group_names.append(outName)
                RenameWorkspace('__tmp_load', outName, EnableLogging=False)

        if len(runs) > 1:
            GroupWorkspaces(group_names, OutputWorkspace=outWS, EnableLogging=False)

        self.setProperty('OutputWorkspace', outWS)


AlgorithmFactory.subscribe(LoadWAND)
