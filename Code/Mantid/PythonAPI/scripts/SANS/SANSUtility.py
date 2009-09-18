#########################################################
# This module contains utility functions common to the 
# SANS data reduction scripts
########################################################
from mantidsimple import *
import math

# Return the details specific to the instrument and bank given
# as:
#   dimension, specmin, specmax, backmon_start, backmon_end
def GetInstrumentDetails(instr_name, detbank):
	if instr_name == 'LOQ':
		if detbank == 'main-detector-bank':
			return 128, 3, 16386, 31000, 39000
		elif detbank == 'HAB':
			return 128, 16387, 17792, 31000, 39000
		else:
			return -1, -1, -1, -1, -1
	else:
		# This is the number of monitors before the first set of detectors 
		monstart = 8
		dim = 192
		if detbank == 'front-detector':
			smin = dim*dim + 1 + monstart
			smax = dim*dim*2 + monstart
			return dim, smin, smax, 85000, 100000
		elif detbank == 'rear-detector':
			smin = 1 + monstart
			smax = dim*dim + monstart
			return 192, smin, smax, 85000, 100000
		else:
			return -1, -1, -1, -1, -1
			
# Parse a log file containing run information and return the detector positions
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
    MaskDetectorsInShape(workspace,xmldef)

# Mask the inside of a cylinder
def MaskInsideCylinder(workspace, radius, xcentre = '0.0', ycentre = '0.0'):
    '''Mask out the inside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, xcentre, ycentre, '')

# Mask the outside of a cylinder
def MaskOutsideCylinder(workspace, radius, xcentre = '0.0', ycentre = '0.0'):
    '''Mask out the outside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, xcentre, ycentre, '#')

# Work out the spectra IDs for block of detectors
def spectrumBlock(start_spec, ydim, xdim, strip_dim):
    '''Compile a list of spectrum IDs for rectangular block of size xdim by ydim'''
    output = ''
    for y in range(0, ydim):
        for x in range(0, xdim):
            output += str(start_spec + x + (y*strip_dim)) + ','
    output = output.rstrip(",")
    return output

# Convert a mask string to a spectra list
# 6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)
def ConvertToSpecList(maskstring, firstspec, dimension):
    '''Compile spectra ID list'''
    if maskstring == '':
        return ''
    masklist = maskstring.split(',')
    speclist = ''
    for x in masklist:
        x = x.lower()
#   new stuff -----------------------------------------------------------
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
                speclist += spectrumBlock(firstspec + low*dimension + low2, ydim, xdim, dimension) + ','
            elif 'v' in bigPieces[0] and 'h' in bigPieces[1]:
                xdim=abs(upp-low)+1
                ydim=abs(upp2-low2)+1
                speclist += spectrumBlock(firstspec + low2*dimension + low, ydim, xdim, dimension) + ','
            else:
                print "error in mask, ignored:  " + x
# end of new stuff --------------------------------------------
# CHECK taking abs(upp-low) may avoid array index issues, but to get h9>h8 to work need to use low = min(low,upp) ???
        elif '>' in x:
            pieces = x.split('>')
            low = int(pieces[0].lstrip('hvs'))
            upp = int(pieces[1].lstrip('hvs'))
            if 'h' in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec + low*dimension, nstrips, dimension, dimension) + ','
            elif 'v' in pieces[0]:
                nstrips = abs(upp - low) + 1
                speclist += spectrumBlock(firstspec + low, dimension, nstrips, dimension) + ','
            else:
                for i in range(low, upp + 1):
                    speclist += str(i) + ','
        elif 'h' in x:
            low = int(x.lstrip('h'))
            speclist += spectrumBlock(firstspec + low*dimension, 1, dimension, dimension) + ','
        elif 'v' in x:
            speclist += spectrumBlock(firstspec + int(x.lstrip('v')), dimension, 1, dimension) + ','
        else:
            speclist += x.lstrip('s') + ','
    
    return speclist

# Mask by detector number
def MaskBySpecNumber(workspace, speclist):
    speclist = speclist.rstrip(',')
    if speclist == '':
        return ''
    MaskDetectors(workspace, SpectraList = speclist)

# Setup the transmission workspace
def SetupTransmissionWorkspace(inputWS, spec_list, backmon_start, backmon_end, wavbining, loqremovebins):
	tmpWS = inputWS + '_tmp'
	if loqremovebins == True:
		RemoveBins(inputWS,tmpWS, 19900, 20500, Interpolation='Linear')
		FlatBackground(tmpWS, tmpWS, spec_list, backmon_start, backmon_end)
	else:
		FlatBackground(inputWS, tmpWS, spec_list, backmon_start, backmon_end)
	# Convert and rebin
	ConvertUnits(tmpWS,tmpWS,"Wavelength")
	Rebin(tmpWS, tmpWS, wavbining)
	return tmpWS

# Correct of for the volume of the sample/can. Dimensions should be in order: width, height, thickness
def ScaleByVolume(inputWS, scalefactor, geomid, width, height, thickness):
	correction = scalefactor
	# Divide by the area
	if geomid == 1:
		# Volume = circle area * height
		# Factor of four comes from radius = width/2
		correction /= (height*math.pi*math.pow(width,2)/4.0)
	elif geomid == 2:
		correction /= (width*height*thickness)
	else:
		# Factor of four comes from radius = width/2
		correction /= (thickness*math.pi*math.pow(width, 2)/4.0)
	
	CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
	Multiply(inputWS, "scalar", inputWS)
	mantid.deleteWorkspace("scalar")

def InfinitePlaneXML(id, planept, normalpt):
	return  '<infinite-plane id="' + str(id) + '">' + \
	    '<point-in-plane x="' + str(planept[0]) + '" y="' + str(planept[1]) + '" z="' + str(planept[2]) + '" />' + \
	    '<normal-to-plane x="' +str(normalpt[0]) + '" y="' + str(normalpt[1]) + '" z="' + str(normalpt[2]) + '" />' + \
	    '</infinite-plane>'
								     
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
	xmlstring += '<algebra val="(#(' + cout_id + ':(#' + cin_id  + '))) ' + p1id + ' ' + p2id + '"/>\n' 
	return xmlstring

# Create a workspace with a quadrant value in it 
def CreateQuadrant(workspace, quadrant, xcentre, ycentre, rmin, rmax):
	objxml = QuadrantXML([xcentre,ycentre, 0.0], rmin, rmax, quadrant)
	finddead = FindDetectorsInShape(workspace, ShapeXML=objxml)
	groupdet = GroupDetectors(workspace, quadrant, DetectorList = finddead.getPropertyValue("DetectorList"))
	# Replaces NANs with zeroes
	ReplaceSpecialValues(InputWorkspace=quadrant,OutputWorkspace=quadrant,NaNValue="0",InfinityValue="0")
	# Crop Workspace to remove leading and trailing zeroes
	StripEndZeroes(quadrant)

# Create 4 quadrants for the centre finding algorithm and return their names
def GroupIntoQuadrants(workspace, xcentre, ycentre, rmin, rmax):
	left_ws = 'Left'
	CreateQuadrant(workspace, left_ws,xcentre, ycentre, rmin, rmax)
	right_ws = 'Right'
	CreateQuadrant(workspace, right_ws,xcentre, ycentre, rmin, rmax)
	up_ws = 'Up'
	CreateQuadrant(workspace, up_ws,xcentre, ycentre, rmin, rmax)
	dw_ws = 'Down'
	CreateQuadrant(workspace, dw_ws,xcentre, ycentre, rmin, rmax)
	return (left_ws, right_ws, up_ws, dw_ws)

# Calcluate the sum squared difference of the given workspaces. This assumes that a single spectrum workspace in Q
# for each of the quadrants. The order should be L,R,U,D
def CalculateResidue(quadrants):
	ly = mtd.getMatrixWorkspace(quadrants[0]).readY(0)
	ry = mtd.getMatrixWorkspace(quadrants[1]).readY(0)
	uy = mtd.getMatrixWorkspace(quadrants[2]).readY(0)
	dy = mtd.getMatrixWorkspace(quadrants[3]).readY(0)
	residue = 0
	nvals = len(ly)
	for index in range(0, nvals):
		residue += pow(ly[index] - ry[index], 2) + pow(uy[index] - dy[index], 2)
	return residue

def StripEndZeroes(workspace):
        result_ws = mantid.getMatrixWorkspace(workspace)
        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
                if ( y_vals[i] != 0.0 ):
                        start = i
                        break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0,-1):
                if ( y_vals[j] != 0.0 ):
                        stop = j
                        break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001*x_vals[stop+1]
        CropWorkspace(workspace,workspace,startX,endX)
