from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import wks_utility

data_file = '/mnt/hgfs/j35/results/REF_L_66420_event.nxs'
LoadEventNexus(Filename=data_file, OutputWorkspace='EventData')
rebin_parameters = '0,200,200000'  #microS
rebin(InputWorkspace='EventData', OutputWorkspace='HistoData', Params=rebin_parameters)
mt = mtd['HistoData']

cpix = 256 / 2

#dimension of the detector (256 by 304 pixels)
maxX = 304
maxY = 256

sample = mt.getInstrument().getSample()
source = mt.getInstrument().getSource()
dSM = sample.getDistance(source)
#create array of distances pixel->sample
dPS_array = zeros((maxY, maxX))
for x in range(maxX):
    for y in range(maxY):
        _index = maxY * x + y
        detector = mt.getDetector(_index)
        dPS_array[y, x] = sample.getDistance(detector)
#array of distances pixel->source
dMP_array = dPS_array + dSM
#distance sample->center of detector
dSD = dPS_array[maxY / 2, maxX / 2]

#print wks_utility.ref_beamdiv_correct(cpix, mt, dSD)

x = maxX/2.
dPS_array = zeros(maxY)
for y in range(maxY):
    _index = maxY*x + y
    detector = mt.getDetector(int(_index))
    dPS_array[y] = sample.getDistance(detector)
    
dSD = dPS_array[maxY / 2]
dMD = dSD + dSM
value = wks_utility.ref_beamdiv_correct(cpix, mt, dSD, 128)

#Get metadata
mt_run = mt.getRun()
##get angles value
ths_value = mt_run.getProperty('ths').value[0]
ths_units = mt_run.getProperty('ths').units
tthd_value = mt_run.getProperty('tthd').value[0]
tthd_units = mt_run.getProperty('tthd').units
ths_rad = wks_utility.angleUnitConversion(value=ths_value,
                              from_units=ths_units,
                              to_units='rad')
tthd_rad = wks_utility.angleUnitConversion(value=tthd_value,
                               from_units=tthd_units,
                               to_units='rad')

theta = tthd_rad - ths_rad
_tof_axis = mt.readX(0)[:]

q_array_before = wks_utility.convertToRvsQ(dMD=dMD,
                                           theta=theta,
                                           tof=_tof_axis)

print 'q_array before'
print q_array_before

list_data_peak = [126, 134]
yrange = arange(134-126+1) + 126

q_array_after = wks_utility.convertToRvsQWithCorrection(mt,
                                                         dMD=dMD,
                                                         theta=theta,
                                                         tof=_tof_axis,
                                                         yrange=yrange,
                                                         cpix=cpix)


print 'q_array after'
print q_array_after





