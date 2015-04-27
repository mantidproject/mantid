#pylint: disable=invalid-name
#########################################################
# This module contains utility functions common to the
# SANS data reduction scripts
########################################################
from mantid.simpleapi import *
from mantid.api import IEventWorkspace, MatrixWorkspace, WorkspaceGroup
import inspect
import math
import os
import re
import types

sanslog = Logger("SANS")

ADDED_EVENT_DATA_TAG = '_added_event_data'

REG_DATA_NAME = '-add' + ADDED_EVENT_DATA_TAG + '[_1-9]*$'
REG_DATA_MONITORS_NAME = '-add_monitors' + ADDED_EVENT_DATA_TAG + '[_1-9]*$'

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

def InfinitePlaneXML(id, plane_pt, normal_pt):
    return '<infinite-plane id="' + str(id) + '">' + \
        '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
        '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
        '</infinite-plane>'

def InfiniteCylinderXML(id, centre, radius, axis):
    return  '<infinite-cylinder id="' + str(id) + '">' + \
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
    while phimax > 360 : phimax -= 360
    while phimax < 0 : phimax += 360
    while phimin > 360 : phimin -= 360
    while phimin < 0 : phimin += 360
    while phimax<phimin : phimax += 360

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
    elif orientation == Orientation.HorizontalFlipped:
        start_spec = base + ylow*det_dimension + xlow
        for y in range(0,ydim):
            max_row = base + (y+1)*det_dimension - 1
            min_row = base + (y)*det_dimension
            for x in range(0,xdim):
                std_i = start_spec + x + (y*det_dimension)
                diff_s = std_i - min_row
                output += str(max_row - diff_s) + ','

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
        if not val: return val
        return [val[0], MARK]

    def _parse_upper(inpstr):
        val = _check_match(inpstr, upbound, 1)
        if not val: return val
        return [MARK, val[0]]

    def _parse_start_step_stop(inpstr):
        val = _check_match(inpstr, sss_pat, 3)
        if not val: return val
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

def mask_detectors_with_masking_ws(ws_name, masking_ws_name):
    """
    Rolling our own MaskDetectors wrapper since masking is broken in a couple
    of places that affect us here:

    1. Calling MaskDetectors(Workspace=ws_name, MaskedWorkspace=mask_ws_name)
       is not something we can do because the algorithm masks by ws index
       rather than detector id, and unfortunately for SANS the detector table
       is not the same for MaskingWorkspaces as it is for the workspaces
       containing the data to be masked.  Basically, we get a mirror image of
       what we expect.  Instead, we have to extract the det IDs and use those
       via the DetectorList property.

    2. For some reason Detector.isMasked() does not work for MaskingWorkspaces.
       We use masking_ws.readY(ws_index)[0] == 1 instead.

    @param ws :: the workspace to be masked.
    @param masking_ws :: the masking workspace that contains masking info.
    """
    ws, masking_ws = mtd[ws_name], mtd[masking_ws_name]

    masked_det_ids = []

    for ws_index in range(masking_ws.getNumberHistograms()):
        if masking_ws.readY(ws_index)[0] == 1:
            masked_det_ids.append(masking_ws.getDetector(ws_index).getID())

    MaskDetectors(Workspace=ws, DetectorList=masked_det_ids)


def check_child_ws_for_name_and_type_for_added_eventdata(wsGroup):
    '''
    Ensure that the while loading added event data, we are dealing with
    1. The correct naming convention. This is XXXXXXXX-add[_1-9]* for 
        the event data and XXXXXXXX-add_monitors[_1-9]* for the monitor data
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

    if (len(ws_handles) != 2):
        raise RuntimeError('Expected two child workspaces when loading added event data.'/
                           'Please make sure that you have loaded added event data which was generated by the Add tab of the SANS Gui.')

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
    file = open(logfile, 'rU')
    for line in file:
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
    # This is for the empty instrument
    HorizontalFlipped = 4

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
