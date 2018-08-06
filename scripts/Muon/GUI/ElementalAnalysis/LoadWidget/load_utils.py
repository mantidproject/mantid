import glob
import os

from six import iteritems

from mantid import config
import mantid.simpleapi as mantid

type_keys = {"10": "Prompt", "20": "Delayed", "99": "Total"}


def pad_run(run):
    """ Pads run number: i.e. 123 -> 00123; 2695- > 02695 """
    return str(run).zfill(5)


def search_user_dirs(run):
    files = []
    for user_dir in config["datasearch.directories"].split(";"):
        path = os.path.join(user_dir,
                            "ral{}.rooth*.dat".format(pad_run(run)))
        for g in glob.iglob(path):
            files.append(g)
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
    return list(detectors)


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


def flatten_run_data(workspace):
    detectors = [mantid.mtd[d] for d in workspace]
    return sorted([w.getName() for d in detectors for w in d])


def hyphenise(vals):
    vals = sorted(list(set(vals)))
    pos, out = 0, []
    l = len(vals)
    for i in range(1, l):
        last = (i == l - 1)
        if vals[i] - vals[i - 1] != 1:
            if i == 1:
                out.append(str(vals[pos]))
            else:
                out.append("{}-{}".format(vals[pos], vals[i - 1]))
            if last:
                out.append(str(vals[i]))
            pos = i
        elif last:
            out.append("{}-{}".format(vals[pos], vals[i]))
    return ", ".join(out)
