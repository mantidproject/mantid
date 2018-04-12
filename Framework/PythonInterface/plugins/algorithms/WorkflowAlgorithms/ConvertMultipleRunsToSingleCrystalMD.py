from __future__ import (absolute_import, division, print_function)

from mantid.api import (DataProcessorAlgorithm, mtd, AlgorithmFactory,
                        FileProperty, FileAction,
                        MultipleFileProperty, WorkspaceProperty,
                        PropertyMode, Progress)
from mantid.simpleapi import (LoadIsawUB, LoadInstrument,
                              SetGoniometer, ConvertToMD, Load,
                              LoadIsawDetCal, LoadMask,
                              DeleteWorkspace, MaskDetectors,
                              ConvertToMDMinMaxGlobal)
from mantid.kernel import VisibleWhenProperty, PropertyCriterion, Direction
from mantid import logger


class ConvertMultipleRunsToSingleCrystalMD(DataProcessorAlgorithm):
    def category(self):
        return "MDAlgorithms\\Creation"

    def seeAlso(self):
        return [ "ConvertToDiffractionMDWorkspace","ConvertToMD" ]

    def name(self):
        return "ConvertMultipleRunsToSingleCrystalMD"

    def summary(self):
        return "Convert multiple runs to one Single Crystal MDEventWorkspace"

    def PyInit(self):
        # files to reduce
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
                             "Files to combine in reduction")

        # Filter by time
        self.copyProperties('LoadEventNexus', ['FilterByTofMin', 'FilterByTofMax', 'FilterByTimeStop'])

        # UBMatrix
        self.declareProperty(FileProperty(name="UBMatrix",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".mat", ".ub", ".txt"]),
                             doc="Path to an ISAW-style UB matrix text file. See :ref:`LoadIsawUB <algm-LoadIsawUB>`")
        # Goniometer
        self.declareProperty('SetGoniometer', False, "Set which Goniometer to use. See :ref:`SetGoniometer <algm-SetGoniometer>`")
        condition = VisibleWhenProperty("SetGoniometer", PropertyCriterion.IsNotDefault)
        self.copyProperties('SetGoniometer', ['Goniometers', 'Axis0', 'Axis1', 'Axis2'])
        self.setPropertySettings("Goniometers", condition)
        self.setPropertySettings('Axis0', condition)
        self.setPropertySettings('Axis1', condition)
        self.setPropertySettings('Axis2', condition)

        # Corrections
        self.declareProperty(FileProperty(name="LoadInstrument",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml"]),
                             "Load a different instrument IDF onto the data from a file. See :ref:`LoadInstrument <algm-LoadInstrument>`")
        self.declareProperty(FileProperty(name="DetCal",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".detcal"]),
                             "Load an ISAW DetCal calibration onto the data from a file. See :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`")
        self.declareProperty(FileProperty(name="MaskFile",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml",".msk"]),
                             "Masking file for masking. Supported file format is XML and ISIS ASCII. See :ref:`LoadMask <algm-LoadMask>`")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")

        # Convert Settings
        self.copyProperties('ConvertToMD', ['Uproj', 'Vproj', 'Wproj',
                                            'MinValues', 'MaxValues', 'SplitInto', 'SplitThreshold',
                                            'MaxRecursionDepth', 'OverwriteExisting'])

        self.setPropertyGroup('FilterByTofMin', 'Loading')
        self.setPropertyGroup('FilterByTofMax', 'Loading')
        self.setPropertyGroup('FilterByTimeStop', 'Loading')

        # Goniometer
        self.setPropertyGroup("SetGoniometer","Goniometer")
        self.setPropertyGroup("Goniometers","Goniometer")
        self.setPropertyGroup("Axis0","Goniometer")
        self.setPropertyGroup("Axis1","Goniometer")
        self.setPropertyGroup("Axis2","Goniometer")

        # Corrections
        self.setPropertyGroup("LoadInstrument","Corrections")
        self.setPropertyGroup("DetCal","Corrections")
        self.setPropertyGroup("MaskFile","Corrections")

        # ConvertToMD
        self.setPropertyGroup('Uproj', 'ConvertToMD')
        self.setPropertyGroup('Vproj', 'ConvertToMD')
        self.setPropertyGroup('Wproj', 'ConvertToMD')
        self.setPropertyGroup('MinValues', 'ConvertToMD')
        self.setPropertyGroup('MaxValues', 'ConvertToMD')
        self.setPropertyGroup('SplitInto', 'ConvertToMD')
        self.setPropertyGroup('SplitThreshold', 'ConvertToMD')
        self.setPropertyGroup('MaxRecursionDepth', 'ConvertToMD')

    def PyExec(self):
        _load_inst = bool(self.getProperty("LoadInstrument").value)
        _detcal = bool(self.getProperty("DetCal").value)
        _masking = bool(self.getProperty("MaskFile").value)
        _outWS_name = self.getPropertyValue("OutputWorkspace")
        _UB = bool(self.getProperty("UBMatrix").value)

        MinValues = self.getProperty("MinValues").value
        MaxValues = self.getProperty("MaxValues").value

        if self.getProperty("OverwriteExisting").value:
            if mtd.doesExist(_outWS_name):
                DeleteWorkspace(_outWS_name)

        progress = Progress(self, 0.0, 1.0, len(self.getProperty("Filename").value))

        for run in self.getProperty("Filename").value:
            logger.notice("Working on " + run)

            Load(Filename=run,
                 OutputWorkspace='__run',
                 FilterByTofMin=self.getProperty("FilterByTofMin").value,
                 FilterByTofMax=self.getProperty("FilterByTofMax").value,
                 FilterByTimeStop=self.getProperty("FilterByTimeStop").value)

            if _load_inst:
                LoadInstrument(Workspace='__run', Filename=self.getProperty("LoadInstrument").value, RewriteSpectraMap=False)

            if _detcal:
                LoadIsawDetCal(InputWorkspace='__run', Filename=self.getProperty("DetCal").value)

            if _masking:
                if not mtd.doesExist('__mask'):
                    LoadMask(Instrument=mtd['__run'].getInstrument().getName(),
                             InputFile=self.getProperty("MaskFile").value,
                             OutputWorkspace='__mask')
                MaskDetectors(Workspace='__run',MaskedWorkspace='__mask')

            if self.getProperty('SetGoniometer').value:
                SetGoniometer(Workspace='__run',
                              Goniometers=self.getProperty('Goniometers').value,
                              Axis0=self.getProperty('Axis0').value,
                              Axis1=self.getProperty('Axis1').value,
                              Axis2=self.getProperty('Axis2').value)

            if _UB:
                LoadIsawUB(InputWorkspace='__run', Filename=self.getProperty("UBMatrix").value)
                if len(MinValues) == 0 or len(MaxValues) == 0:
                    MinValues, MaxValues = ConvertToMDMinMaxGlobal('__run', dEAnalysisMode='Elastic',Q3DFrames='HKL',QDimensions='Q3D')
                ConvertToMD(InputWorkspace='__run',
                            OutputWorkspace=_outWS_name,
                            QDimensions='Q3D',
                            dEAnalysisMode='Elastic',
                            Q3DFrames='HKL',
                            QConversionScales='HKL',
                            Uproj=self.getProperty('Uproj').value,
                            Vproj=self.getProperty('Vproj').value,
                            Wproj=self.getProperty('Wproj').value,
                            MinValues=MinValues,
                            MaxValues=MaxValues,
                            SplitInto=self.getProperty('SplitInto').value,
                            SplitThreshold=self.getProperty('SplitThreshold').value,
                            MaxRecursionDepth=self.getProperty('MaxRecursionDepth').value,
                            OverwriteExisting=False)
            else:
                if len(MinValues) == 0 or len(MaxValues) == 0:
                    MinValues, MaxValues = ConvertToMDMinMaxGlobal('__run', dEAnalysisMode='Elastic',Q3DFrames='Q',QDimensions='Q3D')
                ConvertToMD(InputWorkspace='__run',
                            OutputWorkspace=_outWS_name,
                            QDimensions='Q3D',
                            dEAnalysisMode='Elastic',
                            Q3DFrames='Q_sample',
                            Uproj=self.getProperty('Uproj').value,
                            Vproj=self.getProperty('Vproj').value,
                            Wproj=self.getProperty('Wproj').value,
                            MinValues=MinValues,
                            MaxValues=MaxValues,
                            SplitInto=self.getProperty('SplitInto').value,
                            SplitThreshold=self.getProperty('SplitThreshold').value,
                            MaxRecursionDepth=self.getProperty('MaxRecursionDepth').value,
                            OverwriteExisting=False)
            DeleteWorkspace('__run')
            progress.report()

        if mtd.doesExist('__mask'):
            DeleteWorkspace('__mask')

        self.setProperty("OutputWorkspace", mtd[_outWS_name])


AlgorithmFactory.subscribe(ConvertMultipleRunsToSingleCrystalMD)
