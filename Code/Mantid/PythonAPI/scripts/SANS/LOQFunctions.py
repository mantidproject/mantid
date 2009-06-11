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
        x = x.lower()
        if '>' in x:
            pieces = x.split('>')
            low = int(pieces[0].lstrip('hv'))
            upp = int(pieces[1].lstrip('hv'))
            if 'h' in pieces[0]:
                nstrips = abs(upp - low) + 1
                detlist += detBlock(2 + low + 1,nstrips,128) + ','
            elif 'v' in pieces[0]:
                nstrips = abs(upp - low) + 1
                detlist += detBlock(2 + low*128,128,nstrips) + ','
            else:
                for i in range(low, upp + 1):
                    detlist += str(i) + ','
        elif 'h' in x:
            detlist += detBlock(2 + int(x.lstrip('h')) + 1, 1, 128) + ','
        elif 'v' in x:
            low = int(x.lstrip('v'))
            detlist += detBlock(2 + low*128,128, 1) + ','
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
def GetMonitor(inputWS, outputWS, monitorid):
    '''Isolate the monitor data'''
    #  Account for Mantid's off by one storage
    CropWorkspace(inputWS, OutputWorkspace=outputWS, StartSpectrum=str(monitorid - 1), EndSpectrum=str(monitorid - 1))
    RemoveBins(outputWS,outputWS,"19900","20500",Interpolation="Linear")
    FlatBackground(outputWS,outputWS,"0","31000","39000")
    
# Isolate small angle bank
def GetMainBank(inputWS, startid, endid, outputWS):
    '''Isolate the small angle bank data'''
    #  Account for Mantid's off by one storage
    CropWorkspace(inputWS, OutputWorkspace=outputWS, StartSpectrum=str(startid - 1),EndSpectrum=str(endid - 1))

# Setup the transmission data
def SetupTransmissionData(inputWS, wavbining):
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
