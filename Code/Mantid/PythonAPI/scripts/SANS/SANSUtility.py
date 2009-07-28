#########################################################
# This module contains utility functions common to the 
# SANS data reduction scripts
########################################################
from mantidsimple import *
import math

# Mask a cylinder, specifying the algebra to use
def MaskWithCylinder(workspace, radius, xcentre, ycentre, algebra):
    '''Mask a cylinder on the input workspace.'''
    xmldef = "<infinite-cylinder id='shape'> "
    xmldef += "<centre x=" + xcentre + "y=" + ycentre + "z='0.0' /> " 
    xmldef += "<axis x='0.0' y='0.0' z='1' /> "
    xmldef += "<radius val='"+str(radius)+"' /> "
    xmldef += "</infinite-cylinder> "
    xmldef += "<algebra val='" + algebra + "shape' /> "
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


# Correct of for the volume of the sample/can
def ScaleByVolume(inputWS, factor):
    thickness = 1.0
    area = math.pi*8.*8./4
    correction = factor/(thickness*area)
    
    CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
    Multiply(inputWS, "scalar", inputWS)
    mantid.deleteWorkspace("scalar")
