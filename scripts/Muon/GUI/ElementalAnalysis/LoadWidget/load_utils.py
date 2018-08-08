import glob
import os

from six import iteritems

from mantid import config
import mantid.simpleapi as mantid

type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}


class LModel(object):
    def __init__(self):
        self.run = 0
        self.loaded_runs = {}
        self.alg = None

    def _load(self, inputs):
        """ inputs is a dict mapping filepaths to output names """
        if self.alg is not None:
            raise RuntimeError("Loading already in progress")
        self.alg = mantid.AlgorithmManager.create("LoadAscii")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        for path, output in iteritems(inputs):
            self.alg.setProperty("Filename", path)
            self.alg.setProperty("OutputWorkspace", output)
            self.alg.execute()
            mantid.AnalysisDataService.addOrReplace(
                output, self.alg.getProperty("OutputWorkspace").value)
        self.alg = None

    def load_run(self):
        to_load = search_user_dirs(self.run)
        if not to_load:
            return None
        workspaces = {f: get_filename(
            f) for f in to_load if get_filename(f) is not None}
        self._load(workspaces)
        self.loaded_runs[self.run] = group_by_detector(
            self.run, workspaces.values())
        return self.loaded_runs[self.run]

    def output(self):
        return

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def loadData(self, inputs):
        return

    def set_run(self, run):
        self.run = run


def pad_run(run):
    """ Pads run number: i.e. 123 -> 00123; 2695- > 02695 """
    return str(run).zfill(5)


def search_user_dirs(run):
    files = []
    for user_dir in config["datasearch.directories"].split(";"):
        path = os.path.join(user_dir,
                            "ral{}.rooth*.dat".format(pad_run(run)))
        files.extend([g for g in glob.iglob(path)])
    return files


def group_by_detector(run, workspaces):
    """ where workspaces is a tuple of form:
            (filepath, ws name)
    """
    d_string = "{}; Detector {}"
    detectors = {d_string.format(run, x): [] for x in range(1, 5)}
    for w in workspaces:
        detector_number = get_detector_num_from_ws(w)
        detectors[d_string.format(run, detector_number)].append(w)
    for d, v in iteritems(detectors):
        mantid.GroupWorkspaces(v, OutputWorkspace=str(d))
    detector_list = sorted(list(detectors))
    group_grouped_workspaces(run, detector_list)
    return detector_list


def group_grouped_workspaces(name, workspaces):
    tmp = mantid.CreateSampleWorkspace()
    overall = mantid.GroupWorkspaces(tmp, OutputWorkspace=str(name))
    for w in workspaces:
        overall.add(w)
    mantid.AnalysisDataService.remove("tmp")


def get_detector_num_from_ws(name):
    return int(name[0])


def get_detectors_num(path):
    return int(path.rsplit(".", 2)[1][5]) - 1


def get_end_num(path):
    return path.rsplit(".")[1][-9:]


def get_run_type(path):
    return type_keys[path.rsplit(".")[1][-2:]]


def get_filename(path):
    try:
        return "{}_{}_{}".format(get_detectors_num(
            path), get_run_type(path), get_end_num(path))
    except KeyError:
        return None


def flatten_run_data(*workspaces):
    out = []
    for ws in workspaces:
        detectors = [mantid.mtd[d] for d in ws]
        out.append(sorted([w.getName() for d in detectors for w in d]))
    return out


def hyphenise(vals):
    if not len(vals):
        return ""
    vals = [str(s) for s in sorted(list(set(vals)))]
    diffs = [int(vals[i + 1]) - int(vals[i]) for i in range(len(vals) - 1)]
    a = b = vals[0]
    out = []
    for i, d in enumerate(diffs):
        if d != 1:
            out.append("-".join([a, b]) if a != b else a)
            a = vals[i + 1]
        b = vals[i + 1]
        if i == len(diffs) - 1:
            out.append("-".join([a, b]) if a != b else vals[i + 1])
    return ", ".join(out)
