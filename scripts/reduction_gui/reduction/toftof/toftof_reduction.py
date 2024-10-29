# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable = too-many-instance-attributes, too-many-locals, too-many-branches
# pylint: disable = attribute-defined-outside-init
# pylint: disable = invalid-name
# pylint: disable = W0622
"""
TOFTOF reduction workflow gui.
"""

from itertools import repeat, compress
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter

# -------------------------------------------------------------------------------


class OptionalFloat(object):
    """value can be either a float or None. if value is None, str(self) == ''"""

    def __init__(self, value=None):
        super(OptionalFloat, self).__init__()
        self.value = float(value) if value else None

    def _bind(self, function, default=None):
        return function(self.value) if self.value is not None else default

    def __str__(self):
        return self._bind(str, default="")

    def __format__(self, format_spec):
        return self._bind(lambda v: v.__format__(format_spec), default="")

    def __bool__(self):
        return self.value is not None

    def __nonzero__(self):
        return self.__bool__()

    def __eq__(self, other):
        return self.value == (other.value if type(other) is OptionalFloat else other)


class TOFTOFScriptElement(BaseScriptElement):
    # normalisation
    NORM_NONE = 0
    NORM_MONITOR = 1
    NORM_TIME = 2

    # TOF correction
    CORR_TOF_NONE = 0
    CORR_TOF_VAN = 1
    CORR_TOF_SAMPLE = 2

    # default values
    DEF_prefix = "ws"
    DEF_ecFactor = 1.0
    DEF_vanEcFactor = 1.0

    DEF_binEon = True
    DEF_binEstart = 0.0
    DEF_binEstep = 0.0
    DEF_binEend = 0.0

    DEF_binQon = True
    DEF_binQstart = 0.0
    DEF_binQstep = 0.0
    DEF_binQend = 0.0

    DEF_subECVan = False
    DEF_replaceNaNs = False
    DEF_createDiff = False
    DEF_keepSteps = False
    DEF_normalise = NORM_NONE
    DEF_correctTof = CORR_TOF_NONE

    DEF_saveSofTWNxspe = False
    DEF_saveSofTWNexus = False
    DEF_saveSofTWAscii = False
    DEF_saveSofQWNexus = False
    DEF_saveSofQWAscii = False

    XML_TAG = "TOFTOFReduction"

    def reset(self):
        self.facility_name = ""
        self.instrument_name = ""

        # prefix of (some) workspace names
        self.prefix = self.DEF_prefix

        # data files are here
        self.dataDir = ""

        # vanadium runs & comment
        self.vanRuns = ""
        self.vanCmnt = ""
        self.vanTemp = OptionalFloat()
        self.vanEcFactor = self.DEF_vanEcFactor

        # empty can runs, comment, and factor
        self.ecRuns = ""
        self.ecTemp = OptionalFloat()
        self.ecFactor = self.DEF_ecFactor

        # data runs: [(runs,comment, temperature), ...]
        self.dataRuns = []

        # additional parameters
        self.binEon = self.DEF_binEon
        self.binEstart = self.DEF_binEstart
        self.binEstep = self.DEF_binEstep
        self.binEend = self.DEF_binEend

        self.binQon = self.DEF_binQon
        self.binQstart = self.DEF_binQstart
        self.binQstep = self.DEF_binQstep
        self.binQend = self.DEF_binQend

        self.maskDetectors = ""

        # options
        self.subtractECVan = self.DEF_subECVan
        self.normalise = self.DEF_normalise
        self.correctTof = self.DEF_correctTof
        self.replaceNaNs = self.DEF_replaceNaNs
        self.createDiff = self.DEF_createDiff
        self.keepSteps = self.DEF_keepSteps

        # save data
        self.saveDir = ""
        self.saveSofTWNxspe = self.DEF_saveSofTWNxspe
        self.saveSofTWNexus = self.DEF_saveSofTWNexus
        self.saveSofTWAscii = self.DEF_saveSofTWAscii
        self.saveSofQWNexus = self.DEF_saveSofQWNexus
        self.saveSofQWAscii = self.DEF_saveSofQWAscii

    def to_xml(self):
        res = [""]

        def put(tag, val):
            res[0] += "  <{0}>{1}</{0}>\n".format(tag, str(val))

        put("prefix", self.prefix)
        put("data_dir", self.dataDir)

        put("van_runs", self.vanRuns)
        put("van_comment", self.vanCmnt)
        put("van_temperature", self.vanTemp)
        put("van_ec_factor", self.vanEcFactor)

        put("ec_runs", self.ecRuns)
        put("ec_temp", self.ecTemp)
        put("ec_factor", self.ecFactor)

        for runs, cmnt, temp in self.dataRuns:
            put("data_runs", runs)
            put("data_comment", cmnt)
            put("data_temperature", temp)

        put("rebin_energy_on", self.binEon)
        put("rebin_energy_start", self.binEstart)
        put("rebin_energy_step", self.binEstep)
        put("rebin_energy_end", self.binEend)

        put("rebin_q_on", self.binQon)
        put("rebin_q_start", self.binQstart)
        put("rebin_q_step", self.binQstep)
        put("rebin_q_end", self.binQend)

        put("mask_detectors", self.maskDetectors)

        put("subtract_ecvan", self.subtractECVan)
        put("normalise", self.normalise)
        put("correct_tof", self.correctTof)
        put("replace_nans", self.replaceNaNs)
        put("create_diff", self.createDiff)
        put("keep_steps", self.keepSteps)

        put("save_dir", self.saveDir)
        put("saveSofTWNxspe", self.saveSofTWNxspe)
        put("saveSofTWNexus", self.saveSofTWNexus)
        put("saveSofTWAscii", self.saveSofTWAscii)
        put("saveSofQWNexus", self.saveSofQWNexus)
        put("saveSofQWAscii", self.saveSofQWAscii)

        return "<{0}>\n{1}</{0}>\n".format(self.XML_TAG, res[0])

    def from_xml(self, xmlString):
        self.reset()

        dom = xml.dom.minidom.parseString(xmlString)
        els = dom.getElementsByTagName(self.XML_TAG)

        if els:
            dom = els[0]

            def get_str(tag, default=""):
                return BaseScriptElement.getStringElement(dom, tag, default=default)

            def get_optFloat(tag, default=None):
                return OptionalFloat(BaseScriptElement.getStringElement(dom, tag, default=default))

            def get_int(tag, default):
                return BaseScriptElement.getIntElement(dom, tag, default=default)

            def get_flt(tag, default):
                return BaseScriptElement.getFloatElement(dom, tag, default=default)

            def get_strlst(tag):
                return BaseScriptElement.getStringList(dom, tag)

            def get_optFloat_list(tag):
                # in Python3 map returns an iterator, make it a list here:
                return list(map(OptionalFloat, get_strlst(tag)))

            def get_bol(tag, default):
                return BaseScriptElement.getBoolElement(dom, tag, default=default)

            self.prefix = get_str("prefix", self.DEF_prefix)
            self.dataDir = get_str("data_dir")

            self.vanRuns = get_str("van_runs")
            self.vanCmnt = get_str("van_comment")
            self.vanTemp = get_optFloat("van_temperature")
            self.vanEcFactor = get_flt("van_ec_factor", self.DEF_vanEcFactor)

            self.ecRuns = get_str("ec_runs")
            self.ecTemp = get_optFloat("ec_temp")
            self.ecFactor = get_flt("ec_factor", self.DEF_ecFactor)

            dataRuns = get_strlst("data_runs")
            dataCmts = get_strlst("data_comment")
            dataTemps = get_optFloat_list("data_temperature")

            # make sure the lengths of these lists match
            assert len(dataRuns) == len(dataCmts)
            if dataTemps:
                assert len(dataRuns) == len(dataTemps)
            else:
                # no temperatures in xml file, so generate empty OptionalFloats:
                dataTemps = (OptionalFloat() for _ in repeat(""))

            for dataRun in zip(dataRuns, dataCmts, dataTemps):
                self.dataRuns.append(list(dataRun))

            self.binEon = get_bol("rebin_energy_on", self.DEF_binEon)
            self.binEstart = get_flt("rebin_energy_start", self.DEF_binEstart)
            self.binEstep = get_flt("rebin_energy_step", self.DEF_binEstep)
            self.binEend = get_flt("rebin_energy_end", self.DEF_binEend)

            self.binQon = get_bol("rebin_q_on", self.DEF_binQon)
            self.binQstart = get_flt("rebin_q_start", self.DEF_binQstart)
            self.binQstep = get_flt("rebin_q_step", self.DEF_binQstep)
            self.binQend = get_flt("rebin_q_end", self.DEF_binQend)

            self.maskDetectors = get_str("mask_detectors")

            self.subtractECVan = get_bol("subtract_ecvan", self.DEF_subECVan)
            self.normalise = get_int("normalise", self.DEF_normalise)
            self.correctTof = get_int("correct_tof", self.DEF_correctTof)
            self.replaceNaNs = get_bol("replace_nans", self.DEF_replaceNaNs)
            self.createDiff = get_bol("create_diff", self.DEF_createDiff)
            self.keepSteps = get_bol("keep_steps", self.DEF_keepSteps)

            self.saveDir = get_str("save_dir")

            # for backwards compatibility:
            SofQW = get_bol("save_sofqw", False)
            SofTW = get_bol("save_softw", False)
            NXSPE = get_bol("save_nxspe", False)
            Nexus = get_bol("save_nexus", False)
            Ascii = get_bol("save_ascii", False)

            self.saveSofTWNxspe = get_bol("saveSofTWNxspe", (SofTW and NXSPE) or self.DEF_saveSofTWNxspe)
            self.saveSofTWNexus = get_bol("saveSofTWNexus", (SofTW and Nexus) or self.DEF_saveSofTWNexus)
            self.saveSofTWAscii = get_bol("saveSofTWAscii", (SofTW and Ascii) or self.DEF_saveSofTWAscii)
            self.saveSofQWNexus = get_bol("saveSofQWNexus", (SofQW and Nexus) or self.DEF_saveSofQWNexus)
            self.saveSofQWAscii = get_bol("saveSofQWAscii", (SofQW and Ascii) or self.DEF_saveSofQWAscii)

    def validate_inputs(self):
        # must have vanadium for TOF correction
        if self.CORR_TOF_VAN == self.correctTof:
            if not self.vanRuns:
                self.error("missing vanadium runs")

        # must have vanadium and empty can for subtracting EC from van
        if self.subtractECVan:
            if not self.vanRuns:
                self.error("missing vanadium runs")
            if not self.ecRuns:
                self.error("missing empty can runs")

        # binning parameters
        def check_bin_params(start, step, end):
            if not (start < end and step > 0 and start + step <= end):
                self.error("incorrect binning parameters")

        if self.binEon:
            check_bin_params(self.binEstart, self.binEstep, self.binEend)
        if self.binQon:
            check_bin_params(self.binQstart, self.binQstep, self.binQend)

        # must have some data runs
        if not self.dataRuns:
            self.error("missing data runs")

        # must have a comment for runs
        for i, run in enumerate(self.dataRuns, start=1):
            if not run[1]:
                self.error("missing comment for data runs No. {}".format(i))

        if self.vanRuns and not self.vanCmnt:
            self.error("missing vanadium comment")

        # saving settings must be consistent
        if (
            any([self.saveSofTWNxspe, self.saveSofTWNexus, self.saveSofTWAscii, self.saveSofQWNexus, self.saveSofQWAscii])
            and not self.saveDir
        ):
            self.error("missing directory to save the data")

    @staticmethod
    def error(message):
        raise RuntimeError("TOFTOF reduction error: " + message)

    @staticmethod
    def get_log(workspace, tag):
        return "{}.getRun().getLogData('{}').value".format(workspace, tag)

    def merge_runs(self, ws_raw, raw_runs, outws, comment, temperature=None):
        self.add_line_to_script("{} = Load(Filename='{}')".format(ws_raw, raw_runs))
        self.add_line_to_script("{} = MergeRuns({})".format(outws, ws_raw))
        self.add_line_to_script("{}.setComment('{}')".format(outws, comment))
        if not temperature:
            self.add_line_to_script("temperature = np.mean({})".format(self.get_log(outws, "temperature")))
        else:
            self.add_line_to_script("temperature = {}".format(temperature))
        self.add_line_to_script(
            "AddSampleLog({}, LogName='temperature', LogText=str(temperature), LogType='Number', LogUnit='K')".format(outws)
        )
        if not self.keepSteps:
            self.add_line_to_script("DeleteWorkspace({})".format(ws_raw))
        self.add_line_to_script()

    def load_runs(self, allGroup, dataGroup, dataRawGroup):
        # if not self.keepSteps, delete the workspaces immediately, to free the memory
        # vanadium runs
        if self.vanRuns:
            wsRawVan = self.prefix + "RawVan"
            wsVan = self.prefix + "Van"

            self.add_line_to_script("# vanadium runs")
            self.merge_runs(wsRawVan, self.vanRuns, wsVan, self.vanCmnt, self.vanTemp)
            allGroup.append(wsVan)

        # empty can runs
        if self.ecRuns:
            wsRawEC = self.prefix + "RawEC"
            wsEC = self.prefix + "EC"

            self.add_line_to_script("# empty can runs")
            self.merge_runs(wsRawEC, self.ecRuns, wsEC, "EC", self.ecTemp)
            allGroup.append(wsEC)

        # data runs
        for i, (runs, cmnt, temp) in enumerate(self.dataRuns):
            if not runs:
                self.error("missing data runs value")
            if not cmnt:
                self.error("missing data runs comment")

            postfix = str(i + 1)

            wsRawData = self.prefix + "RawData" + postfix
            wsData = self.prefix + "Data" + postfix

            dataRawGroup.append(wsRawData)
            dataGroup.append(wsData)
            allGroup.append(wsData)

            self.add_line_to_script("# data runs {}".format(postfix))
            self.merge_runs(wsRawData, runs, wsData, cmnt, temp)

    def delete_workspaces(self, workspaces):
        if not self.keepSteps:
            self.add_line_to_script("DeleteWorkspaces({})".format(self.group_list(workspaces)))
        self.add_line_to_script()

    def add_line_to_script(self, line=""):
        self.script[0] += line + "\n"

    # helpers
    @staticmethod
    def group_list(listVal, postfix=""):
        return ("[" + ", ".join(listVal) + "]" + postfix) if listVal else ""

    def get_ei(self, workspace):
        return self.get_log(workspace, "Ei")

    def get_time(self, workspace):
        return self.get_log(workspace, "duration")

    allowed_save_formats = ["nxspe", "nexus", "ascii"]

    def save_wsgroup(self, wsgroup, suffix, spectrumMetaData, saveFormats):
        assert saveFormats <= set(self.allowed_save_formats)
        if len(saveFormats) == 0:
            return
        self.add_line_to_script("# save {}".format(wsgroup))
        self.add_line_to_script("for ws in {}:".format(wsgroup))
        self.add_line_to_script("    name = ws.getComment() + {}".format(suffix))
        if "nxspe" in saveFormats:
            self.add_line_to_script(
                "    SaveNXSPE(ws, join(r'{}', name + '.nxspe'), Efixed=Ei)".format(
                    self.saveDir,
                )
            )
        if "nexus" in saveFormats:
            self.add_line_to_script("    SaveNexus(ws, join(r'{}', name + '.nxs'))".format(self.saveDir))
        if "ascii" in saveFormats:
            self.add_line_to_script(
                "    SaveAscii(ws, join(r'{}', name + '.txt'), SpectrumMetaData='{}')".format(self.saveDir, spectrumMetaData)
            )
        self.add_line_to_script()

    def normalize_data(self, gPrefix, gDataRuns, wsEC="", wsVan=""):
        if self.NORM_MONITOR == self.normalise:
            gDataNorm = gPrefix + "Norm"
            self.add_line_to_script("# normalise to monitor")
            if self.vanRuns:
                wsVanNorm = wsVan + "Norm"
                self.add_line_to_script("{} = MonitorEfficiencyCorUser({})".format(wsVanNorm, wsVan))

            if self.ecRuns:
                wsECNorm = wsEC + "Norm"
                self.add_line_to_script("{} = MonitorEfficiencyCorUser({})".format(wsECNorm, wsEC))

            self.add_line_to_script("{} = MonitorEfficiencyCorUser({})".format(gDataNorm, gDataRuns))
            return True

        elif self.NORM_TIME == self.normalise:
            gDataNorm = gPrefix + "Norm"
            self.add_line_to_script("# normalise to time")
            if self.vanRuns:
                wsVanNorm = wsVan + "Norm"
                self.add_line_to_script("{} = Scale({}, 1.0 / float({}), 'Multiply')".format(wsVanNorm, wsVan, self.get_time(wsVan)))

            if self.ecRuns:
                wsECNorm = wsEC + "Norm"
                self.add_line_to_script("{} = Scale({}, 1.0 / float({}), 'Multiply')".format(wsECNorm, wsEC, self.get_time(wsEC)))

            self.add_line_to_script("names = []")
            self.add_line_to_script("for ws in {}:".format(gDataRuns))
            self.add_line_to_script("    name = ws.name() + 'Norm'")
            self.add_line_to_script("    names.append(name)")
            self.add_line_to_script("    Scale(ws, 1.0 / float({}), 'Multiply', OutputWorkspace=name)".format(self.get_time("ws")))
            self.add_line_to_script()
            self.add_line_to_script("{} = GroupWorkspaces(names)".format(gDataNorm))
            return True

        # none, simply use the not normalised workspaces
        return False

    def correct_tof(self, gData):
        gData2 = gData + "TofCorr"
        gDataCleanFrame = gData + "CleanFrame"
        eppTable = self.prefix + "EppTable"
        if self.CORR_TOF_VAN == self.correctTof:
            self.add_line_to_script("# apply vanadium TOF correction")
            self.add_line_to_script("{} = CorrectTOF({}, {})".format(gData2, gDataCleanFrame, eppTable))
            if self.ecRuns:
                self.delete_workspaces([gDataCleanFrame, gData, eppTable])
                return True
            self.delete_workspaces([gDataCleanFrame, eppTable])
            return True

        elif self.CORR_TOF_SAMPLE == self.correctTof:
            eppTables = self.prefix + "EppTables"
            self.add_line_to_script("# apply sample TOF correction")
            self.add_line_to_script("{} = FindEPP({})".format(eppTables, gData))
            self.add_line_to_script("{} = CorrectTOF({}, {})".format(gData2, gDataCleanFrame, eppTables))
            if self.ecRuns:
                self.delete_workspaces([gDataCleanFrame, gData, eppTables])
                return True
            self.delete_workspaces([gDataCleanFrame, eppTables])
            return True

        if self.vanRuns and self.ecRuns:
            self.delete_workspaces([eppTable, gData])
        elif self.vanRuns:
            self.delete_workspaces([eppTable])
        elif self.ecRuns:
            self.delete_workspaces([gData])
        return False

    def mask_detectors(self, gPrefix, gAll):
        gDetectorsToMask = gPrefix + "DetectorsToMask"
        self.add_line_to_script("# mask detectors")
        self.add_line_to_script("({}, numberOfFailures) = FindDetectorsOutsideLimits({})".format(gDetectorsToMask, gAll))
        self.add_line_to_script("MaskDetectors({}, MaskedWorkspace={})".format(gAll, gDetectorsToMask))
        if self.maskDetectors:
            self.add_line_to_script("MaskDetectors({}, DetectorList='{}')".format(gAll, self.maskDetectors))
        self.delete_workspaces([gDetectorsToMask])

    def vanadium_correction(self, gData, wsVanNorm=""):
        if self.vanRuns:
            gDataCorr = gData + "Corr"
            detCoeffs = self.prefix + "DetCoeffs"
            eppTable = self.prefix + "EppTable"
            self.add_line_to_script("# normalise to vanadium")
            self.add_line_to_script("{} = FindEPP({})".format(eppTable, wsVanNorm))
            self.add_line_to_script("{} = ComputeCalibrationCoefVan({}, {})".format(detCoeffs, wsVanNorm, eppTable))
            self.add_line_to_script("badDetectors = np.where(np.array({}.extractY()).flatten() <= 0)[0]".format(detCoeffs))
            self.add_line_to_script("MaskDetectors({}, DetectorList=badDetectors)".format(gData))
            self.add_line_to_script("{} = Divide({}, {})".format(gDataCorr, gData, detCoeffs))
            self.delete_workspaces([detCoeffs])
            return gDataCorr
        return gData

    def rename_workspaces(self, gData):
        self.add_line_to_script("# make nice workspace names")
        self.add_line_to_script("for ws in {}:".format(gData + "S"))
        self.add_line_to_script("    RenameWorkspace(ws, OutputWorkspace='{}_S_' + ws.getComment())".format(self.prefix))
        if self.binEon:
            self.add_line_to_script("for ws in {}:".format(gData + "BinE"))
            self.add_line_to_script("    RenameWorkspace(ws, OutputWorkspace='{}_E_' + ws.getComment())".format(self.prefix))
        if self.binQon and self.binEon:
            self.add_line_to_script("for ws in {}:".format(gData + "SQW"))
            self.add_line_to_script("    RenameWorkspace(ws, OutputWorkspace='{}_' + ws.getComment() + '_sqw')".format(self.prefix))

    def to_script(self):
        # sanity checks
        self.validate_inputs()

        # generated script
        self.script = [""]

        self.add_line_to_script("from __future__ import (absolute_import, division, print_function, unicode_literals)")
        self.add_line_to_script()
        self.add_line_to_script("# import mantid algorithms, numpy and matplotlib")
        self.add_line_to_script("from mantid.simpleapi import *")
        self.add_line_to_script("import matplotlib.pyplot as plt")
        self.add_line_to_script("import numpy as np")
        self.add_line_to_script()
        self.add_line_to_script("from os.path import join")
        self.add_line_to_script()
        self.add_line_to_script("config['default.facility'] = '{}'".format(self.facility_name))
        self.add_line_to_script("config['default.instrument'] = '{}'".format(self.instrument_name))
        self.add_line_to_script()
        self.add_line_to_script("config.appendDataSearchDir(r'{}')".format(self.dataDir))
        self.add_line_to_script()

        dataRawGroup = []
        dataGroup = []
        allGroup = []

        self.load_runs(allGroup, dataGroup, dataRawGroup)
        wsEC = self.prefix + "EC"
        wsVan = self.prefix + "Van"

        gPrefix = "g" + self.prefix
        gDataRuns = gPrefix + "DataRuns"
        gDataRawRuns = gPrefix + "DataRawRuns"
        gAll = gPrefix + "All"

        self.add_line_to_script("# grouping")
        if self.keepSteps:
            self.add_line_to_script("{} = GroupWorkspaces({})".format(gDataRawRuns, self.group_list(dataRawGroup)))
        self.add_line_to_script("{} = GroupWorkspaces({})".format(gDataRuns, self.group_list(dataGroup)))
        self.add_line_to_script("{} = GroupWorkspaces({})".format(gAll, self.group_list(allGroup)))
        self.add_line_to_script()

        self.add_line_to_script("# Ei")
        if len(allGroup) > 1:
            self.add_line_to_script("if CompareSampleLogs({}, 'Ei', 0.001):".format(gAll))
            self.add_line_to_script("    raise RuntimeError('Ei values do not match')")
            self.add_line_to_script()

        self.add_line_to_script("Ei = {}".format(self.get_ei(dataGroup[0])))
        self.add_line_to_script()

        # mask detectors
        self.mask_detectors(gPrefix, gAll)

        normalized = self.normalize_data(gPrefix, gDataRuns, wsEC, wsVan)
        gDataNorm = gPrefix + "Norm" if normalized else gDataRuns
        wsVanNorm = wsVan + "Norm" if normalized else wsVan
        wsECNorm = wsEC + "Norm" if normalized else wsEC
        if normalized:
            self.delete_workspaces([gAll])

        if self.ecRuns:
            gDataSubEC = gPrefix + "DataSubEC"
            scaledEC = self.prefix + "ScaledEC"
            self.add_line_to_script("# subtract empty can")
            self.add_line_to_script("ecFactor = {:.3f}".format(self.ecFactor))
            self.add_line_to_script("{} = Scale({}, Factor=ecFactor, Operation='Multiply')".format(scaledEC, wsECNorm))
            self.add_line_to_script("{} = Minus({}, {})".format(gDataSubEC, gDataNorm, scaledEC))
            wslist = [scaledEC]
            if self.subtractECVan:
                wsVanSubEC = wsVan + "SubEC"
                scaledECvan = self.prefix + "ScaledECvan"
                self.add_line_to_script("van_ecFactor = {:.3f}".format(self.vanEcFactor))
                self.add_line_to_script("{} = Scale({}, Factor=van_ecFactor, Operation='Multiply')".format(scaledECvan, wsECNorm))
                self.add_line_to_script("{} = Minus({}, {})".format(wsVanSubEC, wsVanNorm, scaledECvan))
                wslist.append(scaledECvan)
            self.delete_workspaces(wslist)

        self.add_line_to_script("# group data for processing")
        gDataSource = gDataSubEC if self.ecRuns else gDataNorm
        gData = gPrefix + "Data"
        if self.ecRuns:
            wsECNorm2 = wsECNorm + "2"
            self.add_line_to_script("{} = CloneWorkspace({})".format(wsECNorm2, wsECNorm))

        if self.vanRuns:
            if self.subtractECVan:
                wsVanNorm = wsVanSubEC
            else:
                wsVanNorm2 = wsVanNorm + "2"
                self.add_line_to_script("{} = CloneWorkspace({})".format(wsVanNorm2, wsVanNorm))
                wsVanNorm = wsVanNorm2

            if self.ecRuns:
                self.add_line_to_script(
                    "{} = GroupWorkspaces({}list({}.getNames()))".format(gData, self.group_list([wsVanNorm, wsECNorm2], " + "), gDataSource)
                )
            else:
                self.add_line_to_script(
                    "{} = GroupWorkspaces({}list({}.getNames()))".format(gData, self.group_list([wsVanNorm], " + "), gDataSource)
                )
        else:
            if self.ecRuns:
                self.add_line_to_script(
                    "{} = GroupWorkspaces({}list({}.getNames()))".format(gData, self.group_list([wsECNorm2], " + "), gDataSource)
                )
            else:
                self.add_line_to_script("{} = CloneWorkspace({})".format(gData, gDataSource))
        self.add_line_to_script()

        gDataCorr = self.vanadium_correction(gData, wsVanNorm)

        gDataCleanFrame = gData + "CleanFrame"
        self.add_line_to_script("# remove half-filled time bins (clean frame)")
        self.add_line_to_script("{} = TOFTOFCropWorkspace({})".format(gDataCleanFrame, gDataCorr))
        if self.vanRuns:
            self.delete_workspaces([gDataCorr])

        tof_corrected = self.correct_tof(gData)
        gData2 = gData + "TofCorr" if tof_corrected else gDataCleanFrame

        gDataDeltaE = gData + "DeltaE"
        self.add_line_to_script("# convert units")
        self.add_line_to_script("{} = ConvertUnits({}, Target='DeltaE', EMode='Direct', EFixed=Ei)".format(gDataDeltaE, gData2))
        self.add_line_to_script("ConvertToDistribution({})".format(gDataDeltaE))
        self.delete_workspaces([gData2])

        gDataCorrDeltaE = gData + "CorrDeltaE"
        self.add_line_to_script("# correct for energy dependent detector efficiency")
        self.add_line_to_script("{} = DetectorEfficiencyCorUser({})".format(gDataCorrDeltaE, gDataDeltaE))
        self.delete_workspaces([gDataDeltaE])

        gDataS = gData + "S"
        self.add_line_to_script("# calculate S (Ki/kF correction)")
        self.add_line_to_script("{} = CorrectKiKf({})".format(gDataS, gDataCorrDeltaE))
        self.delete_workspaces([gDataCorrDeltaE])

        gLast = gDataS
        if self.binEon:
            gDataBinE = gData + "BinE"
            self.add_line_to_script("# energy binning")
            self.add_line_to_script("rebinEnergy = '{:.3f}, {:.3f}, {:.3f}'".format(self.binEstart, self.binEstep, self.binEend))
            self.add_line_to_script("{} = Rebin({}, Params=rebinEnergy, IgnoreBinErrors=True)".format(gDataBinE, gLast))
            self.add_line_to_script()
            gLast = gDataBinE

        if self.binEon and self.createDiff:
            gDataD = gData + "D"
            self.add_line_to_script("# create diffractograms")
            self.add_line_to_script("for ws in {}:".format(gLast))
            self.add_line_to_script("    step1 = RemoveMaskedSpectra(ws)")
            self.add_line_to_script("    step2 = IntegrateByComponent(step1)")
            self.add_line_to_script("    step3 = ConvertSpectrumAxis(step2, Target='Theta', EMode='Direct', EFixed=Ei)")
            self.add_line_to_script("    Transpose(step3, OutputWorkspace='{}_D_' + ws.getComment())".format(self.prefix))
            self.add_line_to_script("{} = GroupWorkspaces(['{}_D_'+ ws.getComment() for ws in {}])".format(gDataD, self.prefix, gLast))
            self.add_line_to_script("DeleteWorkspaces('step1,step2,step3')")
            self.add_line_to_script()

        if self.binQon and self.binEon:
            gDataBinQ = gData + "SQW"
            self.add_line_to_script("# calculate momentum transfer Q for sample data")
            self.add_line_to_script("rebinQ = '{:.3f}, {:.3f}, {:.3f}'".format(self.binQstart, self.binQstep, self.binQend))
            self.add_line_to_script(
                "{} = SofQW3({}, QAxisBinning=rebinQ, EMode='Direct', EFixed=Ei, ReplaceNaNs=False)".format(gDataBinQ, gLast)
            )
            if self.replaceNaNs:
                self.add_line_to_script("{} = ReplaceSpecialValues({}, NaNValue=0, NaNError=1)".format(gDataBinQ, gDataBinQ))
            self.add_line_to_script()

        self.rename_workspaces(gData)
        self.add_line_to_script()

        # save S(2theta, w), has to be done after renaming
        suf = "'_Ei_{}'.format(round(Ei,2))"
        saveFormats = set(compress(self.allowed_save_formats, [self.saveSofTWNxspe, self.saveSofTWNexus, self.saveSofTWAscii]))
        self.save_wsgroup(gLast, suf, "Angle", saveFormats)
        self.add_line_to_script()

        # save S(Q, w), has to be done after renaming
        if self.binQon and self.binEon:
            saveFormats = set(compress(self.allowed_save_formats, [False, self.saveSofQWNexus, self.saveSofQWAscii]))
            self.save_wsgroup(gDataBinQ, "'_SQW'", "Q", saveFormats)

        return self.script[0]


#  -------------------------------------------------------------------------------


class TOFTOFReductionScripter(BaseReductionScripter):
    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)


#  -------------------------------------------------------------------------------
#  eof
