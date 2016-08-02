from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

import xml.dom.minidom

#-------------------------------------------------------------------------------

class TOFTOFScriptElement(BaseScriptElement):

    correctTofNone   = 0
    correctTofVan    = 1
    correctTofSample = 2

    default_prefix     = 'ws'
    default_ecFactor   = 1.0
    default_subECVan   = False
    default_correctTof = correctTofNone

    def reset(self):
        self.facility_name   = ''
        self.instrument_name = ''

        # prefix of (some) workspace names
        self.prefix   = self.default_prefix

        # data files are here
        self.dataDir  = ''

        # vanadium runs & comment
        self.vanRuns  = ''
        self.vanCmnt  = ''

        # empty can runs, comment, and factor
        self.ecRuns   = ''
        self.ecCmnt   = ''
        self.ecFactor = self.default_ecFactor

        # data runs: [(runs,comment), ...]
        self.dataRuns = []

        # additional parameters
        self.rebinEnergy   = ''
        self.rebinQ        = ''
        self.maskDetectors = ''

        # flags
        self.subtractECVan = self.default_subECVan
        self.correctTof    = self.default_correctTof

    xmlTag = 'TOFTOFReduction'

    def to_xml(self):
        xml = ['']

        def put(tag, val):
            xml[0] += '  <%s>%s</%s>\n' % (tag, str(val), tag)

        put('prefix',      self.prefix)
        put('data_dir',    self.dataDir)

        put('van_runs',    self.vanRuns)
        put('van_comment', self.vanCmnt)

        put('ec_runs',     self.ecRuns)
        put('ec_comment',  self.ecCmnt)
        put('ec_factor',   self.ecFactor)

        for (runs, cmnt) in self.dataRuns:
            put('data_runs',    runs)
            put('data_comment', cmnt)

        put('rebin_energy',   self.rebinEnergy)
        put('rebin_q',        self.rebinQ)
        put('mask_detectors', self.maskDetectors)

        put('subtract_ecvan', self.subtractECVan)
        put('correct_tof',    self.correctTof)

        xml = '<%s>\n%s</%s>\n' % (self.xmlTag, xml[0], self.xmlTag)

        return xml

    def from_xml(self, xmlString):
        self.reset()

        dom = xml.dom.minidom.parseString(xmlString)
        els = dom.getElementsByTagName(self.xmlTag)

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

            self.prefix   = getStr('prefix', self.default_prefix)
            self.dataDir  = getStr('data_dir')

            self.vanRuns  = getStr('van_runs')
            self.vanCmnt  = getStr('van_comment')

            self.ecRuns   = getStr('ec_runs')
            self.ecCmnt   = getStr('ec_comment')
            self.ecFactor = getFlt('ec_factor', self.default_ecFactor)

            dataRuns = getStrLst('data_runs')
            dataCmts = getStrLst('data_comment')
            for i in range(min(len(dataRuns), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataCmts[i]))

            self.rebinEnergy   = getStr('rebin_energy')
            self.rebinQ        = getStr('rebin_q')
            self.maskDetectors = getStr('mask_detectors')

            self.subtractECVan = getBool('subtract_ecvan')
            self.correctTof    = getInt('correct_tof', self.default_correctTof)

    def to_script(self):

        def error(message):
            raise RuntimeError('TOFTOF reduction error: ' + message)

        # sanity checks

        if self.correctTofVan == self.correctTof:
            if not self.vanRuns:
                error('missing vanadium run')

        if self.subtractECVan:
            if not self.vanRuns:
                error('missing vanadium run')
            if not self.ecRuns:
                error('missing empty can run')

        if not self.dataRuns:
            error('missig data runs')

        if self.vanRuns and not self.vanCmnt:
            error('missing vanadium comment')

        if self.ecRuns and not self.ecCmnt:
            error('missing empty can comment')

        if not self.rebinEnergy:
            error('missing rebin energy')

        if not self.rebinQ:
            error('missing rebin Q')

        # generate script

        script = 'import numpy as np\n\n'

        script += 'config["default.facility"]   = "%s"\n'   % (self.facility_name)   +\
                  'config["default.instrument"] = "%s"\n\n' % (self.instrument_name) +\
                  'config.appendDataSearchDir("%s")\n\n'    % (self.dataDir)         +\
                  'rebinEnergy = "%s"\n'                    % (self.rebinEnergy)     +\
                  'rebinQ      = "%s"\n\n'                  % (self.rebinQ)

        group = []  # grouped workspaces
        ecIdx = 0   # the index of EC, if any

        # vanadium runs
        if self.vanRuns:
            wsRawVan = self.prefix + 'RawVan'
            wsVan    = self.prefix + 'Van'

            script += '# vanadium runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawVan, self.vanRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsVan, wsRawVan)        +\
                      '%s.setComment("%s")\n\n'      % (wsVan, self.vanCmnt)

            group.append(wsVan)
            ecIdx = ecIdx + 1

        # empty can runs
        if self.ecRuns:
            wsRawEC = self.prefix + 'RawEC'
            wsEC    = self.prefix + 'EC'

            script += '# empty can runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawEC, self.ecRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsEC, wsRawEC)        +\
                      '%s.setComment("%s")\n\n'      % (wsEC, self.ecCmnt)

            group.append(wsEC)

        # data runs
        for i, (runs, cmnt) in enumerate(self.dataRuns):
            if not runs:
                error('missing data runs value')
            if not cmnt:
                error('missing data runs comment')

            postfix = str(i + 1)

            wsRawData = self.prefix + 'RawData' + postfix
            wsData    = self.prefix + 'Data'    + postfix

            script += '# data runs %s\n' % (postfix) +\
                      '%s = Load(Filename="%s")\n'   % (wsRawData, runs)   +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsData, wsRawData) +\
                      '%s.setComment("%s")\n\n'      % (wsData, cmnt)

            group.append(wsData)

        # group all workspaces
        script += '# group runs for processing\n' +\
                  'group = GroupWorkspaces("%s")\n\n' % (', '.join(group))

        # Ei
        # TODO loop through all workspaces and check that Ei is (almost) the same
        script += 'Ei = group.getItem(0).getRun().getLogData("Ei").value\n\n'

        # mask detectors
        script += '# mask detectors\n' +\
                  '(detToMask,numberOfFailures) = FindDetectorsOutsideLimits(group)\n' +\
                  'MaskDetectors(group, MaskedWorkspace=detToMask)\n'

        if self.maskDetectors:
            script += 'MaskDetectors(group, "%s")\n\n' % (self.maskDetectors)
        else:
            script += '\n'

        # normalize
        script += '# normalization\n' +\
                  'gNormToMonitor = MonitorEfficiencyCorUser(group)\n\n'

        # subtract empty can
        if self.ecRuns:
            script += '# scale empty can data\n' +\
                      'scaledEC = Scale(gNormToMonitor.getItem(%d), Factor=%f, Operation="Multiply")\n\n' % (ecIdx, self.ecFactor)

            if self.subtractECVan or not self.vanRuns: # have vanadium in a can
                script += '# remove empty can from the group\n' +\
                          'names = gNormToMonitor.getNames()\n'   +\
                          'del names[%d]\n' % (ecIdx)             +\
                          'gNoEC = GroupWorkspaces(names)\n\n'    +\
                          '# subtract empty can\n'                +\
                          'gData = Minus(gNoEC, scaledEC)\n'      +\
                          'UnGroupWorkspace(gNoEC)\n\n'
            else: # have vanadium without a can
                script += '# remove vanadium and empty can from the group\n' +\
                          'names = gNormToMonitor.getNames()\n'   +\
                          'van = names[0]\n'                      +\
                          'del names[0:2]\n'                      +\
                          'gNoVanEC = GroupWorkspaces(names)\n\n' +\
                          '# subtract empty can\n'                +\
                          'gMinus = Minus(gNoVanEC, scaledEC)\n'  +\
                          'UnGroupWorkspace(gNoVanEC)\n\n'        +\
                          '# put vanadium back\n'                 +\
                          'gData = GroupWorkspaces([van] + list(gMinus.getnames())\n\n'
        else:
            script += '\n# clone group\n' +\
                      'gData = CloneWorkspace(gNormToMonitor)\n\n'

        # subtract empty can
        if self.ecRuns:
            script += '# scale empty can data\n' +\
                      'scaledEC = Scale(gNormToMonitor.getItem(%d), Factor=%f, Operation="Multiply")\n\n' % (ecIdx, self.ecFactor)

            script += '# remove empty can from the group\n' +\
                      'names = gNormToMonitor.getNames()\n' +\
                      'del names[%d]\n' % (ecIdx)           +\
                      'gNoEC = GroupWorkspaces(names)\n\n'  +\
                      '# subtract empty can\n'

            if self.subtractECVan:
                script += 'gData = Minus(gNoEC, scaledEC)\n'
            else:
                script += 'del names[0]\n' +\
                          'gNoECNoVan = GroupWorkspaces(names)\n'  +\
                          'gMinus = Minus(gNoECNoVan, scaledEC)\n' +\
                          'UnGroupWorkspace(gNoECNoVan)\n'         +\
                          'gData = CloneWorkspace(gMinus)\n'       +\
                          'UnGroupWorkspace(gMinus)\n\n'

            script += 'UnGroupWorkspace(gNoEC)\n\n'

        else:
            script += '\n# clone group\n' +\
                      'gData = CloneWorkspace(gNormToMonitor)\n\n'

        # normalize to vanadium
        if self.vanRuns:
            wsVanToNorm = self.prefix + 'VanToNorm'
            script += '# normalise to vanadium\n' +\
                      'van = gData.getItem(0)\n'  +\
                      'eppTable = FindEPP(van)\n' +\
                      '%s = ComputeCalibrationCoefVan(van, eppTable)\n'                      % (wsVanToNorm) +\
                      'badDetectors = np.where(np.array(%s.extractY()).flatten() <= 0)[0]\n' % (wsVanToNorm) +\
                      'MaskDetectors(gData, DetectorList=badDetectors)\n' +\
                      'gDataCorr = Divide(gData, %s)\n\n'                                    % (wsVanToNorm)

        # clean frame
        script += '# remove half-filled time bins (clean frame)\n' +\
                  'gDataCleanFrame = TOFTOFCropWorkspace(%s)\n\n' % ('gDataCorr' if self.vanRuns else 'gData')

        # TOF correction
        if self.correctTofVan == self.correctTof:
            script += '# apply vanadium TOF correction\n' +\
                      'gDataTofCorr = CorrectTOF(gDataCleanFrame, eppTable)\n\n'

        elif self.correctTofSample == self.correctTof:
            script += '# apply sample TOF correction\n' +\
                      'eppTables = FindEPP(gData)\n'    +\
                      'gDataTofCorr = CorrectTOF(gDataCleanFrame, eppTables)\n\n'

        else:
            script += '# no TOF correction\n' +\
                      'gDataTofCorr = CloneWorkspace(gDataCleanFrame)\n\n'

        # ... and the rest
        script += '# convert units\n' +\
                  'gData_dEraw = ConvertUnits(gDataTofCorr, Target="DeltaE", EMode="Direct", EFixed=Ei)\n' +\
                  'ConvertToDistribution(gData_dEraw)\n\n'

        script += '# correct for energy dependent detector efficiency\n' +\
                  'DetectorEfficiencyCorUser(InputWorkspace="gData_dEraw", OutputWorkspace="gData_dEcorr")\n\n'

        script += '# calculate S (Ki/kF correction)\n' +\
                  'CorrectKiKf(InputWorkspace="gData_dEcorr", OutputWorkspace="gData_S")\n\n'

        script += '# rebinEnergy\n' +\
                  'Rebin(InputWorkspace="gData_S", OutputWorkSpace="mBData_dE", Params=rebinEnergy)\n\n'

        script += '# calculate momentum transfer Q for sample data\n' +\
                  'mBData = SofQW3(InputWorkspace="mBData_dE", QAxisBinning=rebinQ, EMode="Direct", EFixed=Ei)\n\n'

        script += '# make nice workspace names\n' +\
                  'for ws in mBData:\n' +\
                  '    RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws.getComment())\n\n'

        return script

#-------------------------------------------------------------------------------

class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)

#-------------------------------------------------------------------------------
# eof
