#pylint: disable=invalid-name
#########################################################
# This module contains utility functions common to the
# SANS data reduction scripts
########################################################
from mantid.simpleapi import *
from mantid.api import IEventWorkspace, MatrixWorkspace, WorkspaceGroup
import mantid
from mantid.kernel import time_duration
import inspect
import math
import os
import re
import types

sanslog = Logger("SANS")

ADDED_EVENT_DATA_TAG = '_added_event_data'

REG_DATA_NAME = '-add' + ADDED_EVENT_DATA_TAG + '[_1-9]*$'
REG_DATA_MONITORS_NAME = '-add_monitors' + ADDED_EVENT_DATA_TAG + '[_1-9]*$'

ZERO_ERROR_DEFAULT = 1e6

INCIDENT_MONITOR_TAG = '_incident_monitor'

def deprecated(obj):
    """
    Decorator to apply to functions or classes that we think are not being (or
    should not be) used anymore.  Prints a warning to the log.
    """
    if inspect.isfunction(obj) or inspect.ismethod(obj):
        if inspect.isfunction(obj):
            obj_desc = "\"%s\" function" % obj.__name__
        else:
            obj_desc = "\"%s\" class" % obj.im_class.__name__
        def print_warning_wrapper(*args, **kwargs):
            sanslog.warning("The %s has been marked as deprecated and may be "\
                            "removed in a future version of Mantid.  If you "\
                            "believe this to have been marked in error, please "\
                            "contact the member of the Mantid team responsible "\
                            "for ISIS SANS." % obj_desc)
            return obj(*args, **kwargs)
        return print_warning_wrapper

    # Add a @deprecated decorator to each of the member functions in the class
    # (by recursion).
    if inspect.isclass(obj):
        for name, fn in inspect.getmembers(obj):
            if isinstance(fn, types.UnboundMethodType):
                setattr(obj, name, deprecated(fn))
        return obj

    assert False, "Programming error.  You have incorrectly applied the "\
                  "@deprecated decorator.  This is only for use with functions "\
                  "or classes."

def GetInstrumentDetails(instrum):
    """
        Return the details specific to the instrument's current detector bank
        @return number of pixels ac, first spectrum in the current detector, its last spectrum
    """
    det = instrum.cur_detector()
    #LOQ HAB is not a square detector and so has no width
    #for backwards compatibility we have to return a width
    if instrum.name() == 'LOQ' and instrum.cur_detector().name() == 'HAB':
        if det.n_columns is None :
            return 128, det.get_first_spec_num(), det.last_spec_num

    first_spectrum = det.get_first_spec_num()
    last_spectrum = det.last_spec_num
    if instrum.name() == 'SANS2D':
        first_spectrum += 4
        last_spectrum += 4

    return det.n_columns, first_spectrum, last_spectrum

def InfinitePlaneXML(id_name, plane_pt, normal_pt):
    return '<infinite-plane id="' + str(id_name) + '">' + \
        '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
        '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
        '</infinite-plane>'

def InfiniteCylinderXML(id_name, centre, radius, axis):
    return  '<infinite-cylinder id="' + str(id_name) + '">' + \
    '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
    '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
    '<radius val="' + str(radius) + '" />' + \
    '</infinite-cylinder>\n'

# Mask a cylinder, specifying the algebra to use
def MaskWithCylinder(workspace, radius, xcentre, ycentre, algebra):
    '''Mask a cylinder on the input workspace.'''
    xmldef = InfiniteCylinderXML('shape', [xcentre, ycentre, 0.0], radius, [0,0,1])
    xmldef += '<algebra val="' + algebra + 'shape" />'
    # Apply masking
    MaskDetectorsInShape(Workspace=workspace,ShapeXML=xmldef)

# Mask such that the remainder is that specified by the phi range
def LimitPhi(workspace, centre, phimin, phimax, use_mirror=True):
    # convert all angles to be between 0 and 360
    while phimax > 360 :
        phimax -= 360
    while phimax < 0 :
        phimax += 360
    while phimin > 360 :
        phimin -= 360
    while phimin < 0 :
        phimin += 360
    while phimax<phimin :
        phimax += 360

    #Convert to radians
    phimin = math.pi*phimin/180.0
    phimax = math.pi*phimax/180.0
    xmldef =  InfinitePlaneXML('pla',centre, [math.cos(-phimin + math.pi/2.0),math.sin(-phimin + math.pi/2.0),0]) + \
    InfinitePlaneXML('pla2',centre, [-math.cos(-phimax + math.pi/2.0),-math.sin(-phimax + math.pi/2.0),0]) + \
    InfinitePlaneXML('pla3',centre, [math.cos(-phimax + math.pi/2.0),math.sin(-phimax + math.pi/2.0),0]) + \
    InfinitePlaneXML('pla4',centre, [-math.cos(-phimin + math.pi/2.0),-math.sin(-phimin + math.pi/2.0),0])

    if use_mirror :
        xmldef += '<algebra val="#((pla pla2):(pla3 pla4))" />'
    else:
        #the formula is different for acute verses obstruse angles
        if phimax-phimin > math.pi :
            # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
            xmldef += '<algebra val="#(pla:pla2)" />'
        else :
            # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
            xmldef += '<algebra val="#(pla pla2)" />'

    MaskDetectorsInShape(Workspace=workspace,ShapeXML= xmldef)

# Work out the spectra IDs for block of detectors
def spectrumBlock(base, ylow, xlow, ydim, xdim, det_dimension, orientation):
    '''Compile a list of spectrum IDs for rectangular block of size xdim by ydim'''
    output = ''
    if orientation == Orientation.Horizontal:
        start_spec = base + ylow*det_dimension + xlow
        for y in range(0, ydim):
            for x in range(0, xdim):
                output += str(start_spec + x + (y*det_dimension)) + ','
    elif orientation == Orientation.Vertical:
        start_spec = base + xlow*det_dimension + ylow
        for x in range(det_dimension - 1, det_dimension - xdim-1,-1):
            for y in range(0, ydim):
                std_i = start_spec + y + ((det_dimension-x-1)*det_dimension)
                output += str(std_i ) + ','
    elif orientation == Orientation.Rotated:
        # This is the horizontal one rotated so need to map the xlow and vlow to their rotated versions
        start_spec = base + ylow*det_dimension + xlow
        max_spec = det_dimension*det_dimension + base - 1
        for y in range(0, ydim):
            for x in range(0, xdim):
                std_i = start_spec + x + (y*det_dimension)
                output += str(max_spec - (std_i - base)) + ','

    return output.rstrip(",")

# Mask by bin range
def MaskByBinRange(workspace, timemask):
    # timemask should be a ';' separated list of start/end values
    ranges = timemask.split(';')
    for r in ranges:
        limits = r.split()
        if len(limits) == 2:
            MaskBins(InputWorkspace=workspace,OutputWorkspace= workspace, XMin= limits[0] ,XMax=limits[1])

def QuadrantXML(centre,rmin,rmax,quadrant):
    cin_id = 'cyl-in'
    xmlstring = InfiniteCylinderXML(cin_id, centre, rmin, [0,0,1])
    cout_id = 'cyl-out'
    xmlstring+= InfiniteCylinderXML(cout_id, centre, rmax, [0,0,1])
    plane1Axis=None
    plane2Axis=None
    if quadrant == 'Left':
        plane1Axis = [-1,1,0]
        plane2Axis = [-1,-1,0]
    elif quadrant == 'Right':
        plane1Axis = [1,-1,0]
        plane2Axis = [1,1,0]
    elif quadrant == 'Up':
        plane1Axis = [1,1,0]
        plane2Axis = [-1,1,0]
    elif quadrant == 'Down':
        plane1Axis = [-1,-1,0]
        plane2Axis = [1,-1,0]
    else:
        return ''
    p1id = 'pl-a'
    xmlstring += InfinitePlaneXML(p1id, centre, plane1Axis)
    p2id = 'pl-b'
    xmlstring += InfinitePlaneXML(p2id, centre, plane2Axis)
    xmlstring += '<algebra val="(#((#(' + cout_id + ':(#' + cin_id  + '))) ' + p1id + ' ' + p2id + '))"/>\n'
    return xmlstring

def getWorkspaceReference(ws_pointer):
    if isinstance(ws_pointer, str):
        ws_pointer = mtd[ws_pointer]
    if str(ws_pointer) not in mtd:
        raise RuntimeError("Invalid workspace name input: " + str(ws_pointer))
    return ws_pointer

def isEventWorkspace(ws_reference):
    return isinstance(getWorkspaceReference(ws_reference),IEventWorkspace)

def getBinsBoundariesFromWorkspace(ws_reference):
    ws_reference = getWorkspaceReference(ws_reference)
    Xvalues = ws_reference.dataX(0)
    binning = str(Xvalues[0])
    binGap = Xvalues[1] - Xvalues[0]
    binning = binning + ',' + str(binGap)
    for j in range(2, len(Xvalues)):
        nextBinGap = Xvalues[j] - Xvalues[j-1]
        if nextBinGap != binGap:
            binGap = nextBinGap
            binning = binning + ',' + str(Xvalues[j-1]) + ',' + str(binGap)
    binning = binning + "," + str(Xvalues[-1])
    return binning

def getFilePathFromWorkspace(ws):
    ws_pointer = getWorkspaceReference(ws)
    file_path = None

    try:
        for hist in ws_pointer.getHistory():
            try:
                if 'Load' in hist.name():
                    file_path = hist.getPropertyValue('Filename')
            except:
                pass
    except:
        try:
            hist = ws_pointer.getHistory().lastAlgorithm()
            file_path = hist.getPropertyValue('Filename')
        except:
            raise RuntimeError("Failed while looking for file in workspace: " + str(ws))

    if not file_path:
        raise RuntimeError("Can not find the file name for workspace " + str(ws))
    return file_path

def fromEvent2Histogram(ws_event, ws_monitor, binning = ""):
    """Transform an event mode workspace into a histogram workspace.
    It does conjoin the monitor and the workspace as it is expected from the current
    SANS data inside ISIS.

    A non-empty binning string will specify a rebin param list to use instead of using
    the binning of the monitor ws.

    Finally, it copies the parameter map from the workspace to the resulting histogram
    in order to preserve the positions of the detectors components inside the workspace.

    It will finally, replace the input workspace with the histogram equivalent workspace.
    """
    assert ws_monitor != None

    name = '__monitor_tmp'

    if binning != "":
        aux_hist = Rebin(ws_event, binning, False)
        Rebin(ws_monitor, binning, False, OutputWorkspace=name)
    else:
        aux_hist = RebinToWorkspace(ws_event, ws_monitor, False)
        ws_monitor.clone(OutputWorkspace=name)

    ConjoinWorkspaces(name, aux_hist, CheckOverlapping=True)
    CopyInstrumentParameters(ws_event, OutputWorkspace=name)

    ws_hist = RenameWorkspace(name, OutputWorkspace=str(ws_event))

    return ws_hist

def getChargeAndTime(ws_event):
    r = ws_event.getRun()
    charges = r.getLogData('proton_charge')
    total_charge = sum(charges.value)
    time_passed = (charges.times[-1] - charges.times[0]).total_microseconds()
    time_passed /= 1e6
    return total_charge, time_passed

def sliceByTimeWs(ws_event, time_start=None, time_stop=None):
    def formatTime(time_val):
        return "_T%.1f" % time_val
    params = dict()
    outname=str(ws_event)
    if time_start:
        outname +=formatTime(time_start)
        params['StartTime'] = time_start
    if time_stop:
        outname += formatTime(time_stop)
        params['StopTime'] = time_stop

    params['OutputWorkspace'] = outname
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
        raise RuntimeError("The workspace "+str(ws_event)+ " is not a valid Event workspace")

    tot_c, tot_t = getChargeAndTime(ws_event)

    if (time_start == -1) and (time_stop == -1):
        hist = fromEvent2Histogram(ws_event, monitor, binning)
        return hist, (tot_t, tot_c, tot_t, tot_c)

    if time_start == -1:
        time_start = 0.0
    if time_stop == -1:
        time_stop = tot_t+0.001

    sliced_ws = sliceByTimeWs(ws_event, time_start, time_stop)
    sliced_ws = RenameWorkspace(sliced_ws, OutputWorkspace=ws_event.name())

    part_c, part_t = getChargeAndTime(sliced_ws)
    scaled_monitor = monitor * (part_c/tot_c)


    hist = fromEvent2Histogram(sliced_ws, scaled_monitor, binning)
    return hist, (tot_t, tot_c, part_t, part_c)


def sliceParser(str_to_parser):
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
    num_pat = r'(\d+(?:\.\d+)?(?:[eE][+-]\d+)?)' # float without sign
    slice_pat = num_pat + r'-' + num_pat
    lowbound = '>'+num_pat
    upbound = '<'+num_pat
    sss_pat = num_pat+r':'+num_pat+r':'+num_pat
    exception_pattern = 'Invalid input for Slicer: %s'
    MARK = -1

    def _check_match(inpstr, patternstr, qtde_nums):
        match = re.match(patternstr, inpstr)
        if match:
            answer = match.groups()
            if len(answer) != qtde_nums:
                raise SyntaxError(exception_pattern %(inpstr))
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

    parts = str_to_parser.split(',')
    result = []
    for inps in parts:
        inps = inps.replace(' ','')
        aux_res = _extract_simple_input(inps)
        if aux_res:
            result.append(aux_res)
            continue
        aux_res = _extract_composed_input(inps)
        if aux_res:
            result += aux_res
            continue
        raise SyntaxError('Invalid input '+ str_to_parser +'. Failed caused by this term:'+inps)

    return result

def getFileAndName(incomplete_path):
    this_path = FileFinder.getFullPath(incomplete_path)
    if not this_path:
        # do not catch exception, let it goes.
        this_path = FileFinder.findRuns(incomplete_path)
        # if list, get first value
        if hasattr(this_path, '__iter__'):
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
    if not current_range in ranges:
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
    LoadMask(
        Instrument=idf_path,
        InputFile=mask_file_path,
        OutputWorkspace=mask_ws_name)
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

    MaskDetectors(Workspace=ws, DetectorList=masked_det_ids)


def check_child_ws_for_name_and_type_for_added_eventdata(wsGroup):
    '''
    Ensure that the while loading added event data, we are dealing with
    1. The correct naming convention. For event data this is the run number,
       an add tag and possibly underscores and numbers when the same workspace
       is reloaded. For monitor data it is the run number, an add tag, a monitor
       tag and the possibly underscores and numbers when the same workspace is
       reloaded
    2. The correct workspace types.
    @param wsGroup ::  workspace group.
    '''
    hasData = False
    hasMonitors = False

    # Check if there are only two children in the group workspace
    if len(wsGroup) != 2:
        return False

    assert isinstance(wsGroup, WorkspaceGroup)

    for index in range(len(wsGroup)):
        childWorkspace = wsGroup.getItem(index)
        if re.search(REG_DATA_NAME, childWorkspace.getName()):
            if isinstance(childWorkspace, IEventWorkspace):
                hasData = True
        elif re.search(REG_DATA_MONITORS_NAME, childWorkspace.getName()):
            if isinstance(childWorkspace, MatrixWorkspace):
                hasMonitors = True

    return hasData and hasMonitors

def extract_child_ws_for_added_eventdata(ws_group, appendix):
    '''
    Extract the the child workspaces from a workspace group which was
    created by adding event data. The workspace group must contains a data
    workspace which is an EventWorkspace and a monitor workspace which is a
    matrix workspace.
    @param ws_group :: workspace group.
    @param appendix :: what to append to the names of the child workspaces
    '''
    # Store the name of the group workspace in a string
    ws_group_name = ws_group.getName()

    # Get a handle on each child workspace
    ws_handles = []
    for index in range(len(ws_group)):
        ws_handles.append(ws_group.getItem(index))

    if len(ws_handles) != 2:
        raise RuntimeError("Expected two child workspaces when loading added event data."
                           "Please make sure that you have loaded added event data which was generated by the Add tab of the SANS Gui."
                          )

    # Now ungroup the group
    UnGroupWorkspace(ws_group)

    # Rename the child workspaces to be of the expected format. (see _get_workspace_name in sans_reduction_steps)
    for ws_handle in ws_handles:
        # Check if the child is an event data workspace or a monitor workspace
        if appendix in ws_handle.getName():
            new_name = ws_group_name + appendix
            RenameWorkspace(InputWorkspace = ws_handle.getName(), OutputWorkspace = new_name)
        else:
            new_name = ws_group_name
            RenameWorkspace(InputWorkspace = ws_handle.getName(), OutputWorkspace = new_name)

def bundle_added_event_data_as_group(out_file_name, out_file_monitors_name):
    """
    We load an added event data file and its associated monitor file. Combine
    the data in a group workspace and delete the original files.
    @param out_file_name :: the file name of the event data file
    @param out_file_monitors_name :: the file name of the monitors file
    @return the name fo the new group workspace file
    """
    # Extract the file name and the extension
    file_name, file_extension = os.path.splitext(out_file_name)

    event_data_temp = file_name + ADDED_EVENT_DATA_TAG
    Load(Filename = out_file_name, OutputWorkspace = event_data_temp)
    event_data_ws = mtd[event_data_temp]

    monitor_temp = file_name + '_monitors' + ADDED_EVENT_DATA_TAG
    Load(Filename = out_file_monitors_name, OutputWorkspace = monitor_temp)

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
    GroupWorkspaces(InputWorkspaces = [event_data_ws, monitor_ws], OutputWorkspace = out_group_ws_name)
    group_ws = mtd[out_group_ws_name]

    # Save the group
    SaveNexusProcessed(InputWorkspace = group_ws, Filename = out_group_file_name, Append=False)
    # Delete the files and the temporary workspaces
    DeleteWorkspace(event_data_ws)
    DeleteWorkspace(monitor_ws)

    return out_group_file_name

def get_full_path_for_added_event_data(file_name):
    path,base = os.path.split(file_name)
    if path == '' or base not in os.listdir(path):
        path = config['defaultsave.directory'] + path
        # If the path is still an empty string check in the current working directory
        if path == '':
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
    ExtractSpectra(InputWorkspace=ws,OutputWorkspace=output_ws_name, DetectorList=det_ids)
    return mtd[output_ws_name]

def get_masked_det_ids(ws):
    """
    Given a workspace, will return a list of all the IDs that correspond to
    detectors that have been masked.

    @param ws :: the workspace to extract the det IDs from

    @returns a list of IDs for masked detectors
    """
    for ws_index in range(ws.getNumberHistograms()):
        try:
            det = ws.getDetector(ws_index)
        except RuntimeError:
            # Skip the rest after finding the first spectra with no detectors,
            # which is a big speed increase for SANS2D.
            break
        if det.isMasked():
            yield det.getID()
def create_zero_error_free_workspace(input_workspace_name, output_workspace_name):
    '''
    Creates a cloned workspace where all zero-error values have been replaced with a large value
    @param input_workspace_name :: The input workspace name
    @param output_workspace_name :: The output workspace name
    @returns a message and a completion flag
    '''
    # Load the input workspace
    message = ""
    complete = False
    if not input_workspace_name in mtd:
        message = 'Failed to create a zero error free cloned workspace: The input workspace does not seem to exist.'
        return message, complete

    # Create a cloned workspace
    ws_in = mtd[input_workspace_name]

    # Remove all zero errors from the cloned workspace
    CloneWorkspace(InputWorkspace=ws_in, OutputWorkspace=output_workspace_name)
    if not output_workspace_name in mtd:
        message = 'Failed to create a zero error free cloned workspace: A clone could not be created.'
        return message, complete

    ws_out = mtd[output_workspace_name]
    try:
        remove_zero_errors_from_workspace(ws_out)
        complete = True
    except ValueError:
        DeleteWorkspace(Workspace=output_workspace_name)
        message = 'Failed to create a zero error free cloned workspace: Could not remove the zero errors.'

    return message, complete

def remove_zero_errors_from_workspace(ws):
    '''
    Removes the zero errors from a Matrix workspace
    @param ws :: The input workspace
    '''
    # Make sure we are dealing with a MatrixWorkspace
    if not isinstance(ws, MatrixWorkspace) or isinstance(ws,IEventWorkspace):
        raise ValueError('Cannot remove zero errors from a workspace which is not of type MatrixWorkspace.')
    # Iterate over the workspace and replace the zero values with a large default value
    numSpectra = ws.getNumberHistograms()
    errors = ws.dataE
    for index in range(0,numSpectra):
        spectrum = errors(index)
        spectrum[spectrum <= 0.0] = ZERO_ERROR_DEFAULT

def delete_zero_error_free_workspace(input_workspace_name):
    '''
    Deletes the zero-error free workspace
    @param ws :: The input workspace
    '''
    complete = False
    message = ""
    if input_workspace_name in mtd:
        DeleteWorkspace(Workspace=input_workspace_name)
        complete = True
    else:
        message = 'Failed to delete a zero-error free workspace'
    return message, complete

def is_valid_ws_for_removing_zero_errors(input_workspace_name):
    '''
    Check if a workspace has been created via Q1D or Qxy.
    @param ws :: The input workspace
    '''
    isValid = False
    message = ""

    ws = mtd[input_workspace_name]
    workspaceHistory= ws.getHistory()
    histories = workspaceHistory.getAlgorithmHistories()
    for history in histories:
        name = history.name()
        if name == 'Q1D' or name == 'Qxy':
            isValid = True
            break

    if not isValid:
        message = ("Workspace does not seem valid for zero error removal."
                   "It must have been reduced with Q1D or Qxy."
                  )

    return message, isValid

class AddOperation(object):
    """
    The AddOperation allows to add two workspaces at a time.
    """
    def __init__(self,isOverlay, time_shifts):
        """
        The AddOperation requires to know if the workspaces are to
        be plainly added or to be overlayed. Additional time shifts can be
        specified
        @param isOverlay :: true if the operation is an overlay operation
        @param time_shifts :: a string with comma-separted time shift values
        """
        super(AddOperation, self).__init__()
        factory = CombineWorkspacesFactory()
        self.adder = factory.create_add_algorithm(isOverlay)
        self.time_shifter = TimeShifter(time_shifts)

    def add(self, LHS_workspace, RHS_workspace, output_workspace, run_to_add):
        """
        Add two workspaces together and place the result into the outputWorkspace.
        The user needs to specify which run is being added in order to determine
        the correct time shift
        @param LHS_workspace :: first workspace, this workspace is a reference workspace
                                and hence never shifted
        @param RHS_workspace :: second workspace which can be shifted in time
        @param run_to_add :: the number of the nth added workspace
        """
        current_time_shift = self.time_shifter.get_Nth_time_shift(run_to_add)
        self.adder.add(LHS_workspace=LHS_workspace,
                       RHS_workspace= RHS_workspace,
                       output_workspace= output_workspace,
                       time_shift = current_time_shift)

class CombineWorkspacesFactory(object):
    """
    Factory to determine how to add workspaces
    """
    def __init__(self):
        super(CombineWorkspacesFactory, self).__init__()
    def create_add_algorithm(self, isOverlay):
        """
        @param isOverlay :: if true we provide the OverlayWorkspaces functionality
        """
        if isOverlay:
            return OverlayWorkspaces()
        else:
            return PlusWorkspaces()

class PlusWorkspaces(object):
    """
    Wrapper for the Plus algorithm
    """
    def __init__(self):
        super(PlusWorkspaces, self).__init__()

    def add(self, LHS_workspace, RHS_workspace, output_workspace, time_shift = 0.0):
        """
        @param LHS_workspace :: the first workspace
        @param RHS_workspace :: the second workspace
        @param output_workspace :: the output workspace
        @param time_shift :: unused parameter
        """
        dummy_shift = time_shift
        Plus(LHSWorkspace=LHS_workspace,RHSWorkspace= RHS_workspace,OutputWorkspace= output_workspace)

class OverlayWorkspaces(object):
    """
    Overlays (in time) a workspace  on top of another workspace. The two
    workspaces overlayed such that the first time entry of their proton_charge entry matches.
    This overlap can be shifted by the specified time_shift in seconds
    """
    def __init__(self):
        super(OverlayWorkspaces, self).__init__()

    def add(self, LHS_workspace, RHS_workspace, output_workspace, time_shift = 0.0):
        """
        @param LHS_workspace :: the first workspace
        @param RHS_workspace :: the second workspace
        @param output_workspace :: the output workspace
        @param time_shift :: an additional time shift for the overlay procedure
        """
        rhs_ws = self._get_workspace(RHS_workspace)
        lhs_ws = self._get_workspace(LHS_workspace)
        # Find the time difference between LHS and RHS workspaces and add optional time shift
        time_difference = self._extract_time_difference_in_seconds(lhs_ws, rhs_ws)
        total_time_shift = time_difference + time_shift

        # Create a temporary workspace with shifted time values from RHS, if the shift is necesary
        temp = rhs_ws
        temp_ws_name = 'shifted'
        if total_time_shift != 0.0:
            temp = ChangeTimeZero(InputWorkspace=rhs_ws, OutputWorkspace=temp_ws_name, RelativeTimeOffset=total_time_shift)

        # Add the LHS and shifted workspace
        Plus(LHSWorkspace=LHS_workspace,RHSWorkspace= temp ,OutputWorkspace= output_workspace)

        # Remove the shifted workspace
        if mtd.doesExist(temp_ws_name):
            mtd.remove(temp_ws_name)

    def _extract_time_difference_in_seconds(self, ws1, ws2):
        # The times which need to be compared are the first entry in the proton charge log
        time_1 = self._get_time_from_proton_charge_log(ws1)
        time_2 = self._get_time_from_proton_charge_log(ws2)

        return time_duration.total_nanoseconds(time_1- time_2)/1e9

    def _get_time_from_proton_charge_log(self, ws):
        times = ws.getRun().getProperty("proton_charge").times
        if len(times) == 0:
            raise ValueError("The proton charge does not have any time entry")
        return times[0]

    def _get_workspace(self, workspace):
        if isinstance(workspace, MatrixWorkspace):
            return workspace
        elif isinstance(workspace, basestring) and mtd.doesExist(workspace):
            return mtd[workspace]

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
        @param n :: the nth addition
        """
        if len(self._time_shifts) >= (n+1):
            return self._cast_to_float(self._time_shifts[n])
        else:
            return 0.0
    def _cast_to_float(self, element):
        float_element = 0.0
        try:
            float_element = float(element)
        except ValueError:
            pass# Log here
        return float_element

def load_monitors_for_multiperiod_event_data(workspace, data_file, monitor_appendix):
    '''
    Takes a multi-period event workspace and loads the monitors
    as a group workspace
    @param workspace: Multi-period event workspace
    @param data_file: The data file
    @param monitor_appendix: The appendix for monitor data
    '''
    # Load all monitors
    mon_ws_group_name = "temp_ws_group"
    LoadNexusMonitors(Filename=data_file, OutputWorkspace=mon_ws_group_name)
    mon_ws = mtd["temp_ws_group"]
    # Rename all monitor workspces
    rename_monitors_for_multiperiod_event_data(monitor_workspace = mon_ws, workspace=workspace, appendix=monitor_appendix)

def rename_monitors_for_multiperiod_event_data(monitor_workspace, workspace, appendix):
    '''
    Takes a multi-period event workspace and loads the monitors
    as a group workspace
    @param workspace: Multi-period event workspace
    @param data_file: The data file
    @param monitor_appendix: The appendix for monitor data
    '''
    if len(monitor_workspace) != len(workspace):
        raise RuntimeError("The workspace and monitor workspace lengths do not match.")
    for index in range(0,len(monitor_workspace)):
        monitor_name = workspace[index].name() + appendix
        RenameWorkspace(InputWorkspace=monitor_workspace[index], OutputWorkspace=monitor_name)
    # Finally rename the group workspace
    monitor_group_ws_name = workspace.name() + appendix
    RenameWorkspace(InputWorkspace=monitor_workspace, OutputWorkspace=monitor_group_ws_name)

def is_convertible_to_int(input_value):
    '''
    Check if the input can be converted to int
    @param input_value :: a general input
    '''
    try:
        dummy_converted = int(input_value)
    except ValueError:
        return False
    return True

def is_convertible_to_float(input_value):
    '''
    Check if the input can be converted to float
    @param input_value :: a general input
    '''
    try:
        dummy_converted = float(input_value)
    except ValueError:
        return False
    return True

def is_valid_xml_file_list(input_value):
    '''
    Check if the input is a valid xml file list. We only check
    the form and not the existence of the file
    @param input :: a list input
    '''
    if not isinstance(input_value, list) or not input or len(input_value) == 0:
        return False
    for element in input_value:
        if not isinstance(element, str) or not element.endswith('.xml'):
            return False
    return True

def convert_from_string_list(to_convert):
    '''
    Convert a Python string list to a comma-separted string
    @param to_convert :: a string list input
    '''
    return ','.join(element.replace(" ", "") for element in to_convert)

def convert_to_string_list(to_convert):
    '''
    Convert a comma-separted string to a Python string list in a string form
    "file1.xml, file2.xml" -> "['file1.xml','file2.xml']"
    @param to_convert :: a comma-spearated string
    '''
    string_list = to_convert.replace(" ", "").split(",")
    output_string = "[" + ','.join("'"+element+"'" for element in string_list) + "]"
    return output_string

###############################################################################
######################### Start of Deprecated Code ############################
###############################################################################

# Parse a log file containing run information and return the detector positions
@deprecated
def parseLogFile(logfile):
    logkeywords = {'Rear_Det_X':0.0, 'Rear_Det_Z':0.0, 'Front_Det_X':0.0, 'Front_Det_Z':0.0, \
        'Front_Det_Rot':0.0}
    if logfile == None:
        return tuple(logkeywords.values())
    file_log = open(logfile, 'rU')
    for line in file_log:
        entry = line.split()[1]
        if entry in logkeywords.keys():
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
def MaskInsideCylinder(workspace, radius, xcentre = '0.0', ycentre = '0.0'):
    '''Mask out the inside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, xcentre, ycentre, '')

# Mask the outside of a cylinder
@deprecated
def MaskOutsideCylinder(workspace, radius, xcentre = '0.0', ycentre = '0.0'):
    '''Mask out the outside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, xcentre, ycentre, '#')

# Convert a mask string to a spectra list
# 6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)
@deprecated
def ConvertToSpecList(maskstring, firstspec, dimension, orientation):
    '''Compile spectra ID list'''
    if maskstring == '':
        return ''
    masklist = maskstring.split(',')
    speclist = ''
    for x in masklist:
        x = x.lower()
        if '+' in x:
            bigPieces = x.split('+')
            if '>' in bigPieces[0]:
                pieces = bigPieces[0].split('>')
                low = int(pieces[0].lstrip('hv'))
                upp = int(pieces[1].lstrip('hv'))
            else:
                low = int(bigPieces[0].lstrip('hv'))
                upp = low
            if '>' in bigPieces[1]:
                pieces = bigPieces[1].split('>')
                low2 = int(pieces[0].lstrip('hv'))
                upp2 = int(pieces[1].lstrip('hv'))
            else:
                low2 = int(bigPieces[1].lstrip('hv'))
                upp2 = low2
            if 'h' in bigPieces[0] and 'v' in bigPieces[1]:
                ydim=abs(upp-low)+1
                xdim=abs(upp2-low2)+1
                speclist += spectrumBlock(firstspec,low, low2,ydim, xdim, dimension,orientation) + ','
            elif 'v' in bigPieces[0] and 'h' in bigPieces[1]:
                xdim=abs(upp-low)+1
                ydim=abs(upp2-low2)+1
                speclist += spectrumBlock(firstspec,low2, low,nstrips, dimension, dimension,orientation)+ ','
            else:
                print "error in mask, ignored:  " + x
        elif '>' in x:
            pieces = x.split('>')
            low = int(pieces[0].lstrip('hvs'))
            upp = int(pieces[1].lstrip('hvs'))
            if 'h' in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec,low, 0,nstrips, dimension, dimension,orientation)  + ','
            elif 'v' in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec,0,low, dimension, nstrips, dimension,orientation)  + ','
            else:
                for i in range(low, upp + 1):
                    speclist += str(i) + ','
        elif 'h' in x:
            speclist += spectrumBlock(firstspec,int(x.lstrip('h')), 0,1, dimension, dimension,orientation) + ','
        elif 'v' in x:
            speclist += spectrumBlock(firstspec,0,int(x.lstrip('v')), dimension, 1, dimension,orientation) + ','
        else:
            speclist += x.lstrip('s') + ','

    return speclist

# Mask by detector number
@deprecated
def MaskBySpecNumber(workspace, speclist):
    speclist = speclist.rstrip(',')
    if speclist == '':
        return ''
    MaskDetectors(Workspace=workspace, SpectraList = speclist)

@deprecated
def SetupTransmissionWorkspace(inputWS, spec_list, backmon_start, backmon_end, wavbining, interpolate, loqremovebins):
    tmpWS = inputWS + '_tmp'
    CropWorkspace(InputWorkspace=inputWS,OutputWorkspace=tmpWS, StartWorkspaceIndex=0, EndWorkspaceIndex=2)

    if loqremovebins == True:
        RemoveBins(InputWorkspace=tmpWS,OutputWorkspace=tmpWS,XMin= 19900,XMax= 20500, Interpolation='Linear')
    if backmon_start != None and backmon_end != None:
        CalculateFlatBackground(InputWorkspace=tmpWS,OutputWorkspace= tmpWS, StartX = backmon_start, EndX = backmon_end, WorkspaceIndexList = spec_list, Mode='Mean')

    # Convert and rebin
    ConvertUnits(InputWorkspace=tmpWS,OutputWorkspace=tmpWS,Target="Wavelength")

    if interpolate :
        InterpolatingRebin(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,Params= wavbining)
    else :
        Rebin(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,Params= wavbining)

    return tmpWS

# Setup the transmission workspace
# Correct of for the volume of the sample/can. Dimensions should be in order: width, height, thickness
@deprecated
def ScaleByVolume(inputWS, scalefactor, geomid, width, height, thickness):
    # Divide by the area
    if geomid == 1:
        # Volume = circle area * height
        # Factor of four comes from radius = width/2
        scalefactor /= (height*math.pi*math.pow(width,2)/4.0)
    elif geomid == 2:
        scalefactor /= (width*height*thickness)
    else:
        # Factor of four comes from radius = width/2
        scalefactor /= (thickness*math.pi*math.pow(width, 2)/4.0)
    # Multiply by the calculated correction factor
    ws = mtd[inputWS]
    ws *= scalefactor

@deprecated
def StripEndZeroes(workspace, flag_value = 0.0):
    result_ws = mtd[workspace]
    y_vals = result_ws.readY(0)
    length = len(y_vals)
        # Find the first non-zero value
    start = 0
    for i in range(0, length):
        if  y_vals[i] != flag_value :
            start = i
            break
        # Now find the last non-zero value
    stop = 0
    length -= 1
    for j in range(length, 0,-1):
        if  y_vals[j] != flag_value :
            stop = j
            break
        # Find the appropriate X values and call CropWorkspace
    x_vals = result_ws.readX(0)
    startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
    endX = 1.001*x_vals[stop + 1]
    CropWorkspace(InputWorkspace=workspace,OutputWorkspace=workspace,XMin=startX,XMax=endX)

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
        run_number = str(run_number).split('-add')[0]
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

if __name__ == '__main__':
    pass
