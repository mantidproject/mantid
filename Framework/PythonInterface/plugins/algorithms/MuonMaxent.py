# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math
from Muon.MaxentTools.multimaxalpha import MULTIMAX
from Muon.MaxentTools.dead_detector_handler import removeDeadDetectors
from mantid.api import (
    AlgorithmFactory,
    ITableWorkspaceProperty,
    Progress,
    PropertyMode,
    PythonAlgorithm,
    RawCountValidator,
    WorkspaceFactory,
    WorkspaceProperty,
)
from mantid.kernel import Direction, IntListValidator


"""
 input params from MuonMaxEnt
 RunNo (int)
 Frames (int)
 Ires (int) = resolution in ps
 Tzeroch (int)
 firstgoodch (int)
 fitphase (int==bool)
 fitdt (int==bool)
 deflevel (real)
 sigloose(real)
 ptstofit(int)
 histolen(int)
 nhisto(int)
 counts (int array)

 outputs from MuonMaxEnt
 fchan (real array)
 taud (real array)
 phi (real array)
"""


class MuonMaxent(PythonAlgorithm):
    def category(self):
        return "Muon;Arithmetic\\FFT"

    def seeAlso(self):
        return ["PhaseQuad", "FFT"]

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", direction=Direction.Input, validator=RawCountValidator(True)),
            doc="Raw muon workspace to process",
        )
        self.declareProperty(
            ITableWorkspaceProperty("InputPhaseTable", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Phase table (initial guess)",
        )  # from CalMuonDetectorPhases
        self.declareProperty(
            ITableWorkspaceProperty("InputDeadTimeTable", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Dead time table (initial)",
        )  # from LoadMuonNexus or blanl=k
        self.declareProperty(
            ITableWorkspaceProperty("GroupTable", "", direction=Direction.Input, optional=PropertyMode.Optional), doc="Group Table"
        )  # from LoadMuonNexus, none=do all spectra individually
        self.declareProperty(
            WorkspaceProperty("GroupWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional), doc="Group Workspace"
        )  # from LoadDetectorsGroupingTable, none=do all spectra individually
        self.declareProperty("FirstGoodTime", 0.1, doc="First good data time")
        self.declareProperty("LastGoodTime", 33.0, doc="Last good data time")
        self.declareProperty(
            "Npts",
            2,
            doc="Number of frequency points to fit (should be power of 2)",
            validator=IntListValidator([2**i for i in range(8, 21)]),
        )
        self.declareProperty("MaxField", 1000.0, doc="Maximum field for spectrum")
        self.declareProperty("FixPhases", False, doc="Fix phases to initial values")
        self.declareProperty("FitDeadTime", True, doc="Fit deadtimes")
        self.declareProperty("DoublePulse", False, doc="Double pulse data")
        self.declareProperty("OuterIterations", 10, doc="Number of loops to optimise phase, amplitudes, backgrounds and dead times")
        self.declareProperty("InnerIterations", 10, doc="Number of loops to optimise the spectrum")
        self.declareProperty("DefaultLevel", 0.1, doc="Default Level")
        self.declareProperty("Factor", 1.04, doc="Used to control the value chi-squared converge to", direction=Direction.InOut)
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output Spectrum (combined) versus field"
        )
        self.declareProperty(
            ITableWorkspaceProperty("OutputPhaseTable", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output phase table (optional)",
        )
        self.declareProperty(
            ITableWorkspaceProperty("OutputDeadTimeTable", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output dead time table (optional)",
        )
        self.declareProperty(
            WorkspaceProperty("ReconstructedSpectra", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Reconstructed time spectra (optional)",
        )
        self.declareProperty(
            WorkspaceProperty("PhaseConvergenceTable", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Convergence of phases (optional)",
        )

    def validateInputs(self):
        issues = dict()
        first = self.getProperty("FirstGoodTime").value
        last = self.getProperty("LastGoodTime").value

        if first >= last:
            issues["FirstGoodTime"] = "FirstGoodTime must be smaller than LastGoodTime"
        if first < 0:
            issues["FirstGoodTime"] = "FirstGoodTime must be positive or zero"
        field = self.getProperty("MaxField").value
        if field <= 0:
            issues["MaxField"] = "MaxField must be a positive non-zero number"
        outer = self.getProperty("OuterIterations").value
        if outer <= 0:
            issues["OuterIterations"] = "OuterIterations must be a positive non-zero number"
        inner = self.getProperty("InnerIterations").value
        if inner <= 0:
            issues["InnerIterations"] = "InnerIterations must be a positive non-zero number"

        default = self.getProperty("DefaultLevel").value
        if default <= 0.0:
            issues["DefaultLevel"] = "DefaultLevel (A) must be a positive non-zero number"

        factor = self.getProperty("Factor").value
        if factor <= 0.0:
            issues["Factor"] = "Factor (Lagrange multiplier) must be a positive non-zero number"

        return issues

    def checkRValues(self, rg9, rg0, xv, mylog):
        if rg9 - rg0 < 4:
            raise ValueError("Data too short after trimming")
        if rg0 > 0 or rg9 < len(xv):
            mylog.warning("Trimmed {} odd sized bins from start and {} bins from end".format(rg0, len(xv) - rg9))

    def getPulse(self):
        if self.getProperty("DoublePulse").value:
            return 2
        else:
            return 1

    def doGrouping(self, POINTS_nhists, nhisto):
        GROUPING_group, POINTS_ngroups = None, None
        if not self.getProperty("GroupWorkspace").isDefault:
            # group Workspace (LoadDetectorsGroupingFile format) provided, use
            # it
            gwsp = self.getProperty("GroupWorkspace").value
            gwdets = gwsp.getNumberHistograms()
            if gwdets != POINTS_nhists:
                raise Exception("Grouping workspace has a different number of spectra")
            GROUPING_group = np.zeros([nhisto], dtype=int)
            for hh in range(POINTS_nhists):
                GROUPING_group[hh] = int(gwsp.dataY(hh)[0] - 1)
            POINTS_ngroups = np.amax(GROUPING_group) + 1
        elif self.getProperty("GroupTable").isDefault:
            # no table provided, map 1:1 and use all spectra
            POINTS_ngroups = POINTS_nhists
            GROUPING_group = np.arange(POINTS_ngroups, dtype=int)
        else:
            GROUPING_group = np.zeros([nhisto], dtype=int)
            GROUPING_group[:] = -1  # for unused histograms in no group
            POINTS_ngroups = len(self.getProperty("GroupTable").value)
            for g, row in enumerate(self.getProperty("GroupTable").value):
                for hh in map(int, row["Detectors"].split(",")):
                    GROUPING_group[hh - 1] = g
        return GROUPING_group, POINTS_ngroups

    def doDeadTimes(self, POINTS_ngroups, GROUPING_group, ws, FLAGS_fitdead, mylog):
        RUNDATA_frames = None
        SENSE_taud = np.zeros([POINTS_ngroups])  # default zero if not provided
        tmpTaud = [[] for i in range(POINTS_ngroups)]
        if not self.getProperty("InputDeadTimeTable").isDefault:
            # load data from standard Mantid dead time table
            for r in self.getProperty("InputDeadTimeTable").value:
                if GROUPING_group[r["spectrum"] - 1] >= 0:
                    tmpTaud[GROUPING_group[r["spectrum"] - 1]].append(r["dead-time"])
            for g in range(POINTS_ngroups):
                SENSE_taud[g] = np.mean(tmpTaud[g])
        try:
            RUNDATA_frames = ws.getRun().getProperty("goodfrm").value  # need frames for dead time calc
        except:
            if (not self.getProperty("InputDeadTimeTable").isDefault) or FLAGS_fitdead:
                raise Exception("Need number of frames to use or fit dead time")
            else:
                mylog.notice("No dead time fitting, assuming arbitrary number of frames")
                RUNDATA_frames = 1000000
        return SENSE_taud, RUNDATA_frames

    def getPhase(self, FLAGS_fixphase, POINTS_ngroups, POINTS_nhists, mylog):
        filePHASE = None
        if self.getProperty("InputPhaseTable").isDefault:
            if FLAGS_fixphase and POINTS_ngroups > 2:
                raise ValueError("Supply phases to fix to")
            if POINTS_ngroups > 2:
                mylog.warning("Generating default phases which may be wrong")
            else:
                mylog.notice("Initialising phases of the 2 groups to 0 and 180 degrees")
            filePHASE = np.arange(POINTS_ngroups) * math.pi * 2.0 / POINTS_ngroups
        else:
            filePHASE = np.zeros([POINTS_ngroups])
            pt = self.getProperty("InputPhaseTable").value
            names = pt.getColumnNames()
            phaseLabel = None
            IDLabel = None
            asymmLabel = None
            for name in names:
                name_lower = name.lower()
                if name_lower in {"phi", "phase"}:
                    phaseLabel = name
                if name_lower in {"detid", "spectrum number"}:
                    IDLabel = name
                if name_lower in {"asymmetry", "asymm", "asym"}:
                    asymmLabel = name
            if phaseLabel is None:
                raise ValueError("Phase/phi are not labelled in the phase table")
            if IDLabel is None:
                raise ValueError("Spectrum number/DetID are not labelled in the phase table")
            if asymmLabel is None:
                raise ValueError("Asymmetry is not labelled in the phase table")

            if len(pt) == POINTS_ngroups:  # phase table for grouped data, or when not grouping
                for row in pt:
                    filePHASE[row[IDLabel] - 1] = row[phaseLabel]
                    # sign of phase now OK for Mantid 3.12 onwards
            elif len(pt) == POINTS_nhists:  # phase table for ungrouped data. Pick a representative detector for each group (the last one)
                for row in pt:
                    filePHASE[GROUPING_group[row[IDLabel] - 1]] = row[phaseLabel]
            else:  # muat be some dead Detectors
                offset = 0
                for row in pt:
                    if row[asymmLabel] == 999:
                        offset += 1
                    else:
                        filePHASE[row[IDLabel] - 1 - offset] = row[phaseLabel]
        return filePHASE

    def phaseConvergenceTable(self, POINTS_ngroups, deadDetectors, OuterIter, filePHASE):
        if not self.getProperty("PhaseConvergenceTable").isDefault:
            phaseconvWS = WorkspaceFactory.create(
                "Workspace2D", NVectors=POINTS_ngroups + len(deadDetectors), XLength=OuterIter + 1, YLength=OuterIter + 1
            )
            offset = 0
            for i in range(POINTS_ngroups + len(deadDetectors)):
                if i + 1 in deadDetectors:
                    offset += 1
                    phaseconvWS.dataX(i)[0] = 0.0
                    phaseconvWS.dataY(i)[0] = 0.0
                else:
                    phaseconvWS.dataX(i)[0] = 0.0
                    phaseconvWS.dataY(i)[0] = filePHASE[i - offset]
        else:
            phaseconvWS = None
        return phaseconvWS

    def PyExec(self):
        # logging
        mylog = self.log()
        #
        originalWS = self.getProperty("InputWorkspace").value
        ws, deadDetectors = removeDeadDetectors(originalWS)

        # crop off odd sized bins at start and end (if present)
        xv = ws.readX(0)
        rg0 = 0
        rg9 = len(xv)
        while rg9 > rg0 and abs((2 * xv[rg9 - 2] - xv[rg9 - 3] - xv[rg9 - 1]) / (xv[rg9 - 1] - xv[rg9 - 3])) > 0.001:
            rg9 = rg9 - 1
        while rg9 > rg0 and abs((2 * xv[rg0 + 1] - xv[rg0] - xv[rg0 + 2]) / (xv[rg0 + 2] - xv[rg0])) > 0.001:
            rg0 = rg0 + 1
        self.checkRValues(rg9, rg0, xv, mylog)
        RUNDATA_res = (ws.readX(0)[rg9 - 1] - ws.readX(0)[rg0]) / (rg9 - rg0 - 1.0)  # assume linear!
        mylog.notice("resolution {0} us".format(RUNDATA_res))
        CHANNELS_itzero = rg0 + int(math.floor(-ws.readX(0)[rg0] / RUNDATA_res))
        # Bin with t0 in it. note, may be negative for pre-cropped
        # data. Remove +0.5
        TZERO_fine = (
            ws.readX(0)[CHANNELS_itzero] + ws.readX(0)[CHANNELS_itzero + 1]
        ) / 2.0  # since it's not an exact boundary. Error if t0<0 or t0 is in an odd sized bin
        mylog.notice(
            "time zero bin has boundaries {} and {} giving tzero={}".format(
                ws.readX(0)[CHANNELS_itzero], ws.readX(0)[CHANNELS_itzero + 1], TZERO_fine
            )
        )
        t1stgood = self.getProperty("FirstGoodTime").value
        CHANNELS_i1stgood = rg0 + max(
            int(math.floor((t1stgood - ws.readX(0)[rg0]) / RUNDATA_res + 1.0)), 0
        )  # was 1.0. i1stgood is first bin with purely good data in it (and good sized)
        FLAGS_fixphase = self.getProperty("FixPhases").value
        FLAGS_fitdead = self.getProperty("FitDeadTime").value
        OuterIter = self.getProperty("OuterIterations").value
        InnerIter = self.getProperty("InnerIterations").value
        # progress
        prog = Progress(self, start=0.0, end=1.0, nreports=OuterIter * InnerIter)

        tlast = self.getProperty("LastGoodTime").value
        ilast = min(
            rg0 + int(math.floor((tlast - ws.readX(0)[rg0]) / RUNDATA_res)), rg9 - 1
        )  # first bin with some bad data in it, or end (excluding bad sized bins)
        nhisto = ws.getNumberHistograms()
        POINTS_nhists = nhisto
        histlen = ilast  # -CHANNELS_itzero # actual data points to process, including before i1stgood
        # fill rdata with raw counts
        CHANNELS_itotal = histlen
        mylog.notice("channels t0={0} tgood={1} to {2}".format(CHANNELS_itzero, CHANNELS_i1stgood, CHANNELS_itotal))
        DATALL_rdata = np.zeros([nhisto, ilast])
        for i in range(nhisto):
            DATALL_rdata[i, :] = ws.readY(i)[:ilast]
        PULSES_npulse = self.getPulse()
        PULSES_def = self.getProperty("DefaultLevel").value
        FAC_factor = self.getProperty("Factor").value
        #
        # note on lengths of transforms, etc:
        # input data has CHANNELS_itotal data points with time spacing RUNDATA_res
        # Frequency spectrum has MAXPAGE_n data points with frequency spacing fperchan
        # maximum frequency fperchan*MAXPAGE_n should be greater than anything expected in the data (or resolved due to pulse width, etc)
        # Frequency spectrum is zero padded to POINTS_npts points and another POINTS_npts negative frequency components added,
        # all of those are zeros
        # Fourier transform performed on POINTS_npts*2 points (signed frequency axis)
        # after transform, only the first CHANNELS_itotal values are compared to the raw data, the others can take any value.
        #  (Data set actually padded to POINTS_npts with errors set to 1.E15 beyond good range)
        # length constraints:
        # POINTS_npts >=CHANNELS_itotal and POINTS_npts >= MAXPAGE_n
        # POINTS_npts should be a power of 2 for speed (though numpy.fft.fft() will cope if it isn't)
        # no requirement that CHANNELS_itotal or MAXPAGE_n are sub-multiples of POINTS_npts, or powers of 2 themselves
        # relationship between bin sizes, etc:
        # fperchan=1./(RUNDATA_res*float(POINTS_npts)*2.)
        #
        POINTS_npts = self.getProperty("Npts").value
        # e.g. npts=8192
        # i2pwr=log2(8192)=13
        # in zft and opus: call FFT with i2pwr+1 (=14)
        # in FFT: uses 2**14 points
        # so set I2 to be 2*npts (allows for all the negative ones!)
        if CHANNELS_itotal > POINTS_npts:
            mylog.notice("truncating useful data set from {0} to {1} data points".format(CHANNELS_itotal, POINTS_npts))
            CHANNELS_itotal = POINTS_npts  # short transform, omit some data points
        SAVETIME_i2 = POINTS_npts * 2
        maxfield = self.getProperty("MaxField").value
        MAXPAGE_n = int(maxfield * 0.01355 * 2 * POINTS_npts * RUNDATA_res)
        # number of active frequency points, need not now be a
        # power of 2?
        if MAXPAGE_n < 256:
            MAXPAGE_n = 256
        if MAXPAGE_n > POINTS_npts:
            MAXPAGE_n = POINTS_npts
        # load grouping. Mantid group table is different: one row per group, 1
        # column "detectors" with list of values
        RUNDATA_hists = np.zeros(nhisto)  # not necessary?
        GROUPING_group, POINTS_ngroups = self.doGrouping(POINTS_nhists, nhisto)
        # load dead times (note Maxent needs values per GROUP!)
        # standard dead time table is per detector. Take averages

        SENSE_taud, RUNDATA_frames = self.doDeadTimes(POINTS_ngroups, GROUPING_group, ws, FLAGS_fitdead, mylog)

        # sum histograms for total counts (not necessary?)
        # load Phase Table (previously done in BACK.for)
        # default being to distribute phases uniformly over 2pi, will work for
        # 2 groups F,B
        filePHASE = self.getPhase(FLAGS_fixphase, POINTS_ngroups, POINTS_nhists, mylog)
        #
        # debugging
        phaseconvWS = self.phaseConvergenceTable(POINTS_ngroups, deadDetectors, OuterIter, filePHASE)
        # do the work! Lots to pass in and out
        (
            MISSCHANNELS_mm,
            RUNDATA_fnorm,
            RUNDATA_hists,
            MAXPAGE_f,
            FAC_factor,
            FAC_facfake,
            FAC_ratio,
            DETECT_a,
            DETECT_b,
            DETECT_c,
            DETECT_d,
            DETECT_e,
            PULSESHAPE_convol,
            SENSE_taud,
            FASE_phase,
            SAVETIME_ngo,
            AMPS_amp,
            SENSE_phi,
            OUTSPEC_test,
            OUTSPEC_guess,
        ) = MULTIMAX(
            POINTS_nhists,
            POINTS_ngroups,
            POINTS_npts,
            CHANNELS_itzero,
            CHANNELS_i1stgood,
            CHANNELS_itotal,
            RUNDATA_res,
            RUNDATA_frames,
            GROUPING_group,
            DATALL_rdata,
            FAC_factor,
            SENSE_taud,
            MAXPAGE_n,
            filePHASE,
            PULSES_def,
            PULSES_npulse,
            FLAGS_fitdead,
            FLAGS_fixphase,
            SAVETIME_i2,
            OuterIter,
            InnerIter,
            mylog,
            prog,
            phaseconvWS,
            TZERO_fine,
            deadDetectors,
        )
        #
        fperchan = 1.0 / (RUNDATA_res * float(POINTS_npts) * 2.0)
        fchan = np.linspace(0.0, MAXPAGE_n * fperchan / 135.5e-4, MAXPAGE_n, endpoint=False)
        # write results! Frequency spectra
        outSpec = WorkspaceFactory.create(ws, NVectors=1, XLength=MAXPAGE_n, YLength=MAXPAGE_n)
        outSpec.dataX(0)[:] = fchan
        outSpec.dataY(0)[:] = MAXPAGE_f
        outSpec.getAxis(0).setUnit("Label").setLabel("Field", "Gauss")
        outSpec.setYUnitLabel("Probability")
        self.setProperty("OutputWorkspace", outSpec)
        # revised dead times
        if not self.getProperty("OutputDeadTimeTable").isDefault:
            outTaud = WorkspaceFactory.createTable()
            outTaud.addColumn("int", "spectrum", 1)
            outTaud.addColumn("double", "dead-time", 2)
            offset = 0
            for i in range(POINTS_ngroups + len(deadDetectors)):
                if i + 1 in deadDetectors:
                    outTaud.addRow([i + 1, 0.0])
                    offset += 1
                else:
                    outTaud.addRow([i + 1, SENSE_taud[i - offset]])
            self.setProperty("OutputDeadTimeTable", outTaud)
        # revised phases (and amplitudes since they're in the table too)
        if not self.getProperty("OutputPhaseTable").isDefault:
            outPhase = WorkspaceFactory.createTable()
            outPhase.addColumn("int", "Spectrum number", 1)
            outPhase.addColumn("double", "Asymmetry", 2)
            outPhase.addColumn("double", "Phase", 2)
            offset = 0
            for i in range(POINTS_ngroups + len(deadDetectors)):
                if i + 1 in deadDetectors:
                    outPhase.addRow([i + 1, 999, 0.0])
                    offset += 1
                else:
                    outPhase.addRow([i + 1, AMPS_amp[i - offset], SENSE_phi[i - offset]])
                    # sign of phase now OK for Mantid 3.12 onwards
            self.setProperty("OutputPhaseTable", outPhase)
        # reconstructed spectra passed back from OUTSPEC
        if not self.getProperty("ReconstructedSpectra").isDefault:
            k2 = CHANNELS_itotal  # channel range in source workspace accounting for instrumental t0
            k1 = CHANNELS_i1stgood
            i1 = k1 - CHANNELS_itzero  # channel range in guess, etc (t0 at start)
            i2 = k2 - CHANNELS_itzero
            mylog.notice(
                "i1={} i2={} k1={} k2={} len(srcX)={} len(guess)={}".format(i1, i2, k1, k2, len(ws.dataX(0)), OUTSPEC_guess.shape[0])
            )
            recSpec = WorkspaceFactory.create(ws, NVectors=POINTS_ngroups + len(deadDetectors), XLength=i2 - i1 + 1, YLength=i2 - i1)
            offset = 0
            for j in range(POINTS_ngroups + len(deadDetectors)):
                if j + 1 in deadDetectors:
                    offset += 1
                    recSpec.dataX(j)[:] = originalWS.dataX(j)[k1 : k2 + 1]
                    recSpec.dataY(j)[:] = np.zeros(k2 - k1)
                else:
                    recSpec.dataX(j)[:] = originalWS.dataX(j)[k1 : k2 + 1]
                    recSpec.dataY(j)[:] = OUTSPEC_guess[i1:i2, j - offset]
            self.setProperty("ReconstructedSpectra", recSpec)
        if phaseconvWS:
            self.setProperty("PhaseConvergenceTable", phaseconvWS)
        # final converged Factor
        self.setProperty("Factor", FAC_factor)
        # final chisquared?


AlgorithmFactory.subscribe(MuonMaxent)
