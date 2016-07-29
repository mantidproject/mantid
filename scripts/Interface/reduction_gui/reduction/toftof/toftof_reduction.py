from reduction_gui.reduction.scripter import BaseReductionScripter, BaseScriptElement

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

import time

class TOFTOFScriptElement(BaseScriptElement):

    # defaults
    pieceOfData = 0.5 # TODO

    def __init__(self):
        BaseScriptElement.__init__(self)
        self.reset()

    def reset(self):
        # data runs
        self.dataRuns = []

    def numDataRunsRows(self):
        return len(self.dataRuns)

    def _getDataRunsRow(self, row):
        return self.dataRuns[row] if row < self.numDataRunsRows() else ('', '')

    def _isDataRunsRowEmpty(self, row):
        (t1, t2) = self._getDataRunsRow(row)
        return not t1.strip() and not t2.strip()

    def _ensureDataRunsRows(self, rows):
        while self.numDataRunsRows() < rows:
            self.dataRuns.append(('', ''))

    def updateDataRuns(self):
        ''' Remove trailing empty rows. '''
        for row in reversed(range(self.numDataRunsRows())):
            if self._isDataRunsRowEmpty(row):
                del self.dataRuns[row]
            else:
                break

    def setDataRun(self, row, col, text):
        self._ensureDataRunsRows(row + 1)
        (runText, comment) = self.dataRuns[row]

        text = text.strip()
        if 0 == col:
            runText = text
        else:
            comment = text
        self.dataRuns[row] = (runText, comment)

    def getDataRun(self, row, col):
        return self._getDataRunsRow(row)[col]
        cls = self.__class__
        self.pieceOfData = cls.pieceOfData

    def to_script(self):
        script  = ''
        # script += "SetPieceOfData(%g)\n" % (self.pieceOfData)
        return script

    def to_xml(self): # save
        xml_out  = "<DirectBeam>\n"
        xml_out += "  <sample_file>%s</sample_file>\n" % self.pieceOfData
        xml_out += "</DirectBeam>\n"
        return xml_out

    def from_xml(self, dom): # on load
        self.reset()
#        element_list = dom.getElementsByTagName("DirectBeam")
#        if len(element_list)>0:
#            instrument_dom = element_list[0]
#            self.pieceOfData = BaseScriptElement.getStringElement(instrument_dom, "sample_file")

#    def from_setup_info(self, xml_str):
#        print 'FROM_SETUP_INFO>>', xml_str, '<<\n'
#        self.reset()
#        (alg, _) = BaseScriptElement.getAlgorithmFromXML(xml_str)
#        self.pieceOfData = BaseScriptElement.getPropertyValue(
#            alg, "SampleDetectorDistance", default=ReductionOptions.sample_detector_distance)

class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)

    def _error(self, message):
        raise RuntimeError('TOFTOF reduction error: ' + message)

    def to_script(self, file_name = None):
        for item in self._observers:
            print item
#            if item.state() is not None:
#                script += str(item.state())

        prefix = 'ws'

        dataSearchDir    = '/home/jan/C/scg/toftof_data'
        rebiningInEnergy = '-15,0.008,1.5'
        rebiningInQ      = '0.2,0.1,2.0'

        vanRuns  = '12:14'
        vanCmnt  = 'Van'

        ecRuns   = '15:17'
        ecCmnt   = 'EC'
        ecFactor = 0.9

        dataRuns = [('27:29','H2O_21C_sqw'), ('30:31','H2O_34C_sqw')]

        maskDetectors = '308-312,314'

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
            vanCmnt = vanCmnt.strip()
            if not vanCmnt:
                _error('missing vanadium comment')

            wsRawVan = prefix + 'RawVan'
            wsVan    = prefix + 'Van'

            script += '\n# vanadium runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawVan, vanRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsVan, wsRawVan)   +\
                      '%s.setComment("%s")\n'        % (wsVan, vanCmnt)

            group.append(wsVan)
            ecIdx = ecIdx + 1

        # empty can runs
        ecRuns = ecRuns.strip()
        if ecRuns:
            ecCmnt = ecCmnt.strip()
            if not ecCmnt:
                _error('missing empty can comment')

            wsRawEC = prefix + 'RawEC'
            wsEC    = prefix + 'EC'

            script += '\n# empty can runs\n' +\
                      '%s = Load(Filename="%s")\n'   % (wsRawEC, ecRuns) +\
                      '%s = TOFTOFMergeRuns("%s")\n' % (wsEC, wsRawEC)    +\
                      '%s.setComment("%s")\n'        % (wsEC, ecCmnt)

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

    def set_options(self):
        """
            Set up the reduction options, without executing
        """
        if HAS_MANTID:
            self.update()
            table_ws = "__patch_options"
            script = "SetupHFIRReduction(\n"
            for item in self._observers:
                if item.state() is not None:
                    if hasattr(item.state(), "options"):
                        script += item.state().options()

            script += "ReductionProperties='%s')" % table_ws
            mantidplot.runPythonScript(script, True)
            return table_ws
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"
