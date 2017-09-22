#pylint: disable = too-many-instance-attributes, too-many-locals, too-many-branches
#pylint: disable = attribute-defined-outside-init
#pylint: disable = invalid-name
#pylint: disable = W0622
"""
TOFTOF reduction workflow gui.
"""
from __future__ import (absolute_import, division, print_function)
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter

# -------------------------------------------------------------------------------


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
    DEF_replaceNaNs = False
    DEF_createDiff  = False
    DEF_keepSteps   = False
    DEF_normalise  = NORM_NONE
    DEF_correctTof = CORR_TOF_NONE

    DEF_saveSofQW  = False
    DEF_saveSofTW  = False
    DEF_saveNXSPE  = False
    DEF_saveNexus  = False
    DEF_saveAscii  = False

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
        self.replaceNaNs   = self.DEF_replaceNaNs
        self.createDiff    = self.DEF_createDiff
        self.keepSteps     = self.DEF_keepSteps

        # save data
        self.saveDir      = ''
        self.saveSofQW    = self.DEF_saveSofQW
        self.saveSofTW    = self.DEF_saveSofTW
        self.saveNXSPE    = self.DEF_saveNXSPE
        self.saveNexus    = self.DEF_saveNexus
        self.saveAscii    = self.DEF_saveAscii

    def to_xml(self):
        res = ['']

        def put(tag, val):
            res[0] += '  <{0}>{1}</{0}>\n'.format(tag, str(val))

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
        put('replace_nans',   self.replaceNaNs)
        put('create_diff',    self.createDiff)
        put('keep_steps',     self.keepSteps)

        put('save_dir',      self.saveDir)
        put('save_sofqw',    self.saveSofQW)
        put('save_softw',    self.saveSofTW)
        put('save_nxspe',    self.saveNXSPE)
        put('save_nexus',    self.saveNexus)
        put('save_ascii',    self.saveAscii)

        return '<{0}>\n{1}</{0}>\n'.format(self.XML_TAG, res[0])

    def from_xml(self, xmlString):
        self.reset()

        dom = xml.dom.minidom.parseString(xmlString)
        els = dom.getElementsByTagName(self.XML_TAG)

        if els:
            dom = els[0]

            def get_str(tag, default=''):
                return BaseScriptElement.getStringElement(dom, tag, default=default)

            def get_int(tag, default):
                return BaseScriptElement.getIntElement(dom, tag, default=default)

            def get_flt(tag, default):
                return BaseScriptElement.getFloatElement(dom, tag, default=default)

            def get_strlst(tag):
                return BaseScriptElement.getStringList(dom, tag)

            def get_bol(tag, default):
                return BaseScriptElement.getBoolElement(dom, tag, default=default)

            self.prefix   = get_str('prefix', self.DEF_prefix)
            self.dataDir  = get_str('data_dir')

            self.vanRuns  = get_str('van_runs')
            self.vanCmnt  = get_str('van_comment')

            self.ecRuns   = get_str('ec_runs')
            self.ecFactor = get_flt('ec_factor', self.DEF_ecFactor)

            dataRuns = get_strlst('data_runs')
            dataCmts = get_strlst('data_comment')
            for i in range(min(len(dataRuns), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataCmts[i]))

            self.binEon    = get_bol('rebin_energy_on',    self.DEF_binEon)
            self.binEstart = get_flt('rebin_energy_start', self.DEF_binEstart)
            self.binEstep  = get_flt('rebin_energy_step',  self.DEF_binEstep)
            self.binEend   = get_flt('rebin_energy_end',   self.DEF_binEend)

            self.binQon    = get_bol('rebin_q_on',         self.DEF_binQon)
            self.binQstart = get_flt('rebin_q_start',      self.DEF_binQstart)
            self.binQstep  = get_flt('rebin_q_step',       self.DEF_binQstep)
            self.binQend   = get_flt('rebin_q_end',        self.DEF_binQend)

            self.maskDetectors = get_str('mask_detectors')

            self.subtractECVan = get_bol('subtract_ecvan', self.DEF_subECVan)
            self.normalise     = get_int('normalise',      self.DEF_normalise)
            self.correctTof    = get_int('correct_tof',    self.DEF_correctTof)
            self.replaceNaNs   = get_bol('replace_nans',   self.DEF_replaceNaNs)
            self.createDiff    = get_bol('create_diff',    self.DEF_createDiff)
            self.keepSteps     = get_bol('keep_steps',     self.DEF_keepSteps)

            self.saveDir     = get_str('save_dir')
            self.saveSofQW   = get_bol('save_sofqw',   self.DEF_saveSofQW)
            self.saveSofTW   = get_bol('save_softw',   self.DEF_saveSofTW)
            self.saveNXSPE   = get_bol('save_nxspe',   self.DEF_saveNXSPE)
            self.saveNexus   = get_bol('save_nexus',   self.DEF_saveNexus)
            self.saveAscii   = get_bol('save_ascii',   self.DEF_saveAscii)

    def to_script(self):

        def error(message):
            raise RuntimeError('TOFTOF reduction error: ' + message)

        # sanity checks

        # must have vanadium for TOF correction
        if self.CORR_TOF_VAN == self.correctTof:
            if not self.vanRuns:
                error('missing vanadium runs')

        # must have vanadium and empty can for subtracting EC from van
        if self.subtractECVan:
            if not self.vanRuns:
                error('missing vanadium runs')
            if not self.ecRuns:
                error('missing empty can runs')

        # binning parameters
        def check_bin_params(start, step, end):
            if not (start < end and step > 0 and start + step <= end):
                error('incorrect binning parameters')

        if self.binEon:
            check_bin_params(self.binEstart, self.binEstep, self.binEend)
        if self.binQon:
            check_bin_params(self.binQstart, self.binQstep, self.binQend)

        # must have some data runs
        if not self.dataRuns:
            error('missing data runs')

        # must have a comment for runs
        if self.vanRuns and not self.vanCmnt:
            error('missing vanadium comment')

        # saving settings must be consistent
        if self.saveNXSPE or self.saveNexus or self.saveAscii:
            if not self.saveDir:
                error('missing directory to save the data')
            elif not (self.saveSofQW or self.saveSofTW):
                error('you must select workspaces to save')
        elif self.saveDir or self.saveSofQW or self.saveSofTW:
            error('missing data format to save')

        # generated script
        script = ['']

        # helper: add a line to the script
        def l(line=''):
            script[0] += line + '\n'

        # helpers
        def get_log(workspace, tag):
            return "{}.getRun().getLogData('{}').value".format(workspace, tag)

        def get_ei(workspace):
            return get_log(workspace, 'Ei')

        def get_time(workspace):
            return get_log(workspace, 'duration')

        def save_wsgroup(wsgroup, suffix):
            l("# save {}".format(wsgroup))
            l("for ws in {}:".format(wsgroup))
            l("    name = ws.getComment() + {}".format(suffix))
            if self.saveNXSPE:
                l("    SaveNXSPE(ws, join('{}',name + '.nxspe'), Efixed=Ei)".format(self.saveDir))
            if self.saveNexus:
                l("    SaveNexus(ws, join('{}',name + '.nxs'))".format(self.saveDir))
            l()


        l("import numpy as np")
        l("from os.path import join")
        l()
        l("config['default.facility'] = '{}'"   .format(self.facility_name))
        l("config['default.instrument'] = '{}'" .format(self.instrument_name))
        l()
        l("config.appendDataSearchDir(r'{}')"   .format(self.dataDir))
        l()

        dataRawGroup = []
        dataGroup    = []
        allGroup     = []

        # if not self.keepSteps, delete the workspaces imediately, to free the memory
        # vanadium runs
        if self.vanRuns:
            wsRawVan = self.prefix + 'RawVan'
            wsVan    = self.prefix + 'Van'

            l("# vanadium runs")
            l("{} = Load(Filename='{}')" .format(wsRawVan, self.vanRuns))
            l("{} = TOFTOFMergeRuns({})" .format(wsVan, wsRawVan))
            l("{}.setComment('{}')"      .format(wsVan, self.vanCmnt))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(wsRawVan))
            l()

            allGroup.append(wsVan)

        # empty can runs
        if self.ecRuns:
            wsRawEC = self.prefix + 'RawEC'
            wsEC    = self.prefix + 'EC'

            l("# empty can runs")
            l("{} = Load(Filename='{}')" .format(wsRawEC, self.ecRuns))
            l("{} = TOFTOFMergeRuns({})" .format(wsEC, wsRawEC))
            l("{}.setComment('EC')".format(wsEC))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(wsRawEC))
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

            l("# data runs {}"           .format(postfix))
            l("{} = Load(Filename='{}')" .format(wsRawData, runs))
            l("{} = TOFTOFMergeRuns({})" .format(wsData, wsRawData))
            l("{}.setComment('{}')"      .format(wsData, cmnt))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(wsRawData))
            l()

            if i == 0:
                wsData0 = wsData

        def group_list(listVal, postfix=''):
            return ('[' + ', '.join(listVal) + ']' + postfix) if listVal else ''

        gPrefix = 'g' + self.prefix
        gDataRuns    = gPrefix + 'DataRuns'
        gDataRawRuns = gPrefix + 'DataRawRuns'
        gAll         = gPrefix + 'All'

        l("# grouping")
        if self.keepSteps:
            l("{} = GroupWorkspaces({})" .format(gDataRawRuns, group_list(dataRawGroup)))
        l("{} = GroupWorkspaces({})" .format(gDataRuns,    group_list(dataGroup)))
        l("{} = GroupWorkspaces({})" .format(gAll,         group_list(allGroup)))
        l()

        l("# Ei")
        if len(allGroup) > 1:
            l("if CompareSampleLogs({}, 'Ei', 0.001):" .format(gAll))
            l("    raise RuntimeError('Ei values do not match')")
            l()

        l("Ei = {}" .format(get_ei(wsData0)))
        l()

        gDetectorsToMask = gPrefix + 'DetectorsToMask'
        l("# mask detectors")
        l("({}, numberOfFailures) = FindDetectorsOutsideLimits({})" .format(gDetectorsToMask, gAll))
        l("MaskDetectors({}, MaskedWorkspace={})" .format(gAll, gDetectorsToMask))
        if not self.keepSteps:
            l("DeleteWorkspace({})" .format(gDetectorsToMask))

        if self.maskDetectors:
            l("MaskDetectors({}, DetectorList='{}')" .format(gAll, self.maskDetectors))

        l()

        if self.NORM_MONITOR == self.normalise:
            gDataNorm = gPrefix + 'Norm'

            l("# normalise to monitor")

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("{} = MonitorEfficiencyCorUser({})" .format(wsVanNorm, wsVan))

            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("{} = MonitorEfficiencyCorUser({})" .format(wsECNorm, wsEC))

            l("{} = MonitorEfficiencyCorUser({})"     .format(gDataNorm, gDataRuns))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(gAll))
            l()

        elif self.NORM_TIME == self.normalise:
            gDataNorm = gPrefix + 'Norm'

            l("# normalise to time")

            if self.vanRuns:
                wsVanNorm = wsVan + 'Norm'
                l("{} = Scale({}, 1.0 / float({}), 'Multiply')"
                  .format(wsVanNorm, wsVan, get_time(wsVan)))

            if self.ecRuns:
                wsECNorm = wsEC + 'Norm'
                l("{} = Scale({}, 1.0 / float({}), 'Multiply')"
                  .format(wsECNorm, wsEC, get_time(wsEC)))

            l("names = []")
            l("for ws in {}:" .format(gDataRuns))
            l("    name = ws.getName() + 'Norm'")
            l("    names.append(name)")
            l("    Scale(ws, 1.0 / float({}), 'Multiply', OutputWorkspace=name)"
              .format(get_time('ws')))
            l()
            l("{} = GroupWorkspaces(names)" .format(gDataNorm))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(gAll))
            l()

        else:  # none, simply use the not normalised workspaces
            gDataNorm = gDataRuns

            if self.vanRuns:
                wsVanNorm = wsVan
            if self.ecRuns:
                wsECNorm = wsEC

        if self.ecRuns:
            gDataSubEC = gPrefix + 'DataSubEC'
            scaledEC   = self.prefix + 'ScaledEC'
            l("# subtract empty can")
            l("ecFactor = {:.3f}" .format(self.ecFactor))
            l("{} = Scale({}, Factor=ecFactor, Operation='Multiply')"
              .format(scaledEC, wsECNorm))
            l("{} = Minus({}, {})" .format(gDataSubEC, gDataNorm, scaledEC))
            if self.subtractECVan:
                wsVanSubEC = wsVan + 'SubEC'
                l("{} = Minus({}, {})" .format(wsVanSubEC, wsVanNorm, scaledEC))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(scaledEC))
            l()

        l("# group data for processing")  # without empty can
        gDataSource = gDataSubEC if self.ecRuns else gDataNorm
        gData = gPrefix + 'Data'
        if self.ecRuns:
            wsECNorm2 = wsECNorm + '2'
            l("{} = CloneWorkspace({})".format(wsECNorm2, wsECNorm))
        if self.vanRuns:
            wsVanNorm = wsVanSubEC if self.subtractECVan else wsVanNorm
            if self.ecRuns:
                l("{} = GroupWorkspaces({}list({}.getNames()) + [{}])"
                    .format(gData, group_list([wsVanNorm], ' + '), gDataSource, wsECNorm2))
            else:
                l("{} = GroupWorkspaces({}list({}.getNames()))"
                    .format(gData, group_list([wsVanNorm], ' + '), gDataSource))
        else:
            if self.ecRuns:
                l("{} = GroupWorkspaces({}list({}.getNames()))"
                  .format(gData, group_list([wsECNorm2], ' + '), gDataSource))
                #l("{} = GroupWorkspaces({})" .format(gData, group_list(gDataSource, wsECNorm2))
            else:
                l("{} = CloneWorkspace({})" .format(gData, gDataSource))
        l()

        if self.vanRuns:
            gDataCorr = gData + 'Corr'
            detCoeffs = self.prefix + 'DetCoeffs'
            eppTable  = self.prefix + 'EppTable'
            l("# normalise to vanadium")
            l("{} = FindEPP({})" .format(eppTable, wsVanNorm))
            l("{} = ComputeCalibrationCoefVan({}, {})" .format(detCoeffs, wsVanNorm, eppTable))
            l("badDetectors = np.where(np.array({}.extractY()).flatten() <= 0)[0]" .format(detCoeffs))
            l("MaskDetectors({}, DetectorList=badDetectors)" .format(gData))
            l("{} = Divide({}, {})" .format(gDataCorr, gData, detCoeffs))
            if not self.keepSteps:
                l("DeleteWorkspace({})" .format(detCoeffs))
            l()

        gDataCleanFrame = gData + 'CleanFrame'
        l("# remove half-filled time bins (clean frame)")
        l("{} = TOFTOFCropWorkspace({})"
          .format(gDataCleanFrame, gDataCorr if self.vanRuns else gData))
        if self.vanRuns and not self.keepSteps:
            l("DeleteWorkspace({})" .format(gDataCorr))
        l()

        gData2 = gData + 'TofCorr'
        if self.CORR_TOF_VAN == self.correctTof:
            l("# apply vanadium TOF correction")
            l("{} = CorrectTOF({}, {})" .format(gData2, gDataCleanFrame, eppTable))
            if not self.keepSteps:
                l("DeleteWorkspaces([{}, {}])" .format(gDataCleanFrame, eppTable))
            l()

        elif self.CORR_TOF_SAMPLE == self.correctTof:
            eppTables  = self.prefix + 'EppTables'
            l("# apply sample TOF correction")
            l("{} = FindEPP({})" .format(eppTables, gData))
            l("{} = CorrectTOF({}, {})" .format(gData2, gDataCleanFrame, eppTables))
            if not self.keepSteps:
                l("DeleteWorkspaces([{}, {}])" .format(gDataCleanFrame, eppTables))
            l()

        else:
            gData2 = gDataCleanFrame
            if self.vanRuns and not self.keepSteps:
                l("DeleteWorkspace({})" .format(eppTable))
            elif not self.keepSteps:
                l("DeleteWorkspace({})" .format(gData))

        gDataDeltaE = gData + 'DeltaE'
        l("# convert units")
        l("{} = ConvertUnits({}, Target='DeltaE', EMode='Direct', EFixed=Ei)"
          .format(gDataDeltaE, gData2))
        l("ConvertToDistribution({})" .format(gDataDeltaE))
        if not self.keepSteps:
            l("DeleteWorkspace({})" .format(gData2))
        l()

        gDataCorrDeltaE = gData + 'CorrDeltaE'
        l("# correct for energy dependent detector efficiency")
        l("{} = DetectorEfficiencyCorUser({})" .format(gDataCorrDeltaE, gDataDeltaE))
        if not self.keepSteps:
            l("DeleteWorkspace({})" .format(gDataDeltaE))
        l()

        gDataS = gData + 'S'
        l("# calculate S (Ki/kF correction)")
        l("{} = CorrectKiKf({})" .format(gDataS, gDataCorrDeltaE))
        if not self.keepSteps:
            l("DeleteWorkspace({})" .format(gDataCorrDeltaE))
        l()

        gLast = gDataS
        if self.binEon:
            gDataBinE = gData + 'BinE'
            l("# energy binning")
            l("rebinEnergy = '{:.3f}, {:.3f}, {:.3f}'"
              .format(self.binEstart, self.binEstep, self.binEend))
            l("{} = Rebin({}, Params=rebinEnergy, IgnoreBinErrors=True)" .format(gDataBinE, gLast))
            l()
            gLast = gDataBinE

        if self.binEon and self.createDiff:
            gDataD = gData + 'D'
            l("# create diffractograms")
            l("for ws in {}:" .format(gLast))
            l("    step1 = RemoveMaskedSpectra(ws)")
            l("    step2 = IntegrateByComponent(step1)")
            l("    step3 = ConvertSpectrumAxis(step2, Target='Theta', EMode='Direct', EFixed=Ei, OrderAxis=True)")
            l("    Transpose(step3, OutputWorkspace='{}_D_' + ws.getComment())"
              .format(self.prefix))
            l("{} = GroupWorkspaces(['{}_D_'+ ws.getComment() for ws in {}])"
              .format(gDataD, self.prefix, gLast))
            l("DeleteWorkspaces('step1,step2,step3')")
            l()

        if self.saveSofTW:
            suf = "'_Ei_{}'.format(round(Ei,2))"
            save_wsgroup(gLast, suf)

        if self.binQon:
            gDataBinQ = gData + 'SQW'
            l("# calculate momentum transfer Q for sample data")
            l("rebinQ = '{:.3f}, {:.3f}, {:.3f}'"
              .format(self.binQstart, self.binQstep, self.binQend))
            l("{} = SofQW3({}, QAxisBinning=rebinQ, EMode='Direct', EFixed=Ei, ReplaceNaNs={})"
              .format(gDataBinQ, gLast, self.replaceNaNs))
            l()
            if self.saveSofQW:
                save_wsgroup(gDataBinQ, "'_SQW'")

        l("# make nice workspace names")
        l("for ws in {}:" .format(gDataS))
        l("    RenameWorkspace(ws, OutputWorkspace='{}_S_' + ws.getComment())"
          .format(self.prefix))
        if self.binEon:
            l("for ws in {}:" .format(gDataBinE))
            l("    RenameWorkspace(ws, OutputWorkspace='{}_E_' + ws.getComment())"
              .format(self.prefix))
        if self.binQon:
            l("for ws in {}:" .format(gDataBinQ))
            l("    RenameWorkspace(ws, OutputWorkspace='{}_' + ws.getComment() + '_sqw')"
              .format(self.prefix))

        return script[0]

# -------------------------------------------------------------------------------


class TOFTOFReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)

# -------------------------------------------------------------------------------
# eof
