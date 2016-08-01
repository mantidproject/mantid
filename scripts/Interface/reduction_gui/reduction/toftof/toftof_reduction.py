from reduction_gui.reduction.scripter import BaseReductionScripter, BaseScriptElement

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

import xml.dom.minidom
import time

class TOFTOFScriptElement(BaseScriptElement):

    def __init__(self):
        BaseScriptElement.__init__(self)
        self.reset()

    def reset(self):

        self.prefix   = 'ws' # prefix of some workspace names

#self.dataSearchDir    = '/home/jan/C/scg/toftof_data'
#self.rebiningInEnergy = '-15,0.008,1.5'
#self.rebiningInQ      = '0.2,0.1,2.0'
#self.vanRuns  = '12:14'
#self.vanCmts  = 'Van'
#self.ecRuns   = '15:17'
#self.ecCmts   = 'EC'
#self.ecFactor = 0.9
#self.dataRuns = [('27:29','H2O_21C_sqw'), ('30:31','H2O_34C_sqw')]
#self.maskDetectors = '308-312,314'

        self.dataSearchDir    = 'AAA'
        self.rebiningInEnergy = ''
        self.rebiningInQ      = ''

        # vanadium runs
        self.vanRuns  = ''
        self.vanCmts  = ''

        # empty can runs
        self.ecRuns   = ''
        self.ecCmts   = ''
        self.ecFactor = 1.0

        # data runs
        self.dataRuns = []

        # additional detectors to mask
        self.maskDetectors = ''

    xmlTag = 'TOFTOFReductionData'

    def to_xml(self):
        xml = ['']

        def put(tag, s):
            xml[0] += '  <%s>%s</%s>\n' % (tag, str(s), tag)

        put('prefix',           self.prefix)
        put('data_dir',         self.dataSearchDir)
        put('rebinning_energy', self.rebiningInEnergy)
        put('rebinning_q',      self.rebiningInQ)

        put('van_runs',         self.vanRuns)
        put('van_comment',      self.vanCmts)

        put('ec_runs',          self.ecRuns)
        put('ec_comment',       self.ecCmts)
        put('ec_factor',        self.ecFactor)

        for (run, cmt) in self.dataRuns:
            put('data_runs',    run)
            put('data_comment', cmt)

        # additional detectors to mask
        put('mask_detectors', self.maskDetectors)

        xml = '<%s>\n%s</%s>\n' % (self.xmlTag, xml[0], self.xmlTag)

        return xml

    def from_xml(self, xmlString):
        self.reset()

        dom = xml.dom.minidom.parseString(xmlString)
        els = dom.getElementsByTagName(self.xmlTag)

        if els:
            self._from_dom(els[0])

    def _from_dom(self, dom):

        def getStr(tag, default = ''):
            return BaseScriptElement.getStringElement(dom, tag, default)

        def getFlt(tag, default):
            return BaseScriptElement.getFloatElement(dom, tag, default)

        def getStrLst(tag):
            return BaseScriptElement.getStringList(dom, tag)

        self.prefix           = getStr('prefix', 'ws')
        self.dataSearchDir    = getStr('data_dir')
        self.rebiningInEnergy = getStr('rebinning_energy')
        self.rebiningInQ      = getStr('rebinning_q')

        self.vanRuns          = getStr('van_runs')
        self.vanCmts          = getStr('van_comment')

        self.ecRuns           = getStr('ec_runs')
        self.ecCmts           = getStr('ec_comment')
        self.ecFactor         = getFlt('ec_factor', 1.0)

        dataRuns              = getStrLst('data_runs')
        dataCmts              = getStrLst('data_comment')

        for i in range(min(len(dataRuns), len(dataCmts))):
            self.dataRuns.append((dataRuns[i], dataCmts[i]))

        self.maskDetectors = getStr('mask_detectors')

class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility, scriptElement):
        BaseReductionScripter.__init__(self, name, facility)

        self.data = scriptElement # TODO is there a better way to access scriptElement?

    def _error(self, message):
        raise RuntimeError('TOFTOF reduction error: ' + message)

    def to_script(self, file_name = None):

        # briefer access to reduction data

        data   = self.data

        prefix = data.prefix

        dataSearchDir    = data.dataSearchDir
        rebiningInEnergy = data.rebiningInEnergy
        rebiningInQ      = data.rebiningInQ

        vanRuns  = data.vanRuns
        vanCmts  = data.vanCmts

        ecRuns   = data.ecRuns
        ecCmts   = data.ecCmts
        ecFactor = data.ecFactor

        dataRuns = data.dataRuns

        maskDetectors = data.maskDetectors

        # header
        script = '# TOFTOF reduction script\n'                      +\
                 '# Generated on %s\n' % (time.ctime(time.time()))  +\
                 '\nimport numpy as np\n'

        # config
        script += '\n# configuration\n' +\
                  'config["default.facility"]   = "%s"\n' % (self.facility_name)   +\
                  'config["default.instrument"] = "%s"\n' % (self.instrument_name) +\
                  'config.appendDataSearchDir("%s")\n'    % (dataSearchDir)

        script += '\n# binning parameters\n' +\
                  'rebiningInEnergy = "%s"\n' % (rebiningInEnergy) +\
                  'rebiningInQ      = "%s"\n' % (rebiningInQ)

        group = []  # grouped workspaces
        ecIdx = 0   # index of EC, if any

        # vanadium runs
        vanRuns = vanRuns.strip()
        if vanRuns:
            vanCmts = vanCmts.strip()
            if not vanCmts:
                _error('missing vanadium comment')

            wsRawVan = prefix + 'RawVan'
            wsVan    = prefix + 'Van'

            script += '\n# vanadium runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawVan, vanRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsVan, wsRawVan)   +\
                      '%s.setComment("%s")\n'        % (wsVan, vanCmts)

            group.append(wsVan)
            ecIdx = ecIdx + 1

        # empty can runs
        ecRuns = ecRuns.strip()
        if ecRuns:
            ecCmts = ecCmts.strip()
            if not ecCmts:
                _error('missing empty can comment')

            wsRawEC = prefix + 'RawEC'
            wsEC    = prefix + 'EC'

            script += '\n# empty can runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawEC, ecRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsEC, wsRawEC)    +\
                      '%s.setComment("%s")\n'        % (wsEC, ecCmts)

            group.append(wsEC)

        # data runs
        for i, (runs, cmnt) in enumerate(dataRuns):
            runs = runs.strip()
            cmnt = cmnt.strip()

            if not runs:
                _error('missing data run value')
            if not cmnt:
                _error('missing data run comment')

            tag = str(i + 1)

            wsRawData = prefix + 'RawData' + tag
            wsData    = prefix + 'Data'    + tag

            script += '\n# data runs %s\n' % (tag) +\
                      '%s = Load(Filename="%s")\n'   % (wsRawData, runs)   +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsData, wsRawData) +\
                      '%s.setComment("%s")\n'        % (wsData, cmnt)

            group.append(wsData)

        # process
        script += '\n# group runs for processing\n' +\
                  'group = GroupWorkspaces(%s)\n' % str(group)

        script += '\n# mask detectors\n' +\
                  '(detToMask,numberOfFailures) = FindDetectorsOutsideLimits(group)\n' +\
                  'MaskDetectors(group, MaskedWorkspace=detToMask)\n'

        maskDetectors = maskDetectors.strip()
        if maskDetectors:
            script += 'MaskDetectors(group, "%s")\n' % (maskDetectors)

        script += '\n# normalization\n' +\
                  'gNormToMonitor = MonitorEfficiencyCorUser(group)\n'

        if ecRuns:
            script += '\n# scale empty can data\n' +\
                      'scaledEC = Scale(gNormToMonitor.getItem(%d), Factor=%f, Operation="Multiply")' % (ecIdx, ecFactor)

            script += '\n# remove empty can from the group\n' +\
                      'namesNoEC = gNormToMonitor.getNames()\n' +\
                      'del namesNoEC[%d]\n' % (ecIdx)           +\
                      'gNoEC = GroupWorkspaces(namesNoEC)\n'

            script += '\n# subtract empty can\n' +\
                      'gData = Minus(gNoEC, scaledEC)\n'
        else:
            script += '\n# clone group\n' +\
                      'gData = CloneWorkspace(gNormToMonitor)\n'

        if vanRuns:
            wsVanToNorm = prefix + 'VanToNorm'
            script += '\n# normalise to vanadium\n' +\
                      '\nvan = gData.getItem(0)\n'  +\
                      'eppTable = FindEPP(van)\n' +\
                      '%s = ComputeCalibrationCoefVan(van, eppTable)\n'                      % (wsVanToNorm) +\
                      'badDetectors = np.where(np.array(%s.extractY()).flatten() <= 0)[0]\n' % (wsVanToNorm) +\
                      'MaskDetectors(gData, DetectorList=badDetectors)\n' +\
                      'gDataCorr = Divide(gData, %s)\n'                                      % (wsVanToNorm)

        script += '\n# remove half-filled time bins (clean frame)\n' +\
                  'gDataCleanFrame = TOFTOFCropWorkspace(%s)\n' % ('gDataCorr' if vanRuns else 'gData')

        script += '\n# apply TOF correction\n' +\
                  'gDataTofCorr = CorrectTOF(gDataCleanFrame, eppTable)\n' +\
                  'energyIn = gDataTofCorr.getItem(0).getRun().getLogData("Ei").value\n' +\
                  'gData_dEraw = ConvertUnits(gDataTofCorr, Target="DeltaE", EMode="Direct", EFixed=energyIn)\n' +\
                  'ConvertToDistribution(gData_dEraw)\n'

        script += '\n# correct for energy dependent detector efficiency\n' +\
                  'DetectorEfficiencyCorUser(InputWorkspace="gData_dEraw", OutputWorkspace="gData_dEcorr")\n'

        script += '\n# calculate S (Ki/kF correction)\n' +\
                  'CorrectKiKf(InputWorkspace="gData_dEcorr", OutputWorkspace="gData_S")\n'

        script += '\n# rebiningInEnergy\n' +\
                  'Rebin(InputWorkspace="gData_S", OutputWorkSpace="mBData_dE", Params=rebiningInEnergy)\n'

        script += '\n# calculate momentum transfer Q for sample data\n' +\
                  'mBData = SofQW3(InputWorkspace="mBData_dE", QAxisBinning=rebiningInQ, EMode="Direct", EFixed=energyIn)\n'

        script += '\n# make nice workspace names\n' +\
                  'for ws in mBData:\n' +\
                  '    RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws.getComment())\n'

        return script
