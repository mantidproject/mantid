.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm rebins data with new bin boundaries. The 'params' property
defines new boundaries in intervals :math:`x_i-x_{i+1}\,`. Positive
:math:`\Delta x_i\,` make constant width bins, whilst negative ones
create logarithmic binning using the formula
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`

This algorithms is useful both in data reduction, but also in remapping
`ragged workspaces <http://www.mantidproject.org/Ragged_Workspace>`__ to a regular set of bin
boundaries.

Unless the FullBinsOnly option is enabled, the bin immediately before
the specified boundaries :math:`x_2`, :math:`x_3`, ... :math:`x_i` is
likely to have a different width from its neighbours because there can
be no gaps between bins. Rebin ensures that any of these space filling
bins cannot be less than 25% or more than 125% of the width that was
specified.


.. _rebin-example-strings:

Example Rebin param strings
###########################

* **"-0.0001"**: from min(TOF) to max(TOF) among all events in Logarithmic bins of 0.0001
* **"*0,100,20000"**: from 0 rebin in constant size bins of 100 up to 20,000
* **"2,-0.035,10"**: from 10 rebin in Logarithmic bins of 0.035 up to 10
* **"0,100,10000,200,20000"**: from 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000

For EventWorkspaces
###################

If the input is an `EventWorkspace <www.mantidproject.org/EventWorkspace>`__ and the "Preserve
Events" property is True, the rebinning is performed in place, and only
the X axes of the workspace are set. The actual Y histogram data will
only be requested as needed, for example, when plotting or displaying
the data.

If "Preserve Events" is false., then the output workspace will be
created as a :ref:`Workspace2D <Workspace2D>`, with fixed histogram bins,
and all Y data will be computed immediately. All event-specific data is
lost at that point.

For Data-Point Workspaces
#########################

If the input workspace contains data points, rather than histograms,
then Rebin will automatically use the
:ref:`ConvertToHistogram <algm-ConvertToHistogram>` and
:ref:`ConvertToHistogram <algm-ConvertToPointData>` algorithms before and after
the rebinning has taken place.

FullBinsOnly option
###################

If FullBinsOnly option is enabled, each range will only contain bins of
the size equal to the step specified. In other words, the will be no
space filling bins which are bigger or smaller than the other ones.

This, however, means that specified bin boundaries might get amended in
the process of binning. For example, if rebin *Param* string is
specified as "0, 2, 4.5, 3, 11" and FullBinsOnly is enabled, the
following will happen:

-  From 0 rebin in bins of size 2 **up to 4**. 4.5 is ignored, because
   otherwise we would need to create a filling bin of size 0.5.
-  **From 4** rebin in bins of size 3 **up to 10**.

Hence the actual *Param* string used is "0, 2, 4, 3, 10".


Remove Background during rebinning options
##########################################

These options allow you to remove flat background, defined by the a single bin 
histogram workspace with X-axis in the units of TOF from a workspace in any 
units with known conversion into TOF. The background removal occurs during rebinning

These options are especially useful during reduction 
of event workspaces in multirep mode, where different event regions are associated with 
different incident energies and rebinned into appropriate energy range together with 
background removal on-the-fly.

The algorithm used during background removal is equivalent to the one, presented below, except 
intermediate workspaces are not created and the background removal calculations
performed during rebinning::

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
       # jacobian for the unit conversion
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

Blue line on this image represents the results, obtained using Rebin with background removal. The results produced using 
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
  
    noBgWorkspace= Rebin(InputWorkspace=resultEtransf, Params=[e_min,dE,e_max],PreserveEvents=False,FlatBkgWorkspace='Bg',EMode='Direct')
    nHist = Bg.getNumberHistograms()
    removedBkgSum = SumSpectra(noBgWorkspace ,0,nHist);    

.. _rebin-usage:

Usage
-----

**Example - simple rebin of a histogram workspace:**

.. testcode:: ExHistSimple

   # create histogram workspace
   dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10)
   dataY = [1,1,1,1,1,1,1,1,1] # or use dataY=[1]*9
   ws = CreateWorkspace(dataX, dataY)

   # rebin from min to max with size bin = 2
   ws = Rebin(ws, 2)

   print "The rebinned X values are: " + str(ws.readX(0))
   print "The rebinned Y values are: " + str(ws.readY(0))

Output:

.. testoutput:: ExHistSimple

   The rebinned X values are: [ 0.  2.  4.  6.  8.  9.]
   The rebinned Y values are: [ 2.  2.  2.  2.  1.]

**Example - logarithmic rebinning:**

.. testcode:: ExHistLog

   # create histogram workspace
   dataX = [1,2,3,4,5,6,7,8,9,10] # or use dataX=range(1,11)
   dataY = [1,2,3,4,5,6,7,8,9] # or use dataY=range(1,10)
   ws = CreateWorkspace(dataX, dataY)

   # rebin from min to max with logarithmic bins of 0.5
   ws = Rebin(ws, -0.5)

   print "The 2nd and 3rd rebinned X values are: " + str(ws.readX(0)[1:3])

Output:

.. testoutput:: ExHistLog

   The 2nd and 3rd rebinned X values are: [ 1.5   2.25]

**Example - custom two regions rebinning:**

.. testcode:: ExHistCustom

   # create histogram workspace
   dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10)
   dataY = [0,1,2,3,4,5,6,7,8] # or use dataY=range(0,9)
   ws = CreateWorkspace(dataX, dataY)

   # rebin from 0 to 3 in steps of 2 and from 3 to 9 in steps of 3
   ws = Rebin(ws, "1,2,3,3,9")

   print "The rebinned X values are: " + str(ws.readX(0))

Output:

.. testoutput:: ExHistCustom

   The rebinned X values are: [ 1.  3.  6.  9.]

**Example - use option FullBinsOnly:**

.. testcode:: ExHistFullBinsOnly

   # create histogram workspace
   dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10)
   dataY = [1,1,1,1,1,1,1,1,1] # or use dataY=[1]*9
   ws = CreateWorkspace(dataX, dataY)

   # rebin from min to max with size bin = 2
   ws = Rebin(ws, 2, FullBinsOnly=True)

   print "The rebinned X values are: " + str(ws.readX(0))
   print "The rebinned Y values are: " + str(ws.readY(0))

Output:

.. testoutput:: ExHistFullBinsOnly

   The rebinned X values are: [ 0.  2.  4.  6.  8.]
   The rebinned Y values are: [ 2.  2.  2.  2.]

**Example - use option PreserveEvents:**

.. testcode:: ExEventRebin

   # create some event workspace
   ws = CreateSampleWorkspace(WorkspaceType="Event")

   print "What type is the workspace before 1st rebin: " + str(type(ws))
   # rebin from min to max with size bin = 2 preserving event workspace (default behaviour)
   ws = Rebin(ws, 2)
   print "What type is the workspace after 1st rebin: " + str(type(ws))
   ws = Rebin(ws, 2, PreserveEvents=False)
   print "What type is the workspace after 2nd rebin: " + str(type(ws))
   # note you can also check the type of a workspace using: print isinstance(ws, IEventWorkspace)

Output:

.. testoutput:: ExEventRebin

   What type is the workspace before 1st rebin: <class 'mantid.api._api.IEventWorkspace'>
   What type is the workspace after 1st rebin: <class 'mantid.api._api.IEventWorkspace'>
   What type is the workspace after 2nd rebin: <class 'mantid.api._api.MatrixWorkspace'>

**Example - Background removal during rebinning**

.. testcode:: ExRebinWithBkgRemoval

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
   # Calculate histograms for event workspace in energy binning and background removed
   Result   = Rebin(Test_BgDE,Params='-20,2,20',PreserveEvents=False,FlatBkgWorkspace='Bg',EMode='Direct');
   
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
   
.. testoutput:: ExRebinWithBkgRemoval

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
