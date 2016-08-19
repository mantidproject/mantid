from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

import xml.dom.minidom

#-------------------------------------------------------------------------------

class TOFTOFScriptElement(BaseScriptElement):

    # normalisation
    NORM_NONE       = 0
    NORM_MONITOR    = 1
    NORM_TIME       = 2

    # TOF correction
    CORR_TOF_NONE   = 0
    CORR_TOF_VAN    = 1
    CORR_TOF_SAMPLE = 2

    # default values
    DEF_prefix     = 'ws'
    DEF_ecFactor   = 1.0

    DEF_binEon     = True
    DEF_binEstart  = 0.0
    DEF_binEstep   = 0.0
    DEF_binEend    = 0.0

    DEF_binQon     = True
    DEF_binQstart  = 0.0
    DEF_binQstep   = 0.0
    DEF_binQend    = 0.0

    DEF_subECVan   = False
    DEF_normalise  = NORM_NONE
    DEF_correctTof = CORR_TOF_NONE

    XML_TAG = 'TOFTOFReduction'

    def reset(self):
        self.facility_name   = ''
        self.instrument_name = ''

        # prefix of (some) workspace names
        self.prefix   = self.DEF_prefix

        # data files are here
        self.dataDir  = ''

        # vanadium runs & comment
        self.vanRuns  = ''
        self.vanCmnt  = ''

        # empty can runs, comment, and factor
        self.ecRuns   = ''
        self.ecFactor = self.DEF_ecFactor

        # data runs: [(runs,comment), ...]
        self.dataRuns = []

        # additional parameters
        self.binEon        = self.DEF_binEon
        self.binEstart     = self.DEF_binEstart
        self.binEstep      = self.DEF_binEstep
        self.binEend       = self.DEF_binEend

        self.binQon        = self.DEF_binQon
        self.binQstart     = self.DEF_binQstart
        self.binQstep      = self.DEF_binQstep
        self.binQend       = self.DEF_binQend

        self.maskDetectors = ''

        # options
        self.subtractECVan = self.DEF_subECVan
        self.normalise     = self.DEF_normalise
        self.correctTof    = self.DEF_correctTof

    def to_xml(self):
        xml = ['']

        def put(tag, val):
            xml[0] += '  <%s>%s</%s>\n' % (tag, str(val), tag)

        put('prefix',      self.prefix)
        put('data_dir',    self.dataDir)

        put('van_runs',    self.vanRuns)
        put('van_comment', self.vanCmnt)

        put('ec_runs',     self.ecRuns)
        put('ec_factor',   self.ecFactor)

        for (runs, cmnt) in self.dataRuns:
            put('data_runs',    runs)
            put('data_comment', cmnt)

        put('rebin_energy_on',    self.binEon)
        put('rebin_energy_start', self.binEstart)
        put('rebin_energy_step',  self.binEstep)
        put('rebin_energy_end',   self.binEend)

        put('rebin_q_on',     self.binQon)
        put('rebin_q_start',  self.binQstart)
        put('rebin_q_step',   self.binQstep)
        put('rebin_q_end',    self.binQend)

        put('mask_detectors', self.maskDetectors)

        put('subtract_ecvan', self.subtractECVan)
        put('normalise',      self.normalise)
        put('correct_tof',    self.correctTof)

        xml = '<%s>\n%s</%s>\n' % (self.XML_TAG, xml[0], self.XML_TAG)

        return xml

    def from_xml(self, xmlString):
        self.reset()

        dom = xml.dom.minidom.parseString(xmlString)
        els = dom.getElementsByTagName(self.XML_TAG)

        if els:
            dom = els[0]

            def getStr(tag, default = ''):
                return BaseScriptElement.getStringElement(dom, tag, default=default)

            def getInt(tag, default):
                return BaseScriptElement.getIntElement(dom, tag, default=default)

            def getFlt(tag, default):
                return BaseScriptElement.getFloatElement(dom, tag, default=default)

            def getStrLst(tag):
                return BaseScriptElement.getStringList(dom, tag)

            def getBol(tag, default):
                return BaseScriptElement.getBoolElement(dom, tag, default=default)

            self.prefix   = getStr('prefix', self.DEF_prefix)
            self.dataDir  = getStr('data_dir')

            self.vanRuns  = getStr('van_runs')
            self.vanCmnt  = getStr('van_comment')

            self.ecRuns   = getStr('ec_runs')
            self.ecFactor = getFlt('ec_factor', self.DEF_ecFactor)

            dataRuns = getStrLst('data_runs')
            dataCmts = getStrLst('data_comment')
            for i in range(min(len(dataRuns), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataCmts[i]))

            self.binEon    = getBol('rebin_energy_on',    self.DEF_binEon)
            self.binEstart = getFlt('rebin_energy_start', self.DEF_binEstart)
            self.binEstep  = getFlt('rebin_energy_step',  self.DEF_binEstep)
            self.binEend   = getFlt('rebin_energy_end',   self.DEF_binEend)

            self.binQon    = getBol('rebin_q_on',         self.DEF_binQon)
            self.binQstart = getFlt('rebin_q_start',      self.DEF_binQstart)
            self.binQstep  = getFlt('rebin_q_step',       self.DEF_binQstep)
            self.binQend   = getFlt('rebin_q_end',        self.DEF_binQend)

            self.maskDetectors = getStr('mask_detectors')

            self.subtractECVan = getBol('subtract_ecvan', self.DEF_subECVan)
            self.normalise     = getInt('normalise',      self.DEF_normalise)
            self.correctTof    = getInt('correct_tof',    self.DEF_correctTof)

    def to_script(self):

        def error(message):
            raise RuntimeError('TOFTOF reduction error: ' + message)

        # sanity checks

        # must have vanadium for TOF correction
        if self.CORR_TOF_VAN == self.correctTof:
            if not self.vanRuns:
                error('missing vanadium run')

        # must have vanadium and empty can for subtracting EC from van
        if self.subtractECVan:
            if not self.vanRuns:
                error('missing vanadium run')
            if not self.ecRuns:
                error('missing empty can run')

        # binning parameters
        def checkBinningParams(start, step, end):
            if not (start < end and 0 < step and start + step <= end):
                error('incorrect binning parameters')

        if self.binEon:
            checkBinningParams(self.binEstart, self.binEstep, self.binEend)
        if self.binQon:
            checkBinningParams(self.binQstart, self.binQstep, self.binQend)

        # must have some data runs
        if not self.dataRuns:
            error('missing data runs')

        # must have a comment for runs
        if self.vanRuns and not self.vanCmnt:
            error('missing vanadium comment')

        # generated script
        script = ['']

        # helper: add a line to the script
        def l(s = ''):
            script[0] += (s + '\n')
        # helpers
        def logData(ws,tag):
            return "%s.getRun().getLogData('%s').value" % (ws, tag)
        def logEi(ws):
            return logData(ws, 'Ei')
        def logTime(ws):
            return logData(ws, 'duration')

        l("import numpy as np")
        l()
        l("config['default.facility'] = '%s'"   % (self.facility_name))
        l("config['default.instrument'] = '%s'" % (self.instrument_name))
        l()
        l("config.appendDataSearchDir('%s')"    % (self.dataDir))
        l()

        dataRawGroup = []
        dataGroup    = []
        allGroup     = []

        # vanadium runs
        if self.vanRuns:
            wsRawVan = self.prefix + 'RawVan'
            wsVan    = self.prefix + 'Van'

            l("# vanadium runs")
            l("%s = Load(Filename='%s')" % (wsRawVan, self.vanRuns))
            l("%s = TOFTOFMergeRuns(%s)" % (wsVan, wsRawVan))
            l("%s.setComment('%s')"      % (wsVan, self.vanCmnt))
            l()

            allGroup.append(wsVan)

        # empty can runs
        if self.ecRuns:
            wsRawEC = self.prefix + 'RawEC'
            wsEC    = self.prefix + 'EC'

            l("# empty can runs")
            l("%s = Load(Filename='%s')" % (wsRawEC, self.ecRuns))
            l("%s = TOFTOFMergeRuns(%s)" % (wsEC, wsRawEC))
            l()

            allGroup.append(wsEC)

        # data runs
        for i, (runs, cmnt) in enumerate(self.dataRuns):
            if not runs:
                error('missing data runs value')
            if not cmnt:
                error('missing data runs comment')

            postfix = str(i + 1)

            wsRawData = self.prefix + 'RawData' + postfix
            wsData    = self.prefix + 'Data'    + postfix

            dataRawGroup.append(wsRawData)
            dataGroup.append(wsData)
            allGroup.append(wsData)

            l("# data runs %s" % (postfix))
            l("%s = Load(Filename='%s')" % (wsRawData, runs))
            l("%s = TOFTOFMergeRuns(%s)" % (wsData, wsRawData))
            l("%s.setComment('%s')"      % (wsData, cmnt))
            l()

            if 0 == i:
                wsData0 = wsData

        def gr(list, postfix = ''):
            return ('[' + ', '.join(list) + ']' + postfix) if list else ''

        gPrefix = 'g' + self.prefix
        gDataRuns    = gPrefix + 'DataRuns'
        gDataRawRuns = gPrefix + 'DataRawRuns'
        gAll         = gPrefix + 'All'

        l("# grouping")
        l("%s = GroupWorkspaces(%s)" % (gDataRawRuns, gr(dataRawGroup)))
        l("%s = GroupWorkspaces(%s)" % (gDataRuns,    gr(dataGroup)))
        l("%s = GroupWorkspaces(%s)" % (gAll,         gr(allGroup)))
        l()

        l("# Ei")
        l("if CompareSampleLogs(%s, 'Ei', 0.001):" % (gAll))
        l("    raise RuntimeError('Ei values do not match')")
        l()
        l("Ei = %s" % (logEi(wsData0)))
        l()

        gDetectorsToMask = gPrefix + 'DetectorsToMask'
        l("# mask detectors")
        l("(%s, numberOfFailures) = FindDetectorsOutsideLimits(%s)" % (gDetectorsToMask, gAll))
        l("MaskDetectors(%s, MaskedWorkspace=%s)" % (gAll, gDetectorsToMask))

        if self.maskDetectors:
            l("MaskDetectors(%s, '%s')" % (gAll, self.maskDetectors))

        l()

        if self.NORM_MONITOR == self.normalise:
            gDataNorm = gPrefix + 'Norm'

            l("# normalise to monitor")

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("%s = MonitorEfficiencyCorUser(%s)" % (wsVanNorm, wsVan))

            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("%s = MonitorEfficiencyCorUser(%s)" % (wsECNorm, wsEC))

            l("%s = MonitorEfficiencyCorUser(%s)" % (gDataNorm, gDataRuns))
            l()

        elif self.NORM_TIME == self.normalise:
            gDataNorm = gPrefix + 'Norm'

            # TODO (ask Wiebke how to do it best)
            l("# normalise to time")

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("%s = Scale(%s, 1/%s, 'Multiply')" % (wsVanNorm, wsVan, logTime(wsVan)))

            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("%s = Scale(%s, 1/%s, 'Multiply')" % (wsECNorm, wsEC, logTime(wsEC)))

            l("names = []")
            l("for ws in %s:" % (gDataRuns))
            l("    name = ws.getName() + 'Norm'")
            l("    names.append(name)")
            l("    Scale(ws, 1/%s, 'Multiply', OutputWorkspace=name)" % (logTime('ws')))
            l()
            l("%s = GroupWorkspaces(names)" % (gDataNorm))

            l()

        else: # none, simply use the not normalised workspaces
            gDataNorm = gDataRuns

            if self.vanRuns:
                wsVanNorm = wsVan
            if self.ecRuns:
                wsECNorm = wsEC

        if self.ecRuns:
            gDataSubEC = gPrefix + 'DataSubEC'
            scaledEC   = self.prefix + 'ScaledEC'
            l("# subtract empty can")
            l("ecFactor = '%f'" % (self.ecFactor))
            l("%s = Scale(%s, Factor=ecFactor, Operation='Multiply')" % (scaledEC, wsECNorm))
            l("%s = Minus(%s, %s)" % (gDataSubEC, gDataNorm, scaledEC))
            if self.subtractECVan:
                wsVanSubEC = wsVan + 'SubEC'
                l("%s = Minus(%s, %s)" % (wsVanSubEC, wsVanNorm, scaledEC))
            l()

        l("# group data for processing") # without empty can
        gDataSource = gDataSubEC if self.ecRuns else gDataNorm
        gData = gPrefix + 'Data'
        if self.vanRuns:
            wsVanNorm = wsVanSubEC if self.subtractECVan else wsVanNorm
            l("%s = GroupWorkspaces(%slist(%s.getNames()))" % (gData, gr([wsVanNorm], ' + '), gDataSource))
        else:
            l("%s = CloneWorkspace(%s)" % (gData, gDataSource))
        l()

        if self.vanRuns:
            gDataCorr = gData + 'Corr'
            detCoeffs = self.prefix + 'DetCoeffs'
            eppTable  = self.prefix + 'EppTable'
            l("# normalise to vanadium")
            l("%s = FindEPP(%s)" % (eppTable, wsVanNorm))
            l("%s = ComputeCalibrationCoefVan(%s, %s)" % (detCoeffs, wsVanNorm, eppTable))
            l("badDetectors = np.where(np.array(%s.extractY()).flatten() <= 0)[0]" % (detCoeffs))
            l("MaskDetectors(%s, DetectorList=badDetectors)" % (gData))
            l("%s = Divide(%s, %s)" % (gDataCorr, gData, detCoeffs))
            l()

        gDataCleanFrame = gData + 'CleanFrame'
        l("# remove half-filled time bins (clean frame)")
        l("%s = TOFTOFCropWorkspace(%s)" % (gDataCleanFrame, gDataCorr if self.vanRuns else gData))
        l()

        gData2 = gData + 'TofCorr'
        if self.CORR_TOF_VAN == self.correctTof:
            l("# apply vanadium TOF correction")
            l("%s = CorrectTOF(%s, %s)" % (gData2, gDataCleanFrame, eppTable))
            l()

        elif self.CORR_TOF_SAMPLE == self.correctTof:
            eppTables  = self.prefix + 'EppTables'
            l("# apply sample TOF correction")
            l("%s = FindEPP(%s)" % (eppTables, gData))
            l("%s = CorrectTOF(%s, %s)" % (gData2, gDataCleanFrame, eppTables))
            l()

        else:
            gData2 = gDataCleanFrame

        gDataDeltaE = gData + 'DeltaE'
        l("# convert units")
        l("%s = ConvertUnits(%s, Target='DeltaE', EMode='Direct', EFixed=Ei)" % (gDataDeltaE, gData2))
        l("ConvertToDistribution(%s)" % (gDataDeltaE))
        l()

        gDataCorrDeltaE = gData + 'CorrDeltaE'
        l("# correct for energy dependent detector efficiency")
        l("%s = DetectorEfficiencyCorUser(%s)" % (gDataCorrDeltaE, gDataDeltaE))
        l()

        gDataS = gData + 'S'
        l("# calculate S (Ki/kF correction)")
        l("%s = CorrectKiKf(%s)" % (gDataS, gDataCorrDeltaE))
        l()

        gLast = gDataS
        if self.binEon:
            gDataBinE = gData + 'BinE'
            l("# energy binning")
            l("rebinEnergy = '%f, %f, %f'" % (self.binEstart, self.binEstep, self.binEend))
            l("%s = Rebin(%s, Params=rebinEnergy)" % (gDataBinE, gLast))
            l()
            gLast = gDataBinE

        if self.binQon:
            gDataBinQ = gData + 'SQW'
            l("# calculate momentum transfer Q for sample data")
            l("rebinQ = '%f, %f, %f'" % (self.binQstart, self.binQstep, self.binQend))
            l("%s = SofQW3(%s, QAxisBinning=rebinQ, EMode='Direct', EFixed=Ei)" % (gDataBinQ, gLast))
            l()

        l("# make nice workspace names")
        l("for ws in %s:" % (gDataS))
        l("    RenameWorkspace(ws, OutputWorkspace='%s_S_' + ws.getComment())" % (self.prefix))
        if self.binEon:
            l("for ws in %s:" % (gDataBinE))
            l("    RenameWorkspace(ws, OutputWorkspace='%s_E_' + ws.getComment())" % (self.prefix))
        if self.binQon:
            l("for ws in %s:" % (gDataBinQ))
            l("    RenameWorkspace(ws, OutputWorkspace='%s_SQW_' + ws.getComment())" % (self.prefix))

        return script[0]

#-------------------------------------------------------------------------------

class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)

#-------------------------------------------------------------------------------
# eof
