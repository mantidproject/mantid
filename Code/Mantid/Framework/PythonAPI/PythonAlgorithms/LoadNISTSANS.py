"""
    Load algorithm for NIST SANS raw date files.
    Code contributed by Andrew Jackson (NIST).
"""
from MantidFramework import *
from mantidsimple import *
import math
  
from struct import unpack
import struct
import numpy

class LoadNISTSANS(PythonAlgorithm):
    
    def category(self):
        return "SANS"

    def name(self):
        return "LoadNISTSANS"

    def PyInit(self):
        # Input file
        self.declareFileProperty("Filename", "", FileAction.Load)
        # Output workspace to put the transmission histo into
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        input_file = self.getProperty("Filename")
        output_ws = self.getPropertyValue("OutputWorkspace")

        if not os.path.isfile(input_file):
            raise ValueError, \
            "Specified file %s is not a regular file" % input_file
        
        data, meta = readNCNRData(input_file)
        wavelength = meta['resolution.lmda']
        dwavelength = meta['resolution.dlmda']
        for item in meta:
            print item, meta[item]
        
        LoadEmptyInstrument(Filename="NGSANS_Definition.xml", OutputWorkspace=output_ws)
        ws = mtd[output_ws]
        
        # Monitor
        ws.dataX(0)[0] = wavelength-dwavelength/2.0
        ws.dataX(0)[1] = wavelength+dwavelength/2.0
        ws.dataY(0)[0] = meta['run.moncnt']
        ws.dataE(0)[0] = 0

        # Timer
        ws.dataX(1)[0] = wavelength-dwavelength/2.0
        ws.dataX(1)[1] = wavelength+dwavelength/2.0
        ws.dataY(1)[0] = meta['run.ctime']
        ws.dataE(1)[0] = 0
        
        n_monitors = int(ws.getInstrument().getNumberParameter("number-of-monitors")[0])

        
        for ix in range(data.shape[0]):
            for iy in range(data.shape[1]):
                i_spec = iy*data.shape[0]+ix+n_monitors
                ws.dataX(i_spec)[0] = wavelength-dwavelength/2.0
                ws.dataX(i_spec)[1] = wavelength+dwavelength/2.0
                ws.dataY(i_spec)[0] = data[ix, iy]
                ws.dataE(i_spec)[0] = math.sqrt(math.fabs(data[ix, iy]))

        ws.getRun().addProperty_dbl("sample_detector_distance", meta['det.dis']*1000.0, True)
        axis = ws.getAxis(0).setUnit("Wavelength")
        self.setProperty("OutputWorkspace", ws)
        
        
mtd.registerPyAlgorithm(LoadNISTSANS())

def R4toFloat(vaxasstring):
    """Takes a 4 character string represention of a VAX REAL*4 from struct.unpack
           and returns a python float"""    

    if ord(vaxasstring[1]) == 0:
        ieeeasstring = vaxasstring[2]+vaxasstring[3]+vaxasstring[0]+vaxasstring[1]
        return unpack('<f',ieeeasstring)[0]
    else:
        ieeeasstring = vaxasstring[2]+vaxasstring[3]+vaxasstring[0]+chr(ord(vaxasstring[1])-1)
        return unpack('<f',ieeeasstring)[0]

def readNCNRData(inputfile):

    f = open(inputfile, 'rb')    
    data = f.read()
    f.close()

    #filename
    dat = struct.unpack('<21s',data[2:23])
    filename = dat[0].replace(' ','')

    #metadata
    metadata = {}
    reals = {}

    formatstring = '<4i4s4s4s4s20s3s11s1s8s' #run
    formatstring += '60s4s4s4s4s3i4s4s2i6s6s' #sample
    formatstring += '6s4s4s4s4s4s4s2i4s4s4s4s4s4s4s' #det
    formatstring += '4s4s4s4s4s4s' #resolution
    formatstring += 'L2i' #tslice
    formatstring += 'L4s4s4s2i' #temp
    formatstring += '2L4s4s4s4s4s' #magnet
    formatstring += '4s4s' #bmstp
    formatstring += '3i4s4s4s4s42s' #params
    formatstring += 'L4s4si' #voltage
    formatstring += '2L4s4s' #polarization
    formatstring += '4i4s4s4s4s4s' #analysis

    #print formatstring

    #print struct.calcsize(formatstring)

    (metadata['run.npre'],
     metadata['run.ctime'],
     metadata['run.rtime'],
     metadata['run.numruns'],
     reals['run.moncnt'],
     reals['run.savmon'],
     reals['run.detcnt'],
     reals['run.atten'],
     metadata['run.datetime'],
     metadata['run.type'],
     metadata['run.defdir'], 
     metadata['run.mode'],
     metadata['run.reserve'],
     metadata['sample.labl'],
     reals['sample.trns'],
     reals['sample.thk'],
     reals['sample.position'],
     reals['sample.rotang'],
     metadata['sample.table'],
     metadata['sample.holder'],
     metadata['sample.blank'],
     reals['sample.temp'],
     reals['sample.field'],
     metadata['sample.tctrlr'],
     metadata['sample.magnet'],
     metadata['sample.tunits'],
     metadata['sample.funits'],
     metadata['det.typ'],
     reals['det.calx1'],
     reals['det.calx2'],
     reals['det.calx3'],
     reals['det.caly1'],
     reals['det.caly2'],
     reals['det.caly3'],
     metadata['det.num'],
     metadata['det.spacer'],
     reals['det.beamx'],
     reals['det.beamy'],
     reals['det.dis'],
     reals['det.ang'],
     reals['det.siz'],
     reals['det.bstop'],
     reals['det.blank'],
     reals['resolution.ap1'],
     reals['resolution.ap2'],
     reals['resolution.ap12dis'],
     reals['resolution.lmda'],
     reals['resolution.dlmda'],
     reals['resolution.save'],
     metadata['tslice.slicing'], 
     metadata['tslice.multfact'],
     metadata['tslice.ltslice'],
     metadata['temp.printemp'],
     reals['temp.hold'],
     reals['temp.err'],
     reals['temp.blank'],
     metadata['temp.extra'],
     metadata['temp.reserve'],
     metadata['magnet.printmag'],
     metadata['magnet.sensor'],
     reals['magnet.current'],
     reals['magnet.conv'],
     reals['magnet.fieldlast'],
     reals['magnet.blank'],
     reals['magnet.spacer'],
     reals['bmstp.xpos'],
     reals['bmstp.ypos'],
     metadata['params.blank1'],
     metadata['params.blank2'],
     metadata['params.blank3'],
     reals['params.trsncnt'],
     reals['params.extra1'],
     reals['params.extra2'],
     reals['params.extra3'],
     metadata['params.reserve'],
     metadata['voltage.printvolt'],
     reals['voltage.volts'],
     reals['voltage.blank'],
     metadata['voltage.spacer'],
     metadata['polarization.printpol'],
     metadata['polarization.flipper'],
     reals['polarization.horiz'],
     reals['polarization.vert'],
     metadata['analysis.rows1'],
     metadata['analysis.rows2'],
     metadata['analysis.cols1'],
     metadata['analysis.cols2'],
     reals['analysis.factor'],
     reals['analysis.qmin'],
     reals['analysis.qmax'],
     reals['analysis.imin'],
     reals['analysis.imax']) = struct.unpack(formatstring,data[23:514])


    #Process reals into metadata
    for k,v in reals.items():
        #print k,type(v)
        metadata[k] = R4toFloat(v)

    dataformatstring = '<16401h'
    rawdata = numpy.array(struct.unpack(dataformatstring,data[514:]))

    detdata = numpy.empty(16384)

    ii=0
    skip=0
    while(ii < 16384):
        if(((ii+skip) %1022)==0):
            skip+=1
        detdata[ii] = I2Decompress(rawdata[ii+skip])
        ii+=1

    detdata.resize(128,128)
     
    return (detdata,metadata)

def I2Decompress(val):
    """Take a 'compressed' I*2 value and convert to I*4.

       Code taken from IGOR Pro macros by SRK. VAX Fortran code is ultimate source (RW_DATAFILE.FOR)"""

    ib=10
    nd=4
    ipw = math.pow(ib,nd)
    if val <= -ipw:
        npw=trunc(-val/ipw)
        val=(-val % ipw)*(math.pow(ib,npw))
        return val
    else:
        return val

def arrayI2Decompress(datarray):
    """Apply the I2 to I4 decompression routine to a whole array"""
    for element in datarray.flat:
        element = I2Decompress(element)

    return datarray


def trunc(val):
    """Return the integer closest to the supplied value in the direction of zero"""

    if val < 0:
        return math.ceil(val)
    elif val > 0:
        return math.floor(val)
    else:
        return val
