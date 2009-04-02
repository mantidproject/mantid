##############################################
# This module contains utility functions
# common to the LOQ scripts
##############################################
from mantidsimple import *

# Determine the detector numbers to mask out
def detBlock(startID, ydim, xdim, stripdim = 128):
    '''Compile a list of detector IDs for rectangular block of size xdim by ydim'''
    output = ""
    for j in range(0, xdim):
        for i in range(0, ydim):
            output += str(startID + i + (stripdim*j)) + ","
    
    output = output.rstrip(",")
    return output

# Mask a cylinder, specifying the algebra to use
def MaskWithCylinder(workspace, radius, algebra):
    '''Mask a cylinder on the input workspace.'''
    xmldef = "<infinite-cylinder id='shape'> "
    xmldef += "<centre x='0.0' y='0.0' z='0.0' /> " 
    xmldef += "<axis x='0.0' y='0.0' z='1' /> "
    xmldef += "<radius val='"+str(radius)+"' /> "
    xmldef += "</infinite-cylinder> "
    xmldef += "<algebra val='" + algebra + "shape' /> "
    # Apply masking
    MaskDetectorsInShape(workspace,xmldef)

# Mask the inside of a cylinder
def MaskInsideCylinder(workspace, radius):
    '''Mask out the inside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, '')

# Mask the outside of a cylinder
def MaskOutsideCylinder(workspace, radius):
    '''Mask out the outside of a cylinder or specified radius'''
    MaskWithCylinder(workspace, radius, '#')

# Convert a mask string from the GUI to a list of detector numbers
def ConvertToDetList(maskstring):
    '''Compile detector ID list'''
    masklist = maskstring.split(',')
    detlist = ''
    for x in masklist:
        if '>' in x:
            pieces = x.split('>')
            low = int(pieces[0].lstrip('h'))
            upp = int(pieces[1].lstrip('h'))
            nstrips = abs(upp - low) + 1
            detlist += detBlock(2 + low + 1,nstrips,128) + ','
        elif 'h' in x:
            low = int(x.lstrip('h'))
            detlist += detBlock(2 + low + 1, 1, 128) + ','
        else:
            detlist += x + ','
    
    return detlist

# Mask by detector number
def MaskByDetNumber(workspace, detlist):
    detlist = detlist.rstrip(',')
    if detlist == '':
        return
    MaskDetectors(workspace, DetectorList = detlist)

# Isolate the monitor data
def GetMonitor(inputWS, outputWS):
    '''Isolate the monitor data'''
    CropWorkspace(inputWS, OutputWorkspace=outputWS, StartSpectrum="1", EndSpectrum="1")
    RemoveBins(outputWS,outputWS,"19900","20500",Interpolation="Linear")
    FlatBackground(outputWS,outputWS,"0","31000","39000")
    
# Isolate small angle bank
def GetMainBank(inputWS, startid, endid, outputWS):
    '''Isolate the small angle bank data'''
    CropWorkspace(inputWS, OutputWorkspace=outputWS, StartSpectrum=str(startid - 1),EndSpectrum=str(endid - 1))

# Setup the data to process
def SetupSmallAngle(inputWS, outputWS, firstsmall, lastsmall, rmin, rmax, maskstring, xshift, yshift):
    # Get the monitor
    GetMonitor(inputWS, "Monitor")
    # Get the small angle banks
    GetMainBank(inputWS, firstsmall, lastsmall, outputWS)
    # Mask beam stop
    MaskInsideCylinder(outputWS, rmin)
    # Mask corners
    MaskOutsideCylinder(outputWS, rmax)
    # Mask others that are defined
    detlist = ConvertToDetList(maskstring)
    MaskByDetNumber(outputWS, detlist)
    MoveInstrumentComponent(outputWS, "main-detector-bank", X = xshift, Y = yshift, RelativePosition="1")

# Setup the transmission data
def SetupTransmissionData(inputWS, instr_file, wavbining):
    # Change the instrument definition to the correct one
    LoadInstrument(inputWS, instr_file)
    tmpWS = inputWS + '_tmp'
    RemoveBins(inputWS,tmpWS,"19900","20500",Interpolation="Linear")
    FlatBackground(tmpWS,tmpWS,"1,2","31000","39000")
    ConvertUnits(tmpWS,tmpWS,"Wavelength")
    Rebin(tmpWS, tmpWS, wavbining)
    return tmpWS

# Correct of for the volume of the sample/can
def ScaleByVolume(inputWS, factor):
    thickness = 1.0
    area = 3.14159265*8*8/4
    correction = factor/(thickness*area)
    
    CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
    Multiply(inputWS, "scalar", inputWS)
    mantid.deleteWorkspace("scalar")
