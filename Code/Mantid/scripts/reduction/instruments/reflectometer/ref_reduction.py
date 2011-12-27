from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import wks_utility

#This will activate or not the various 2d or 1d plots
bPlot = True
bDataBack = False
bNorm = False


h = 6.626e-34  #m^2 kg s^-1
m = 1.675e-27     #kg

#dimension of the detector (256 by 304 pixels)
maxX = 304
maxY = 256

TOFrange = [9000, 23600] #microS

#Due to the frame effect, it's sometimes necessary to narrow the range
#over which we add all the pixels along the low resolution
Xrange = [115, 210]


nexus_path = '/mnt/hgfs/j35/results/'
list_data = ['66421',
             '66420',
             '66422',
             '66423',
             '66424',
             '66347,66355,66363,66371,66379',
             '66348,66356,66364,66372,66380',
             '66309,66317,66325,66333,66341']
list_norm = ['66196']

rebin_parameters = '0,200,200000'

pre = 'REF_L_'
post = '_event.nxs'

list_data_peak = [[126, 134],
                  [126, 134],
                  [126, 134],
                  [126, 134],
                  [126, 134],
                  [125, 135],
                  [123, 137],
                  [126, 140]]
list_data_back = [[123, 137],
                  [123, 137],
                  [123, 137],
                  [123, 137],
                  [123, 137],
                  [123, 137],
                  [123, 137],
                  [123, 137]]
                  
list_norm_peak = [[125, 135]]
list_norm_back = [[123, 137]]


#check with first one first if it works
data_file = nexus_path + pre + list_data[0] + post

#load data
LoadEventNexus(Filename=data_file, OutputWorkspace='EventData')
mt = mtd['EventData']

#get the proton charge in picoCoulomb
proton_charge = wks_utility.getProtonCharge(mt) #ex: 30961787025.2
mt_run = mt.getRun()
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

#rebin data
rebin(InputWorkspace='EventData',
      OutputWorkspace='HistoData',
      Params=rebin_parameters)
mt = mtd['HistoData']

#retrieve geometry of instrument
# distance Sample detector

sample = mt.getInstrument().getSample()
source = mt.getInstrument().getSource()
dSM = sample.getDistance(source)
dPS_array = zeros((maxY, maxX))
for x in range(maxX):
    for y in range(maxY):
        _index = maxY * x + y
        detector = mt.getDetector(_index)
        dPS_array[y, x] = sample.getDistance(detector)
dMP_array = dPS_array + dSM
dSD = dPS_array[maxY / 2, maxX / 2]
dMD = dSD + dSM
#if bPlot:
#    fig3 = figure()
#    plt.imshow(dMP_array)
#    colorbar()
         
#print 'distance SD: ' , dSD 
#print      

#Normalized by Current (proton charge)
NormaliseByCurrent(InputWorkspace='HistoData', OutputWorkspace='HistoData')

###############
##debugging only
pixelXpixelY = wks_utility.getPixelXPixelY(mt, maxX=maxX, maxY=maxY)
if bPlot:
    fig1 = plt.figure(figsize=(10, 10))
    ax = fig1.add_subplot(2, 2, 1)
    plt.imshow(log(pixelXpixelY), aspect='auto', origin='lower')
    colorbar()
    xlabel('Pixel X')
    ylabel('Pixel Y')
    title('Pixel Y vs Pixel X')

pixelXtof_data = wks_utility.getPixelXTOF(mt, maxX=maxX, maxY=maxY)
if bPlot:
    fig1.add_subplot(2, 2, 2)
    plt.imshow(log(pixelXtof_data), aspect='auto', origin='lower')
    colorbar()
    xlabel('Bins #')
    ylabel('Pixel')
    title('Pixel X vs TOF bins')

#let's calculate here the exact pixel peak 
# used to be the center of the peak-min and peak-max values, now
# we gonna used a weighted calculation
old_method_cpix = (list_data_peak[0][0] + list_data_peak[0][1]) / 2
#print 'data central pixel using old method: %f' % old_method_cpix

#integrated over tof
pixelXtof_1d = pixelXtof_data.sum(axis=1)

#keep only range of pixels
pixelXtof_roi = pixelXtof_1d[list_data_peak[0][0]:list_data_peak[0][1]]
sz = pixelXtof_roi.size
_num = 0
_den = 0
start_pixel = list_data_peak[0][0]
for i in range(sz):
    _num += (start_pixel * pixelXtof_roi[i])
    start_pixel = start_pixel + 1
    _den += pixelXtof_roi[i]
data_cpix = _num / _den    
#print 'data central pixel using new method: %f' % data_cpix
             
#We need to create a new EventWorkspace of only the xRange of pixels of 
#interest and normalize by the proton charge

fromYpixel = list_data_back[0][0]
toYpixel = list_data_back[0][1]

mt2 = wks_utility.createIntegratedWorkspace(mt, "IntegratedDataWks",
#                                            proton_charge,
                                            fromXpixel=Xrange[0],
                                            toXpixel=Xrange[1],
                                            fromYpixel=fromYpixel,
                                            toYpixel=toYpixel,
                                            maxX=maxX,
                                            maxY=maxY)

if bPlot:
    _tof_axis = mt2.readX(0)[:]
    _y_axis = zeros((maxY, len(_tof_axis) - 1))
    for x in range(maxY):
        _y_axis[x, :] = mt2.readY(x)[:]
    fig1.add_subplot(2, 2, 3)
    plt.imshow(log(_y_axis), aspect='auto', origin='lower')
    colorbar()
    xlabel('Bins #')
    ylabel('Pixel X')
    title('Pixel X vs TOF (for only X and Y range of interest')


if bPlot:
    tof_axis = mt2.readX(0)[:]
    counts_vs_tof = zeros(len(tof_axis) - 1)
    for x in range(maxY):
        counts_vs_tof += mt2.readY(x)[:]
    index_tof_min = wks_utility.getIndex(TOFrange[0], tof_axis)
    index_tof_max = wks_utility.getIndex(TOFrange[1], tof_axis)
    _tof_axis = tof_axis[index_tof_min:index_tof_max]
    _y_axis = counts_vs_tof[index_tof_min:index_tof_max]
    fig1.add_subplot(2, 2, 4)
    plt.plot(_tof_axis, _y_axis, 'b', label='Before background sub.')
    xlabel('TOF (microS)')
    ylabel('Counts')
    title('Counts vs TOF')

if bDataBack:

    #Background subtraction ======================

    #Due to the way the FlatBackground algorithm works, we need to transpose the 
    #workspace
    # TOF axis which used to be DataX is now Pixels(MaxY)
    # 
    Transpose(InputWorkspace='IntegratedDataWks',
              OutputWorkspace='TransposedID')
    ConvertToHistogram(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedID')
    FlatBackground(InputWorkspace='TransposedID',
                   OutputWorkspace='TransposedFlatID',
                   StartX=fromYpixel,
                   EndX=list_data_peak[0][0])
    Transpose(InputWorkspace='TransposedFlatID',
              OutputWorkspace='DataWks')

    if bPlot:
        mt3 = mtd['DataWks']
        tof_axis = mt3.readX(0)[:]
        counts_vs_tof = zeros(len(tof_axis))
        for x in range(maxY):
            counts_vs_tof += mt3.readY(x)[:]
            index_tof_min = wks_utility.getIndex(TOFrange[0], tof_axis)
            index_tof_max = wks_utility.getIndex(TOFrange[1], tof_axis)
            _tof_axis = tof_axis[index_tof_min:index_tof_max]
            _y_axis = counts_vs_tof[index_tof_min:index_tof_max]
            fig1.add_subplot(2, 2, 4)
            plt.plot(_tof_axis, _y_axis, 'gx', label='After background sub.')

if bNorm:

    ## Work on Normalization file now
    #check with first one first if it works
    norm_file = nexus_path + pre + list_norm[0] + post

    #load normalization file
    LoadEventNexus(Filename=norm_file, OutputWorkspace='EventData')
    mt = mtd['EventData']

    #get the proton charge in picoCoulomb
    proton_charge = wks_utility.getProtonCharge(mt) #ex: 30961787025.2

    #rebin data
    rebin(InputWorkspace='EventData',
          OutputWorkspace='HistoData',
          Params=rebin_parameters)
    mt = mtd['HistoData']

    #Normalized by Current (proton charge)
    NormaliseByCurrent(InputWorkspace='HistoData', OutputWorkspace='HistoData')

    ###############
    ##debugging only
    pixelXpixelY = wks_utility.getPixelXPixelY(mt, maxX=maxX, maxY=maxY)
    if bPlot:
        
        fig2 = plt.figure(figsize=(10, 10))
        ax = fig2.add_subplot(2, 2, 1)
        plt.imshow(log(pixelXpixelY), aspect='auto', origin='lower')
        colorbar()
        xlabel('Pixel X')
        ylabel('Pixel Y')
        title('Pixel Y vs Pixel X')

    pixelXtof_norm = wks_utility.getPixelXTOF(mt, maxX=maxX, maxY=maxY)
    if bPlot:
        subplot(2, 2, 2)
        plt.imshow(log(pixelXtof_norm), aspect='auto', origin='lower')
        colorbar()
        xlabel('Bins #')
        ylabel('Pixel')
        title('Pixel X vs TOF bins')

    #We need to create a new EventWorkspace of only the xRange of pixels of 
    #interest and normalize by the proton charge

    fromYpixel = list_norm_back[0][0]
    toYpixel = list_norm_back[0][1]

    #let's calculate here the exact pixel peak 
    # used to be the center of the peak-min and peak-max values, now
    # we gonna used a weighted calculation
    old_method_cpix = (list_norm_peak[0][0] + list_norm_peak[0][1]) / 2
    print 'norm central pixel using old method: %f' % old_method_cpix

    #integrated over tof
    pixelXtof_1d = pixelXtof_norm.sum(axis=1)

    #keep only range of pixels
    pixelXtof_roi = pixelXtof_1d[list_norm_peak[0][0]:list_norm_peak[0][1]]
    sz = pixelXtof_roi.size
    _num = 0
    _den = 0
    start_pixel = list_data_peak[0][0]
    for i in range(sz):
        _num += (start_pixel * pixelXtof_roi[i])
        start_pixel = start_pixel + 1
        _den += pixelXtof_roi[i]
    norm_cpix = _num / _den    
    print 'norm central pixel using new method: %f' % data_cpix

    mt2 = wks_utility.createIntegratedWorkspace(mt, "IntegratedDataWks",
#                                                proton_charge,
                                                fromXpixel=Xrange[0],
                                                toXpixel=Xrange[1],
                                                fromYpixel=fromYpixel,
                                                toYpixel=toYpixel,
                                                maxX=maxX,
                                                maxY=maxY)

    if bPlot:
        _tof_axis = mt2.readX(0)[:]
        _y_axis = zeros((maxY, len(_tof_axis) - 1))
        for x in range(maxY):
            _y_axis[x, :] = mt2.readY(x)[:]
        subplot(2, 2, 3)
        plt.imshow(log(_y_axis), aspect='auto', origin='lower')
        colorbar()
        xlabel('Bins #')
        ylabel('Pixel X')
        title('Pixel X vs TOF (for only X and Y range of interest')

    if bPlot:
        tof_axis = mt2.readX(0)[:]
        counts_vs_tof = zeros(len(tof_axis) - 1)
        for x in range(maxY):
            counts_vs_tof += mt2.readY(x)[:]
        index_tof_min = wks_utility.getIndex(TOFrange[0], tof_axis)
        index_tof_max = wks_utility.getIndex(TOFrange[1], tof_axis)
        _tof_axis = tof_axis[index_tof_min:index_tof_max]
        _y_axis = counts_vs_tof[index_tof_min:index_tof_max]
        subplot(2, 2, 4)
        plt.plot(_tof_axis, _y_axis, 'b', label='Before background sub.')
        xlabel('TOF (microS)')
        ylabel('Counts')
        title('Counts vs TOF')

    #Background subtraction ======================

    #Due to the way the FlatBackground algorithm works, we need to transpose the 
    #workspace
    # TOF axis which used to be DataX is now Pixels(MaxY)
    # 
    Transpose(InputWorkspace='IntegratedDataWks',
              OutputWorkspace='TransposedID')
    ConvertToHistogram(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedID')
    FlatBackground(InputWorkspace='TransposedID',
                   OutputWorkspace='TransposedFlatID',
                   StartX=fromYpixel,
                   EndX=list_norm_peak[0][0])
    Transpose(InputWorkspace='TransposedFlatID',
              OutputWorkspace='NormWks')

    if bPlot:
        mt3 = mtd['NormWks']
        tof_axis = mt3.readX(0)[:]
        counts_vs_tof = zeros(len(tof_axis))
        for x in range(maxY):
            counts_vs_tof += mt3.readY(x)[:]
        index_tof_min = wks_utility.getIndex(TOFrange[0], tof_axis)
        index_tof_max = wks_utility.getIndex(TOFrange[1], tof_axis)
        _tof_axis = tof_axis[index_tof_min:index_tof_max]
        _y_axis = counts_vs_tof[index_tof_min:index_tof_max]
        subplot(2, 2, 4)
        plt.plot(_tof_axis, _y_axis, 'gx', label='After background sub.')

        #### divide data by normalize histo workspace
        Divide(LHSWorkspace='DataWks',
               RHSWorkspace='NormWks',
               OutputWorkspace='NormalizedWks')

        mt = mtd['NormalizedWks']
        _tof_axis = mt.readX(0)[:]
        _y_axis = zeros((maxY, len(_tof_axis)))
        if bPlot:
            figure()
            for x in range(maxY):
                _y_axis[x, :] = mt.readY(x)[:]
            #    subplot(2, 2, 1)
            plt.imshow(log(_y_axis), aspect='auto', origin='lower')
            colorbar()
            xlabel('Bins #')
            ylabel('Pixel X')
            title('Normalized data')

        #### go from pixel/tof to theta/lambda
        theta = tthd_rad - ths_rad
        _pixel_axis = range(maxY)    

        figure()
        subplot(2, 2, 1)
        plt.imshow(log(_y_axis), aspect='auto', origin='lower')
        xlabel('TOF bins')
        ylabel('Pixel')
        title('3d view after data/normalization')

        subplot(2, 2, 2)
        tof_axis = mt.readX(0)[:]
        print tof_axis
        counts_vs_tof = zeros(len(tof_axis))
        for x in range(maxY):
            counts_vs_tof += mt.readY(x)[:]
        index_tof_min = wks_utility.getIndex(TOFrange[0], tof_axis)
        index_tof_max = wks_utility.getIndex(TOFrange[1], tof_axis)
        _tof_axis = tof_axis[index_tof_min:index_tof_max]
        _y_axis = counts_vs_tof[index_tof_min:index_tof_max]

        plt.plot(_tof_axis, _y_axis, 'b', label='data/normalization')
        xlabel('TOF (microS)')
        ylabel('Counts')








if bPlot: #show plots
    legend()
    show()
