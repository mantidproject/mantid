#########################################################
# This module contains utility functions common to the 
# SANS data reduction scripts
########################################################
from mantidsimple import *
import math

# Mask a cylinder, specifying the algebra to use
def MaskWithCylinder(workspace, radius, xcentre, ycentre, algebra):
    '''Mask a cylinder on the input workspace.'''
    xmldef = '<infinite-cylinder id="shape">'
    xmldef += '<centre x="' + str(xcentre) + '" y="' + str(ycentre) + '" z="0.0" />'
    xmldef += '<axis x="0.0" y="0.0" z="1" />'
    xmldef += '<radius val="' + str(radius) + '" />'
    xmldef += '</infinite-cylinder>'
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
def ConvertToSpecList(maskstring, firstspec, dimension):
    '''Compile spectra ID list'''
    if maskstring == '':
        return ''
    masklist = maskstring.split(',')
    speclist = ''
    for x in masklist:
        x = x.lower()
        if '>' in x:
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
		correction /= (height**math.pi*pow(width,2)/4.0)
	elif geomid == 2:
		correction /= (width*height*thickness)
	else:
		# Factor of four comes from radius = width/2
		correction /= (thickness**math.pi*math.pow(width, 2)/4.0)
	
	CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
	Multiply(inputWS, "scalar", inputWS)
	mantid.deleteWorkspace("scalar")

# Define a cone in XML for the centre finding algorithm 
def ConeXML(id, tip_pt, height, axis, angle = 45.0):
	'''Defiine a cone in XML'''
	return'<cone id="' + id + '" >' + \
	'<tip-point x="' + str(tip_pt[0]) + '" y="' + str(tip_pt[1]) + '" z="' + str(tip_pt[2]) + '" />' + \
	'<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
	'<angle val="' + str(angle) + '" />' + \
	'<height val="' + str(height) +'" />' + \
	'</cone>\n'

# Define a sphere in XML for the centre finding algorithm 
def SphereXML(id, centre, radius):
	'''Define a hemisphere where the axis points from the base to the arc'''
	return '<sphere id="' + id + '">' + \
	'<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' +str(centre[2]) + '" />' + \
	'<radius val="' + str(radius) + '" />' + \
	'</sphere>\n'

# Define a plane in XML for the centre finding algorithm 
def InfinitePlaneXML(id, centre, axis):
	return '<infinite-plane id="' + id+ '">' + \
	'<point-in-plane x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' +str(centre[2]) + '" />' + \
	'<normal-to-plane x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
	'</infinite-plane>\n'

# Define a cone with a hemisphere attached to its base for the centre finding algorithm 
def RoundedConeXML(tip_pt, hypot, axis, id):
	coneheight = hypot/math.sqrt(2.0)
	cid = id+ '-cone'
	xmlstring = ConeXML(cid , tip_pt, coneheight, axis)
	# The sphere centre is the coneheight times the axis direction (assuming a unit axis vector)
	spherecentre = [ tip_pt[idx] + coneheight*(-1.*axis[idx]) for idx in range(0,3) ]
	sphradius = hypot*( 1.0 - (1.0/math.sqrt(2.0)) )
	sid = id + '-sph'
	xmlstring += SphereXML(sid, spherecentre, sphradius)
	# Not cut off half of the sphere with a plane
	pid = id + '-pla'
	xmlstring += InfinitePlaneXML(pid, spherecentre, [-1*axis[idx] for idx in range(0,3)])
	# This first takes the intersection of the plane and the sphere and then takes the union of that with the
	# cone to produce a cone with a hemisphere stuck on the end
	xmlstring+= '<algebra val = "' + cid + ':' + '(' + sid + ' ' + pid + ')' + '"/>\n'
	return xmlstring

# Create a workspace with a quadrant value in it 
def CreateQuadrant(workspace, quadrant, xcentre, ycentre, zpos, rlimit, axis):
	objxml = RoundedConeXML([xcentre,ycentre,zpos], rlimit, axis, 'quadrant')
	finddead = FindDetectorsInShape(workspace, ShapeXML=objxml)
	groupdet = GroupDetectors(workspace, quadrant, DetectorList = finddead.getPropertyValue("DetectorList"))
	Integration(quadrant, quadrant)

# Create 4 quadrants for the centre finding algorithm and return their names
def GroupIntoQuadrants(workspace, xcentre, ycentre, zpos, rlimit):
	left_ws = 'Left'
	CreateQuadrant(workspace, left_ws,xcentre, ycentre, zpos, rlimit, [1,0,0])
	right_ws = 'Right'
	CreateQuadrant(workspace, right_ws, xcentre, ycentre, zpos, rlimit, [-1,0,0])
	up_ws = 'Up'
	CreateQuadrant(workspace, up_ws, xcentre, ycentre, zpos, rlimit, [0,-1,0])
	dw_ws = 'Down'
	CreateQuadrant(workspace, dw_ws,xcentre, ycentre, zpos, rlimit, [0,1,0])
	return (left_ws, right_ws, up_ws, dw_ws)
