# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import re
import math
import numpy as np
import h5py
import json

from scipy.interpolate import RectBivariateSpline
from typing import Tuple, List, Union, NamedTuple, Dict, Any, Callable, TypeVar

from configparser import ConfigParser, NoOptionError, NoSectionError

from mantid import mtd
from mantid.api import NumericAxis, Algorithm, Progress
from mantid.kernel import DateAndTime
from mantid.geometry import Instrument
from mantid.simpleapi import (
    Workspace,
    FileFinder,
    LoadNexusProcessed,
    SaveNexusProcessed,
    DeleteWorkspace,
    MaskDetectors,
    RenameWorkspace,
    MergeRuns,
    FindEPP,
    Integration,
    CreateWorkspace,
    ConvertSpectrumAxis,
)

T = TypeVar("T")

TOption = Union[T, None]
IntOption = Union[int, None]
FloatOption = Union[float, None]
StrOption = Union[str, None]
RangeOption = Union[Tuple[int, int], None]
TypeCast = Callable[[str], T]
ConfigType = Union[bool, int, float, str]

NDArray = np.ndarray
LoaderOptions = Dict[str, Any]


class SingleRun(NamedTuple):
    cycle: IntOption
    run: int
    dataset: IntOption

    def __repr__(self) -> str:
        prefix = str(self.cycle) + "::" if self.cycle else ""
        suffix = ":" + str(self.dataset) if self.dataset else ""
        return prefix + str(self.run) + suffix


class RunGroup(NamedTuple):
    cycle: str
    runs: str
    dataset: str

    def __repr__(self) -> str:
        prefix = self.cycle + "::" if self.cycle else ""
        suffix = ":" + self.dataset if self.dataset else ""
        return prefix + self.runs + suffix


class HdfKey(NamedTuple):
    key: str
    hdf_tag: str
    default: Any


class LoadLog(NamedTuple):
    # used for comparing load option with workspace log parameter
    load: str
    log: str
    tol: float


# const definitions
CYCLE_TAG = "NNN"


def start_end_time(source_ws: str) -> Tuple[DateAndTime, DateAndTime]:
    run = mtd[source_ws].getRun()
    start_time = run.startTime()
    end_time = run.endTime()
    if not end_time > start_time:
        duration = int(1e9 * run.getProperty("dur").value)
        end_time = DateAndTime(start_time.totalNanoseconds() + duration)
    return start_time, end_time


def num_detectors(ws: Workspace) -> int:
    # number of detectors in a workspace ignoring monitors
    sum = 0
    detector_info = ws.detectorInfo()
    for i in range(ws.getNumberHistograms()):
        if not detector_info.isMonitor(i):
            sum += 1
    return sum


def range_to_values(rng: str) -> Tuple[float, ...]:
    # converts a comma separated list of numbers to a tuple of floats
    return tuple([float(x) for x in rng.split(",")])


def find_nearest_index(values: List[float], target: float) -> int:
    return int(np.abs(np.array(values) - target).argmin())


def split_run_index(run: str) -> Tuple[str, int]:
    # splits fpath.hdf:n to fpath.hdf and n
    gp = re.search(r":([0-9]+?)$", run)
    if gp:
        index = int(gp.group(1))
        base = run[: gp.start()]
    else:
        index = 0
        base = run
    return base, index


def find_file(fname: str, directories: List[str]) -> str:
    # search for the file in the list of directories otherwise try
    # mantid file finder
    for dir in directories:
        fpath = os.path.join(dir, fname)
        if os.path.isfile(fpath):
            return fpath
    return FileFinder.getFullPath(fname)


def find_event_folder(hdf_path: str, search_directories: List[str]) -> Tuple[str, str, NDArray]:
    # TOF ansto data is stored typically as a pair of files (hdf, binary) where the
    # name of the binary event file is saved in the hdf file. The function extracts
    # name from the hdf and looks through the search directories for a match.
    base_dir, _ = os.path.split(hdf_path)

    # open hdf file and dir for event
    with h5py.File(hdf_path, "r") as fp:
        tag = "entry1/instrument/detector/"
        try:
            dir_name = fp[tag + "daq_dirname"][0]
            datasets = fp[tag + "dataset_number"][:]
        except KeyError:
            raise RuntimeError("Missing {} dirname or dataset in {}".format(tag, hdf_path))

        # attempt to decode if necessary
        try:
            dir_name = dir_name.decode()
        except AttributeError:
            pass

    # first check for adjacent histmem subfolder
    for reldir in ["../histserv", "./hsdata"]:
        ev_base = os.path.normpath(os.path.join(base_dir, reldir))
        ev_path = os.path.normpath(os.path.join(ev_base, dir_name))
        if os.path.isdir(ev_path):
            return ev_base, dir_name, datasets

    # else run through the list of search directories
    # for sdir in config.getDataSearchDirs():
    for sdir in search_directories:
        # print 'search {}'.format(sdir)
        ev_path = os.path.normpath(os.path.join(sdir, dir_name))
        if os.path.isdir(ev_path):
            return sdir, dir_name, datasets

    # if it got to here assume the bin file is in the base folder, if not an exception
    # will be raised either way
    return "./", "", datasets


def seq_to_list(iseqn: str) -> List[int]:
    # convert a comma separated range of numbers returned as a list
    # first clean all whitespaces
    seqn = iseqn.replace(" ", "")
    nlist = []
    sqlist = seqn.split(",")
    for rg in sqlist:
        if rg == "":
            continue
        ss = rg.split("-")
        try:
            lo = int(ss[0])
            hi = int(ss[-1])
            if len(ss) == 1:
                nlist.append(lo)
            else:
                for i in range(lo, hi + 1):
                    nlist.append(i)
        except ValueError:
            raise RuntimeError("Unexpected run sequence: {}".format(seqn))

    return nlist


def datasets_in_file(file_path: str) -> List[int]:
    # return an array with the dataset indexes
    tags = [HdfKey("ds", "/entry1/instrument/detector/dataset_number/", None)]
    values, _ = extract_hdf_params(file_path, tags)
    return list(values["ds"])


def dataset_indexes(fpath: str, ds_seqn: str) -> List[int]:
    # the loader uses the index into the dataset to find the number
    all_datasets = datasets_in_file(fpath)
    ds_map = dict([(x, i) for i, x in enumerate(all_datasets)])
    if ds_seqn.strip() == "*":
        return list(ds_map.values())
    else:
        ds_values = seq_to_list(ds_seqn)
        try:
            return [ds_map[ds] for ds in ds_values]
        except KeyError:
            raise ValueError("Cannot find dataset {} in file {}".format(ds_seqn, fpath))


def get_variants(tag: str) -> List[str]:
    # extract all the variants from a single tag
    values = re.findall(r"\[(.*?)\]", tag)
    variants = []
    for value in values[0].split(","):
        try:
            numerics = seq_to_list(value)
            # ansto uses 3 digit cycle numbers with a leading zero
            variants = variants + [f"{i:03d}" for i in numerics]
        except RuntimeError:
            # catches non numeric variants
            variants = variants + [value]
    return variants


def replace_variants(infolders: List[str], tags: List[str]) -> List[str]:
    # expand the folder list with variants from the first tag and recursively
    # call for the remaining tags
    if tags:
        outfolders = []
        tag = tags[0]
        variants = get_variants(tag)
        for fpath in infolders:
            for variant in variants:
                outfolders.append(fpath.replace(tag, variant.strip()))
        olist = replace_variants(outfolders, tags[1:])
        return olist
    else:
        return infolders


def expand_directories(included: List[str]) -> List[str]:
    # expand the search path with confirmed variants
    exp_folders = []
    for folder in included:
        # get the [] variants - if numeric convert to numeric list, else split by comma
        allvariants = re.findall(r"\[.*?\]", folder)
        folders = replace_variants([folder], allvariants)
        for fpath in folders:
            # only include valid folders in the search folders
            if os.path.isdir(fpath) or re.findall(CYCLE_TAG, fpath):
                exp_folders.append(fpath)
    return exp_folders


def find_cycle_folders(cycle_no: int, folder_list: List[str]) -> List[str]:
    # look for the pattern 'NNN' in the folder path
    cycle_folders = []
    for folder in folder_list:
        if re.findall(CYCLE_TAG, folder):
            fpath = folder.replace(CYCLE_TAG, "{:03d}".format(cycle_no))
            cycle_folders.append(fpath)
    return cycle_folders


def list_to_seq(nlist: List[int]) -> str:
    # converts a list of numbers into an ordered string sequence,
    # for example [1,2,3,10,7,6,8] -> '1-3,6-8,10'

    # return for trivial cases
    if len(nlist) == 0:
        return ""
    if len(nlist) == 1:
        return str(nlist[0])

    # rewrite using range notation to a simple sequence
    sequence = []
    sorted_runs = sorted(nlist)
    run_end = run_start = next_run = sorted_runs[0]
    for next_run in sorted_runs[1:]:
        if next_run == run_end + 1:
            run_end = next_run
        else:
            # add the range
            if run_start != run_end:
                sequence.append(str(run_start) + "-" + str(run_end))
            else:
                sequence.append(str(run_start))
            run_start, run_end = next_run, next_run
    if run_start != next_run:
        sequence.append(str(run_start) + "-" + str(next_run))
    else:
        sequence.append(str(next_run))
    return ",".join(sequence)


def encode_run_list(file_list: List[str]) -> str:
    # filename is of the form ../EMUnnnnnnn.nx.hdf:ds
    # encode the run number and dataset index
    seqn = {}
    for fpath in file_list:
        m = re.search(r"[A-Z]{3}([0-9]{7}).*:([0-9]{1,3})", fpath)
        if m:
            run, ds = m[1], int(m[2])
            run = int(run.lstrip("0"))
            try:
                seqn[run].append(ds)
            except KeyError:
                seqn[run] = [ds]
        else:
            raise ValueError("Could not extract run and dataset for {}".format(fpath))
    merged = []
    dset = None
    common_dset = []
    for run in sorted(seqn.keys()):
        ds_tag = list_to_seq(seqn[run])
        if ds_tag == dset:
            # same dataset index so merge the runs and continue
            common_dset.append(run)
            continue
        # valid run exists but new run has a different dataset
        # close it off and start new collection
        if common_dset and dset:
            run_seqn = list_to_seq(common_dset)
            merged.append("{}:{}".format(run_seqn, dset))
        dset = ds_tag
        common_dset = [run]

    if common_dset and dset:
        run_seqn = list_to_seq(common_dset)
        merged.append("{}:{}".format(run_seqn, dset))

    return ";".join(merged)


def build_file_list(search_path: List[str], file_prefix: str, file_extn: str, rungrp: RunGroup) -> List[str]:
    data_files = []
    for run in seq_to_list(rungrp.runs):
        fname = "{}{:07d}{}".format(file_prefix, run, file_extn)
        fpath = find_file(fname, search_path)
        if not fpath:
            raise RuntimeError("Cannot find file: {}".format(fname))

        datasets = dataset_indexes(fpath, rungrp.dataset) if rungrp.dataset else [0]
        for ds in datasets:
            data_files.append("{}:{}".format(fpath, ds))

    return data_files


def run_groups(allruns: str) -> List[RunGroup]:
    """
    The complete runs sequence can be expressed using the following defns:
    rungroup : [cycle::]runseqn[:ds]
          where runseqn is a comma separated list of runs such as
              '123,126,127-135,140'
    allruns : rungroup [;rungroup; ...]
    """
    groups = []
    for rgp in allruns.split(";"):
        # assumes [cycle::]runseqn[:ds] format
        cycle = dataset = ""
        runs = rgp
        if "::" in rgp:
            cycle, runs = rgp.split("::")[:2]
        if ":" in runs:
            runs, dataset = runs.split(":")[:2]
        groups.append(RunGroup(cycle, runs, dataset))
    return groups


def expanded_runs(allruns: str) -> List[SingleRun]:
    exruns = []
    for rgp in run_groups(allruns):
        cycle = int(rgp.cycle) if rgp.cycle else None
        dataset = int(rgp.dataset) if rgp.dataset else None
        for run in seq_to_list(rgp.runs):
            exruns.append(SingleRun(cycle, int(run), dataset))
    return exruns


def hdf_files_from_runs(all_runs: str, search_dirs: List[str], file_prefix: str, file_extn: str) -> List[str]:
    # the run format is cycle:: runs; cycle:: runs; ..
    # to collect all the data the split sequence ';', '::'
    #
    valid_dirs = [fpath for fpath in search_dirs if os.path.isdir(fpath)]
    cycle_dirs = [fpath for fpath in search_dirs if re.findall(CYCLE_TAG, fpath)]

    # split by cycle first
    analyse_runs = []
    for rgp in run_groups(all_runs):
        # refine the search path
        if rgp.cycle and cycle_dirs:
            search_path = find_cycle_folders(int(rgp.cycle), search_dirs)
        else:
            search_path = valid_dirs

        # get the list of filenames
        data_files = build_file_list(search_path, file_prefix, file_extn, rgp)
        analyse_runs += data_files

    # remove any repeated files and sort
    return sorted(list(set(analyse_runs)))


def extract_hdf_params(fpath: str, tags: List[HdfKey]) -> Tuple[Dict[str, Any], Dict[str, Dict[str, str]]]:
    # gets the parameters from the base file to be able to complete the setup
    # such as, doppler amplitude and speed
    # if the hdf parameter is mssing and no default is provide an
    # exception is raised

    values = {}
    attrs = {}
    with h5py.File(fpath, "r") as fp:
        for key, hdf_tag, def_value in tags:
            try:
                values[key] = fp[hdf_tag][()]
                attrs[key] = {}
                for name, value in fp[hdf_tag].attrs.items():
                    attrs[key][name] = value
            except KeyError:
                if def_value is None:
                    raise RuntimeError("Missing {} in {}".format(key, fpath))
                values[key] = def_value
    return values, attrs


def total_time(src: str) -> float:
    run = mtd[src].getRun()
    return run.getProperty("dur").value


def setup_axis(ws_tag: str, curAxis: Dict[str, Any]):
    """
    create an axis object and initiate the label, units and values
    then replace the axis in the workspace
    """
    ws = mtd[ws_tag]
    nhist = ws.getNumberHistograms()
    newAxis = NumericAxis.create(nhist)
    if curAxis["unitID"] in ["MomentumTransfer", "Degrees", "QSquared"]:
        newAxis.setUnit(curAxis["unitID"])
    else:
        newAxis.setUnit(curAxis["unitID"]).setLabel(curAxis["label"], curAxis["units"])
    for ix, value in enumerate(curAxis["values"]):
        newAxis.setValue(ix, value)
    ws.replaceAxis(1, newAxis)


def append_ini_params(output_ws: str, ini_options: Dict[str, str], ui_options: Dict[str, str]) -> None:
    # ui_options have precedent over the ini file options
    run = mtd[output_ws].getRun()
    options = {}
    for k, v in ini_options.items():
        options[k] = v
    for k, v in ui_options.items():
        options[k] = v
    skeys = sorted(options.keys())
    for k in skeys:
        run.addProperty("ini_" + k, options[k], True)


class FilterPixelsTubes:
    """
    Filters a workspace generated from tube data where:
     spectra_j = tube_i * pixels_per_tube + pixel_k + pixel_offset
    """

    _valid_tubes: StrOption
    _valid_pixels: RangeOption
    _pixels_per_tube: int
    _pixel_offset: int
    _nhist: int
    _instrument: Instrument
    _include: np.ndarray

    def __init__(self, valid_tubes: StrOption, valid_pixels: RangeOption, pixels_per_tube: int, pixel_offset: int):
        self._valid_tubes = valid_tubes
        self._valid_pixels = valid_pixels
        self._pixels_per_tube = pixels_per_tube
        self._pixel_offset = pixel_offset

    def set(
        self,
        valid_tubes: StrOption = None,
        valid_pixels: RangeOption = None,
        pixels_per_tube: IntOption = None,
        pixel_offset: IntOption = None,
    ):
        if valid_tubes:
            self._valid_tubes = valid_tubes
        if valid_pixels:
            self._valid_pixels = valid_pixels
        if pixels_per_tube:
            self._pixels_per_tube = pixels_per_tube
        if pixel_offset:
            self._pixel_offset = pixel_offset

    def _setup(self, ws_tag: str):
        event_ws = mtd[ws_tag]
        self._instrument = event_ws.getInstrument()
        self._nhist = event_ws.getNumberHistograms()
        self._include = np.full(self._nhist, False)

    def _map_to_spectra(self, tube: int, pixel: int):
        return tube * self._pixels_per_tube + pixel + self._pixel_offset

    def _drop_monitors(self, ws_tag: str):
        detector_info = mtd[ws_tag].detectorInfo()
        for i in range(self._nhist):
            if detector_info.isMonitor(i):
                self._include[i] = False

    def _mask_to_tube(self):
        if self._valid_tubes is None:
            self._include[:] = True
        else:
            for tube in sorted(seq_to_list(self._valid_tubes)):
                lo = self._map_to_spectra(tube, 0)
                hi = self._map_to_spectra(tube + 1, 0)
                self._include[lo:hi] = True

    def _mask_to_pixels(self):
        # mask to the valid pixel range per tube
        if self._valid_pixels is not None:
            det_ids = np.arange(self._nhist) - self._pixel_offset
            lo_ids = det_ids % self._pixels_per_tube >= self._valid_pixels[0]
            hi_ids = det_ids % self._pixels_per_tube <= self._valid_pixels[1]
            self._include *= lo_ids * hi_ids

    def _scan_spectra(self, ws_tag: str):
        # scan over the spectrum
        event_ws = mtd[ws_tag]
        for i in range(self._nhist):
            if not self._include[i]:
                evl = event_ws.getSpectrum(i)
                evl.clear(False)

    def _finalise_and_mask(self, ws_tag: str, output_ws: str):
        # mask the spectra, exlicitly convert numpy.int32 to int as MaskDetectors fails
        mask = np.invert(self._include)
        masked_spectra = [int(x) for x in np.arange(self._nhist)[mask]]
        MaskDetectors(Workspace=ws_tag, SpectraList=masked_spectra)

        if ws_tag != output_ws:
            RenameWorkspace(InputWorkspace=ws_tag, OutputWorkspace=output_ws)

    def filter_workspace(self, ws_tag: str, output_ws: str):
        # first set all pixels on the valid tubes enabled
        # then turn of the pixels outside the valid range

        self._setup(ws_tag)
        self._mask_to_tube()
        self._mask_to_pixels()
        self._drop_monitors(ws_tag)
        self._scan_spectra(ws_tag)
        self._finalise_and_mask(ws_tag, output_ws)


class ScratchFolder:
    """
    Utility class to managed loading and saving workspaces to a scratch folder.
    The ANSTO event files are slow loading relative to event workspaces saved
    as nexus files. The functionality loads an ansto event file and saves it
    to the scratch folder as a nexus file. If it needs to be reloaded it looks for
    an existing nexus file first.
    When a merged file is saved to the scratch folder the loader options are
    appended to the run log. A merged file is successfuly restored if the loader
    options match.
    """

    _temp_folder: str

    def __init__(self, path: str) -> None:
        if not os.path.isdir(path):
            raise RuntimeError("Not a valid directory: {}".format(path))
        self._temp_folder = path

    @property
    def folder(self) -> str:
        return self._temp_folder

    def build_temp_fpath(self, run: str, dataset: int, name: str) -> str:
        # returns basename_suffix.nxs as it will be saved as a nexus file
        # if the name includes dataset greater than 0 append it to the name
        dset = "_{}".format(dataset) if dataset > 0 else ""
        basename = os.path.basename(run).split(".")[0]
        tmp = os.path.join(self._temp_folder, basename + dset + name + ".nxs")
        return os.path.normpath(tmp)

    def restore_runs_from_scratch_folder(self, output_ws: str, runs: List[str], lopts: LoaderOptions) -> Tuple[List[str], str]:
        # look for a workspace that is a match or a subset
        # of the runs required with the same load options
        # returning the merged workspace and the files that are already loaded
        # if the merged workspace includes more runs it will need to be rebuilt

        # split the base name and the dataset index and looks for a file
        base_run, base_ix = split_run_index(runs[0])
        loaded, empty_ws = [], ""
        fpath = self.build_temp_fpath(base_run, base_ix, output_ws)
        if not os.path.isfile(fpath):
            return loaded, empty_ws

        # load the file and check if the required ini params and loader options match
        LoadNexusProcessed(Filename=fpath, OutputWorkspace=output_ws)
        mrun = mtd[output_ws].getRun()

        # followed by the load opts as the load can continue if the loaded merge is
        # a subset of the desired runs
        run_lopts = mrun.getProperty("loader_options").value
        dump_lopts = json.dumps(lopts, sort_keys=True)
        if run_lopts == dump_lopts:
            # build the list of loaded files and test if it is subset of the
            # required runs
            for prop in mrun.getProperties():
                if re.match(r"^merged_[0-9]+$", prop.name):
                    loaded.append(prop.value)

            def base_index(run):
                base, ix = split_run_index(run)
                return os.path.basename(base) + ":{}".format(ix)

            needed = [base_index(run) for run in runs]
            if set(loaded) <= set(needed):
                # if it is a subset it can continue by adding the
                # additional files
                return sorted(loaded), output_ws

        # if gets to here then match failed and it needs to reload
        # all the files as the events were all merged
        DeleteWorkspace(Workspace=output_ws)
        return [], empty_ws

    def copy_to_scratch_folder(self, output_ws: str, loaded: List[str], lopts: LoaderOptions) -> None:
        # add the lopts to the properties
        run = mtd[output_ws].getRun()
        run.addProperty("loader_options", json.dumps(lopts, sort_keys=True), True)

        # add the list of merged files that make up the work space
        # to the properties
        for ix, name in enumerate(loaded):
            run.addProperty("merged_" + str(ix), name, True)

        fpath = self.build_temp_fpath(loaded[0], 0, output_ws)
        SaveNexusProcessed(InputWorkspace=output_ws, Filename=fpath)

    def check_parameters_match(self, output_ws: str, load_opts: LoaderOptions, params: List[LoadLog]) -> bool:
        mrun = mtd[output_ws].getRun()
        for otag, rtag, tol in params:
            try:
                set_pm = load_opts[otag]
                try:
                    act_pm = mrun.getProperty(rtag).value[0]
                except TypeError:
                    act_pm = mrun.getProperty(rtag).value
                if isinstance(set_pm, bool):
                    if (act_pm != 0) != set_pm:
                        return False
                elif math.fabs(act_pm - set_pm) > tol:
                    return False
            except:
                raise RuntimeError("Cannot find property {}".format(rtag))
        return True

    def load_run_from_scratch(
        self,
        run: str,
        dataset: int,
        loader: Algorithm,
        lopts: LoaderOptions,
        output_ws: str,
        params: List[LoadLog],
        event_dirs: List[str],
        filter: Union[FilterPixelsTubes, None],
    ) -> bool:
        # looks for a nxs file file in the temp folder and either loads the nxs
        # file if it is present or loads the raw file and saves a nxs copy
        fpath = self.build_temp_fpath(run, dataset, "")
        nxs_ws = None
        if os.path.isfile(fpath):
            nxs_ws = "_nxs_tmp"
            LoadNexusProcessed(Filename=fpath, OutputWorkspace=nxs_ws)
            if params:
                if not self.check_parameters_match(nxs_ws, lopts, params):
                    DeleteWorkspace(Workspace=nxs_ws)
                    nxs_ws = None

        if nxs_ws is None:
            nxs_ws = "_nxs_tmp"
            full_opts = lopts.copy()
            try:
                del full_opts["SelectDetectorTubes"]
            except KeyError:
                pass
            full_opts["BinaryEventPath"] = find_event_folder(run, event_dirs)[0]
            loader(Filename=run, OutputWorkspace=nxs_ws, **full_opts)
            SaveNexusProcessed(InputWorkspace=nxs_ws, Filename=fpath, CompressNexus=False)

        # filter the workspace for the selected tubes and pixels
        if filter:
            filter.filter_workspace(nxs_ws, output_ws)
        else:
            RenameWorkspace(InputWorkspace=nxs_ws, OutputWorkspace=output_ws)
        return True


def load_merge(
    loader: Algorithm,
    runs: List[str],
    output_ws: str,
    lopts: LoaderOptions,
    event_dirs: List[str] = [],
    params: List[LoadLog] = [],
    filter: Union[FilterPixelsTubes, None] = None,
    scratch: Union[ScratchFolder, None] = None,
    check_file: Union[Callable[[str], None], None] = None,
    progress: Union[Progress, None] = None,
) -> None:
    """
    Loads and sums the event data.
    If a temp folder is provided it looks for an existing file in folder to
    reload the file if it matches the runs and loader options.

    The loader looks for the completely loaded or a proper subset that
    matches the loader option conditions. When loading individual files
    it also looks for a match .nxs file that matches the doppler phase
    value and then filters the events to the load options.
    If a new file is loaded it is loaded with filtering, saved as a .nxs
    file to the scratch folder and filtered appropriately.

    If no scratch folder is available it loads the files with the load
    options.

    """
    if len(runs) == 0:
        return None

    # if using a temp folder look for a matching workspace that is a subset
    # of the runs required and same load options returning the merged workspace
    # and the file that are already loaded
    loaded = []
    merged = ""
    updated = False
    min_start_time = DateAndTime(2**62)
    max_end_time = DateAndTime(0)
    if scratch is not None:
        loaded, merged = scratch.restore_runs_from_scratch_folder(output_ws, runs, lopts)

    for ix, esource in enumerate(runs):
        # esource contains dataset as an suffix fpath:n
        source, ds_index = split_run_index(esource)

        # if the file has been loaded as part of the temp load
        # update progress and skip to next

        basename = os.path.basename(source) + ":{}".format(ds_index)
        if basename in loaded:
            if progress is not None:
                progress.report("Loaded " + esource)
            continue

        # load the source file, set update true
        tmp_ws = "_src_" + str(ix)
        run_opts = lopts.copy()
        run_opts["SelectDataset"] = ds_index
        if scratch:
            scratch.load_run_from_scratch(source, ds_index, loader, run_opts, tmp_ws, params, event_dirs, filter)
        else:
            run_opts["BinaryEventPath"] = find_event_folder(source, event_dirs)[0]
            loader(Filename=source, OutputWorkspace=tmp_ws, **run_opts)
        if check_file:
            try:
                check_file(tmp_ws)
            except ValueError as e:
                raise RuntimeError("{} difference between {} and the sample file\n".format(str(e), source))
        if progress is not None:
            progress.report("Loaded " + esource)
        loaded.append(basename)
        updated = True

        if ix == 0:
            merged = tmp_ws
        else:
            # as the merged scan may not be continuous get the end for each file
            start_time, end_time = start_end_time(tmp_ws)
            min_start_time = min(min_start_time, start_time)
            max_end_time = max(max_end_time, end_time)

            # combined the events to the merged output and add the last filename to
            # the run log
            m_run = mtd[merged].getRun()
            m_run.addProperty("merged_" + str(ix), loaded[-1], True)
            tmp_merged = "__tmp_" + merged
            MergeRuns(InputWorkspaces=[merged, tmp_ws], OutputWorkspace=tmp_merged)

            DeleteWorkspace(Workspace=merged)
            DeleteWorkspace(Workspace=tmp_ws)
            RenameWorkspace(InputWorkspace=tmp_merged, OutputWorkspace=merged)

    # update the end time log information in the merged workspace
    # to match the total duration
    start_time, end_time = start_end_time(merged)
    min_start_time = min(min_start_time, start_time)
    max_end_time = max(max_end_time, end_time)
    m_run = mtd[merged].getRun()
    m_run.setStartAndEndTime(min_start_time, max_end_time)

    # update the title to reflect the data loaded
    title = encode_run_list(loaded)
    mtd[merged].setTitle(title)

    if merged != output_ws:
        RenameWorkspace(InputWorkspace=merged, OutputWorkspace=output_ws)

    if scratch is not None and updated:
        scratch.copy_to_scratch_folder(output_ws, loaded, lopts)

    if filter:
        filter.filter_workspace(output_ws, output_ws)


def integrate_over_peak(input_ws: str, output_ws: str, average_peak_width: bool) -> None:
    # performs an 3 sigma integration around a gaussian fitted peak
    iws = mtd[input_ws]
    nhist = iws.getNumberHistograms()

    # get the gaussian fit parameters per spectra
    epps = FindEPP(iws)
    lo_vals = np.empty(nhist)
    hi_vals = np.empty(nhist)
    for i in range(nhist):
        peak = epps.cell("PeakCentre", i)
        sigma = epps.cell("Sigma", i)
        lo_vals[i] = peak - 3 * sigma
        hi_vals[i] = peak + 3 * sigma
    DeleteWorkspace(Workspace=epps)

    if average_peak_width:
        lo = lo_vals[np.nonzero(lo_vals)].mean()
        hi = hi_vals[np.nonzero(hi_vals)].mean()
        Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws, RangeLower=lo, RangeUpper=hi)
    else:
        Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws, RangeLowerList=lo_vals, RangeUpperList=hi_vals)


class IniParameters(ConfigParser):
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        ConfigParser.__init__(self, *args, **kwargs)

    def get_param(self, ftype: Callable[[str], T], section: str, option: str, default: T) -> T:
        try:
            value = ftype(self.get(section, option))
        except (NoOptionError, NoSectionError):
            value = default
        return value

    def get_bool(self, section: str, option: str, default: bool) -> bool:
        try:
            value = self.getboolean(section, option)
        except (NoOptionError, NoSectionError):
            value = default
        return value

    def get_section(self, tag: str) -> Dict[str, str]:
        try:
            options = dict(self.items(tag))
        except (NoOptionError, NoSectionError):
            options = {}
        return options


# fractional area support code


class Point:
    # point in 3D space
    x: float
    y: float
    z: float

    def __init__(self, x_: float, y_: float, z_: float) -> None:
        self.x = x_
        self.y = y_
        self.z = z_


def triangle_base_area(p1: Point, p2: Point, p3: Point) -> float:
    # calculates the area of a triangle projected onto the xy plane
    calc = p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)
    return 0.5 * math.fabs(calc)


def find_intercept(p1: Point, p2: Point, qlevel: float) -> Point:
    # parametrise the ray from p1 to p2 by t : 0 -> 1 so
    #   p = p1 + (p2 - p1) * t and determine t by solving
    #   qlevel = p1.z + (p2.z - p1.z) * t
    # the input is assumed to be filtered such that p2.z != p1.z
    t = (qlevel - p1.z) / (p2.z - p1.z)
    x = p1.x + (p2.x - p1.x) * t
    y = p1.y + (p2.y - p1.y) * t
    return Point(x, y, qlevel)


def fractional_facet_area(p1: Point, p2: Point, p3: Point, qlevel: float) -> float:
    # calculates the fractional area of the facet projected onto the xy plane for
    # the region that is below qlevel

    # handle the simple cases first:
    # if qlevel <  min(zvals) return 0
    # if qlevel >= max(zvals) return 1
    # else find intercept points and get the fractional area
    pts = sorted([p1, p2, p3], key=lambda p: p.z)
    if qlevel < pts[0].z:
        return 0.0
    if qlevel >= pts[2].z:
        return triangle_base_area(p1, p2, p3)

    if pts[1].z <= qlevel:
        # the area above qlevel is a simple triangle with max pt as one point
        ip1 = find_intercept(pts[1], pts[2], qlevel)
        ip2 = find_intercept(pts[0], pts[2], qlevel)
        return triangle_base_area(p1, p2, p3) - triangle_base_area(pts[2], ip1, ip2)
    else:
        # the area below qlevel is a simple triangle with min pt as one point
        ip1 = find_intercept(pts[0], pts[1], qlevel)
        ip2 = find_intercept(pts[0], pts[2], qlevel)
        return triangle_base_area(pts[0], ip1, ip2)


def fractional_quad_area(zvals: Tuple[float, float, float, float, float], qlevel: float) -> float:
    # calculates the area of the region below qlevel over the unit square defined
    # by [(0,0),(1,0),(1,1),(0,1)] and the centre point at (0.5, 0.5) by using
    # triangular facets
    p0 = Point(0, 0, zvals[0])
    p1 = Point(1, 0, zvals[1])
    pc = Point(0.5, 0.5, zvals[4])
    area0 = fractional_facet_area(p0, p1, pc, qlevel)
    p2 = Point(1, 1, zvals[2])
    area1 = fractional_facet_area(p1, p2, pc, qlevel)
    p3 = Point(0, 1, zvals[3])
    area2 = fractional_facet_area(p2, p3, pc, qlevel)
    area3 = fractional_facet_area(p3, p0, pc, qlevel)

    return area0 + area1 + area2 + area3


def extrapolate_grid_edges(grid):
    """Simple linear extrapolation

    Using the difference along each edge and vertices (dx+dy).
    """
    m, n = grid.shape
    grid_ = np.zeros((m + 2, n + 2), dtype=np.float64)
    dgx = np.diff(grid, axis=1)
    dgy = np.diff(grid, axis=0)
    grid_[1:-1, 0] = grid[:, 0] - dgx[:, 0]
    grid_[1:-1, -1] = grid[:, -1] + dgx[:, -1]
    grid_[0, 1:-1] = grid[0, :] - dgy[0, :]
    grid_[-1, 1:-1] = grid[-1, :] + dgy[-1, :]

    grid_[0, 0] = grid[0, 0] - dgx[0, 0] - dgy[0, 0]
    grid_[0, -1] = grid[0, -1] + dgx[0, -1] - dgy[0, -1]
    grid_[-1, -1] = grid[-1, -1] + dgx[-1, -1] + dgy[-1, -1]
    grid_[-1, 0] = grid[-1, 0] - dgx[-1, 0] + dgy[-1, 0]

    grid_[1:-1, 1:-1] = grid[:, :]

    return grid_


def fractional_map(
    grids: List[NDArray], base_pixels: List[int], bin_edges: List[float]
) -> Tuple[Dict[int, List[int]], Dict[int, List[float]]]:
    # Assumes a collection of 2D panels with base pixel offset.
    # The bands are monotonically increasing.
    pixel_map = {i: [] for i in range(len(bin_edges) - 1)}
    pixel_wgts = {i: [] for i in range(len(bin_edges) - 1)}

    for grid, base_pixel in zip(grids, base_pixels):
        # Note the mapping from pixel to theta, q, q**2 is a smooth function
        # it is reasonable to use a cubic interpolation of the map.
        # However, the current 2D spline implementaions from scipy only provide
        # nearest neighbour extrapolation. To handle the edge points the extrapolation
        # is handled manually before the

        egrid = extrapolate_grid_edges(grid)
        m, n = grid.shape
        x = np.arange(-1, m + 1, 1)
        y = np.arange(-1, n + 1, 1)
        fn = RectBivariateSpline(x, y, egrid)
        xnew = np.arange(-0.5, m + 1, 1)
        ynew = np.arange(-0.5, n + 1, 1)
        fgrid = fn(xnew, ynew)

        area_values = np.zeros(len(bin_edges), dtype=np.float64)
        for i in range(m):
            for j in range(n):
                det_id = i * n + j + base_pixel
                zvalues = (fgrid[i, j], fgrid[i + 1, j], fgrid[i + 1, j + 1], fgrid[i, j + 1], grid[i, j])
                qmin = np.min(zvalues)
                qmax = np.max(zvalues)
                for k, level in enumerate(bin_edges):
                    if level < qmin:
                        area_values[k] = 0.0
                        continue
                    elif level >= qmax:
                        area_values[k] = 1.0
                    else:
                        area_values[k] = fractional_quad_area(zvalues, level)
                bin_area = np.diff(area_values)
                for k, area in enumerate(bin_area):
                    if area > 0:
                        pixel_map[k].append(det_id)
                        pixel_wgts[k].append(area)

    return pixel_map, pixel_wgts


class FractionalAreaDetectorTubes:
    _pixels_per_tube: int
    _2theta_range: str
    _q_range: str
    _q2_range: str
    _def_y_bins: int
    _efixed: float
    _analyse_tubes: str
    _spectrum_axis: str
    _panel_tubes: List[Tuple[int, int]]
    _y_axis: Dict[str, Any] = {}
    _fractional_map: Dict[int, List[int]] = {}
    _fractional_wgts: Dict[int, List[float]] = {}

    def __init__(
        self,
        pixels_per_tube: int,
        analyse_tubes: str,
        spectrum_axis: str,
        two_theta_range: str,
        q_range: str,
        q2_range: str,
        efixed: float,
        panel_tubes: List[Tuple[int, int]],
        def_bins: int = 30,
    ) -> None:
        self._pixels_per_tube = pixels_per_tube
        self._2theta_range = two_theta_range
        self._q_range = q_range
        self._q2_range = q2_range
        self._def_y_bins = def_bins
        self._efixed = efixed
        self._analyse_tubes = analyse_tubes
        self._spectrum_axis = spectrum_axis
        self._panel_tubes = panel_tubes

    @property
    def y_axis(self) -> Dict[str, Any]:
        return self._y_axis

    @property
    def detector_map(self) -> Dict[int, List[int]]:
        return self._fractional_map

    @property
    def detector_wgts(self) -> Dict[int, List[float]]:
        return self._fractional_wgts

    def _get_axis_bins(self) -> Tuple[NDArray, float]:
        y_axis = self._y_axis["values"]
        bin_edges = np.zeros(len(y_axis) + 1)
        step_size = np.mean(np.diff(y_axis))
        bin_edges[1:] = y_axis + 0.5 * step_size
        bin_edges[0] = y_axis[0] - 0.5 * step_size
        return bin_edges, step_size

    def build_detector_map(self, input_ws: str, spectrum_axis: str) -> None:
        # assumes that ws spectrum axis has been converted
        # except for tube number which is treated differently
        if spectrum_axis == "TubeNumber":
            return self._build_tube_map()

        # this is a non fractional mapping
        #
        # map the detectors to the spectra group that are in the bins
        axis = mtd[input_ws].getAxis(1)
        detector_values = axis.extractValues()
        bin_edges, step_size = self._get_axis_bins()
        nbins = len(bin_edges) - 1
        min_value = bin_edges[0]

        self._fractional_map = dict([(i, []) for i in range(nbins)])
        self._fractional_wgts = dict([(i, []) for i in range(nbins)])
        for id, value in enumerate(detector_values):
            ix = int((value - min_value) // step_size)
            if ix >= 0 and ix < nbins:
                self._fractional_map[ix].append(id)
                self._fractional_wgts[ix].append(1.0)

    def build_fractional_map(self, input_ws: str, spectrum_axis: str) -> None:
        # assumes that ws spectrum axis has been converted
        # except for tube number which is treated differently
        if spectrum_axis == "TubeNumber":
            return self._build_tube_map()

        # get the bin values from the axis
        bin_edges, _ = self._get_axis_bins()

        # convert detector t spec value map to 2D grid
        # grid = self._det_spec_value.reshape(-1, self._pixels_per_tube)
        axis = mtd[input_ws].getAxis(1)
        grid = axis.extractValues().reshape(-1, self._pixels_per_tube)

        grids, base_pixels = [], []
        for lo, hi in self._panel_tubes:
            grids.append(grid[lo:hi, :])
            base_pixels.append(lo * self._pixels_per_tube)

        self._fractional_map, self._fractional_wgts = fractional_map(grids, base_pixels, bin_edges)

    def apply_fractional_grouping(self, input_ws: str, output_ws: str) -> None:
        # get the fractional contributions to Y and E values from the input
        iws = mtd[input_ws]
        inputX = iws.extractX()
        inputY = iws.extractY()
        inputE = iws.extractE()
        nspec = len(self._fractional_map)
        outputX = inputX[:nspec]
        outputY = np.zeros((nspec, inputY.shape[1]))
        outputE = np.zeros_like(outputY)
        for spec_id, detectors in self._fractional_map.items():
            fractions = self._fractional_wgts[spec_id]
            for id, fa in zip(detectors, fractions):
                outputY[spec_id, :] += fa * inputY[id, :]
                outputE[spec_id, :] += fa * inputE[id, :] * inputE[id, :]
            outputE[spec_id, :] = np.sqrt(outputE[spec_id, :])

        # create a workspace with the new grouping and copy the x axis
        # units to ensure it can be divided
        tmp_ws = "_tmp" + output_ws
        CreateWorkspace(
            OutputWorkspace=tmp_ws,
            DataX=outputX,
            DataY=outputY,
            DataE=outputE,
            NSpec=nspec,
            Distribution=iws.isDistribution(),
            WorkspaceTitle=iws.getTitle(),
            ParentWorkspace=input_ws,
        )
        axis = iws.getAxis(0)
        ows = mtd[tmp_ws]
        ows.replaceAxis(0, axis)

        # map the detector ids to the spectra
        for spec_id, detectors in self._fractional_map.items():
            isp = ows.getSpectrum(spec_id)
            isp.clearDetectorIDs()
            for id in detectors:
                isp.addDetectorID(id)

        RenameWorkspace(InputWorkspace=tmp_ws, OutputWorkspace=output_ws)

    def _convert_spectrum(self, input_ws: str, output_ws: str) -> None:
        # converts the detector histograms to spectrum values and defines the y_axis values
        # noting that TubeNumber is treated differently
        if self._spectrum_axis == "TubeNumber":
            # do nothing handled when building the
            return

        target = None
        if self._spectrum_axis == "2Theta":
            target = "Theta"
            use_bins = self._2theta_range
            unitID = "Degrees"
        elif self._spectrum_axis == "Q":
            target = "ElasticQ"
            use_bins = self._q_range
            unitID = "MomentumTransfer"
        elif self._spectrum_axis == "Q2":
            target = "ElasticQSquared"
            use_bins = self._q2_range
            unitID = "QSquared"
        else:
            raise RuntimeError("Unknown target spectrum {}".format(self._spectrum_axis))

        ConvertSpectrumAxis(
            InputWorkspace=input_ws, OutputWorkspace=output_ws, EMode="Indirect", Target=target, EFixed=self._efixed, OrderAxis=False
        )

        # determine the bins to be used
        if use_bins == "auto":
            axis = mtd[output_ws].getAxis(1)
            spectrum = axis.extractValues()
            min_val, max_val = np.min(spectrum), np.max(spectrum)
            axis_values = np.linspace(min_val, max_val, num=self._def_y_bins)
        else:
            min_val, step_val, max_val = range_to_values(use_bins)
            nbins = int(math.ceil((max_val - min_val) / step_val))
            max_val = min_val + nbins * step_val
            axis_values = np.linspace(min_val, max_val, num=nbins + 1)

        # add the axis values, note that the axis labels are defined by the unitID
        self._y_axis = {"values": axis_values, "unitID": unitID}

    def _build_tube_map(self) -> None:
        # map the detector id a tube and pixel and include
        # if it is in the range selected tubes and pixel range
        anl_tubes = sorted(seq_to_list(self._analyse_tubes))
        self._fractional_map = {}
        for ix, tube in enumerate(anl_tubes):
            lo = tube * self._pixels_per_tube
            hi = lo + self._pixels_per_tube
            self._fractional_map[ix] = [i for i in range(lo, hi)]

        self._fractional_wgts = {}
        for k, v in self._fractional_map.items():
            self._fractional_wgts[k] = [1.0] * len(v)

        self._y_axis = {"values": anl_tubes, "label": "TubeNumber", "units": "", "unitID": "label"}


# end fractional area code
