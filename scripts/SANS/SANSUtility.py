# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines
# pylint: disable=invalid-name
#########################################################
# This module contains utility functions common to the
# SANS data reduction scripts
########################################################
from mantid.api import mtd, AlgorithmManager, IEventWorkspace, MatrixWorkspace, WorkspaceGroup, FileLoaderRegistry, FileFinder
from mantid.kernel import config, DateAndTime, Logger
from mantid.simpleapi import (
    CalculateFlatBackground,
    ChangeTimeZero,
    CloneWorkspace,
    ConjoinWorkspaces,
    ConvertUnits,
    CopyInstrumentParameters,
    CropWorkspace,
    DeleteWorkspace,
    ExtractSpectra,
    FilterByTime,
    GroupWorkspaces,
    InterpolatingRebin,
    Load,
    LoadMask,
    LoadNexusMonitors,
    MaskBins,
    MaskDetectors,
    MaskDetectorsInShape,
    Plus,
    RawFileInfo,
    Rebin,
    RebinToWorkspace,
    RemoveBins,
    RenameWorkspace,
    SaveNexusProcessed,
    UnGroupWorkspace,
)

import inspect
import math
import os
import re
import types
import numpy as np
import h5py as h5

sanslog = Logger("SANS")
ADDED_TAG = "-add"
ADDED_EVENT_DATA_TAG = "_added_event_data"
ADD_TAG = "-add"
ADD_MONITORS_TAG = "-add_monitors"
REG_DATA_NAME = ADD_TAG + ADDED_EVENT_DATA_TAG + "[_1-9]*$"
REG_DATA_MONITORS_NAME = ADD_MONITORS_TAG + ADDED_EVENT_DATA_TAG + "[_1-9]*$"
ZERO_ERROR_DEFAULT = 1e6
INCIDENT_MONITOR_TAG = "_incident_monitor"
MANTID_PROCESSED_WORKSPACE_TAG = "Mantid Processed Workspace"


def deprecated(obj):
    """
    Decorator to apply to functions or classes that we think are not being (or
    should not be) used anymore.  Prints a warning to the log.
    """
    if inspect.isfunction(obj) or inspect.ismethod(obj):
        if inspect.isfunction(obj):
            obj_desc = '"%s" function' % obj.__name__
        else:
            obj_desc = '"%s" class' % obj.__self__.__class__.__name__

        def print_warning_wrapper(*args, **kwargs):
            sanslog.warning(
                "The %s has been marked as deprecated and may be "
                "removed in a future version of Mantid.  If you "
                "believe this to have been marked in error, please "
                "contact the member of the Mantid team responsible "
                "for ISIS SANS." % obj_desc
            )
            return obj(*args, **kwargs)

        return print_warning_wrapper

    # Add a @deprecated decorator to each of the member functions in the class
    # (by recursion).
    if inspect.isclass(obj):
        for name, fn in inspect.getmembers(obj):
            if isinstance(fn, types.MethodType):
                setattr(obj, name, deprecated(fn))
        return obj

    assert False, (
        "Programming error.  You have incorrectly applied the @deprecated decorator.  This is only for use with functions or classes."
    )


def GetInstrumentDetails(instrum):
    """
    Return the details specific to the instrument's current detector bank
    @return number of pixels ac, first spectrum in the current detector, its last spectrum
    """
    det = instrum.cur_detector()
    # LOQ HAB is not a square detector and so has no width
    # for backwards compatibility we have to return a width
    if instrum.name() == "LOQ" and instrum.cur_detector().name() == "HAB":
        if det.n_columns is None:
            return 128, det.get_first_spec_num(), det.last_spec_num

    first_spectrum = det.get_first_spec_num()
    last_spectrum = det.last_spec_num
    if instrum.name() == "SANS2D":
        first_spectrum += 4
        last_spectrum += 4

    return det.n_columns, first_spectrum, last_spectrum


def InfinitePlaneXML(id_name, plane_pt, normal_pt):
    return (
        '<infinite-plane id="'
        + str(id_name)
        + '">'
        + '<point-in-plane x="'
        + str(plane_pt[0])
        + '" y="'
        + str(plane_pt[1])
        + '" z="'
        + str(plane_pt[2])
        + '" />'
        + '<normal-to-plane x="'
        + str(normal_pt[0])
        + '" y="'
        + str(normal_pt[1])
        + '" z="'
        + str(normal_pt[2])
        + '" />'
        + "</infinite-plane>"
    )


def InfiniteCylinderXML(id_name, centre, radius, axis):
    return (
        '<infinite-cylinder id="'
        + str(id_name)
        + '">'
        + '<centre x="'
        + str(centre[0])
        + '" y="'
        + str(centre[1])
        + '" z="'
        + str(centre[2])
        + '" />'
        + '<axis x="'
        + str(axis[0])
        + '" y="'
        + str(axis[1])
        + '" z="'
        + str(axis[2])
        + '" />'
        + '<radius val="'
        + str(radius)
        + '" />'
        + "</infinite-cylinder>\n"
    )


# Mask a cylinder, specifying the algebra to use


def MaskWithCylinder(workspace, radius, xcentre, ycentre, algebra):
    """Mask a cylinder on the input workspace."""
    xmldef = InfiniteCylinderXML("shape", [xcentre, ycentre, 0.0], radius, [0, 0, 1])
    xmldef += '<algebra val="' + algebra + 'shape" />'
    # Apply masking
    MaskDetectorsInShape(Workspace=workspace, ShapeXML=xmldef)


# Mask such that the remainder is that specified by the phi range


def LimitPhi(workspace, centre, phimin, phimax, use_mirror=True):
    # convert all angles to be between 0 and 360
    while phimax > 360:
        phimax -= 360
    while phimax < 0:
        phimax += 360
    while phimin > 360:
        phimin -= 360
    while phimin < 0:
        phimin += 360
    while phimax < phimin:
        phimax += 360

    # Convert to radians
    phimin = math.pi * phimin / 180.0
    phimax = math.pi * phimax / 180.0
    xmldef = (
        InfinitePlaneXML("pla", centre, [math.cos(-phimin + math.pi / 2.0), math.sin(-phimin + math.pi / 2.0), 0])
        + InfinitePlaneXML("pla2", centre, [-math.cos(-phimax + math.pi / 2.0), -math.sin(-phimax + math.pi / 2.0), 0])
        + InfinitePlaneXML("pla3", centre, [math.cos(-phimax + math.pi / 2.0), math.sin(-phimax + math.pi / 2.0), 0])
        + InfinitePlaneXML("pla4", centre, [-math.cos(-phimin + math.pi / 2.0), -math.sin(-phimin + math.pi / 2.0), 0])
    )

    if use_mirror:
        xmldef += '<algebra val="#((pla pla2):(pla3 pla4))" />'
    else:
        # the formula is different for acute verses obstruse angles
        if phimax - phimin > math.pi:
            # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
            xmldef += '<algebra val="#(pla:pla2)" />'
        else:
            # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
            xmldef += '<algebra val="#(pla pla2)" />'

    MaskDetectorsInShape(Workspace=workspace, ShapeXML=xmldef)


# Work out the spectra IDs for block of detectors


def spectrumBlock(base, ylow, xlow, ydim, xdim, det_dimension, orientation):
    """Compile a list of spectrum Nos for rectangular block of size xdim by ydim"""
    output = ""
    if orientation == Orientation.Horizontal:
        start_spec = base + ylow * det_dimension + xlow
        for y in range(0, ydim):
            for x in range(0, xdim):
                output += str(start_spec + x + (y * det_dimension)) + ","
    elif orientation == Orientation.Vertical:
        start_spec = base + xlow * det_dimension + ylow
        for x in range(det_dimension - 1, det_dimension - xdim - 1, -1):
            for y in range(0, ydim):
                std_i = start_spec + y + ((det_dimension - x - 1) * det_dimension)
                output += str(std_i) + ","
    elif orientation == Orientation.Rotated:
        # This is the horizontal one rotated so need to map the xlow and vlow to their rotated versions
        start_spec = base + ylow * det_dimension + xlow
        max_spec = det_dimension * det_dimension + base - 1
        for y in range(0, ydim):
            for x in range(0, xdim):
                std_i = start_spec + x + (y * det_dimension)
                output += str(max_spec - (std_i - base)) + ","

    return output.rstrip(",")


# Mask by bin range


def MaskByBinRange(workspace, timemask):
    # timemask should be a ';' separated list of start/end values
    ranges = timemask.split(";")
    for r in ranges:
        limits = r.split()
        if len(limits) == 2:
            MaskBins(InputWorkspace=workspace, OutputWorkspace=workspace, XMin=limits[0], XMax=limits[1])


def QuadrantXML(centre, rmin, rmax, quadrant):
    cin_id = "cyl-in"
    xmlstring = InfiniteCylinderXML(cin_id, centre, rmin, [0, 0, 1])
    cout_id = "cyl-out"
    xmlstring += InfiniteCylinderXML(cout_id, centre, rmax, [0, 0, 1])
    plane1Axis = None
    plane2Axis = None
    if quadrant == "Left":
        plane1Axis = [-1, 1, 0]
        plane2Axis = [-1, -1, 0]
    elif quadrant == "Right":
        plane1Axis = [1, -1, 0]
        plane2Axis = [1, 1, 0]
    elif quadrant == "Up":
        plane1Axis = [1, 1, 0]
        plane2Axis = [-1, 1, 0]
    elif quadrant == "Down":
        plane1Axis = [-1, -1, 0]
        plane2Axis = [1, -1, 0]
    else:
        return ""
    p1id = "pl-a"
    xmlstring += InfinitePlaneXML(p1id, centre, plane1Axis)
    p2id = "pl-b"
    xmlstring += InfinitePlaneXML(p2id, centre, plane2Axis)

    # The composition of the shape is "(cyl-out (#cyl-in) (pl-a:pl-b))". The breakdown is:
    # 1. Create an infinite hollow cylinder by performing "cyl-out (#cyl-in)". This is the intersection of the
    #    outer radius cylinder with the inverse inner radius cylinder. We have a shell-like selection
    # 2. Create a three-quarter wedge selection by performing (pl-a:pl-b). This selects everything except
    #    for the slice region we don't want to be masked.
    # 3. Create the intersection between 1 and 2. This will provide a three-quarter wedge of the hollow
    #    cylinder.
    xmlstring += '<algebra val="(#(' + cout_id + " (#" + cin_id + ")) : (" + p1id + ":" + p2id + '))"/>\n'
    return xmlstring


def getWorkspaceReference(ws_pointer):
    if isinstance(ws_pointer, str):
        ws_pointer = mtd[ws_pointer]
    if str(ws_pointer) not in mtd:
        raise RuntimeError("Invalid workspace name input: " + str(ws_pointer))
    return ws_pointer


def isEventWorkspace(ws_reference):
    return isinstance(getWorkspaceReference(ws_reference), IEventWorkspace)


def getBinsBoundariesFromWorkspace(ws_reference):
    ws_reference = getWorkspaceReference(ws_reference)
    Xvalues = ws_reference.dataX(0)
    binning = str(Xvalues[0])
    binGap = Xvalues[1] - Xvalues[0]
    binning = binning + "," + str(binGap)
    for j in range(2, len(Xvalues)):
        nextBinGap = Xvalues[j] - Xvalues[j - 1]
        if nextBinGap != binGap:
            binGap = nextBinGap
            binning = binning + "," + str(Xvalues[j - 1]) + "," + str(binGap)
    binning = binning + "," + str(Xvalues[-1])
    return binning


def getFilePathFromWorkspace(ws):
    ws_pointer = getWorkspaceReference(ws)
    if isinstance(ws_pointer, WorkspaceGroup):
        ws_pointer = ws_pointer[0]
    file_path = None

    try:
        for hist in ws_pointer.getHistory():
            try:
                if "Load" in hist.name():
                    file_path = hist.getPropertyValue("Filename")
            # pylint: disable=bare-except
            except:
                pass
    except:
        try:
            hist = ws_pointer.getHistory().lastAlgorithm()
            file_path = hist.getPropertyValue("Filename")
        except:
            raise RuntimeError("Failed while looking for file in workspace: " + str(ws))

    if not file_path:
        raise RuntimeError("Can not find the file name for workspace " + str(ws))
    return file_path


def fromEvent2Histogram(ws_event, ws_monitor, binning=""):
    """Transform an event mode workspace into a histogram workspace.
    It does conjoin the monitor and the workspace as it is expected from the current
    SANS data inside ISIS.

    A non-empty binning string will specify a rebin param list to use instead of using
    the binning of the monitor ws.

    Finally, it copies the parameter map from the workspace to the resulting histogram
    in order to preserve the positions of the detectors components inside the workspace.

    It will finally, replace the input workspace with the histogram equivalent workspace.
    """
    assert ws_monitor is not None

    name = "__monitor_tmp"

    if binning != "":
        aux_hist = Rebin(ws_event, binning, False)
        Rebin(InputWorkspace=ws_monitor, Params=binning, PreserveEvents=False, OutputWorkspace=name)
    else:
        aux_hist = RebinToWorkspace(WorkspaceToRebin=ws_event, WorkspaceToMatch=ws_monitor, PreserveEvents=False)
        ws_monitor.clone(OutputWorkspace=name)

    ConjoinWorkspaces(name, aux_hist, CheckOverlapping=True, CheckMatchingBins=False)
    CopyInstrumentParameters(ws_event, OutputWorkspace=name)

    ws_hist = RenameWorkspace(name, OutputWorkspace=str(ws_event))

    return ws_hist


def getChargeAndTime(ws_event):
    r = ws_event.getRun()
    charges = r.getLogData("proton_charge")
    total_charge = sum(charges.value)
    time_passed = (charges.times[-1] - charges.times[0]) / np.timedelta64(1, "s")
    return total_charge, time_passed


def sliceByTimeWs(ws_event, time_start=None, time_stop=None):
    def formatTime(time_val):
        return "_T%.1f" % time_val

    params = dict()
    outname = str(ws_event)
    if time_start:
        outname += formatTime(time_start)
        params["StartTime"] = time_start
    if time_stop:
        outname += formatTime(time_stop)
        params["StopTime"] = time_stop

    params["OutputWorkspace"] = outname
    sliced_ws = FilterByTime(ws_event, **params)
    return sliced_ws


def slice2histogram(ws_event, time_start, time_stop, monitor, binning=""):
    """Return the histogram of the sliced event and a tuple with the following:
    - total time of the experiment
    - total charge
    - time of sliced data
    - charge of sliced data
    @param ws_event pointer to the event workspace
    @param time_start: the minimum value to filter. Pass -1 to get the minimum available
    @param time_stop: the maximum value to filter. Pass -1 to get the maximum available
    @param monitor: pointer to the monitor workspace
    @param binning: optional binning string to use instead of the binning from the monitor
    """
    if not isEventWorkspace(ws_event):
        raise RuntimeError("The workspace " + str(ws_event) + " is not a valid Event workspace")

    tot_c, tot_t = getChargeAndTime(ws_event)

    if (time_start == -1) and (time_stop == -1):
        hist = fromEvent2Histogram(ws_event, monitor, binning)
        return hist, (tot_t, tot_c, tot_t, tot_c)

    if time_start == -1:
        time_start = 0.0
    if time_stop == -1:
        time_stop = tot_t + 0.001

    sliced_ws = sliceByTimeWs(ws_event, time_start, time_stop)
    sliced_ws = RenameWorkspace(sliced_ws, OutputWorkspace=ws_event.name())

    part_c, part_t = getChargeAndTime(sliced_ws)
    scaled_monitor = monitor * (part_c / tot_c)

    hist = fromEvent2Histogram(sliced_ws, scaled_monitor, binning)
    return hist, (tot_t, tot_c, part_t, part_c)


def sliceParser(str_to_parser):  # noqa: C901
    """
    Create a list of boundaries from a string defing the slices.
    Valid syntax is:
      * From 8 to 9 > '8-9' --> return [[8,9]]
      * From 8 to 9 and from 10 to 12 > '8-9, 10-12' --> return [[8,9],[10,12]]
      * From 5 to 10 in steps of 1 > '5:1:10' --> return [[5,6],[6,7],[7,8],[8,9],[9,10]]
      * From 5 > '>5' --> return [[5,-1]]
      * Till 5 > '<5' --> return [[-1,5]]

    Any combination of these syntax separated by comma is valid.
    A special mark is used to signalize no limit: -1,
    As, so, for an empty string, it will return: [[-1, -1]].

    It does not accept negative values.

    """
    num_pat = r"(\d+(?:\.\d+)?(?:[eE][+-]\d+)?)"  # float without sign
    slice_pat = num_pat + r"-" + num_pat
    lowbound = ">" + num_pat
    upbound = "<" + num_pat
    sss_pat = num_pat + r":" + num_pat + r":" + num_pat
    exception_pattern = "Invalid input for Slicer: %s"
    MARK = -1

    def _check_match(inpstr, patternstr, qtde_nums):
        match = re.match(patternstr, inpstr)
        if match:
            answer = match.groups()
            if len(answer) != qtde_nums:
                raise SyntaxError(exception_pattern % (inpstr))
            return [float(answer[i]) for i in range(qtde_nums)]
        else:
            return False

    def _parse_slice(inpstr):
        return _check_match(inpstr, slice_pat, 2)

    def _parse_lower(inpstr):
        val = _check_match(inpstr, lowbound, 1)
        if not val:
            return val
        return [val[0], MARK]

    def _parse_upper(inpstr):
        val = _check_match(inpstr, upbound, 1)
        if not val:
            return val
        return [MARK, val[0]]

    def _parse_start_step_stop(inpstr):
        val = _check_match(inpstr, sss_pat, 3)
        if not val:
            return val
        start = val[0]
        step = val[1]
        stop = val[2]
        curr_value = start

        vallist = []
        while True:
            next_value = curr_value + step

            if next_value >= stop:
                vallist.append([curr_value, stop])
                return vallist
            else:
                vallist.append([curr_value, next_value])

            curr_value = next_value

    def _extract_simple_input(inpstr):
        for fun in _parse_slice, _parse_lower, _parse_upper:
            val = fun(inpstr)
            if val:
                return val

        return False

    def _extract_composed_input(inpstr):
        return _parse_start_step_stop(inpstr)

    if not str_to_parser:
        return [[MARK, MARK]]

    parts = str_to_parser.split(",")
    result = []
    for inps in parts:
        inps = inps.replace(" ", "")
        aux_res = _extract_simple_input(inps)
        if aux_res:
            result.append(aux_res)
            continue
        aux_res = _extract_composed_input(inps)
        if aux_res:
            result += aux_res
            continue
        raise SyntaxError("Invalid input " + str_to_parser + ". Failed caused by this term:" + inps)

    return result


def getFileAndName(incomplete_path):
    this_path = FileFinder.getFullPath(incomplete_path)
    if not this_path:
        # do not catch exception, let it goes.
        this_path = FileFinder.findRuns(incomplete_path)
        # if list, get first value
        if hasattr(this_path, "__iter__"):
            this_path = this_path[0]

    # this_path contains the full_path
    basename = os.path.basename(this_path)
    # remove extension
    basename = os.path.splitext(basename)[0]

    return this_path, basename


def _merge_to_ranges(ints):
    """
    Given an integer list, will "merge" adjacent integers into "ranges".
    Assumes that the given list will already be sorted and that it contains no
    duplicates.  Best explained with examples:

    Input:  [1, 2, 3, 4]
    Output: [[1, 4]]

    Input:  [1, 2, 3, 5, 6, 7]
    Output: [[1, 3], [5, 7]]

    Input:  [1, 2, 3, 5, 7, 8, 9]
    Output: [[1, 3], [5, 5], [7, 9]]

    Input:  [1, 2, 7, 5, 6, 3, 2, 2]
    Output: Unknown -- the input contains duplicates and is unsorted.

    @params ints :: the integer list to merge, sorted and without duplicates

    @returns a list of ranges
    """
    ranges = []
    current_range = []
    for i in ints:
        if current_range == []:
            current_range = [i, i]
        elif current_range[1] + 1 == i:
            current_range[1] = i
        else:
            ranges.append(current_range)
            current_range = [i, i]
    if current_range not in ranges:
        ranges.append(current_range)
    return ranges


def _yield_masked_det_ids(masking_ws):
    """
    For some reason Detector.isMasked() does not work for MaskingWorkspaces.
    We use masking_ws.readY(ws_index)[0] == 1 instead.
    """
    for ws_index in range(masking_ws.getNumberHistograms()):
        if masking_ws.readY(ws_index)[0] == 1:
            yield masking_ws.getDetector(ws_index).getID()


def get_masked_det_ids_from_mask_file(mask_file_path, idf_path):
    """
    Given a mask file and the (necessary) path to the corresponding IDF, will
    load in the file and return a list of detector IDs that are masked.

    @param mask_file_path :: the path of the mask file to read in
    @param idf_path :: the path to the corresponding IDF. Necessary so that we
                       know exactly which instrument to use, and therefore know
                       the correct detector IDs.

    @returns the list of detector IDs that were masked in the file
    """
    mask_ws_name = "__temp_mask"
    LoadMask(Instrument=idf_path, InputFile=mask_file_path, OutputWorkspace=mask_ws_name)
    det_ids = list(_yield_masked_det_ids(mtd[mask_ws_name]))
    DeleteWorkspace(Workspace=mask_ws_name)

    return det_ids


def mask_detectors_with_masking_ws(ws_name, masking_ws_name):
    """
    Rolling our own MaskDetectors wrapper since masking is broken in a couple
    of places that affect us here.

    Calling MaskDetectors(Workspace=ws_name, MaskedWorkspace=mask_ws_name) is
    not something we can do because the algorithm masks by ws index rather than
    detector id, and unfortunately for SANS the detector table is not the same
    for MaskingWorkspaces as it is for the workspaces containing the data to be
    masked.  Basically, we get a mirror image of what we expect.  Instead, we
    have to extract the det IDs and use those via the DetectorList property.

    @param ws :: the workspace to be masked.
    @param masking_ws :: the masking workspace that contains masking info.
    """
    ws, masking_ws = mtd[ws_name], mtd[masking_ws_name]

    masked_det_ids = list(_yield_masked_det_ids(masking_ws))

    MaskDetectors(Workspace=ws, DetectorList=masked_det_ids, ForceInstrumentMasking=True)


def check_child_ws_for_name_and_type_for_added_eventdata(wsGroup, number_of_entries=None):
    """
    Ensure that the while loading added event data, we are dealing with
    1. The correct naming convention. For event data this is the run number,
       an add tag and possibly underscores and numbers when the same workspace
       is reloaded. For monitor data it is the run number, an add tag, a monitor
       tag and the possibly underscores and numbers when the same workspace is
       reloaded
    2. The correct workspace types.
    @param wsGroup ::  workspace group.
    @param number_of_entries:: how many entries the group workspace may have.
    """
    # Check if there are only two children in the group workspace
    if number_of_entries is not None:
        if len(wsGroup) != number_of_entries:
            return False
    # There has to be an even number of workspaces, for each data workspace
    # there has to be a monitor workspace
    if len(wsGroup) % 2 != 0:
        return False

    assert isinstance(wsGroup, WorkspaceGroup)

    # Check all entries
    has_data = []
    has_monitors = []

    for index in range(len(wsGroup)):
        childWorkspace = wsGroup.getItem(index)
        if re.search(REG_DATA_NAME, childWorkspace.name()):
            is_in = True if isinstance(childWorkspace, IEventWorkspace) else False
            has_data.append(is_in)
        elif re.search(REG_DATA_MONITORS_NAME, childWorkspace.name()):
            is_in = True if isinstance(childWorkspace, MatrixWorkspace) else False
            has_monitors.append(is_in)

    total_has_data = False if len(has_data) == 0 else all(has_data)
    total_has_monitors = False if len(has_data) == 0 else all(has_monitors)
    one_monitor_per_data = len(has_data) == len(has_monitors)
    return total_has_data and total_has_monitors and one_monitor_per_data


def extract_child_ws_for_added_eventdata(ws_group, appendix):
    """
    Extract the child workspaces from a workspace group which was
    created by adding event data. The workspace group must contain a data
    workspace which is an EventWorkspace and a monitor workspace which is a
    matrix workspace.
    @param ws_group :: workspace group.
    @param appendix :: what to append to the names of the child workspaces
    """
    # Store the name of the group workspace in a string
    ws_group_name = ws_group.name()

    # Get a handle on each child workspace
    ws_handles = []
    for index in range(len(ws_group)):
        ws_handles.append(ws_group.getItem(index))

    if len(ws_handles) % 2:
        raise RuntimeError(
            "Expected two child workspaces when loading added event data."
            "Please make sure that you have loaded added event data which was generated by the Add tab of the SANS Gui."
        )

    # Now ungroup the group
    UnGroupWorkspace(ws_group)

    # Rename the child workspaces to be of the expected format. (see _get_workspace_name in sans_reduction_steps)
    data_workspaces = []
    monitor_workspaces = []
    for ws_handle in ws_handles:
        old_workspace_name = ws_handle.name()
        # Get the index of the multiperiod workspace if it is present
        new_workspace_name = get_new_workspace_name(appendix, old_workspace_name, ws_group_name)
        RenameWorkspace(InputWorkspace=ws_handle, OutputWorkspace=new_workspace_name)
        if appendix in old_workspace_name:
            monitor_workspaces.append(new_workspace_name)
        else:
            data_workspaces.append(new_workspace_name)
    # If there is more than one entry for the data and monitor workspaces, then we need to group them
    if len(data_workspaces) != len(monitor_workspaces):
        raise RuntimeError("The number of data workspaces does not match the number of monitor workspaces.")

    if len(data_workspaces) > 1 and len(monitor_workspaces) > 1:
        GroupWorkspaces(InputWorkspaces=monitor_workspaces, OutputWorkspace=ws_group_name + appendix)
        GroupWorkspaces(InputWorkspaces=data_workspaces, OutputWorkspace=ws_group_name)


def get_new_workspace_name(appendix, old_workspace_name, ws_group_name):
    new_workspace_name = ws_group_name
    final_number = re.search(r"_(\d+)$", old_workspace_name)
    if final_number is not None:
        new_workspace_name += final_number.group(0)

    if appendix in old_workspace_name:
        new_workspace_name += appendix
    return new_workspace_name


class WorkspaceType(object):
    class Event(object):
        pass

    class Histogram(object):
        pass

    class MultiperiodEvent(object):
        pass

    class MultiperiodHistogram(object):
        pass

    class Other(object):
        pass


def get_number_of_periods_from_file(file_name):
    full_file_path = FileFinder.findRuns(file_name)
    if hasattr(full_file_path, "__iter__"):
        full_file_path = full_file_path[0]
    try:
        with h5.File(full_file_path, "r") as h5_file:
            first_entry = h5_file["raw_data_1"]
            period_group = first_entry["periods"]
            proton_charge_data_set = period_group["proton_charge"]
            number_of_periods = len(proton_charge_data_set)
    except IOError:
        number_of_periods = -1
    return number_of_periods


def check_if_is_event_data(file_name):
    """
    Event mode files have a class with a "NXevent_data" type
    Structure:
    |--mantid_workspace_1/raw_data_1|
                                    |--some_group|
                                                 |--Attribute: NX_class = NXevent_data
    """
    full_file_path = FileFinder.findRuns(file_name)
    if hasattr(full_file_path, "__iter__"):
        file_name = full_file_path[0]
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        first_entry = h5_file[keys[0]]
        # Open instrument group
        is_event_mode = False
        for value in list(first_entry.values()):
            if "NX_class" in value.attrs:
                if "NXevent_data" == value.attrs["NX_class"].decode():
                    is_event_mode = True
                    break

    return is_event_mode


def is_nexus_file(file_name):
    full_file_path = FileFinder.findRuns(file_name)
    if hasattr(full_file_path, "__iter__"):
        file_name = full_file_path[0]
    is_nexus = True
    try:
        with h5.File(file_name, "r") as h5_file:
            keys = list(h5_file.keys())
            nexus_test = "raw_data_1" in keys or "mantid_workspace_1" in keys
            is_nexus = True if nexus_test else False
    except:
        is_nexus = False
    return is_nexus


def get_workspace_type(file_name):
    if is_nexus_file(file_name):
        number_of_periods = get_number_of_periods_from_file(file_name)
        is_event_data = check_if_is_event_data(file_name)

        if number_of_periods > 1:
            workspace_type = WorkspaceType.MultiperiodEvent if is_event_data else WorkspaceType.MultiperiodHistogram
        else:
            workspace_type = WorkspaceType.Event if is_event_data else WorkspaceType.Histogram
    else:
        workspace_type = WorkspaceType.Other
    return workspace_type


def bundle_added_event_data_as_group(out_file_name, out_file_monitors_name, is_multi_period):
    """
    We load an added event data file and its associated monitor file. Combine
    the data in a group workspace and delete the original files.
    @param out_file_name :: the file name of the event data file
    @param out_file_monitors_name :: the file name of the monitors file
    @param is_multi_period: if the data set is multiperid
    @return the name fo the new group workspace file
    """
    # Extract the file name and the extension
    file_name, file_extension = os.path.splitext(out_file_name)
    event_data_temp = file_name + ADDED_EVENT_DATA_TAG
    Load(Filename=out_file_name, OutputWorkspace=event_data_temp)
    event_data_ws = mtd[event_data_temp]

    monitor_temp = file_name + "_monitors" + ADDED_EVENT_DATA_TAG
    Load(Filename=out_file_monitors_name, OutputWorkspace=monitor_temp)

    monitor_ws = mtd[monitor_temp]

    out_group_file_name = file_name + file_extension
    out_group_ws_name = file_name

    # Delete the intermediate files
    full_data_path_name = get_full_path_for_added_event_data(out_file_name)
    full_monitor_path_name = get_full_path_for_added_event_data(out_file_monitors_name)

    if os.path.exists(full_data_path_name):
        os.remove(full_data_path_name)
    if os.path.exists(full_monitor_path_name):
        os.remove(full_monitor_path_name)

    # Create a grouped workspace with the data and the monitor child workspaces
    workspace_names_to_group = []
    if isinstance(event_data_ws, WorkspaceGroup):
        for workspace in event_data_ws:
            workspace_names_to_group.append(workspace.name())
    else:
        workspace_names_to_group.append(event_data_ws.name())
    if isinstance(monitor_ws, WorkspaceGroup):
        for workspace in monitor_ws:
            workspace_names_to_group.append(workspace.name())
    else:
        workspace_names_to_group.append(monitor_ws.name())

    GroupWorkspaces(InputWorkspaces=workspace_names_to_group, OutputWorkspace=out_group_ws_name)
    group_ws = mtd[out_group_ws_name]

    # Save the group
    SaveNexusProcessed(InputWorkspace=group_ws, Filename=out_group_file_name, Append=False)
    # Delete the files and the temporary workspaces
    if out_group_ws_name in mtd:
        DeleteWorkspace(out_group_ws_name)

    return out_group_file_name


def get_full_path_for_added_event_data(file_name):
    path, base = os.path.split(file_name)
    if path == "" or base not in os.listdir(path):
        path = config["defaultsave.directory"] + path
        # If the path is still an empty string check in the current working directory
        if path == "":
            path = os.getcwd()
        assert base in os.listdir(path)
    full_path_name = os.path.join(path, base)

    return full_path_name


def extract_spectra(ws, det_ids, output_ws_name):
    """
    A more generic version of ExtactSingleSpectrum.  Accepts an arbitrary list
    of ws indices to keep.  Everything else is ignored.
    @param ws :: the workspace from which to extract spectra
    @param det_ids :: the detector IDs corresponding to the spectra to extract
    @param output_ws_name :: the name of the resulting workspace
    @returns :: a workspace containing the extracted spectra
    """
    ExtractSpectra(InputWorkspace=ws, OutputWorkspace=output_ws_name, DetectorList=det_ids)
    return mtd[output_ws_name]


def get_masked_det_ids(ws):
    """
    Given a workspace, will return a list of all the IDs that correspond to
    detectors that have been masked.

    @param ws :: the workspace to extract the det IDs from

    @returns a list of IDs for masked detectors
    """
    spectrumInfo = ws.spectrumInfo()
    for ws_index in range(ws.getNumberHistograms()):
        if not spectrumInfo.hasDetectors(ws_index):
            # Skip the rest after finding the first spectra with no detectors,
            # which is a big speed increase for SANS2D.
            break
        if spectrumInfo.isMasked(ws_index):
            yield ws.getDetector(ws_index).getID()


def create_zero_error_free_workspace(input_workspace_name, output_workspace_name):
    """
    Creates a cloned workspace where all zero-error values have been replaced with a large value
    @param input_workspace_name :: The input workspace name
    @param output_workspace_name :: The output workspace name
    @returns a message and a completion flag
    """
    # Load the input workspace
    message = ""
    complete = False
    if input_workspace_name not in mtd:
        message = "Failed to create a zero error free cloned workspace: The input workspace does not seem to exist."
        return message, complete

    # Create a cloned workspace
    ws_in = mtd[input_workspace_name]

    # Remove all zero errors from the cloned workspace
    CloneWorkspace(InputWorkspace=ws_in, OutputWorkspace=output_workspace_name)
    if output_workspace_name not in mtd:
        message = "Failed to create a zero error free cloned workspace: A clone could not be created."
        return message, complete

    ws_out = mtd[output_workspace_name]
    try:
        remove_zero_errors_from_workspace(ws_out)
        complete = True
    except ValueError:
        DeleteWorkspace(Workspace=output_workspace_name)
        message = "Failed to create a zero error free cloned workspace: Could not remove the zero errors."

    return message, complete


def remove_zero_errors_from_workspace(ws):
    """
    Removes the zero errors from a Matrix workspace
    @param ws :: The input workspace
    """
    # Make sure we are dealing with a MatrixWorkspace
    if not isinstance(ws, MatrixWorkspace) or isinstance(ws, IEventWorkspace):
        raise ValueError("Cannot remove zero errors from a workspace which is not of type MatrixWorkspace.")
    # Iterate over the workspace and replace the zero values with a large default value
    numSpectra = ws.getNumberHistograms()
    errors = ws.dataE
    for index in range(0, numSpectra):
        spectrum = errors(index)
        spectrum[spectrum <= 0.0] = ZERO_ERROR_DEFAULT


def delete_zero_error_free_workspace(input_workspace_name):
    """
    Deletes the zero-error free workspace
    @param ws :: The input workspace
    """
    complete = False
    message = ""
    if input_workspace_name in mtd:
        DeleteWorkspace(Workspace=input_workspace_name)
        complete = True
    else:
        message = "Failed to delete a zero-error free workspace"
    return message, complete


def is_valid_ws_for_removing_zero_errors(input_workspace_name):
    """
    Check if a workspace has been created via Q1D or Qxy.
    @param ws :: The input workspace
    """
    isValid = False
    message = ""

    ws = mtd[input_workspace_name]
    workspaceHistory = ws.getHistory()
    histories = workspaceHistory.getAlgorithmHistories()
    for history in histories:
        name = history.name()
        if name == "Q1D" or name == "Qxy":
            isValid = True
            break

    if not isValid:
        message = "Workspace does not seem valid for zero error removal.It must have been reduced with Q1D or Qxy."

    return message, isValid


class AddOperation(object):
    """
    The AddOperation allows to add two workspaces at a time.
    """

    def __init__(self, is_overlay, time_shifts):
        """
        The AddOperation requires to know if the workspaces are to
        be plainly added or to be overlaid. Additional time shifts can be
        specified
        :param is_overlay :: true if the operation is an overlay operation
        :param time_shifts :: a string with comma-separated time shift values
        """
        super(AddOperation, self).__init__()
        factory = CombineWorkspacesFactory()
        self.adder = factory.create_add_algorithm(is_overlay)
        self.time_shifter = TimeShifter(time_shifts)

    def add(self, LHS_workspace, RHS_workspace, output_workspace, run_to_add, estimate_logs=False):
        """
        Add two workspaces together and place the result into the outputWorkspace.
        The user needs to specify which run is being added in order to determine
        the correct time shift
        :param LHS_workspace :: first workspace, this workspace is a reference workspace
                                and hence never shifted
        :param RHS_workspace :: second workspace which can be shifted in time
        :param output_workspace :: the output workspace name
        :param run_to_add :: the number of the nth added workspace
        :param estimate_logs :: bool. whether or not to estimate good values for bad proton charge logs.
                                Only applicable for Overlay. Default is False.
        """
        current_time_shift = self.time_shifter.get_Nth_time_shift(run_to_add)
        self.adder.add(
            LHS_workspace=LHS_workspace,
            RHS_workspace=RHS_workspace,
            output_workspace=output_workspace,
            time_shift=current_time_shift,
            estimate_logs=estimate_logs,
        )


class CombineWorkspacesFactory(object):
    """
    Factory to determine how to add workspaces
    """

    def __init__(self):
        super(CombineWorkspacesFactory, self).__init__()

    def create_add_algorithm(self, is_overlay):
        """
        :param is_overlay :: if true we provide the OverlayWorkspaces functionality
        """
        if is_overlay:
            return OverlayWorkspaces()
        else:
            return PlusWorkspaces()


def _clean_logs(ws, estimate_logs):
    """
    Remove bad proton charge times from the logs, if present.
    :param ws: The workspace to clean
    :param estimate_logs: bool. If true, estimate "good" values for bad proton logs
    """
    run = ws.getRun()
    if run.hasProperty("proton_charge"):
        GPS_EPOCH = DateAndTime(0, 0).to_datetime64()
        pc = run.getProperty("proton_charge")
        first_valid_index = next((index for index, time in enumerate(pc.times) if time > GPS_EPOCH), None)
        if first_valid_index > 0:
            # Bug caused bad proton charges in 1700s. 1990 is start of epoch.
            start_time = pc.nthTime(first_valid_index)
            # stop time is number of removed intervals + 1 so time averages work out
            stop_time = pc.lastTime() + int(_get_average_time_difference(pc, first_valid_index) * (first_valid_index))
            if estimate_logs:  # add one more if adding in at the end
                stop_time += int(_get_average_time_difference(pc, first_valid_index))
            ws = FilterByTime(InputWorkspace=ws, OutputWorkspace=ws, AbsoluteStartTime=str(start_time), AbsoluteStopTime=str(stop_time))
            # update references that were invalidated
            run = ws.getRun()
            pc = run.getProperty("proton_charge")  # new version of the log
            sanslog.notice("{} Invalid pulsetimes from before {} removed.".format(first_valid_index, start_time))

            if estimate_logs:
                pc = run.getProperty("proton_charge")  # new version of the log
                # Estimate what the data should have been
                _estimate_good_log(pc, first_valid_index)
    return ws  # the reference has been updated


def _estimate_good_log(pc, num):
    """
    The bad proton charge is corrupted data from the end of a run,
    rather than additional data. Therefore, we can estimate what
    the data should have been.
    Naively, we add a log with a time = last time + average time diff
    and a value equal to the average of the values
    :param pc: FloatTimeSeriesProperty. The proton charge logs.
    :param num: Number of logs to append
    """
    average_time_diff = _get_average_time_difference(pc, num)
    estimated_charge = pc.lastValue()
    for _ in range(num):
        estimated_time = pc.lastTime().to_datetime64() + average_time_diff
        pc.addValue(estimated_time, estimated_charge)
        sanslog.notice("A corrected pulsetime of {} has been estimated.".format(estimated_time))


def _get_average_time_difference(pc, num):
    """
    Get the average difference between consecutive values,
    in nanoseconds
    :param pc: The proton charge logs
    :param num: The number of log values that were skipped. This is needed because the log itself doesn't necessarily know what happened
    :return: the average different between times in ns
    """
    # TODO if the log ever has values actually removed, then this needs to be changed
    diffs = pc.filtered_times[num + 1 :] - pc.filtered_times[num:-1]
    return np.mean(diffs)


class PlusWorkspaces(object):
    """
    Wrapper for the Plus algorithm
    """

    def __init__(self):
        super(PlusWorkspaces, self).__init__()

    def add(self, LHS_workspace, RHS_workspace, output_workspace, time_shift=0.0, estimate_logs=False):
        """
        :param LHS_workspace :: the first workspace
        :param RHS_workspace :: the second workspace
        :param output_workspace :: the output workspace
        :param time_shift :: unused parameter
        :param estimate_logs :: unused parameter
        """
        lhs_ws = self._get_workspace(LHS_workspace)
        rhs_ws = self._get_workspace(RHS_workspace)

        # Remove bad proton charges from logs
        lhs_ws = _clean_logs(lhs_ws, estimate_logs)
        rhs_ws = _clean_logs(rhs_ws, estimate_logs)

        # Apply shift to RHS sample logs where necessary. This is a hack because Plus cannot handle
        # cumulative time series correctly at this point
        cummulative_correction = CummulativeTimeSeriesPropertyAdder()
        cummulative_correction.extract_sample_logs_from_workspace(lhs_ws, rhs_ws)
        Plus(LHSWorkspace=LHS_workspace, RHSWorkspace=RHS_workspace, OutputWorkspace=output_workspace)
        out_ws = self._get_workspace(output_workspace)
        cummulative_correction.apply_cummulative_logs_to_workspace(out_ws)

    def _get_workspace(self, workspace):
        if isinstance(workspace, MatrixWorkspace):
            return workspace
        elif isinstance(workspace, str) and mtd.doesExist(workspace):
            return mtd[workspace]


class OverlayWorkspaces(object):
    """
    Overlays (in time) a workspace  on top of another workspace. The two
    workspaces overlaid such that the first time entry of their proton_charge entry matches.
    This overlap can be shifted by the specified time_shift in seconds
    """

    def __init__(self):
        super(OverlayWorkspaces, self).__init__()

    def add(self, LHS_workspace, RHS_workspace, output_workspace, time_shift=0.0, estimate_logs=False):
        """
        :param LHS_workspace :: the first workspace
        :param RHS_workspace :: the second workspace
        :param output_workspace :: the output workspace
        :param time_shift :: an additional time shift for the overlay procedure
        :param estimate_logs :: optional bool. If true, and bad proton logs are present,
                                estimate "good" values to replace them
        """
        rhs_ws = self._get_workspace(RHS_workspace)
        lhs_ws = self._get_workspace(LHS_workspace)

        # Remove bad proton charges from logs
        lhs_ws = _clean_logs(lhs_ws, estimate_logs)
        rhs_ws = _clean_logs(rhs_ws, estimate_logs)

        # Find the time difference between LHS and RHS workspaces and add optional time shift
        time_difference = self._extract_time_difference_in_seconds(lhs_ws, rhs_ws)
        total_time_shift = time_difference + time_shift

        # Apply shift to RHS sample logs where necessary. This is a hack because Plus cannot handle
        # cumulative time series correctly at this point
        cummulative_correction = CummulativeTimeSeriesPropertyAdder()
        cummulative_correction.extract_sample_logs_from_workspace(lhs_ws, rhs_ws)

        # Create a temporary workspace with shifted time values from RHS, if the shift is necessary
        temp = rhs_ws
        temp_ws_name = "shifted"
        if total_time_shift != 0.0:
            temp = ChangeTimeZero(InputWorkspace=rhs_ws, OutputWorkspace=temp_ws_name, RelativeTimeOffset=total_time_shift)

        # Add the LHS and shifted workspace
        Plus(LHSWorkspace=LHS_workspace, RHSWorkspace=temp, OutputWorkspace=output_workspace)

        # Apply sample log correction
        out_ws = self._get_workspace(output_workspace)
        cummulative_correction.apply_cummulative_logs_to_workspace(out_ws)

        # Remove the shifted workspace
        if mtd.doesExist(temp_ws_name):
            mtd.remove(temp_ws_name)

    def _extract_time_difference_in_seconds(self, ws1, ws2):
        # The times which need to be compared are the first entry in the proton charge log
        time_1 = self._get_time_from_proton_charge_log(ws1)
        time_2 = self._get_time_from_proton_charge_log(ws2)

        return float((time_1 - time_2) / np.timedelta64(1, "s"))

    def _get_time_from_proton_charge_log(self, ws):
        times = ws.getRun().getProperty("proton_charge").times
        if len(times) == 0:
            raise ValueError("The proton charge does not have any time entry")
        return times[0]

    def _get_workspace(self, workspace):
        if isinstance(workspace, MatrixWorkspace):
            return workspace
        elif isinstance(workspace, str) and mtd.doesExist(workspace):
            return mtd[workspace]


# pylint: disable=too-few-public-methods


class TimeShifter(object):
    """
    The time shifter stores all time shifts for all runs which are to be added. If there is
    a mismatch the time shifts are set to 0.0 seconds.
    """

    def __init__(self, time_shifts):
        super(TimeShifter, self).__init__()
        self._time_shifts = time_shifts

    def get_Nth_time_shift(self, n):
        """
        Retrieves the specified additional time shift for the nth addition in seconds.
        :param n :: the nth addition
        """
        if len(self._time_shifts) >= (n + 1):
            return self._cast_to_float(self._time_shifts[n])
        else:
            return 0.0

    @staticmethod
    def _cast_to_float(element):
        float_element = 0.0
        try:
            float_element = float(element)
        except ValueError:
            pass  # Log here
        return float_element


def transfer_special_sample_logs(from_ws, to_ws):
    """
    Transfers selected sample logs from one workspace to another
    """
    single_valued_names = ["gd_prtn_chrg"]
    time_series_names = ["good_uah_log", "good_frames"]
    type_map = {"good_uah_log": float, "good_frames": int, "gd_prtn_chrg": float}

    run_from = from_ws.getRun()
    run_to = to_ws.getRun()

    # Populate the time series
    for time_series_name in time_series_names:
        if run_from.hasProperty(time_series_name) and run_to.hasProperty(time_series_name):
            times = run_from.getProperty(time_series_name).times
            values = run_from.getProperty(time_series_name).value

            prop = run_to.getProperty(time_series_name)
            prop.clear()
            for time, value in zip(times, values):
                prop.addValue(time, type_map[time_series_name](value))

    alg_log = AlgorithmManager.createUnmanaged("AddSampleLog")
    alg_log.initialize()
    alg_log.setChild(True)
    for single_valued_name in single_valued_names:
        if run_from.hasProperty(single_valued_name) and run_to.hasProperty(single_valued_name):
            value = run_from.getProperty(single_valued_name).value
            alg_log.setProperty("Workspace", to_ws)
            alg_log.setProperty("LogName", single_valued_name)
            alg_log.setProperty("LogText", str(type_map[single_valued_name](value)))
            alg_log.setProperty("LogType", "Number")
            alg_log.execute()


# pylint: disable=too-many-instance-attributes


class CummulativeTimeSeriesPropertyAdder(object):
    """
    Apply shift to RHS sample logs where necessary. This is a hack because Plus cannot handle
    cumulative time series correctly at this point.
    """

    def __init__(self, total_time_shift_seconds=0):
        super(CummulativeTimeSeriesPropertyAdder, self).__init__()
        self._time_series = ["good_uah_log", "good_frames"]
        self._single_valued = ["gd_prtn_chrg"]

        self._original_times_lhs = dict()
        self._original_values_lhs = dict()
        self._original_times_rhs = dict()
        self._original_values_rhs = dict()

        self._original_single_valued_lhs = dict()
        self._original_single_valued_rhs = dict()

        self._start_time_lhs = None
        self._start_time_rhs = None
        self._total_time_shift_nano_seconds = int(total_time_shift_seconds * 1e9)

        self._type_map = {"good_uah_log": float, "good_frames": int, "gd_prtn_chrg": float}

    def extract_sample_logs_from_workspace(self, lhs, rhs):
        """
        When adding specific logs, we need to make sure that the values are added correctly.
        :param lhs: the lhs workspace
        :param rhs: the rhs workspace
        """
        run_lhs = lhs.getRun()
        run_rhs = rhs.getRun()
        # Get the cumulative time s
        for element in self._time_series:
            if run_lhs.hasProperty(element) and run_rhs.hasProperty(element):
                # Get values for lhs
                property_lhs = run_lhs.getProperty(element)
                self._original_times_lhs[element] = property_lhs.times
                self._original_values_lhs[element] = property_lhs.value

                # Get values for rhs
                property_rhs = run_rhs.getProperty(element)
                self._original_times_rhs[element] = property_rhs.times
                self._original_values_rhs[element] = property_rhs.value

        for element in self._single_valued:
            if run_lhs.hasProperty(element) and run_rhs.hasProperty(element):
                # Get the values for lhs
                property_lhs = run_lhs.getProperty(element)
                self._original_single_valued_lhs[element] = property_lhs.value

                # Get the values for rhs
                property_rhs = run_rhs.getProperty(element)
                self._original_single_valued_rhs[element] = property_rhs.value
            elif element == "gd_prtn_chrg":
                self._original_single_valued_lhs[element] = run_lhs.getProtonCharge()
                self._original_single_valued_rhs[element] = run_rhs.getProtonCharge()

        log_name_start_time = "start_time"
        if run_lhs.hasProperty(log_name_start_time) and run_rhs.hasProperty(log_name_start_time):

            def convert_to_date(val):
                return DateAndTime(val) if isinstance(val, str) else val

            self._start_time_lhs = convert_to_date(run_lhs.getProperty(log_name_start_time).value)
            self._start_time_rhs = convert_to_date(run_rhs.getProperty(log_name_start_time).value)

    def apply_cummulative_logs_to_workspace(self, workspace):
        """
        Restore the original values for the shifted properties
        :param workspace: the workspace which requires correction.
        """
        for element in self._time_series:
            if element in self._original_times_rhs and element in self._original_values_rhs:
                run = workspace.getRun()
                prop = run.getProperty(element)
                prop.clear()
                # Get the cummulated values and times
                times, values = self._get_cummulative_sample_logs(element)
                self._populate_property(prop, times, values, self._type_map[element])

        self._update_single_valued_entries(workspace)

    def _update_single_valued_entries(self, workspace):
        """
        We need to update single-valued entries which are based on the
        cumulative time series
        :param workspace: the workspace which requires the changes
        """
        run = workspace.getRun()

        alg_log = AlgorithmManager.createUnmanaged("AddSampleLog")
        alg_log.initialize()
        alg_log.setChild(True)
        for element in self._single_valued:
            if run.hasProperty(element):
                type_converter = self._type_map[element]
                new_value = type_converter(self._original_single_valued_lhs[element]) + type_converter(
                    self._original_single_valued_rhs[element]
                )
                alg_log.setProperty("Workspace", workspace)
                alg_log.setProperty("LogName", element)
                alg_log.setProperty("LogText", str(new_value))
                alg_log.setProperty("LogType", "Number")
                alg_log.execute()

    def _get_cummulative_sample_logs(self, log_name):
        """
        Gets the added sample logs for a particular log.
        :param log_name: the name of the logs
               an array with times and an array with values
        """
        # Remove data from the beginning of the measurement
        times_lhs, values_lhs, times_rhs, values_rhs = self._get_corrected_times_and_values(log_name)

        # Create the actual entries and not the cumulated ones
        values_raw_lhs = self._get_raw_values(values_lhs)
        values_raw_rhs = self._get_raw_values(values_rhs)

        # Shift the times of the rhs workspace if required
        times_rhs = self._shift_time_series(times_rhs)

        # Now merge and sort the two entries
        time_merged, value_merged = self._create_merged_values_and_times(times_lhs, values_raw_lhs, times_rhs, values_raw_rhs)

        # Create cummulated values
        time_final = time_merged
        value_final = self._create_cummulated_values(value_merged)

        return time_final, value_final

    def _create_cummulated_values(self, values):
        """
        Creates cummulated values
        :param time: a time array
        :param value: a value array which is the basis for the accumulation
        """
        values_accumulated = []
        for index in range(0, len(values)):
            if index == 0:
                values_accumulated.append(values[index])
            else:
                values_accumulated.append(values_accumulated[index - 1] + values[index])
        return values_accumulated

    def _create_merged_values_and_times(self, times_lhs, values_lhs, times_rhs, values_rhs):
        times = []
        times.extend(times_lhs)
        times.extend(times_rhs)

        values = []
        values.extend(values_lhs)
        values.extend(values_rhs)

        zipped = list(zip(times, values))
        # We sort via the times
        zipped.sort(key=lambda z: z[0])
        unzipped = list(zip(*zipped))
        return unzipped[0], unzipped[1]

    def _shift_time_series(self, time_series):
        shifted_series = []
        for element in time_series:
            shifted_series.append(element + self._total_time_shift_nano_seconds)
        return shifted_series

    def _get_raw_values(self, values):
        """
        We extract the original data from the cumulative
        series.
        """
        raw_values = []
        for index in range(0, len(values)):
            if index == 0:
                raw_values.append(values[index])
            else:
                element = values[index] - values[index - 1]
                raw_values.append(element)
        return raw_values

    def _get_corrected_times_and_values(self, log_name):
        """
        Removes times before time 0
        :param log_name: the log to consider
        """
        start_time_lhs = self._start_time_lhs
        start_time_rhs = self._start_time_rhs

        times_lhs = self._original_times_lhs[log_name]
        times_rhs = self._original_times_rhs[log_name]

        values_lhs = self._original_values_lhs[log_name]
        values_rhs = self._original_values_rhs[log_name]

        # At this point we assume that the values are sorted.
        # We search for the index which is larger or equal to the
        # start time

        index_lhs = self._find_start_time_index(times_lhs, start_time_lhs)
        index_rhs = self._find_start_time_index(times_rhs, start_time_rhs)

        times_lhs_corrected = times_lhs[index_lhs:]
        values_lhs_corrected = values_lhs[index_lhs:]

        times_rhs_corrected = times_rhs[index_rhs:]
        values_rhs_corrected = values_rhs[index_rhs:]

        return times_lhs_corrected, values_lhs_corrected, times_rhs_corrected, values_rhs_corrected

    def _find_start_time_index(self, time_series, start_time):
        start_time = start_time.to_datetime64()
        index = 0
        for element in time_series:
            if element > start_time or element == start_time:
                break
            index += 1
        return index

    def _populate_property(self, prop, times, values, type_converter):
        """
        Populates a time series property
        :param prop: the time series property
        :param times: the times array
        :param values: the values array
        :param type_converter: a type converter
        """
        for time, value in zip(times, values):
            prop.addValue(time, type_converter(value))


def load_monitors_for_multiperiod_event_data(workspace, data_file, monitor_appendix):
    """
    Takes a multi-period event workspace and loads the monitors
    as a group workspace
    :param workspace: Multi-period event workspace
    :param data_file: The data file
    :param monitor_appendix: The appendix for monitor data
    """
    # Load all monitors
    mon_ws_group_name = "temp_ws_group"
    LoadNexusMonitors(Filename=data_file, OutputWorkspace=mon_ws_group_name)
    mon_ws = mtd["temp_ws_group"]
    # Rename all monitor workspces
    rename_monitors_for_multiperiod_event_data(monitor_workspace=mon_ws, workspace=workspace, appendix=monitor_appendix)


def rename_monitors_for_multiperiod_event_data(monitor_workspace, workspace, appendix):
    """
    Takes a multi-period event workspace and loads the monitors
    as a group workspace
    :param workspace: Multi-period event workspace
    :param data_file: The data file
    :param monitor_appendix: The appendix for monitor data
    """
    if len(monitor_workspace) != len(workspace):
        raise RuntimeError("The workspace and monitor workspace lengths do not match.")
    for index in range(0, len(monitor_workspace)):
        monitor_name = workspace[index].name() + appendix
        RenameWorkspace(InputWorkspace=monitor_workspace[index], OutputWorkspace=monitor_name)
    # Finally rename the group workspace
    monitor_group_ws_name = workspace.name() + appendix
    RenameWorkspace(InputWorkspace=monitor_workspace, OutputWorkspace=monitor_group_ws_name)


def is_convertible_to_int(input_value):
    """
    Check if the input can be converted to int
    :param input_value :: a general input
    """
    try:
        int(input_value)
    except ValueError:
        return False
    return True


def is_convertible_to_float(input_value):
    """
    Check if the input can be converted to float
    :param input_value :: a general input
    :returns true if input can be converted to float else false
    """
    is_convertible = True
    if not input_value:
        is_convertible = False
    else:
        try:
            float(input_value)
            is_convertible = True
        except ValueError:
            is_convertible = False
    return is_convertible


def is_valid_xml_file_list(input_value):
    """
    Check if the input is a valid xml file list. We only check
    the form and not the existence of the file
    :param input :: a list input
    """
    if not isinstance(input_value, list) or not input or len(input_value) == 0:
        return False
    for element in input_value:
        if not isinstance(element, str) or not element.endswith(".xml"):
            return False
    return True


def convert_from_string_list(to_convert):
    """
    Convert a Python string list to a comma-separted string
    :param to_convert :: a string list input
    """
    return ",".join(element.replace(" ", "") for element in to_convert)


def convert_to_string_list(to_convert):
    """
    Convert a comma-separted string to a Python string list in a string form
    "file1.xml, file2.xml" -> "['file1.xml','file2.xml']"
    :param to_convert :: a comma-spearated string
    """
    string_list = to_convert.replace(" ", "").split(",")
    output_string = "[" + ",".join("'" + element + "'" for element in string_list) + "]"
    return output_string


def convert_to_list_of_strings(to_convert):
    """
    Converts a string of comma-separted values to a list of strings
    @param to_convert: the string to convert
    @returns a list of strings
    """
    values = to_convert.split(",")
    return [element.strip() for element in values]


def can_load_as_event_workspace(filename):
    """
    Check if an file can be loaded into an event workspace
    Currently we check if the file
    1. can be loaded with LoadEventNexus
    2. contains an "event_workspace" nexus group in its first level
    Note that this assumes a specific directory structure for the nexus file.
    @param filename: the name of the input file name
    @returns true if the file can be loaded as an event workspace else false
    """
    is_event_workspace = False

    # Check if it can be loaded with LoadEventNexus
    is_event_workspace = FileLoaderRegistry.canLoad("LoadEventNexus", filename)

    # Original code test it with nexus python. why?
    # This is just a copy of the logic and implement it using h5py
    if not is_event_workspace:
        # pylint: disable=bare-except
        try:
            # We only check the first entry in the root
            # and check for event_eventworkspace in the next level
            with h5.File(filename, "r") as h5f:
                try:
                    rootKeys = list(h5f.keys())  # python3 fix
                    entry0 = h5f[rootKeys[0]]
                    ew = entry0["event_workspace"]
                    is_event_workspace = ew is not None
                except:
                    pass
        except IOError:
            pass
    return is_event_workspace


def get_start_q_and_end_q_values(rear_data_name, front_data_name, rescale_shift):
    """
    Determines the min and max values for Q which are subsequently used for fitting.
    @param rear_data_name: name of the rear data set
    @param front_data_name: name of the front data set
    @param rescale_shift: the rescale and shift object
    """
    min_q = None
    max_q = None
    front_data = mtd[front_data_name]
    front_dataX = front_data.readX(0)

    front_size = len(front_dataX)
    front_q_min = None
    front_q_max = None
    if front_size > 0:
        front_q_min = front_dataX[0]
        front_q_max = front_dataX[front_size - 1]
    else:
        raise RuntimeError("The FRONT detector does not seem to contain q values")

    rear_data = mtd[rear_data_name]
    rear_dataX = rear_data.readX(0)

    rear_size = len(rear_dataX)
    rear_q_min = None
    rear_q_max = None
    if rear_size > 0:
        rear_q_min = rear_dataX[0]
        rear_q_max = rear_dataX[rear_size - 1]
    else:
        raise RuntimeError("The REAR detector does not seem to contain q values")

    if rear_q_max < front_q_min:
        raise RuntimeError("The min value of the FRONT detector data set is largerthan the max value of the REAR detector data set")

    # Get the min and max range
    min_q = max(rear_q_min, front_q_min)
    max_q = min(rear_q_max, front_q_max)

    if rescale_shift.qRangeUserSelected:
        min_q = max(min_q, rescale_shift.qMin)
        max_q = min(max_q, rescale_shift.qMax)

    return min_q, max_q


def get_error_corrected_front_and_rear_data_sets(front_data, rear_data, q_min, q_max):
    """
    Transfers the error data from the front data to the rear data
    @param front_data: the front data set
    @param rear_data: the rear data set
    @param q_min: the minimal q value
    @param q_max: the maximal q value
    """
    # First we want to crop the workspaces
    front_data_cropped = CropWorkspace(InputWorkspace=front_data, XMin=q_min, XMax=q_max)
    # For the rear data set
    rear_data_cropped = CropWorkspace(InputWorkspace=rear_data, XMin=q_min, XMax=q_max)

    # Now transfer the error from front data to the rear data workspace
    # This works only if we have a single QMod spectrum in the workspaces
    front_error = front_data_cropped.dataE(0)
    rear_error = rear_data_cropped.dataE(0)

    rear_error_squared = rear_error * rear_error
    front_error_squared = front_error * front_error

    corrected_error_squared = rear_error_squared + front_error_squared
    corrected_error = np.sqrt(corrected_error_squared)
    rear_error[0 : len(rear_error)] = corrected_error[0 : len(rear_error)]

    return front_data_cropped, rear_data_cropped


def is_1D_workspace(workspace):
    """
    Check if the workspace is 1D, ie if it has only a single spectrum
    @param workspace: the workspace to check
    @returns true if the workspace has a single spectrum else false
    """
    if workspace.getNumberHistograms() == 1:
        return True
    else:
        return False


def meter_2_millimeter(num):
    """
    Converts from m to mm
    @param float in m
    @returns float in mm
    """
    return num * 1000.0


def millimeter_2_meter(num):
    """
    Converts from mm to m
    @param float in mm
    @returns float in m
    """
    return num / 1000.0


def correct_q_resolution_for_can(original_workspace, can_workspace, subtracted_workspace):
    """
    We need to transfer the DX error values from the original workspaces to the subtracted
    workspace. Richard wants us to ignore potential DX values for the CAN workspace (they
    would be very small any way). The Q Resolution functionality only exists currently
    for 1D, ie when only one spectrum is present.
    @param original_workspace: the original workspace
    @param can_workspace: the can workspace
    @param subtracted_workspace: the subtracted workspace
    """
    if original_workspace.getNumberHistograms() == 1 and original_workspace.hasDx(0):
        subtracted_workspace.setDx(0, original_workspace.dataDx(0))


def correct_q_resolution_for_merged(count_ws_front, count_ws_rear, output_ws, scale):
    """
    We need to transfer the DX error values from the original workspaces to the merged worksapce.
    We have:
    C(Q) = Sum_all_lambda_for_particular_Q(Counts(lambda))
    weightedQRes(Q) = Sum_all_lambda_for_particular_Q(Counts(lambda)* qRes(lambda))
    ResQ(Q) = weightedQRes(Q)/C(Q)
    Richard suggested:
    ResQMerged(Q) = (weightedQRes_FRONT(Q)*scale + weightedQRes_REAR(Q))/
                    (C_FRONT(Q)*scale + C_REAR(Q))
    Note that we drop the shift here.
    The Q Resolution functionality only exists currently
    for 1D, ie when only one spectrum is present.
    @param count_ws_front: the front counts
    @param count_ws_rear: the rear counts
    @param output_ws: the output workspace
    """

    def divide_q_resolution_by_counts(q_res, counts):
        # We are dividing DX by Y.
        q_res_buffer = np.divide(q_res, counts)
        return q_res_buffer

    def multiply_q_resolution_by_counts(q_res, counts):
        # We are dividing DX by Y.
        q_res_buffer = np.multiply(q_res, counts)
        return q_res_buffer

    if count_ws_rear.getNumberHistograms() != 1:
        return

    # We require both count workspaces to contain the DX value
    if not count_ws_rear.hasDx(0) or not count_ws_front.hasDx(0):
        return

    q_resolution_front = count_ws_front.readDx(0)
    q_resolution_rear = count_ws_rear.readDx(0)
    counts_front = count_ws_front.readY(0)
    counts_rear = count_ws_rear.readY(0)

    # We need to make sure that the workspaces match in length
    if (len(q_resolution_front) != len(q_resolution_rear)) or (len(counts_front) != len(counts_rear)):
        return

    # Get everything for the FRONT detector
    q_res_front_norm_free = multiply_q_resolution_by_counts(q_resolution_front, counts_front)
    q_res_front_norm_free = q_res_front_norm_free * scale
    counts_front = counts_front * scale

    # Get everything for the REAR detector
    q_res_rear_norm_free = multiply_q_resolution_by_counts(q_resolution_rear, counts_rear)

    # Now add and divide
    new_q_res = np.add(q_res_front_norm_free, q_res_rear_norm_free)
    new_counts = np.add(counts_front, counts_rear)
    q_resolution = divide_q_resolution_by_counts(new_q_res, new_counts)

    # Set the dx error
    output_ws.setDx(0, q_resolution)


class MeasurementTimeFromNexusFileExtractor(object):
    """
    Extracts the measurement from a nexus file
    """

    def __init__(self):
        super(MeasurementTimeFromNexusFileExtractor, self).__init__()

    def _get_str(self, v):
        return v.tostring().decode("UTF-8")

    def _get_measurement_time_processed_file(self, h5entry):
        logs = h5entry["logs"]
        end_time = logs["end_time"]
        value = end_time["value"]
        return self._get_str(value[0])

    def _get_measurement_time_for_non_processed_file(self, h5entry):
        end_time = h5entry["end_time"]
        return self._get_str(end_time[0])

    def _check_if_processed_nexus_file(self, h5entry):
        definition = h5entry["definition"]
        mantid_definition = self._get_str(definition[0])
        is_processed = True if MANTID_PROCESSED_WORKSPACE_TAG in mantid_definition else False
        return is_processed

    def get_measurement_time(self, filename_full):
        measurement_time = ""
        try:
            with h5.File(filename_full, "r") as h5f:
                try:
                    rootKeys = list(h5f.keys())
                    entry0 = h5f[rootKeys[0]]
                    is_processed_file = self._check_if_processed_nexus_file(entry0)
                    if is_processed_file:
                        measurement_time = self._get_measurement_time_processed_file(entry0)
                    else:
                        measurement_time = self._get_measurement_time_for_non_processed_file(entry0)
                except:
                    sanslog.warning("Failed to retrieve the measurement time for " + str(filename_full))
        except IOError:
            sanslog.warning("Failed to open the file: " + str(filename_full))
        return measurement_time


def get_measurement_time_from_file(filename):
    """
    This function extracts the measurement time from either a Nexus or a
    Raw file. In the case of a Nexus file it queries the "end_run" entry,
    in the case of a raw file it uses the RawFileInfo algorithm to get
    the relevant dateTime.
    @param filename: the file to check
    @returns the measurement time
    """

    def get_month(month_string):
        month_conversion = {
            "JAN": "01",
            "FEB": "02",
            "MAR": "03",
            "APR": "04",
            "MAY": "05",
            "JUN": "06",
            "JUL": "07",
            "AUG": "08",
            "SEP": "09",
            "OCT": "10",
            "NOV": "11",
            "DEC": "12",
        }
        month_upper = month_string.upper()
        if month_upper in month_conversion:
            return month_conversion[month_upper]
        else:
            raise RuntimeError("Cannot get measurement time. Invalid month in Raw file: " + month_upper)

    def get_raw_measurement_time(date, time):
        """
        Takes the date and time from the raw workspace and creates the correct format
        @param date: the date part
        @param time: the time part
        """
        year = date[7 : (7 + 4)]
        day = date[0:2]
        month_string = date[3:6]
        month = get_month(month_string)

        date_and_time_string = year + "-" + month + "-" + day + "T" + time
        date_and_time = DateAndTime(date_and_time_string)
        return date_and_time.__str__().strip()

    def get_file_path(run_string):
        listOfFiles = FileFinder.findRuns(run_string)
        firstFile = listOfFiles[0]
        return firstFile

    measurement_time = ""
    filename_capital = filename.upper()

    filename_full = get_file_path(filename)

    if filename_capital.endswith(".RAW"):
        RawFileInfo(Filename=filename_full, GetRunParameters=True)
        time_id = "r_endtime"
        date_id = "r_enddate"
        file_info = mtd["Raw_RPB"]
        keys = file_info.getColumnNames()
        time = []
        date = []
        if time_id in keys:
            time = file_info.column(keys.index(time_id))
        if date_id in keys:
            date = file_info.column(keys.index(date_id))
        time = time[0]
        date = date[0]
        measurement_time = get_raw_measurement_time(date, time)
        DeleteWorkspace(file_info)
    else:
        nxs_extractor = MeasurementTimeFromNexusFileExtractor()
        measurement_time = nxs_extractor.get_measurement_time(filename_full)
    return str(measurement_time).strip()


def are_two_files_identical(file_path_1, file_path_2):
    """
    We want to make sure that two files are binary identical.
    @file_path_1: first file path
    @file_path_2: second file path
    @returns True if the files are identical else False
    """
    import filecmp

    return filecmp.cmp(file_path_1, file_path_2)


def is_valid_user_file_extension(user_file):
    """
    Check if the file name has a valid extension for user files
    @param user_file: the name of the user file
    @returns true if it is valid, else false
    """
    allowed_values = [".TXT"]
    # We need to allow for old user file formats. They started with a number.
    # But there doesn't seem to be a general format. As a very basic check
    # we make sure that the ending starts with a number
    pattern = r"^\.[0-9]+"

    filename, file_extension = os.path.splitext(user_file)
    file_extension = file_extension.upper()
    is_allowed = False
    if file_extension in allowed_values or re.match(pattern, file_extension):
        is_allowed = True
    return is_allowed


def createUnmanagedAlgorithm(name, **kwargs):
    """
    This creates an unmanged child algorithm with the
    provided properties set. The returned algorithm has
    not been executed yet.
    """
    alg = AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    alg.setChild(True)
    for key, value in kwargs.items():
        alg.setProperty(key, value)
    return alg


def extract_fit_parameters(rAnds):
    """
    @param rAnds: a rescale and shift object
    @returns a scale factor, a shift factor and a fit mode
    """
    scale_factor = rAnds.scale
    shift_factor = rAnds.shift

    if rAnds.qRangeUserSelected:
        fit_min = rAnds.qMin
        fit_max = rAnds.qMax
    else:
        fit_min = None
        fit_max = None
    # Set the fit mode
    fit_mode = None
    if rAnds.fitScale and rAnds.fitShift:
        fit_mode = "Both"
    elif rAnds.fitScale:
        fit_mode = "ScaleOnly"
    elif rAnds.fitShift:
        fit_mode = "ShiftOnly"
    else:
        fit_mode = "None"
    return scale_factor, shift_factor, fit_mode, fit_min, fit_max


def check_has_bench_rot(workspace, log_dict=None):
    if log_dict:
        run = workspace.run()
        if not run.hasProperty("Bench_Rot"):
            raise RuntimeError(
                "LARMOR Instrument: Bench_Rot does not seem to be available on {0}. There might be "
                "an issue with your data acquisition. Make sure that the sample_log entry "
                "Bench_Rot is available.".format(workspace.name())
            )


def quaternion_to_angle_and_axis(quaternion):
    """
    Converts a quaterion to an angle + an axis

    The conversion from a quaternion to an angle + axis is explained here:
    http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToAngle/
    """
    angle = 2 * math.acos(quaternion[0])
    s_parameter = math.sqrt(1 - quaternion[0] * quaternion[0])

    axis = []
    # If the angle is zero, then it does not make sense to have an axis
    if s_parameter < 1e-8:
        axis.append(quaternion[1])
        axis.append(quaternion[2])
        axis.append(quaternion[3])
    else:
        axis.append(quaternion[1] / s_parameter)
        axis.append(quaternion[2] / s_parameter)
        axis.append(quaternion[3] / s_parameter)
    return math.degrees(angle), axis


def get_unfitted_transmission_workspace_name(workspace_name):
    suffix = "_unfitted"
    if workspace_name.endswith(suffix):
        unfitted_workspace_name = workspace_name
    else:
        unfitted_workspace_name = workspace_name + suffix
    return unfitted_workspace_name


def get_user_file_name_options_with_txt_extension(user_file_name):
    """
    A user file is a .txt file. The user file can be specified without the .txt ending. Which will prevent the
    FileFinder from picking it up.

    @param user_file_name: the name of the user file
    @return: either the original user file name or a list of user file names with .txt and .TXT extensions
    """
    capitalized_user_file = user_file_name.upper()
    if capitalized_user_file.endswith(".TXT"):
        user_file_with_extension = [user_file_name]
    else:
        user_file_with_extension = [user_file_name + ".txt", user_file_name + ".TXT"]
    return user_file_with_extension


def get_correct_combinDet_setting(instrument_name, detector_selection):
    """
    We want to get the correct combinDet variable for batch reductions from a new detector selection.

    @param instrument_name: the name of the instrument
    @param detector_selection: a detector selection comes directly from the reducer
    @return: a combinedet option
    """
    if detector_selection is None:
        return None

    instrument_name = instrument_name.upper()
    # If we are dealing with LARMOR, then the correct combineDet selection is None
    if instrument_name == "LARMOR":
        return None

    detector_selection = detector_selection.upper()
    # If we are dealing with LOQ, then the correct combineDet selection is
    if instrument_name == "LOQ":
        if detector_selection == "MAIN" or detector_selection == "MAIN-DETECTOR-BANK":
            new_combine_detector_selection = "rear"
        elif detector_selection == "HAB":
            new_combine_detector_selection = "front"
        elif detector_selection == "MERGED":
            new_combine_detector_selection = "merged"
        elif detector_selection == "BOTH":
            new_combine_detector_selection = "both"
        else:
            raise RuntimeError("SANSBatchReduce: Unknown detector {0} for conversion to combineDet.".format(detector_selection))
        return new_combine_detector_selection

    # If we are dealing with SANS2D, then the correct combineDet selection is
    if instrument_name == "SANS2D":
        if detector_selection == "REAR" or detector_selection == "REAR-DETECTOR":
            new_combine_detector_selection = "rear"
        elif detector_selection == "FRONT" or detector_selection == "FRONT-DETECTOR":
            new_combine_detector_selection = "front"
        elif detector_selection == "MERGED":
            new_combine_detector_selection = "merged"
        elif detector_selection == "BOTH":
            new_combine_detector_selection = "both"
        else:
            raise RuntimeError("SANSBatchReduce: Unknown detector {0} for conversion to combineDet.".format(detector_selection))
        return new_combine_detector_selection
    raise RuntimeError("SANSBatchReduce: Unknown instrument {0}.".format(instrument_name))


class ReducedType(object):
    class LAB(object):
        pass

    class HAB(object):
        pass

    class Merged(object):
        pass


def rename_workspace_correctly(instrument_name, reduced_type, final_name, workspace):
    def get_suffix(inst_name, red_type):
        if inst_name == "SANS2D":
            if red_type is ReducedType.LAB:
                suffix = "_rear"
            elif red_type is ReducedType.HAB:
                suffix = "_front"
            elif red_type is ReducedType.Merged:
                suffix = "_merged"
            else:
                raise RuntimeError("Unknown reduction type {0}.".format(red_type))
            return suffix
        elif inst_name == "LOQ":
            if red_type is ReducedType.LAB:
                suffix = "_main"
            elif red_type is ReducedType.HAB:
                suffix = "_hab"
            elif red_type is ReducedType.Merged:
                suffix = "_merged"
            else:
                raise RuntimeError("Unknown reduction type {0}.".format(red_type))
            return suffix
        else:
            return ""

    reduced_workspace = mtd[workspace]
    workspaces_to_rename = [workspace]

    if isinstance(reduced_workspace, WorkspaceGroup):
        workspaces_to_rename += reduced_workspace.getNames()

    run_number = re.findall(r"\d+", workspace)[0] if re.findall(r"\d+", workspace) else None

    if run_number and workspace.startswith(run_number):
        for workspace_name in workspaces_to_rename:
            complete_name = workspace_name.replace(run_number, final_name + "_")
            RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=complete_name)
        return workspace.replace(run_number, final_name + "_")
    else:
        final_suffix = get_suffix(instrument_name, reduced_type)
        for workspace_name in workspaces_to_rename:
            complete_name = workspace_name.replace(workspace, final_name) + final_suffix
            RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=complete_name)
        return final_name + final_suffix


###############################################################################
######################### Start of Deprecated Code ############################
###############################################################################


# Parse a log file containing run information and return the detector positions
@deprecated
def parseLogFile(logfile):
    logkeywords = {"Rear_Det_X": 0.0, "Rear_Det_Z": 0.0, "Front_Det_X": 0.0, "Front_Det_Z": 0.0, "Front_Det_Rot": 0.0}
    if logfile is None:
        return tuple(logkeywords.values())
    file_log = open(logfile, "rU")
    for line in file_log:
        entry = line.split()[1]
        if entry in list(logkeywords.keys()):
            logkeywords[entry] = float(line.split()[2])

    return tuple(logkeywords.values())


@deprecated
def normalizePhi(phi):
    if phi > 90.0:
        phi -= 180.0
    elif phi < -90.0:
        phi += 180.0
    else:
        pass
    return phi


# Mask the inside of a cylinder


@deprecated
def MaskInsideCylinder(workspace, radius, xcentre="0.0", ycentre="0.0"):
    """Mask out the inside of a cylinder or specified radius"""
    MaskWithCylinder(workspace, radius, xcentre, ycentre, "")


# Mask the outside of a cylinder


@deprecated
def MaskOutsideCylinder(workspace, radius, xcentre="0.0", ycentre="0.0"):
    """Mask out the outside of a cylinder or specified radius"""
    MaskWithCylinder(workspace, radius, xcentre, ycentre, "#")


# Convert a mask string to a spectra list
# 6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)


@deprecated
def ConvertToSpecList(maskstring, firstspec, dimension, orientation):
    """Compile spectra ID list"""
    if maskstring == "":
        return ""
    masklist = maskstring.split(",")
    speclist = ""
    for x in masklist:
        x = x.lower()
        if "+" in x:
            bigPieces = x.split("+")
            if ">" in bigPieces[0]:
                pieces = bigPieces[0].split(">")
                low = int(pieces[0].lstrip("hv"))
                upp = int(pieces[1].lstrip("hv"))
            else:
                low = int(bigPieces[0].lstrip("hv"))
                upp = low
            if ">" in bigPieces[1]:
                pieces = bigPieces[1].split(">")
                low2 = int(pieces[0].lstrip("hv"))
                upp2 = int(pieces[1].lstrip("hv"))
            else:
                low2 = int(bigPieces[1].lstrip("hv"))
                upp2 = low2
            if "h" in bigPieces[0] and "v" in bigPieces[1]:
                ydim = abs(upp - low) + 1
                xdim = abs(upp2 - low2) + 1
                speclist += spectrumBlock(firstspec, low, low2, ydim, xdim, dimension, orientation) + ","
            elif "v" in bigPieces[0] and "h" in bigPieces[1]:
                xdim = abs(upp - low) + 1
                ydim = abs(upp2 - low2) + 1
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec, low2, low, nstrips, dimension, dimension, orientation) + ","
            else:
                print("error in mask, ignored:  " + x)
        elif ">" in x:
            pieces = x.split(">")
            low = int(pieces[0].lstrip("hvs"))
            upp = int(pieces[1].lstrip("hvs"))
            if "h" in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec, low, 0, nstrips, dimension, dimension, orientation) + ","
            elif "v" in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec, 0, low, dimension, nstrips, dimension, orientation) + ","
            else:
                for i in range(low, upp + 1):
                    speclist += str(i) + ","
        elif "h" in x:
            speclist += spectrumBlock(firstspec, int(x.lstrip("h")), 0, 1, dimension, dimension, orientation) + ","
        elif "v" in x:
            speclist += spectrumBlock(firstspec, 0, int(x.lstrip("v")), dimension, 1, dimension, orientation) + ","
        else:
            speclist += x.lstrip("s") + ","

    return speclist


# Mask by detector number


@deprecated
def MaskBySpecNumber(workspace, speclist):
    speclist = speclist.rstrip(",")
    if speclist == "":
        return ""
    MaskDetectors(Workspace=workspace, SpectraList=speclist, ForceInstrumentMasking=True)


@deprecated
def SetupTransmissionWorkspace(inputWS, spec_list, backmon_start, backmon_end, wavbining, interpolate, loqremovebins):
    tmpWS = inputWS + "_tmp"
    CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmpWS, StartWorkspaceIndex=0, EndWorkspaceIndex=2)

    if loqremovebins:
        RemoveBins(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, XMin=19900, XMax=20500, Interpolation="Linear")
    if backmon_start is not None and backmon_end is not None:
        CalculateFlatBackground(
            InputWorkspace=tmpWS, OutputWorkspace=tmpWS, StartX=backmon_start, EndX=backmon_end, WorkspaceIndexList=spec_list, Mode="Mean"
        )

    # Convert and rebin
    ConvertUnits(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Target="Wavelength")

    if interpolate:
        InterpolatingRebin(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Params=wavbining)
    else:
        Rebin(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Params=wavbining)

    return tmpWS


# Setup the transmission workspace
# Correct of for the volume of the sample/can. Dimensions should be in order: width, height, thickness


@deprecated
def ScaleByVolume(inputWS, scalefactor, geomid, width, height, thickness):
    # Divide by the area
    if geomid == 1:
        # Volume = circle area * height
        # Factor of four comes from radius = width/2
        scalefactor /= height * math.pi * math.pow(width, 2) / 4.0
    elif geomid == 2:
        scalefactor /= width * height * thickness
    else:
        # Factor of four comes from radius = width/2
        scalefactor /= thickness * math.pi * math.pow(width, 2) / 4.0
    # Multiply by the calculated correction factor
    ws = mtd[inputWS]
    ws *= scalefactor


@deprecated
def StripEndZeroes(workspace, flag_value=0.0):
    result_ws = mtd[workspace]
    y_vals = result_ws.readY(0)
    length = len(y_vals)
    # Find the first non-zero value
    start = 0
    for i in range(0, length):
        if y_vals[i] != flag_value:
            start = i
            break
        # Now find the last non-zero value
    stop = 0
    length -= 1
    for j in range(length, 0, -1):
        if y_vals[j] != flag_value:
            stop = j
            break
        # Find the appropriate X values and call CropWorkspace
    x_vals = result_ws.readX(0)
    startX = x_vals[start]
    # Make sure we're inside the bin that we want to crop
    endX = 1.001 * x_vals[stop + 1]
    CropWorkspace(InputWorkspace=workspace, OutputWorkspace=workspace, XMin=startX, XMax=endX)


@deprecated
class Orientation(object):
    Horizontal = 1
    Vertical = 2
    Rotated = 3


# A small class holds the run number with the workspace name, because the run number is not contained in the workspace at the moment


@deprecated
class WorkspaceDetails(object):
    def __init__(self, name, run_number):
        self._name = name
        run_number = str(run_number).split("-add")[0]
        self._run_number = int(run_number)

    def getName(self):
        return self._name

    def getRunNumber(self):
        return self._run_number

    def reset(self):
        self._name = ""
        self._run_number = -1


# A small class to collect together run information, used to save passing tuples around


@deprecated
class RunDetails(object):
    def __init__(self, raw_ws, final_ws, trans_raw, direct_raw, maskpt_rmin, maskpt_rmax, suffix):
        self._rawworkspace = raw_ws
        self._finalws = final_ws
        self._trans_raw = trans_raw
        self._direct_raw = direct_raw
        self._maskrmin = maskpt_rmin
        self._maskrmax = maskpt_rmax
        self._suffix = suffix

    def getRawWorkspace(self):
        return self._rawworkspace

    def getReducedWorkspace(self):
        return self._finalws

    def setReducedWorkspace(self, wsName):
        self._finalws = wsName

    def getTransRaw(self):
        return self._trans_raw

    def getDirectRaw(self):
        return self._direct_raw

    def getMaskPtMin(self):
        return self._maskrmin

    def setMaskPtMin(self, rmin):
        self._maskrmin = rmin

    def getMaskPtMax(self):
        return self._maskrmax

    def setMaskPtMax(self, rmax):
        self._maskrmax = rmax

    def getSuffix(self):
        return self._suffix


###############################################################################
########################## End of Deprecated Code #############################
###############################################################################


if __name__ == "__main__":
    pass
