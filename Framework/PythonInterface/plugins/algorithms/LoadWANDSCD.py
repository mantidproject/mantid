# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    IMDHistoWorkspaceProperty,
    IMDHistoWorkspace,
    MultipleFileProperty,
    Progress,
    PropertyMode,
    PythonAlgorithm,
    WorkspaceProperty,
    mtd,
)
from mantid.kernel import (
    Direction,
    Property,
    IntArrayProperty,
    StringListValidator,
    FloatTimeSeriesProperty,
)
from mantid.simpleapi import (
    DivideMD,
    LoadEventNexus,
    RemoveLogs,
    ReplicateMD,
    DeleteWorkspace,
    ConvertToMD,
    Rebin,
    GroupDetectors,
    SetUB,
)
from typing import List
import numpy as np
import h5py
import re


class LoadWANDSCD(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["ConvertWANDSCDtoQ"]

    def name(self):
        return "LoadWANDSCD"

    def summary(self):
        return "Load WAND single crystal data into a detector space vs rotation MDHisto"

    def PyInit(self):
        # Input workspace/data info (filename or IPTS+RunNumber)
        # Priority: Filename > IPTS + RunNumber
        self.declareProperty(MultipleFileProperty(name="Filename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]), "Files to load")
        self.declareProperty("IPTS", Property.EMPTY_INT, "IPTS number to load from")
        self.declareProperty(IntArrayProperty("RunNumbers", []), "Run numbers to load")

        # Normalization info (optional, skip normalization if not specified)
        # Priority: IPTS + RunNumber > Filename >  NormalizationFile
        # NOTE:
        #   The current convention for loading Vanadium data is by IPTS+RunNumber, so this is the default\
        # -- default
        self.declareProperty("VanadiumIPTS", Property.EMPTY_INT, "IPTS number to load Vanadium normalization")
        self.declareProperty("VanadiumRunNumber", Property.EMPTY_INT, "Run number to load Vanadium normalization")
        # -- alternative
        self.declareProperty(
            FileProperty(
                name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input, action=FileAction.OptionalLoad
            ),
            doc="File with Vanadium normalization scan data",
        )
        # alternative
        self.declareProperty(
            IMDHistoWorkspaceProperty("VanadiumWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="MDHisto workspace containing vanadium normalization data",
        )
        # normalization method
        self.declareProperty(
            "NormalizedBy", "None", StringListValidator(["None", "Counts", "Monitor", "Time"]), "Normalize to Counts, Monitor, Time."
        )
        # group normalization properties
        self.setPropertyGroup("VanadiumIPTS", "Normalization")
        self.setPropertyGroup("VanadiumRunNumber", "Normalization")
        self.setPropertyGroup("VanadiumFile", "Normalization")
        self.setPropertyGroup("VanadiumWorkspace", "Normalization")
        self.setPropertyGroup("NormalizedBy", "Normalization")

        # Grouping info
        self.declareProperty(
            "Grouping", "None", StringListValidator(["None", "2x2", "4x4"]), "Group pixels (shared by input and normalization)"
        )

        # apply goniometer tilt
        self.declareProperty(
            "ApplyGoniometerTilt",
            False,
            "Apply the goniometer sgl and sgu tilts to the UB-matrix. This is for backwards compatability only.",
        )

        # Output workspace/data info
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output), "Output Workspace"
        )

    def validateInputs(self):
        issues = dict()

        # case 1: missing input
        if not self.getProperty("Filename").value:
            if (self.getProperty("IPTS").value == Property.EMPTY_INT) or len(self.getProperty("RunNumbers").value) == 0:
                issues["Filename"] = "Must specify either Filename or IPTS AND RunNumbers"

        # case 2: vanadium IPTS and run number must be specified at the same time
        va_noIPTS = self.getProperty("VanadiumIPTS").isDefault
        va_useIPTS = not self.getProperty("VanadiumIPTS").isDefault
        va_noRunNumber = self.getProperty("VanadiumRunNumber").isDefault
        va_useRunNumber = not self.getProperty("VanadiumRunNumber").isDefault
        if (va_useIPTS and va_noRunNumber) or (va_noIPTS and va_useRunNumber):
            issues["VanadiumIPTS"] = "VanadiumIPTS and VanadiumRunNumber must be specified together"

        return issues

    def PyExec(self):
        # Process input data
        # get the list of input filenames
        runs = self.get_intput_filenames()
        # load and group
        data = self.load_and_group(runs)

        # check if normalization need to be performed
        # NOTE: the normalization will be skipped if no information regarding Vanadium is provided
        skipNormalization = (
            self.getProperty("VanadiumIPTS").isDefault
            and self.getProperty("VanadiumRunNumber").isDefault
            and self.getProperty("VanadiumFile").isDefault
            and self.getProperty("VanadiumWorkspace").isDefault
        )
        if skipNormalization:
            self.log().warning("No Vanadium data provided, skip normalization")
        else:
            # attempt to get the vanadium via file
            van_filename = self.get_va_filename()
            if van_filename is None:
                # try to load from memory
                norm = self.getProperty("VanadiumWorkspace").value
            else:
                norm = self.load_and_group([van_filename])
            # normalize
            data = self.normalize(data, norm, self.getProperty("NormalizedBy").value.lower())
            # cleanup
            # NOTE: if va is read from memory, we will keep it in case we need it later
            if van_filename is not None:
                DeleteWorkspace(norm)

        # setup output
        self.setProperty("OutputWorkspace", data)
        # cleanup
        DeleteWorkspace(data)

    def get_intput_filenames(self) -> List:
        """
        Get the input filenames via either the input or the IPTS + run number
        """
        ws_filenames = self.getProperty("Filename").value
        if not ws_filenames:
            ipts = self.getProperty("IPTS").value
            runs = self.getProperty("RunNumbers").value
            ws_filenames = [f"/HFIR/HB2C/IPTS-{ipts}/nexus/HB2C_{run}.nxs.h5" for run in runs]
        # edge case where only one dataset is loaded
        if isinstance(ws_filenames, str):
            ws_filenames = [ws_filenames]
        # return the list of workspaces names
        return ws_filenames

    def get_va_filename(self) -> str:
        """
        get the va filename
        """
        # check order
        # IPTS + run number > file name > memory
        if (not self.getProperty("VanadiumIPTS").isDefault) and (not self.getProperty("VanadiumRunNumber").isDefault):
            # we prefer to use IPTS + run number to load Va
            ipts_va = self.getProperty("VanadiumIPTS").value
            run_va = self.getProperty("VanadiumRunNumber").value
            va_filename = f"/HFIR/HB2C/IPTS-{ipts_va}/nexus/HB2C_{run_va}.nxs.h5"
        elif not self.getProperty("VanadiumFile").isDefault:
            # try the file name next
            va_filename = self.getProperty("VanadiumFile").value
        else:
            # we can only hope the data is already loaded in memory
            va_filename = None
        return va_filename

    def load_and_group(self, runs: List[str]) -> IMDHistoWorkspace:
        """
        Load the data with given grouping
        """
        # grouping config
        grouping = self.getProperty("Grouping").value
        if grouping == "None":
            grouping = 1
        else:
            grouping = 2 if grouping == "2x2" else 4
        number_of_runs = len(runs)

        x_dim = 480 * 8 // grouping
        y_dim = 512 // grouping

        data_array = np.empty((number_of_runs, x_dim, y_dim), dtype=np.float64)

        s1_array = []
        sgl_array = []
        sgu_array = []
        duration_array = []
        run_number_array = []
        monitor_count_array = []

        progress = Progress(self, 0.0, 1.0, number_of_runs + 3)

        for n, run in enumerate(runs):
            progress.report("Loading: " + run)
            with h5py.File(run, "r") as f:
                bc = np.zeros((512 * 480 * 8), dtype=np.int64)
                for b in range(8):
                    bc += np.bincount(f["/entry/bank" + str(b + 1) + "_events/event_id"][()], minlength=512 * 480 * 8)
                bc = bc.reshape((480 * 8, 512))
                if grouping == 2:
                    bc = bc[::2, ::2] + bc[1::2, ::2] + bc[::2, 1::2] + bc[1::2, 1::2]
                elif grouping == 4:
                    bc = (
                        bc[::4, ::4]
                        + bc[1::4, ::4]
                        + bc[2::4, ::4]
                        + bc[3::4, ::4]
                        + bc[::4, 1::4]
                        + bc[1::4, 1::4]
                        + bc[2::4, 1::4]
                        + bc[3::4, 1::4]
                        + bc[::4, 2::4]
                        + bc[1::4, 2::4]
                        + bc[2::4, 2::4]
                        + bc[3::4, 2::4]
                        + bc[::4, 3::4]
                        + bc[1::4, 3::4]
                        + bc[2::4, 3::4]
                        + bc[3::4, 3::4]
                    )
                data_array[n] = bc
                s1_array.append(f["/entry/DASlogs/HB2C:Mot:s1.RBV/average_value"][0])
                sgl_array.append(f["/entry/DASlogs/HB2C:Mot:sgl.RBV/average_value"][0])
                sgu_array.append(f["/entry/DASlogs/HB2C:Mot:sgu.RBV/average_value"][0])
                duration_array.append(float(f["/entry/duration"][0]))
                run_number_array.append(float(f["/entry/run_number"][0]))
                monitor_count_array.append(float(f["/entry/monitor1/total_counts"][0]))

        progress.report("Creating MDHistoWorkspace")
        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput", data_array)
        createWS_alg.setProperty("ErrorInput", np.sqrt(data_array))
        createWS_alg.setProperty("Dimensionality", 3)
        createWS_alg.setProperty("Extents", "0.5,{},0.5,{},0.5,{}".format(y_dim + 0.5, x_dim + 0.5, number_of_runs + 0.5))
        createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(y_dim, x_dim, number_of_runs))
        createWS_alg.setProperty("Names", "y,x,scanIndex")
        createWS_alg.setProperty("Units", "bin,bin,number")
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        progress.report("Getting IDF")
        # Get the instrument and some logs from the first file; assume the rest are the same
        _tmp_ws = LoadEventNexus(runs[0], MetaDataOnly=True, EnableLogging=False)
        # The following logs should be the same for all runs
        RemoveLogs(
            _tmp_ws,
            KeepLogs="HB2C:Mot:detz,HB2C:Mot:detz.RBV,HB2C:Mot:s2,HB2C:Mot:s2.RBV,"
            "run_title,start_time,experiment_identifier,HB2C:CS:CrystalAlign:UBMatrix",
            EnableLogging=False,
        )

        time_ns_array = _tmp_ws.run().startTime().totalNanoseconds() + np.append(0, np.cumsum(duration_array) * 1e9)[:-1]

        try:
            UB = np.array(
                re.findall(r"-?\d+\.*\d*", _tmp_ws.run().getProperty("HB2C:CS:CrystalAlign:UBMatrix").value[0]), dtype=float
            ).reshape(3, 3)
            SetUB(_tmp_ws, UB=UB, EnableLogging=False)
        except (RuntimeError, ValueError):
            UB = np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])

        if self.getProperty("ApplyGoniometerTilt").value:
            sgl = np.deg2rad(np.mean(sgl_array))  # 'HB2C:Mot:sgl.RBV,1,0,0,-1'
            sgu = np.deg2rad(np.mean(sgu_array))  # 'HB2C:Mot:sgu.RBV,0,0,1,-1'
            sgl_a = np.array([[1, 0, 0], [0, np.cos(sgl), np.sin(sgl)], [0, -np.sin(sgl), np.cos(sgl)]])
            sgu_a = np.array([[np.cos(sgu), np.sin(sgu), 0], [-np.sin(sgu), np.cos(sgu), 0], [0, 0, 1]])
            UB = sgl_a.dot(sgu_a).dot(UB)  # Apply the Goniometer tilts to the UB matrix

        SetUB(_tmp_ws, UB=UB, EnableLogging=False)

        if grouping > 1:
            detector_list = ""
            for x in range(0, 480 * 8, grouping):
                for y in range(0, 512, grouping):
                    spectra_list = []
                    for j in range(grouping):
                        for i in range(grouping):
                            spectra_list.append(str(y + i + (x + j) * 512))
                    detector_list += "," + "+".join(spectra_list)
            _tmp_ws = GroupDetectors(InputWorkspace=_tmp_ws, GroupingPattern=detector_list, EnableLogging=False)

        progress.report("Adding logs")

        # Hack: ConvertToMD is needed so that a deep copy of the ExperimentInfo can happen
        # outWS.addExperimentInfo(_tmp_ws) # This doesn't work but should, when you delete `ws` `outWS` also loses it's ExperimentInfo
        _tmp_ws = Rebin(_tmp_ws, "0,1,2", EnableLogging=False)
        _tmp_ws = ConvertToMD(_tmp_ws, dEAnalysisMode="Elastic", EnableLogging=False, PreprocDetectorsWS="__PreprocessedDetectorsWS")

        preprocWS = mtd["__PreprocessedDetectorsWS"]
        twotheta = preprocWS.column(2)
        azimuthal = preprocWS.column(3)

        outWS.copyExperimentInfos(_tmp_ws)
        DeleteWorkspace(_tmp_ws, EnableLogging=False)
        DeleteWorkspace("__PreprocessedDetectorsWS", EnableLogging=False)
        # end Hack

        add_time_series_property("HB2C:Mot:s1", outWS.getExperimentInfo(0).run(), time_ns_array, s1_array)
        outWS.getExperimentInfo(0).run().getProperty("HB2C:Mot:s1").units = "deg"
        add_time_series_property("HB2C:Mot:sgl", outWS.getExperimentInfo(0).run(), time_ns_array, sgl_array)
        outWS.getExperimentInfo(0).run().getProperty("HB2C:Mot:sgl").units = "deg"
        add_time_series_property("HB2C:Mot:sgu", outWS.getExperimentInfo(0).run(), time_ns_array, sgu_array)
        outWS.getExperimentInfo(0).run().getProperty("HB2C:Mot:sgu").units = "deg"

        add_time_series_property("duration", outWS.getExperimentInfo(0).run(), time_ns_array, duration_array)
        outWS.getExperimentInfo(0).run().getProperty("duration").units = "second"
        outWS.getExperimentInfo(0).run().addProperty("run_number", run_number_array, True)
        add_time_series_property("monitor_count", outWS.getExperimentInfo(0).run(), time_ns_array, monitor_count_array)
        outWS.getExperimentInfo(0).run().addProperty("twotheta", twotheta, True)
        outWS.getExperimentInfo(0).run().addProperty("azimuthal", azimuthal, True)

        setGoniometer_alg = self.createChildAlgorithm("SetGoniometer", enableLogging=False)
        setGoniometer_alg.setProperty("Workspace", outWS)
        setGoniometer_alg.setProperty("Axis0", "HB2C:Mot:s1,0,1,0,1")
        if not self.getProperty("ApplyGoniometerTilt").value:
            setGoniometer_alg.setProperty("Axis1", "HB2C:Mot:sgl,1,0,0,-1")
            setGoniometer_alg.setProperty("Axis2", "HB2C:Mot:sgu,0,0,1,-1")
        setGoniometer_alg.setProperty("Average", False)
        setGoniometer_alg.execute()

        return outWS

    def normalize(
        self,
        data: IMDHistoWorkspace,
        norm: IMDHistoWorkspace,
        normalize_by: str,
    ) -> IMDHistoWorkspace:
        """
        Normalize the given data with given Va normalization workspace
        """
        # prep
        norm_replicated = ReplicateMD(ShapeWorkspace=data, DataWorkspace=norm)
        data = DivideMD(LHSWorkspace=data, RHSWorkspace=norm_replicated)
        # reduce memory footprint
        DeleteWorkspace(norm_replicated)
        # find the scale
        if normalize_by == "counts":
            scale = norm.getSignalArray().mean()
            self.getLogger().information("scale counts = {}".format(int(scale)))
        elif normalize_by == "monitor":
            scale = np.array(data.getExperimentInfo(0).run().getProperty("monitor_count").value)
            scale /= norm.getExperimentInfo(0).run().getProperty("monitor_count").value[0]
            self.getLogger().information("scale monitor = {}".format(scale))
        elif normalize_by == "time":
            scale = np.array(data.getExperimentInfo(0).run().getProperty("duration").value)
            scale /= norm.getExperimentInfo(0).run().getProperty("duration").value[0]
            self.getLogger().information("van time = {}".format(scale))
        elif normalize_by == "none":
            scale = 1
        else:
            raise RuntimeError(f"Unknown normalization method: {normalize_by}!")
        # exec
        data.setSignalArray(data.getSignalArray() / scale)
        data.setErrorSquaredArray(data.getErrorSquaredArray() / scale**2)
        # return
        return data


def add_time_series_property(name, run, times, values):
    log = FloatTimeSeriesProperty(name)
    for t, v in zip(times, values):
        log.addValue(t, v)
    run[name] = log


AlgorithmFactory.subscribe(LoadWANDSCD)
