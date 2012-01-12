from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import wks_utility

#This will activate or not the various 2d or 1d plots
bPlot = True
bDataBack = True
bNorm = True
bNormBack = True


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

rebin_parameters = '0,200,200000'  #microS

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
                  
list_norm_peak = [[127, 133]]
list_norm_back = [[123, 137]]

########################################################################
########################################################################



#full path of data file
data_file = nexus_path + pre + list_data[0] + post

##load the data into its workspace
LoadEventNexus(Filename=data_file, OutputWorkspace='EventData')
mt = mtd['EventData']

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
##retrieve geometry of instrument
# distance Sample detector
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
#distance source->center of detector
dMD = dSD + dSM


##rebin data (x-axis is in TOF)
rebin(InputWorkspace='EventData', OutputWorkspace='HistoData', Params=rebin_parameters)

##keep only range of TOF of interest
CropWorkspace('HistoData','CropHistoData',XMin=TOFrange[0], XMax=TOFrange[1])

##Normalized by Current (proton charge)
NormaliseByCurrent(InputWorkspace='CropHistoData', OutputWorkspace='HistoData')
mt = mtd['HistoData']

##Calculation of the central pixel (using weighted average)
pixelXtof_data = wks_utility.getPixelXTOF(mt, maxX=maxX, maxY=maxY)
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

##Background subtraction
BackfromYpixel = list_data_back[0][0]
BacktoYpixel = list_data_back[0][1]

#Create a new event workspace of only the range of pixel of interest 
#background range (along the y-axis) and of only the pixel
#of interest along the x-axis (to avoid the frame effect)
mt2 = wks_utility.createIntegratedWorkspace(mt, "IntegratedDataWks",
                                            fromXpixel=Xrange[0],
                                            toXpixel=Xrange[1],
                                            fromYpixel=BackfromYpixel,
                                            toYpixel=BacktoYpixel,
                                            maxX=maxX,
                                            maxY=maxY)

theta = tthd_rad - ths_rad
_tof_axis = mt2.readX(0)[:]
########## This was used to test the R(Q) 
##Convert the data without background subtraction to R(Q)
q_array = wks_utility.convertToRvsQ(dMD=dMD,
                                    theta=theta,
                                    tof=_tof_axis)

q_array_reversed = q_array[::-1]
#if bPlot:
#    counts_vs_tof = zeros(len(_tof_axis) - 1)
#    for x in range(maxY):
#        counts_vs_tof += mt2.readY(x)[:]
#    _y_axis = counts_vs_tof
#    
#    fig2 = plt.figure(figsize=(10, 10))
#    ax = fig2.add_subplot(2, 1, 1)
#    _y_axis = _y_axis[::-1]
#    plt.plot(q_array_reversed, _y_axis, 'b', label='Before back. subtraction')
#    xlabel('Q(Angstroms^-1)')
#    ylabel('Counts')
#    title('R(Q)')

if bDataBack:
    Transpose(InputWorkspace='IntegratedDataWks',
              OutputWorkspace='TransposedID')
    ConvertToHistogram(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedID')
    FlatBackground(InputWorkspace='TransposedID',
                   OutputWorkspace='TransposedFlatID',
                   StartX=BackfromYpixel,
                   Mode='Mean',
                   EndX=list_data_peak[0][0])
    Transpose(InputWorkspace='TransposedFlatID',
              OutputWorkspace='DataWks')
    
#    if bPlot:
#        mt3 = mtd['DataWks']
#        counts_vs_tof = zeros(len(_tof_axis) - 1)
#        for x in range(maxY):
#            counts_vs_tof += mt3.readY(x)[:]
#        _y_axis = counts_vs_tof
#        _y_axis = _y_axis[::-1]
#        plt.plot(q_array_reversed, _y_axis, 'r', label='After back. subtraction')

if bNorm:
    
    ## Work on Normalization file now
    #check with first one first if it works
    norm_file = nexus_path + pre + list_norm[0] + post

    #load normalization file
    LoadEventNexus(Filename=norm_file, OutputWorkspace='EventNorm')
    mt = mtd['EventNorm']

    #rebin data
    rebin(InputWorkspace='EventNorm',
          OutputWorkspace='HistoNorm',
          Params=rebin_parameters)

    ##keep only range of TOF of interest
    CropWorkspace('HistoNorm', 
                  'CropHistoNorm', 
                  XMin=TOFrange[0], 
                  XMax=TOFrange[1])

    #Normalized by Current (proton charge)
    NormaliseByCurrent(InputWorkspace='CropHistoNorm', 
                       OutputWorkspace='NormWks')
    mt = mtd['NormWks']

    if bNormBack:
        
        ##Background subtraction
        BackfromYpixel = list_norm_back[0][0]
        BacktoYpixel = list_norm_back[0][1]

        #Create a new event workspace of only the range of pixel of interest 
        #background range (along the y-axis) and of only the pixel
        #of interest along the x-axis (to avoid the frame effect)
        mt3_norm = wks_utility.createIntegratedWorkspace(mt, "IntegratedNormWks",
                                            fromXpixel=Xrange[0],
                                            toXpixel=Xrange[1],
                                            fromYpixel=BackfromYpixel,
                                            toYpixel=BacktoYpixel,
                                            maxX=maxX,
                                            maxY=maxY)

        Transpose(InputWorkspace='IntegratedNormWks',
                  OutputWorkspace='TransposedID')
        
        ConvertToHistogram(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID')
        
        FlatBackground(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedFlatID',
                       StartX=BackfromYpixel,
                       Mode='Mean',
                       EndX=list_norm_peak[0][0])

        Transpose(InputWorkspace='TransposedFlatID',
                  OutputWorkspace='NormWks')
   
    #collapse data to 1 spectrum (peak region)
    Integration('NormWks','IntegratedNormWks',)
    
#    mt_NormWks = mtd['NormWks']
#    _tof_axis = mt_NormWks.readX(0)[:]
#    counts_vs_tof = zeros(len(_tof_axis))
#    from_norm_peak = list_norm_peak[0][0]
#    to_norm_peak = list_norm_peak[0][1]
#    NormPeakRange = arange(to_norm_peak-from_norm_peak+1) + from_norm_peak
#    for x in NormPeakRange:
#        print x
#        counts_vs_tof += mt_NormWks.readY(int(x))[:]
#    CreateWorkspace('NormWks', DataX=_tof_axis, DataY=counts_vs_tof, DataE=counts_vs_tof, Nspec=1)
   
    #### divide data by normalize histo workspace
    Divide(LHSWorkspace='DataWks',
           RHSWorkspace='NormWks',
           OutputWorkspace='NormalizedWks')

    mt4 = mtd['NormalizedWks']
    
    #display 2d plot at this point to see if we have data
    




else:
    
    mt4 = mtd['DataWks']
    
    
#ConvertToHistogram(InputWorkspace='NormalizedWks',
#                   OutputWorkspace='NormalizedWksHisto')
#    
#    
#Integration('NormalizedWksHisto','IntNormalizedWks')

##replace NaN values
#ReplaceSpecialValues(InputWorkspace='NormalizedWks',
#                     Outputworkspace='NormalizedWks',
#                     NaNValue=0)
    
GroupDetectors(InputWorkspace='NormalizedWks',
               OutputWorkspace='GroupNormalizedWks',
               DetectorList=range(255))
    
    
#    _q_axis = mt4.readX(0)[:]
#    counts_vs_q= zeros(len(_q_axis))
#    for x in range(maxY):
#        counts_vs_q += mt4.readY(x)[:]
#    _y_axis = counts_vs_q
    
    
mt = mtd['GroupNormalizedWks']
_y_axis = mt.readY(0)[:]
    
if bPlot:

    fig1 = plt.figure(figsize=(10, 10))
    ax = fig1.add_subplot(2, 1, 1)
    _y_axis = _y_axis[::-1]
    plt.plot(q_array_reversed, _y_axis)
    xlabel('Q(Angstroms^-1)')
    ylabel('Counts')
    title('R(Q) of Data/Norm')


if bPlot: #show plots
    legend()
    show()
