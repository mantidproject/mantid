#pylint: disable=invalid-name,bare-except
import mantid
import numpy

def ValidateOL(ol):
    if type(ol)==mantid.geometry.OrientedLattice:
        if ol.a()<0.1 or ol.b()<0.1 or ol.c()<0.1:
            return False
        if ol.alpha()<5 or ol.alpha()>175:
            return False
        if ol.beta()<5 or ol.beta()>175:
            return False
        if ol.gamma()<5 or ol.gamma()>175:
            return False
        return True
    return False

def ValidateUB(UBMatrix):
    if numpy.linalg.det(UBMatrix)<0:
        return False
    try:
        __tempol=mantid.geometry.OrientedLattice()
        __tempol.setUB(UBMatrix)
    except:
        return False
    return ValidateOL(__tempol)

