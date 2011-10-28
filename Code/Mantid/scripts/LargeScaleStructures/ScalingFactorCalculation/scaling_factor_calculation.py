'''
Created on Oct 5, 2011

@author: j35
'''

from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import matplotlib.pyplot as plt
import sf_utility
import workspace_utility

#read data
#nexus_file = '/mnt/hgfs/j35/REF_L_55889_event.nxs'    

nexus_path = '/mnt/hgfs/j35/'
list_runs = {'D0': 55889,
             'AiD0': 55890,
             'AiD1': 55891,
             'AiD2': 55892,
             'AiD3': 55893,
             'AiD4': 55894,
             'AiD5': 55895,
             'AiiAiD5': 55896,
             'AiiAiD6': 55897,
             'AiiAiD7': 55898,
             'AiiiAiiAiD7': 55899,
             'AiiiAiiAiD8': 55900,
             'AivAiiiAiiAiD8': 55901,
             'AivAiiiAiiAiD9': 55902}
pre = 'REF_L_'
post = '_event.nxs'

#keep only the range of counts_vs_tof we want
#from tof_min to tof_max
tof_min = 10000  #microS
tof_max = 21600  #microS

#range of x pixel to use in the X integration (we found out that there is a frame effect that introduces noise)
x_pixel_min = 90
x_pixel_max = 190

peak_pixel_min = 129
peak_pixel_max = 136

back_pixel_min = 126
back_pixel_max = 140

#from,width,to in microS
rebin_parameters = '0,200,200000'  

#turn on or off the plots
bPlot = True

#=====================
#work with Numerator
#=====================

#key of working file
working_file = 'AiD1'

run_number = list_runs[working_file]
nexus_file = nexus_path + pre + str(run_number) + post

LoadEventNexus(Filename=nexus_file, OutputWorkspace='EventDataWks')
mt = mtd['EventDataWks']

#test if the error are rights
raw_data_error = sf_utility.getPixelXPixelYError(mt)

#get the proton charge in picoCoulomb
proton_charge = sf_utility.getProtonCharge(mt)

#get slit #1 and #2 heights
s1h_value, s1h_units = sf_utility.getS1h(mt)
s2h_value, s2b_units = sf_utility.getS2h(mt)

#rebin data
rebin(InputWorkspace='EventDataWks', OutputWorkspace='HistoDataWks', Params=rebin_parameters)
mt1 = mtd['HistoDataWks']

pixelX_vs_pixelY = sf_utility.getPixelXPixelY(mt1) #[256,304] array
if bPlot is True:
    fig=plt.figure(figsize=(20,15))
    ax = fig.add_subplot(2,2,1)
    plt.imshow(log(pixelX_vs_pixelY), aspect='auto', origin='lower')
    xlabel('Pixel X')
    ylabel('Pixel Y')
    title('Pixel Y vs Pixel X')

pixelX_vs_pixelY_error = sf_utility.getPixelXPixelYError(mt1) #[256,304] array
pixelX_vs_tof = sf_utility.getPixelXTOF(mt1) #[256,#bins]

#display PixelX vs TOF plot
if bPlot is True:
    subplot(2,2,2)
    plt.imshow(log(pixelX_vs_tof), aspect='auto', origin='lower')
    xlabel('Bins #')
    ylabel('Pixels')
    title('Pixel X vs TOF or rebinned data')

#first step should be to create a new EventWorkspace of only the 256 pixel integrated over the 304 pixels
_x_axis = mt1.readX(0)[:]
mt3 = workspace_utility.create_integrated_workspace(mt1, proton_charge, 
                                                    from_pixel=x_pixel_min, 
                                                    to_pixel=x_pixel_max)

##done just to test the error values
#pixelX_vs_tof_error_mt3 = zeros((256, len(_x_axis)-1))
#for y in range(256):
#    pixelX_vs_tof_error_mt3[y,:] = mt3.readE(y)[:]       

if bPlot is True:
    _y_axis = zeros((256, len(_x_axis) - 1))
    for x in range(256):
        _y_axis[x, :] = mt3.readY(x)[:]
    subplot(2,2,3)
    plt.imshow(log(_y_axis), aspect='auto', origin='lower')
    xlabel('Bins #')
    ylabel('Pixel X')
    title('Pixel X vs TOF (for Y range of interest)')

        
#save counts vs tof of data before background subtraction for debug/plot purpose only
if bPlot is True:
    counts_vs_tof_mt3 = zeros(len(_x_axis) - 1)
    for x in range(256):
        counts_vs_tof_mt3 += mt3.readY(x)[:]
    index_tof_min = sf_utility.get_index(tof_min, _x_axis)
    index_tof_max = sf_utility.get_index(tof_max, _x_axis)
    x_axis_mt3 = _x_axis[index_tof_min:index_tof_max] #tof axis
    y_axis_mt3 = counts_vs_tof_mt3[index_tof_min:index_tof_max] #counts axis
    subplot(2,2,4)
    plt.plot(x_axis_mt3, y_axis_mt3, 'b', label='Before background sub.')
    xlabel('TOF (microS)')
    ylabel('Counts')
    title('Counts vs TOF')


#Background subtraction ===================
#Transpose workspace DataX is now Pixels and DataY is now TOF 
Transpose(Inputworkspace='IntegratedDataWks', OutputWorkspace='TransposeIntegratedDataWks')
mt4 = mtd['TransposeIntegratedDataWks']
nbr_spectrum = mt4.getNumberHistograms()
counts_vs_tof_mt4 = zeros(256)
for x in range(nbr_spectrum):
    counts_vs_tof_mt4 += mt4.readY(x)[:]

_y_axis_mt4 = zeros((nbr_spectrum, 256))
#_y_axis_error_mt4 = zeros((nbr_spectrum, 256))
for x in range(nbr_spectrum):
    _y_axis_mt4[x, :] = mt4.readY(x)[:]
#    _y_axis_error_mt4[x,:] = mt4.readE(x)[:]

#create pixel list to use to calculate background
WorkspaceIndexList_1 = arange(peak_pixel_min - back_pixel_min) + back_pixel_min
WorkspaceIndexList_2 = arange(back_pixel_max - peak_pixel_max) + peak_pixel_max
WorkspaceIndexList = append(WorkspaceIndexList_1, WorkspaceIndexList_2)

#============================================================================
ConvertToHistogram(InputWorkspace='TransposeIntegratedDataWks', OutputWorkspace='TransposeIntegratedDataWks_t')
#window figure just to debug flat background
mt_after_histo = mtd['TransposeIntegratedDataWks_t']
pixel_vs_tof_after_histo = zeros((len(_x_axis) - 1, 256))
#pixel_vs_tof_after_histo_error = zeros((len(_x_axis) - 1, 256))
for x in range(len(_x_axis) - 1):
    pixel_vs_tof_after_histo[x, :] = mt_after_histo.readY(x)[:]
#    pixel_vs_tof_after_histo_error[x, :] = mt_after_histo.readE(x)[:]


FlatBackground(InputWorkspace='TransposeIntegratedDataWks_t', OutputWorkspace='TransposeHistoFlatDataWks', StartX=back_pixel_min, EndX=peak_pixel_min)
#mt_after_flat = mtd['TransposeHistoFlatDataWks']
#pixel_vs_tof_after_flat = zeros((len(_x_axis) - 1, 256))
#for x in range(len(_x_axis) - 1):
#    pixel_vs_tof_after_flat[x, :] = mt_after_flat.readY(x)[:]
#subplot(2, 2, 2)
#plt.imshow(log(pixel_vs_tof_after_flat), aspect='auto', origin='lower')
#title('After flat back')

##Transpose back
Transpose(Inputworkspace='TransposeHistoFlatDataWks', OutputWorkspace='DataWks')
mt5 = mtd['DataWks']

_x_axis_mt5 = mt5.readX(0)[:]
_y_axis_mt5 = zeros((256, nbr_spectrum))
for x in range(256):
    _y_axis_mt5[x,:] = mt5.readY(x)[:]

counts_vs_tof_mt5 = zeros(len(_x_axis) - 1)
counts_vs_tof_error_mt5 = zeros(len(_x_axis)-1)
for x in range(256):
    counts_vs_tof_mt5 += mt5.readY(x)[:]
    counts_vs_tof_error_mt5 += mt5.readE(x)[:]**2
counts_vs_tof_error_mt5 = sqrt(counts_vs_tof_error_mt5)
index_tof_min = sf_utility.get_index(tof_min, _x_axis)
index_tof_max = sf_utility.get_index(tof_max, _x_axis)
x_axis_mt5 = x_axis_mt3
y_axis_numerator = counts_vs_tof_mt5[index_tof_min:index_tof_max]
y_axis_error_numerator = counts_vs_tof_error_mt5[index_tof_min:index_tof_max]

if bPlot is True:
    subplot(2, 2, 4)
    plot(x_axis_mt5, y_axis_numerator, 'gx', label='after back sub')

#======================
#work with Denominator
#======================

#key of working file
working_file = 'AiD0'

run_number = list_runs[working_file]
nexus_file = nexus_path + pre + str(run_number) + post

LoadEventNexus(Filename=nexus_file, OutputWorkspace='EventDataWks')
mt = mtd['EventDataWks']

#get the proton charge in picoCoulomb
proton_charge = sf_utility.getProtonCharge(mt)

#get slit #1 and #2 heights
s1h_value, s1h_units = sf_utility.getS1h(mt)
s2h_value, s2b_units = sf_utility.getS2h(mt)

#rebin data
rebin(InputWorkspace='EventDataWks', OutputWorkspace='HistoDataWks', Params=rebin_parameters)
mt1 = mtd['HistoDataWks']

pixelX_vs_pixelY = sf_utility.getPixelXPixelY(mt1) #[256,304] array
if bPlot is True:
    fig=plt.figure(figsize=(20,15))
    ax = fig.add_subplot(2,2,1)
    plt.imshow(log(pixelX_vs_pixelY), aspect='auto', origin='lower')
    xlabel('Pixel X')
    ylabel('Pixel Y')
    title('Pixel Y vs Pixel X')

pixelX_vs_pixelY_error = sf_utility.getPixelXPixelYError(mt1) #[256,304] array
pixelX_vs_tof = sf_utility.getPixelXTOF(mt1) #[256,#bins]

#display PixelX vs TOF plot
if bPlot is True:
    subplot(2,2,2)
    plt.imshow(log(pixelX_vs_tof), aspect='auto', origin='lower')
    xlabel('Bins #')
    ylabel('Pixels')
    title('Pixel X vs TOF or rebinned data')

#first step should be to create a new EventWorkspace of only the 256 pixel integrated over the 304 pixels
_x_axis = mt1.readX(0)[:]
mt3 = workspace_utility.create_integrated_workspace(mt1, proton_charge, 
                                                    from_pixel=x_pixel_min, 
                                                    to_pixel=x_pixel_max)

if bPlot is True:
    _y_axis = zeros((256, len(_x_axis) - 1))
    for x in range(256):
        _y_axis[x, :] = mt3.readY(x)[:]
    subplot(2,2,3)
    plt.imshow(log(_y_axis), aspect='auto', origin='lower')
    xlabel('Bins #')
    ylabel('Pixel X')
    title('Pixel X vs TOF (for Y range of interest)')

        
#save counts vs tof of data before background subtraction for debug/plot purpose only
if bPlot is True:
    counts_vs_tof_mt3 = zeros(len(_x_axis) - 1)
    for x in range(256):
        counts_vs_tof_mt3 += mt3.readY(x)[:]
    index_tof_min = sf_utility.get_index(tof_min, _x_axis)
    index_tof_max = sf_utility.get_index(tof_max, _x_axis)
    x_axis_mt3 = _x_axis[index_tof_min:index_tof_max] #tof axis
    y_axis_mt3 = counts_vs_tof_mt3[index_tof_min:index_tof_max] #counts axis
    subplot(2,2,4)
    plt.plot(x_axis_mt3, y_axis_mt3, 'b', label='Before background sub.')
    xlabel('TOF (microS)')
    ylabel('Counts')
    title('Counts vs TOF')


#Background subtraction ===================
#Transpose workspace DataX is now Pixels and DataY is now TOF 
Transpose(Inputworkspace='IntegratedDataWks', OutputWorkspace='TransposeIntegratedDataWks')
mt4 = mtd['TransposeIntegratedDataWks']
nbr_spectrum = mt4.getNumberHistograms()
counts_vs_tof_mt4 = zeros(256)
for x in range(nbr_spectrum):
    counts_vs_tof_mt4 += mt4.readY(x)[:]
#subplot(3, 2, 3)
#plot(range(256), counts_vs_tof_mt4, label='before back sub')
#title('Counts vs TOF after transpose')    

_y_axis_mt4 = zeros((nbr_spectrum, 256))
for x in range(nbr_spectrum):
    _y_axis_mt4[x, :] = mt4.readY(x)[:]
#subplot(3, 2, 3)
#title('TOF vs pixel after transpose')
#plt.imshow(log(_y_axis_mt4), aspect='auto', origin='lower')

#create pixel list to use to calculate background
WorkspaceIndexList_1 = arange(peak_pixel_min - back_pixel_min) + back_pixel_min
WorkspaceIndexList_2 = arange(back_pixel_max - peak_pixel_max) + peak_pixel_max
WorkspaceIndexList = append(WorkspaceIndexList_1, WorkspaceIndexList_2)

#============================================================================
ConvertToHistogram(InputWorkspace='TransposeIntegratedDataWks', OutputWorkspace='TransposeIntegratedDataWks_t')
#window figure just to debug flat background
mt_after_histo = mtd['TransposeIntegratedDataWks_t']
pixel_vs_tof_after_histo = zeros((len(_x_axis) - 1, 256))
for x in range(len(_x_axis) - 1):
    pixel_vs_tof_after_histo[x, :] = mt_after_histo.readY(x)[:]
#subplot(2, 2, 3)
#plt.imshow(log(pixel_vs_tof_after_histo), aspect='auto', origin='lower')
#title('After conversion to histo')

FlatBackground(InputWorkspace='TransposeIntegratedDataWks_t', OutputWorkspace='TransposeHistoFlatDataWks', StartX=back_pixel_min, EndX=peak_pixel_min)
#mt_after_flat = mtd['TransposeHistoFlatDataWks']
#pixel_vs_tof_after_flat = zeros((len(_x_axis) - 1, 256))
#for x in range(len(_x_axis) - 1):
#    pixel_vs_tof_after_flat[x, :] = mt_after_flat.readY(x)[:]
#subplot(2, 2, 2)
#plt.imshow(log(pixel_vs_tof_after_flat), aspect='auto', origin='lower')
#title('After flat back')

##Transpose back
Transpose(Inputworkspace='TransposeHistoFlatDataWks', OutputWorkspace='DataWks')
mt5 = mtd['DataWks']
counts_vs_tof_mt5 = zeros(len(_x_axis) - 1)
counts_vs_tof_error_mt5 = zeros(len(_x_axis)-1)
for x in range(256):
    counts_vs_tof_mt5 += mt5.readY(x)[:]
    counts_vs_tof_error_mt5 += mt5.readE(x)[:]**2

counts_vs_tof_error_mt5 = sqrt(counts_vs_tof_error_mt5)    
index_tof_min = sf_utility.get_index(tof_min, _x_axis)
index_tof_max = sf_utility.get_index(tof_max, _x_axis)
x_axis_mt5 = x_axis_mt3
y_axis_denominator = counts_vs_tof_mt5[index_tof_min:index_tof_max]
y_axis_error_denominator = counts_vs_tof_error_mt5[index_tof_min:index_tof_max]
if bPlot is True:
    subplot(2, 2, 4)
    plot(x_axis_mt5, y_axis_denominator, 'gx', label='after back sub')

#=========================
# Numerator / Denominator
#=========================

#ratio of intensities and errors
y_axis = y_axis_numerator/y_axis_denominator
x_axis = _x_axis[index_tof_min:index_tof_max]

_part1 = (y_axis_error_numerator / y_axis_numerator)**2
_part2 = (y_axis_error_denominator / y_axis_denominator)**2
error_y_axis = y_axis * sqrt(_part1 + _part2)


#Now, we need to fit this curve
CreateWorkspace("Data_to_fit", DataX=x_axis, DataY=y_axis, DataE=error_y_axis, Nspec=1)
Fit(InputWorkspace='Data_to_fit', Function="name=UserFunction, Formula=a+b*x, a=1, b=2", Output='res')

res=mtd['res_Parameters']
print 'a= ' + str(res.getDouble("Value",0))
print 'b= ' + str(res.getDouble("Value",1))

#Display the data and the final fitting curve
mt6 = mtd['Data_to_fit']
x_axis = mt6.readX(0)[:]
y_axis = mt6.readY(0)[:]
y_error_axis = mt6.readE(0)[:]

if bPlot is True:
    legend()
    show()


























#nbr_spectrum = mt5.getNumberHistograms()
#counts_vs_tof_mt5 = zeros(256)
#for x in range(nbr_spectrum):
#    counts_vs_tof_mt5 += mt4.readY(x)[:]
#subplot(3,2,2)
#plot(range(256),counts_vs_tof_mt5,'gx', label='after back sub')


#Transpose(InputWorkspace='TransposeHistoFlatDataWks', OutputWorkspace='HistoFlatDataWks')
#
##visualize 2d counts vs TOF and/or pixelY(256) vs TOF
#mt2=mtd['HistoFlatDataWks']
#counts_vs_tof = zeros(len(_x_axis)-1)
#y_axis_error_numerator = zeros(len(_x_axis)-1)
#for x in range(256):
#    counts_vs_tof += mt2.readY(x)[:]
#    y_axis_error_numerator += (mt2.readE(x)[:]*mt2.readE(x)[:])
#
#y_axis_error_numerator = sqrt(y_axis_error_numerator)    
#index_tof_min = get_index(tof_min, _x_axis)
#index_tof_max = get_index(tof_max, _x_axis)
##print 'index_tof_min: ' + str(index_tof_min)
##print 'index_tof_max: ' + str(index_tof_max)
#
#x_axis = _x_axis[index_tof_min:index_tof_max] #tof axis
#y_axis_numerator = counts_vs_tof[index_tof_min:index_tof_max] #counts axis
#y_axis_error_numerator = y_axis_error_numerator[index_tof_min:index_tof_max]
#
#subplot(3,2,2)
#title('Counts vs TOF')
#plot(x_axis, y_axis_numerator,'r^',label='after back sub')
#legend()
#
#subplot(3,2,5)
#title('PixelX vs TOF')
#_y_fit_axis = zeros((256,len(_x_axis)-1))
#for x in range(256):
#    _y_fit_axis[x,:] = mt2.readY(x)[:]
#plt.imshow(log(_y_fit_axis),aspect='auto',origin='lower')
#
#show()







##=====================
##work with Denominator
##=====================
#
##key of working file
#working_file = 'AiD0'
#
#run_number = list_runs[working_file]
#nexus_file = nexus_path + pre + str(run_number) + post
#
#LoadEventNexus(Filename=nexus_file, OutputWorkspace='EventDataWks')
#
#mt = mtd['EventDataWks']
#mt_run = mt.getRun()
#
##get proton charge
#proton_charge_mtd_unit = mt_run.getProperty('gd_prtn_chrg').value
##conversion in picoCoulomb
#proton_charge = proton_charge_mtd_unit / 2.77777778e-10
#
##get value of s1h and s2h 
#s1t = mt_run.getProperty('s1t').value
#s1b = mt_run.getProperty('s1b').value
#unit = mt_run.getProperty('s1t').units
#s1h = float(s1b[0]) - float(s1t[0])
##print 's1h is ' + str(s1h) + unit
#
#s2t = mt_run.getProperty('s2t').value
#s2b = mt_run.getProperty('s2b').value
#unit = mt_run.getProperty('s2t').units
#s2h = float(s2b[0]) - float(s2t[0])
##print 's2h is ' + str(s2h) + unit
#
##rebin data
#rebin(InputWorkspace='EventDataWks', OutputWorkspace='HistoDataWks', Params='0,200,200000')
#mt1 = mtd['HistoDataWks']
#
##Determine pixel X vs pixel Y plot
#pixelX_vs_pixelY = zeros((256,304))
#for x in range(304):
#    for y in range(256):
#        _index = 256*x + y
#        _sum = sum(mt1.readY(_index)[:])
#        pixelX_vs_pixelY[y,x] = _sum
#
##Determine pixel x vs pixel y of error
#pixel_error=zeros((256,304))
#for x in range(304):
#    for y in range(256):
#        _index=256*x+y
#        _sum=sum(mt1.readE(_index)[:])
#        pixel_error[y,x]=_sum       
#    
##determine pixel X vs tof
#_init = mt1.readY(0)[:]
#pixelX_vs_tof = zeros((256,len(_init)))
#for x in range(304):
#    for y in range(256):
#        _index = 256*x + y
#        _array = mt1.readY(_index)[:]
#        pixelX_vs_tof[y,:] += _array
#
##first step should be to create a new EventWorkspace of only the 256 pixel integrated over the 304 pixels
#_x_axis = mt1.readX(0)[:]
#_y_axis = zeros((256,len(_x_axis)-1))
#_y_error_axis = zeros((256,len(_x_axis)-1))
#for x in range(304):
#    for y in y_range:
#        _index = int(256*x+y)
#        _y_axis[y,:] += mt1.readY(_index)[:]
#        _y_error_axis[y,:] += ((mt1.readE(_index)[:])*(mt1.readE(_index)[:]))
#_y_axis = _y_axis.flatten()
#_y_error_axis = sqrt(_y_error_axis)
#plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
#_y_error_axis = _y_error_axis.flatten()
#
##normalization by proton charge
#_y_axis /= (proton_charge*1e-12)
#
#CreateWorkspace("IntegratedDataWks", DataX=_x_axis, DataY=_y_axis, DataE=_y_error_axis, Nspec=256)
###to check that the data are still right
##mt=mtd['IntegratedDataWks']
##_y_axis = zeros((256,len(_x_axis)-1))
##for x in range(256):
##    _y_axis[x,:] = mt.readY(x)[:]
#
##create pixel list to use to calculate background
#WorkspaceIndexList_1 = arange(peak_pixel_min-back_pixel_min)+back_pixel_min
#WorkspaceIndexList_2 = arange(back_pixel_max-peak_pixel_max)+peak_pixel_max
#WorkspaceIndexList = append(WorkspaceIndexList_1, WorkspaceIndexList_2)
#
#FlatBackground(InputWorkspace='IntegratedDataWks', WorkspaceIndexList=WorkspaceIndexList, OutputWorkspace='HistoFlatDataWks', StartX=0, EndX=len(_x_axis)-1)
##visualize 2d counts vs TOF and/or pixelY(256) vs TOF
#mt2=mtd['HistoFlatDataWks']
#counts_vs_tof = zeros(len(_x_axis)-1)
#y_axis_error_denominator = zeros(len(_x_axis)-1)
#for x in range(256):
#    counts_vs_tof += mt2.readY(x)[:]
#    y_axis_error_denominator += ((mt2.readE(x)[:])*(mt2.readE(x)[:]))
#
#y_axis_error_denominator = sqrt(y_axis_error_denominator)    
#
#x_axis = _x_axis[index_tof_min:index_tof_max] #tof axis
#y_axis_denominator = counts_vs_tof[index_tof_min:index_tof_max] #counts axis
#y_axis_error_denominator = y_axis_error_denominator[index_tof_min:index_tof_max]
#
##ratio of intensities and errors
#y_axis = y_axis_numerator/y_axis_denominator
#_part1 = (y_axis_error_numerator / y_axis_numerator)**2
#_part2 = (y_axis_error_denominator / y_axis_denominator)**2
#error_y_axis = y_axis * sqrt(_part1 + _part2)
#
##Now, we need to fit this curve
#CreateWorkspace("Data_to_fit", DataX=x_axis, DataY=y_axis, DataE=error_y_axis, Nspec=1)
#
#Fit(InputWorkspace='Data_to_fit', Function="name=UserFunction, Formula=a+b*x, a=1, b=2", Output='res')
#
#res=mtd['res_Parameters']
#print 'a= ' + str(res.getDouble("Value",0))
#print 'b= ' + str(res.getDouble("Value",1))

    




    

##Determine pixel X vs TOF array
#pixelX_vs_tof = zeros((256,len(_init)))
#for x in range(304):
#    for y in range(256):
#        _index = 256*x + y
#        _array = mt.readY(_index)[:]
#        pixelX_vs_tof[y,:] += _array
#
#_y_axis = range(256) #[0,1,2,3.... 255]

##Determine pixel X vs pixel Y plot
#pixelX_vs_pixelY = zeros((256,304))
#for x in range(304):
#    for y in range(256):
#        _index = 256*x + y
#        _sum = sum(mt.readY(_index)[:])
#        pixelX_vs_pixelY[y,x] = _sum



