from mantid.simpleapi import *  # New API
from mantid.kernel import *
from mantid.geometry import UnitCell
import numpy
import math



class ExtendedUnitCell(UnitCell):
    
    def __init__(self, cell):
        super(ExtendedUnitCell, self).__init__(cell.a(), cell.b(), cell.c(), cell.alpha(), cell.beta(), cell.gamma())
        
        
    def recAnglesFromReflections(self, horizontalReflectionX, horizontalReflectionY, desiredReflection):
        horizontalReflectionXOrth = self.getB()  *  horizontalReflectionX
        horizontalReflectionYOrth= self.getB() * horizontalReflectionY  
        desiredReflectionOrth =  self.getB() * desiredReflection

        toDegrees = 180/math.pi
        
        
        scalarProdRX = numpy.dot( desiredReflectionOrth, horizontalReflectionXOrth )
        sign=numpy.sign(scalarProdRX);
        angle_r1_px=sign * toDegrees * numpy.arccos(scalarProdRX/( numpy.linalg.norm(desiredReflectionOrth) * numpy.linalg.norm(horizontalReflectionXOrth)) );
        
        scalarProdRY = numpy.dot( desiredReflectionOrth, horizontalReflectionYOrth )
        sign=numpy.sign(scalarProdRY);
        angle_r1_py=sign * toDegrees * numpy.arccos(scalarProdRY/( numpy.linalg.norm(desiredReflectionOrth) * numpy.linalg.norm(horizontalReflectionYOrth)) );
        
        #print angle_r1_px
        #print angle_r1_py
        
        return (angle_r1_px, angle_r1_py)
        
        
def createFromLatticeParameters(a, b, c, alpha, beta, gamma):
    cell = UnitCell(a, b, c, alpha, beta, gamma)
    return ExtendedUnitCell(cell)