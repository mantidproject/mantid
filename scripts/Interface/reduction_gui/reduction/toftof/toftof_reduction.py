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
        self.rebinEnergy   = ''
        self.rebinQ        = ''
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

        put('rebin_energy',   self.rebinEnergy)
        put('rebin_q',        self.rebinQ)
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
                return BaseScriptElement.getStringElement(dom, tag, default)

            def getInt(tag, default):
                return BaseScriptElement.getIntElement(dom, tag, default)

            def getFlt(tag, default):
                return BaseScriptElement.getFloatElement(dom, tag, default)

            def getStrLst(tag):
                return BaseScriptElement.getStringList(dom, tag)

            def getBool(tag):
                return BaseScriptElement.getBoolElement(dom, tag)

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

            self.rebinEnergy   = getStr('rebin_energy')
            self.rebinQ        = getStr('rebin_q')
            self.maskDetectors = getStr('mask_detectors')

            self.subtractECVan = getBool('subtract_ecvan')
            self.normalise     = getInt('normalise',   self.DEF_normalise)
            self.correctTof    = getInt('correct_tof', self.DEF_correctTof)

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

        # must have some data runs
        if not self.dataRuns:
            error('missig data runs')

        # must have a comment for runs
        if self.vanRuns and not self.vanCmnt:
            error('missing vanadium comment')

        # must have rebinning energy (TODO ?)
        if not self.rebinEnergy:
            error('missing rebin energy')

        # must have rebinning Q (TODO ?)
        if not self.rebinQ:
            error('missing rebin Q')

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
        l("ecFactor = '%f'"                     % (self.ecFactor))
        l()
        l("rebinEnergy = '%s'"                  % (self.rebinEnergy))
        l("rebinQ = '%s'"                       % (self.rebinQ))
        l()

        # vanadium runs
        if self.vanRuns:
            wsRawVan = self.prefix + 'RawVan'
            wsVan    = self.prefix + 'Van'

            l("# vanadium runs")
            l("%s = Load(Filename='%s')" % (wsRawVan, self.vanRuns))
            l("%s = TOFTOFMergeRuns(%s)" % (wsVan, wsRawVan))
            l("%s.setComment('%s')"      % (wsVan, self.vanCmnt))
            l()

        # empty can runs
        if self.ecRuns:
            wsRawEC = self.prefix + 'RawEC'
            wsEC    = self.prefix + 'EC'

            l("# empty can runs")
            l("%s = Load(Filename='%s')" % (wsRawEC, self.ecRuns))
            l("%s = TOFTOFMergeRuns(%s)" % (wsEC, wsRawEC))
            l()

        # data runs
        dataGroup = []
        for i, (runs, cmnt) in enumerate(self.dataRuns):
            if not runs:
                error('missing data runs value')
            if not cmnt:
                error('missing data runs comment')

            postfix = str(i + 1)

            wsRawData = self.prefix + 'RawData' + postfix
            wsData    = self.prefix + 'Data'    + postfix

            dataGroup.append(wsData)

            l("# data runs %s" % (postfix))
            l("%s = Load(Filename='%s')" % (wsRawData, runs))
            l("%s = TOFTOFMergeRuns(%s)" % (wsData, wsRawData))
            l("%s.setComment('%s')"      % (wsData, cmnt))
            l()
            if 0 == i:
                l("Ei = %s" % (logEi(wsData)))
            else:
                l("if abs(Ei - %s) > 0.0001:" % (logEi(wsData)))
                l("    raise RuntimeError('bad Ei')")
            l()

        def gr(list, postfix = ''):
            return ('[' + ', '.join(list) + ']' + postfix) if list else ''

        l("# grouping")
        l("dataRunsNames = %s" % gr(dataGroup))
        l("gDataRuns = GroupWorkspaces(dataRunsNames)")
        l("gAll = GroupWorkspaces(%s%sdataRunsNames)" % (gr([wsVan],' + ') if self.vanRuns else '',\
                                                         gr([wsEC], ' + ') if self.ecRuns  else ''))
        l()

        l("# mask detectors")
        l("(detToMask, numberOfFailures) = FindDetectorsOutsideLimits(gAll)" )
        l("MaskDetectors(gAll, MaskedWorkspace=detToMask)")

        if self.maskDetectors:
            l("MaskDetectors(gAll, '%s')" % (self.maskDetectors))
        l()

        if self.NORM_MONITOR == self.normalise:
            gDataNorm = 'gDataNorm'

            l("# normaliseto monitor")
            l("%s = MonitorEfficiencyCorUser(gDataRuns)" % (gDataNorm))

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("%s = MonitorEfficiencyCorUser(%s)" % (wsVanNorm, wsVan))
            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("%s = MonitorEfficiencyCorUser(%s)" % (wsECNorm, wsEC))
            l()

        elif self.NORM_TIME == self.normalise:
            gDataNorm = 'gDataNorm'

            # TODO (ask Wiebke how to do it best)
            l("# normalise to time")
            l("names = []")
            l("for ws in gDataRuns:")
            l("    name = ws.getName() + '_norm'")
            l("    names.append(name)")
            l("    Scale(ws, 1/%s, 'Multiply', OutputWorkspace=name)" % (logTime('ws')))
            l()
            l("%s = GroupWorkspaces(names)" % (gDataNorm))

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("%s = Scale(%s, 1/%s, 'Multiply')" % (wsVanNorm, wsVan, logTime(wsVan)))
            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("%s = Scale(%s, 1/%s, 'Multiply')" % (wsECNorm, wsEC, logTime(wsEC)))
            l()

        else: # none, simply use the not normalised workspaces
            gDataNorm = 'gDataRuns'

            if self.vanRuns:
                wsVanNorm = wsVan
            if self.ecRuns:
                wsECNorm = wsEC

        if self.ecRuns:
            l("# subtract empty can")
            l("scaledEC = Scale(%s, Factor=ecFactor, Operation='Multiply')" % (wsECNorm))
            l("gDataSubEC = Minus(%s, scaledEC)" % (gDataNorm))
            if self.subtractECVan:
                wsVanSubEC = wsVan + 'SubEC'
                l("%s = Minus(%s, scaledEC)" % (wsVanSubEC, wsVanNorm))
            l()

        l("# group data for processing") # no empty can, es ist fertig
        gData = 'gDataSubEC' if self.ecRuns else gDataNorm
        if self.vanRuns:
            wsVanNorm = wsVanSubEC if self.subtractECVan else wsVanNorm
            l("gData = GroupWorkspaces(%slist(%s.getNames()))" % (gr([wsVanNorm], ' + '), gData))
        else:
            l("gData = CloneWorkspace(%s)" % (gData))
        l()

        if self.vanRuns:
            l("# normalise to vanadium")
            l("eppTable = FindEPP(%s)" % (wsVanNorm))
            l("detCoeffs = ComputeCalibrationCoefVan(%s, eppTable)" % (wsVanNorm))
            l("badDetectors = np.where(np.array(detCoeffs.extractY()).flatten() <= 0)[0]")
            l("MaskDetectors(gData, DetectorList=badDetectors)")
            l("gDataCorr = Divide(gData, detCoeffs)")
            l()

        l("# remove half-filled time bins (clean frame)")
        l("gDataCleanFrame = TOFTOFCropWorkspace(%s)" % ('gDataCorr' if self.vanRuns else 'gData'))
        l()

        gData = 'gDataTofCorr'
        if self.CORR_TOF_VAN == self.correctTof:
            l("# apply vanadium TOF correction")
            l("%s = CorrectTOF(gDataCleanFrame, eppTable)" % (gData))
            l()

        elif self.CORR_TOF_SAMPLE == self.correctTof:
            l("# apply sample TOF correction")
            l("eppTables = FindEPP(gData)")
            l("%s = CorrectTOF(gDataCleanFrame, eppTables)" % (gData))
            l()

        else:
            gData = 'gDataCleanFrame'

        l("# convert units")
        l("gData_dEraw = ConvertUnits(%s, Target='DeltaE', EMode='Direct', EFixed=Ei)" % (gData))
        l("ConvertToDistribution(gData_dEraw)")
        l()

        l("# correct for energy dependent detector efficiency")
        l("gData_dEcorr = DetectorEfficiencyCorUser(gData_dEraw)")
        l()

        l("# calculate S (Ki/kF correction)")
        l("gData_S = CorrectKiKf(gData_dEcorr)")
        l()

        l("# rebinEnergy")
        l("mBData_dE = Rebin(gData_S, Params=rebinEnergy)")
        l()

        l("# calculate momentum transfer Q for sample data")
        l("mBData = SofQW3(mBData_dE, QAxisBinning=rebinQ, EMode='Direct', EFixed=Ei)")
        l()

        l("# make nice workspace names")
        l("for ws in mBData:")
        l("    RenameWorkspace(ws, OutputWorkspace=ws.getComment())")

        return script[0]

#-------------------------------------------------------------------------------

class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)

#-------------------------------------------------------------------------------
# eof
