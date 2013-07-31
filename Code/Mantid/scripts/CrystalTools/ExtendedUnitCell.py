from mantid.simpleapi import *  # New API
from mantid.kernel import *
from mantid.geometry import UnitCell
import numpy
import math



class ExtendedUnitCell(UnitCell):
    
    def __init__(self, cell):
        super(ExtendedUnitCell, self).__init__(cell.a(), cell.b(), cell.c(), cell.alpha(), cell.beta(), cell.gamma())
        
        
    def recAnglesFromReflections(self, horizontalReflectionX, horizontalReflectionY, desiredReflection):
        horizontalReflectionX = numpy.array(horizontalReflectionX)
        horizontalReflectionY = numpy.array(horizontalReflectionY)
        desiredReflection = numpy.array(desiredReflection)
        
        horizontalReflectionX = numpy.matrix(numpy.reshape( horizontalReflectionX, (3, 1)))
        horizontalReflectionY = numpy.matrix(numpy.reshape( horizontalReflectionY, (3, 1)))
        desiredReflection = numpy.matrix(numpy.reshape( desiredReflection, (3, 1)))
        
        ubMatrix = numpy.matrix(self.getB())

        horizontalReflectionXOrth = numpy.array((ubMatrix  *  horizontalReflectionX).T).flatten()
        horizontalReflectionYOrth= numpy.array((ubMatrix * horizontalReflectionY).T).flatten()
        desiredReflectionOrth =  numpy.array((ubMatrix * desiredReflection).T).flatten()

        toDegrees = 180/math.pi
        
        scalarProdRX = numpy.dot( desiredReflectionOrth, horizontalReflectionXOrth )
        sign=numpy.sign(scalarProdRX);
        angle_r1_px=sign * toDegrees * numpy.arccos(scalarProdRX/( numpy.linalg.norm(desiredReflectionOrth) * numpy.linalg.norm(horizontalReflectionXOrth)) );
        
        scalarProdRY = numpy.dot( desiredReflectionOrth, horizontalReflectionYOrth )
        sign=numpy.sign(scalarProdRY);
        angle_r1_py=sign * toDegrees * numpy.arccos(scalarProdRY/( numpy.linalg.norm(desiredReflectionOrth) * numpy.linalg.norm(horizontalReflectionYOrth)) );
        
        crossProdYX = numpy.cross(horizontalReflectionYOrth, horizontalReflectionXOrth)
        nP=crossProdYX/numpy.linalg.norm(crossProdYX); 
        
        # Now figure out angle between reflection and the scattering plane.
        scalarProdRZ = numpy.dot( desiredReflectionOrth, nP )
        sign=numpy.sign(scalarProdRZ);
        angle_r1_pz = sign * toDegrees * numpy.arccos(scalarProdRZ/( numpy.linalg.norm(desiredReflectionOrth)) );
       
        #print angle_r1_px
        #print angle_r1_py
        #print angle_r1_pz
     
        return (angle_r1_px, angle_r1_py, angle_r1_pz)
        
        
def createFromLatticeParameters(a, b, c, alpha, beta, gamma):
    cell = UnitCell(a, b, c, alpha, beta, gamma)
    return ExtendedUnitCell(cell)