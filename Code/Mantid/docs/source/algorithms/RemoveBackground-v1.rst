.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
Algorithm removes flat background, defined by the a single bin 
histogram workspace with X-axis in the units of TOF from a histogram workspace in any 
units with known conversion to TOF.

These options are especially useful during reduction 
of event workspaces in multirep mode, where different event regions are associated with 
different incident energies and rebinned into appropriate energy range.

The algorithm used during background removal is equivalent to the proof-of concept one, 
presented below, except intermediate workspaces are not created.

Errors of the background workspace are currently ignored and their value 
is calculated as the square root of correspondent background signal.

Proof of concept background removal algorithm::

  from mantid.simpleapi import *
  from mantid import config
  import numpy as np
  import sys
  import os
  
  
  maps_dir = '/home/user/InstrumentFiles/let/'
  data_dir ='/home/user/results'   
  ref_data_dir = '/home/user/SystemTests/AnalysisTests/ReferenceResults' 
  config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
  config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files. Defaults are in
  
  # the name of a workspace containing background
  filename = 'LET0007438'
  groupedFilename = filename+'rings';
  #
  Ei= 25
  e_min = -20
  e_max = 20
  dE = 0.1
  bgRange = [15000,18000]

  if not("Tgrid" in mtd):

    if not(groupedFilename in mtd):
        Load(Filename=filename+'.nxs', OutputWorkspace=filename, LoadMonitors=True)
        GroupDetectors(InputWorkspace=filename, OutputWorkspace=groupedFilename , MapFile='LET_one2one_123.map', Behaviour='Average')

    wsParent = mtd[groupedFilename];
    
    nHist = wsParent.getNumberHistograms();
    print "Parent workspace contains {0:10} histograms".format(nHist)
    # Get the energy binning correspondent to the binning produced by rebin function (not to re-implement the same function)
    ws1s = ExtractSingleSpectrum(wsParent,0);
    ws1s = ConvertUnits(ws1s,'DeltaE','Direct',Ei);
    ws1s = Rebin(ws1s,Params=[e_min,dE,e_max]);
    e_bins = ws1s.dataX(0);
    nBins =e_bins.size;

    x=[e_bins[i] for i in xrange(0,nBins)]
    y=[0 for xx in xrange(0,len(x)-1)]*nHist
    x = x*nHist
    DeleteWorkspace(ws1s);
    
    eGrid = CreateWorkspace(DataX=x,DataY=y,UnitX='DeltaE',NSpec=nHist,VerticalAxisUnit='SpectraNumber',ParentWorkspace=wsParent)
    
    Tgrid=ConvertUnits(eGrid,'TOF',Emode='Direct',EFixed=Ei)
    
  else:
    Tgrid = mtd['Tgrid'];
    eGrid = mtd['eGrid'];
    nHist = Tgrid.getNumberHistograms();
    nBins = Tgrid.dataX(0).size;

  if not('Bg' in mtd):
    Bg=Rebin(InputWorkspace=groupedFilename,  Params=[bgRange[0],bgRange[1]-bgRange[0],bgRange[1]],PreserveEvents=False)
    #Bg=CalculateFlatBackground(InputWorkspace=groupedFilename, StartX=bgRange[0], EndX=bgRange[1], Mode='Mean', OutputMode='Return Background', SkipMonitors=True)
  else:
    Bg = mtd['Bg']
    
  # Assign constant background to the Time grid workspace, minding different time bin width
  for nspec in xrange(0,nHist):
    bg            = Bg.dataY(nspec)
    if bg[0]>0:
       bgT           = Bg.dataX(nspec)  
       TimeScale     = Tgrid.dataX(nspec);
       # Jacobian for the unit conversion
       Jac           = (TimeScale[1:nBins]-TimeScale[0:nBins-1])*(bg[0]/(bgT[1]-bgT[0]));  
       error         = np.sqrt(Jac);
       eGrid.setY(nspec, Jac)
       eGrid.setE(nspec, error)
    else:  # signal and error for background is 0 anyway.
        pass
    #print " bg at spectra {0} equal to : {1}".format(nspec,bg[0])

        
  background = eGrid;
  resultEt   = ConvertUnits(groupedFilename,'DeltaE',Emode='Direct',EFixed=Ei)
  result     = Rebin(InputWorkspace=resultEt, Params=[e_min,dE,e_max],PreserveEvents=False)
  fr         = result-background;
  #
  sourceSum  = SumSpectra(result,0,nHist);
  bckgrdSum  = SumSpectra(background ,0,nHist);
  removedBkgSum = SumSpectra(fr ,0,nHist);

The results of executing this script on workspace contained measured background and the results of the background removal are
presented on the following picture:

.. image:: /images/BgRemoval.png

Blue line on this image represents the results, obtained using Rebin and Background removal after that. The results produced using 
the script below and shifted by one to show that there is another result plotted on the image, as both results 
are identical::

  from mantid.simpleapi import *
  from mantid import config
  import numpy as np
  import sys
  import os
  
  
  maps_dir = '/home/user/InstrumentFiles/let/'
  data_dir ='/home/user/results'   
  ref_data_dir = '/home/user/SystemTests/AnalysisTests/ReferenceResults' 
  config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
  config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files. Defaults are in
  
  # the name of a workspace containing background
  filename = 'LET0007438'
  groupedFilename = filename+'rings';
  #
  Ei= 25
  e_min = -20
  e_max = 20
  dE = 0.1
  bgRange = [15000,18000]


  if not(groupedFilename in mtd):
    Load(Filename=filename+'.nxs', OutputWorkspace=filename, LoadMonitors=True)
    GroupDetectors(InputWorkspace=filename, OutputWorkspace=groupedFilename , MapFile='LET_one2one_123.map', Behaviour='Average')
   
    
  if not('Bg' in mtd):
    Bg=Rebin(InputWorkspace=groupedFilename,  Params=[bgRange[0],bgRange[1]-bgRange[0],bgRange[1]],PreserveEvents=False)
  else:
    Bg = mtd['Bg']
    
  if  'resultEtransf' in mtd:
    resultEtransf   = mtd['resultEtransf']
  else:
    resultEtransf   = ConvertUnits(groupedFilename,'DeltaE',Emode='Direct',EFixed=Ei)
  
  noBgWorkspace = Rebin(InputWorkspace=resultEtransf, Params=[e_min,dE,e_max],PreserveEvents=False)
  noBgWorkspace= Rebin(InputWorkspace=noBgWorkspace,BkgWorkspace='Bg',EMode='Direct')
  nHist = Bg.getNumberHistograms()
  removedBkgSum = SumSpectra(noBgWorkspace ,0,nHist-1);    


Usage
-----


**Example - Background removal from a workspace in energy transfer units**

.. testcode:: ExFlatBkgRemoval

   # Create sample workspace with events
   Test=CreateSampleWorkspace(WorkspaceType='Event', Function='Flat background')
   # Add sample log necessary for unit conversion
   AddSampleLog(Test,'Ei',LogText='25.',LogType='Number');

   # Calculate background
   Bg = Rebin(Test,Params='15000,5000,20000',PreserveEvents=False);
   
   
   # Convert event's units
   Test_BgDE=ConvertUnits(Test,Target='DeltaE',EMode='Direct');
   
   # Calculate histograms for event workspace in energy binning
   Sample = Rebin(Test_BgDE,Params='-20,2,20',PreserveEvents=False);
   # Calculate histograms for event workspace in energy binning    
   Result   = Rebin(Test_BgDE,Params='-20,2,20',PreserveEvents=False);
   # Remove flat background in-place
   Result   = RemoveBackground(Result,BkgWorkspace='Bg',EMode='Direct');
   
   # Get access to the results
   XS = Sample.dataX(0);
   XR = Result .dataX(0);
   
   YS = Sample.dataY(0);
   YR = Result .dataY(0);
   
   ES = Sample.dataE(0);
   ER = Result .dataE(0);
   
   # print first spectra, Note invalid error calculations
   print "| x sampl  | x result | S sample | S no bg  | Err samp | Err no_bg|"
   for i in xrange(0,20):
      print "|{0:10}|{1:10}|{2:10.4f}|{3:10.3f}|{4:10.3f}|{5:10.3f}|".format(XS[i],XR[i],YS[i],YR[i],ES[i],ER[i]);
   
.. testoutput:: ExFlatBkgRemoval

   | x sampl  | x result | S sample | S no bg  | Err samp | Err no_bg|
   |     -20.0|     -20.0|    1.0000|    -0.959|     1.000|     1.216|
   |     -18.0|     -18.0|    2.0000|    -0.101|     1.414|     1.432|
   |     -16.0|     -16.0|    3.0000|     0.740|     1.732|     1.622|
   |     -14.0|     -14.0|    1.0000|    -1.441|     1.000|     1.312|
   |     -12.0|     -12.0|    5.0000|     2.353|     2.236|     1.955|
   |     -10.0|     -10.0|    2.0000|    -0.885|     1.414|     1.563|
   |      -8.0|      -8.0|    5.0000|     1.841|     2.236|     2.020|
   |      -6.0|      -6.0|    2.0000|    -1.481|     1.414|     1.655|
   |      -4.0|      -4.0|    4.0000|     0.139|     2.000|     1.983|
   |      -2.0|      -2.0|    3.0000|    -1.315|     1.732|     1.912|
   |       0.0|       0.0|    6.0000|     1.133|     2.449|     2.331|
   |       2.0|       2.0|    7.0000|     1.454|     2.646|     2.505|
   |       4.0|       4.0|    5.0000|    -1.400|     2.236|     2.388|
   |       6.0|       6.0|    7.0000|    -0.499|     2.646|     2.692|
   |       8.0|       8.0|    9.0000|     0.047|     3.000|     2.996|
   |      10.0|      10.0|   11.0000|     0.054|     3.317|     3.313|
   |      12.0|      12.0|   16.0000|     2.190|     4.000|     3.861|
   |      14.0|      14.0|   16.0000|    -2.188|     4.000|     4.135|
   |      16.0|      16.0|   26.0000|     0.490|     5.099|     5.075|
   |      18.0|      18.0|   39.0000|    -0.581|     6.245|     6.268|

  
.. categories::
