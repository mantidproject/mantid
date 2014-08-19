.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm ProcessBackground() provides several functions for user to
process background to prepare Le Bail Fit.


There are a few functional options for user to choose.
* SelectBackgroundPoints: selecting background points from diffraction data. Usually the output will be used to fit background;
* RemovePeaks: removing peaks from a given MatrixWorks from diffraction data;


Select Background Points
########################

This feature is designed to select many background points with user's
simple input. User is required to select only a few background points in
the middle of two adjacent peaks. Algorithm will fit these few points
(*BackgroundPoints*) to a background function of specified type.

The purpose of this option is to select as many background data points as possible
for future background function fitting. 

Prior information can be given by two modes.  Property 'SelectionMode' determines which modes to use.  
One (1)is from a list of X-values specified by users via property "BackgroundPoints". 
The other (2) is through a (background) function, whose type is specified by property "BackgroundType" and 
values are given via input table workspace "BackgroundTableWorkspace". 


Select background points from given X-values
============================================

Here is how it works.  Assume that the :math:`X^{(u)}` is the list of x values specified by users. 

* Create a data set (X, Y, E) from input workspace, where :math:`X_i` is the nearest value
  to :math:`X^{(u)}_i`;
* Fit the background function against the data set (X, Y, E);
* Select the data points, which are within a certain range above and below the fitted background function;
* The last step is to fit background function against the selected background points 

Select background points from given function
============================================


In this approach, the difference from the other apporach is to use the user given background function
to select data points within a range other than fitting the background function from given data points in the
other approach. 
Thus, it is just the last step of previous approach. 

Output workspaces
=================

- OutputWorkspace: It contains 3 spectra.  

  - spectrum 0: the selected background data points;
  - spectrum 1: the fitted background function against the selected data points;
  - spectrum 2: the difference of sepctrum 0 and 1

- OutputBackgroundParameterWorkspace: A table workspace containing the fitted parameter values including :math:`\chi^2`. 

- UserBackgroundWorkspace: a MatrixWorkspace to visualize by user. 
  
  - spectrum 0: background function (either given by user or fit from given data points) that is used to select background points;
  - spectrum 1: diffraction data with background (spectrum 0) removed;
  - spectrum 2: upper boundary on data points to be selected for spectrum 1;
  - spectrum 3: lower boundary on data points to be selected for spectrum 1


Algorithm properties
====================

Besides the common algorithm properties, below is the list of properties specific to this function option. 

- Inputs:

  - LowerBoundary
  - UpperBoundary
  - BackgroundType
  - SelectionMode
  - BackgroundOrder and BackgroundPoints / BackgroundTableWorkspace
  - NoiseTolerance
  - NegativeNoiseTolerance
  - OutputBackgroundType
  - OutputBackgroundOrder

- Outputs:
  
  - OutputBackgroundParameterWorkspace
  - UserBackgroundWorkspace

A suggested workflow 
####################

Here is a good example to select background points from a powder
diffraction pattern by calling ProcessBackground() in a self-consistent
manner.

1) Select a set of background points (X values only), which can roughly describes the background, manually;

2) Call ProcessBackground with Option='SelectBackgroundPoints' and SelectionMode='UserSpecifyBackground'.
   A good choice for background function to enter is 6-th order polynomial;
3) Plot spectra 2 to 4 in UserBackgroundWorkspace to check whether 'Tolerance' is proper or not.
   If not then reset the tolerance and run ProcessBackground again with previous setup;

4) Fit OutputWorkspace (the selected background) with a background function;

5) Call ProcessBackground with Option='SelectBackgroundPoints' and SelectionMode='UserFunction'.
   Set the background parameter workspace as the output parameter table workspace obtained in the last step;

6) Repeat step 4 and 5 for a few times until the background plot by fitted background function
   from selected background points is close enough to real background.

Simple Remove Peaks
###################

This algorithm is to remove peaks and output the backgrounds,
which can be used to fit an artibrary background function after calling this algorithm. 

It is assumed that the all peaks have been fitted reasonably well. 
Then by removing the peaks within range :math:`X_i^{(0)} \pm FWHM`,
and save the rest data points, which are very likely backgrounds, to an output workspace.  

Required and optional algorithm properties
==========================================

Besides the common algorithm properties, below is the list of properties specific to this function option. 

- Inputs: 

  - BraggPeakTableWorkspace
  - NumberOfFWHM

- Outputs:

  - UserBackgroundWorkspace: a dummy output for not raising trouble with python script


Add Region
##########

Replace a region, which is defined by 'LowerBoundary' and 'UpperBoundary', in a workspace
from another reference workspace. 


Required and optional algorithm properties
==========================================

- Inputs

  - LowerBoundary (required)
  - UpperBoundary (required)
  - ReferenceWorkspace (required)


Delete Region
#############

Removed a specified region, which is defined by 'LowerBoundary' and 'UpperBoundary', from the input workspace. 

Required and optional algorithm properties
==========================================

- Inputs

  - LowerBoundary (required)
  - UpperBoundary (required)


Usage
-----

**Example - Select background from a powgen data:**

.. testcode:: testSelectBkgd

  LoadAscii(Filename=r'PG3_15035-3.dat', OutputWorkspace='PG3_15035-3',Unit='TOF')

  outputs = ProcessBackground(InputWorkspace='PG3_15035-3', WorkspaceIndex = 0, Options='SelectBackgroundPoints',
        LowerBound='9726',UpperBound='119000', BackgroundType = 'Polynomial',  BackgroundOrder = 6,
        SelectionMode='FitGivenDataPoints', BackgroundPointSelectMode = "All Background Points",
        BackgroundPoints='10082,10591,11154,12615,13690,13715,15073,16893,17764,19628,21318,24192,35350,44212,50900,60000,69900,79000',
        NoiseTolerance = 0.10,
        OutputWorkspace='PG3_15035-3_BkgdPts', OutputBackgroundType = "Polynomial", OutputBackgroundOrder = 6,
        OutputBackgroundParameterWorkspace = "OutBackgroundParameters", UserBackgroundWorkspace="UserTheory")

  tbws = outputs[2]

  print "Number of output workspace = %d, Number of selected background points = %d" %( len(outputs),  len(outputs[0].readX(0)))
  print "Fitted background function: A0 = %.5e, A1 = %.5e, A2 = %.5e ..." % (tbws.cell(1, 1), tbws.cell(2, 1), tbws.cell(3,1))

.. testcleanup:: testSelectBkgd

  DeleteWorkspace(Workspace='PG3_15035-3')
  for i in xrange(3):
    DeleteWorkspace(Workspace=outputs[i])

Output:

.. testoutput:: testSelectBkgd

  Number of output workspace = 3, Number of selected background points = 4944
  Fitted background function: A0 = 5.43859e-01, A1 = -5.20674e-05, A2 = 2.84119e-09 ...

**Example - Add Region:**

.. testcode:: testAddRegion

  import math
  import random

  vecx = []
  vecy1 = []
  vecy2 = []
  vece = []

  x0 = 0.0
  dx = 0.01

  random.seed(1)
  for i in xrange(1000):
    x = x0 + float(i) * dx
    vecx.append(x)
    y = (random.random() - 0.5) * 2.0 + 2.0 + math.exp(-(x-4.0)**2/0.1)
    e = math.sqrt(y)
    vecy1.append(y)
    vecy2.append(-y)
    vece.append(e)

  ws1 = CreateWorkspace(DataX = vecx, DataY = vecy1, DataE = vece, NSpec = 1)
  ws2 = CreateWorkspace(DataX = vecx, DataY = vecy2, DataE = vece, NSpec = 1)

  outputs = ProcessBackground(InputWorkspace=ws1, WorkspaceIndex=0, OutputWorkspace="ws12", Options="AddRegion",
        LowerBound = 3.0, UpperBound = 5.0, ReferenceWorkspace = ws2)

  for i in [200, 400, 450, 500, 700]:
    print "X = %.5f, Input Y[%d] = %.5f, Reference Y[%d] = %.5f, Output Y[%d] = %.5f" % (vecx[i], i, ws1.readY(0)[i], i, ws2.readY(0)[i], i, outputs[0].readY(0)[i])

.. testcleanup:: testAddRegion

  DeleteWorkspace(Workspace=ws1)
  DeleteWorkspace(Workspace=ws2)
  for i in xrange(3):
    DeleteWorkspace(Workspace=outputs[i])

Output:

.. testoutput:: testAddRegion

  X = 2.00000, Input Y[200] = 1.65069, Reference Y[200] = -1.65069, Output Y[200] = 1.65069
  X = 4.00000, Input Y[400] = 3.81388, Reference Y[400] = -3.81388, Output Y[400] = -3.81388
  X = 4.50000, Input Y[450] = 2.68751, Reference Y[450] = -2.68751, Output Y[450] = -2.68751
  X = 5.00000, Input Y[500] = 2.00611, Reference Y[500] = -2.00611, Output Y[500] = 1.71367
  X = 7.00000, Input Y[700] = 1.12037, Reference Y[700] = -1.12037, Output Y[700] = 2.87033

**Example - Delete Region:**

.. testcode:: testDelRegion

  import math
  import random

  vecx = []
  vecy = []
  vece = []

  x0 = 0.0
  dx = 0.01

  random.seed(1)
  for i in xrange(1000):
    x = x0 + float(i) * dx
    vecx.append(x)
    y = (random.random() - 0.5) * 2.0 + 2.0 + math.exp(-(x-4.0)**2/0.1)
    e = math.sqrt(y)
    vecy.append(y)
    vece.append(e)

  ws1 = CreateWorkspace(DataX = vecx, DataY = vecy, DataE = vece, NSpec = 1)

  outputs = ProcessBackground(InputWorkspace=ws1, WorkspaceIndex=0, OutputWorkspace="ws2", Options="DeleteRegion",
        LowerBound = 3.0, UpperBound = 5.0)

  print "Input has %d data points; Output has %d data points." % ( len(ws1.readX(0)), len(outputs[0].readX(0)) )

.. testcleanup:: testDelRegion

  DeleteWorkspace(Workspace=ws1)
  for i in xrange(3):
    DeleteWorkspace(Workspace=outputs[i])

Output:

.. testoutput:: testDelRegion

  Input has 1000 data points; Output has 799 data points.

**Example - Remove peaks:**

.. testcode:: testRmPeaks

  import math
  import random

  vecx = []
  vecy = []
  vece = []
  numpts = 1000
  x0 = 0
  dx = 0.01

  random.seed(1)
  for i in xrange(1000):
    x = float(i)*dx
    y = 5 + (random.random() - 1)*2. + 10*math.exp( -(x-2.0)**2/0.1**2 ) + 20*math.exp( -(x-7.5)**2/0.05**2 )
    e = math.sqrt(y)
    vecx.append(x)
    vecy.append(y)
    vece.append(e)

  ws = CreateWorkspace(DataX = vecx, DataY = vecy, DataE = vece, NSpec = 1)
  peaktb = CreateEmptyTableWorkspace()
  peaktb.addColumn("double", "TOF_h")
  peaktb.addColumn("double", "FWHM")
  peaktb.addRow([2.0, 0.3])
  peaktb.addRow([7.40, 0.13])

  outputs = ProcessBackground(InputWorkspace=ws, WorkspaceIndex=0, OutputWorkspace="background",
      Options="RemovePeaks", BraggPeakTableWorkspace=peaktb, NumberOfFWHM=3)

  Fit(Function='name=Polynomial,n=1,A0=0.0,A1=0.0', InputWorkspace='background',  CreateOutput=True, StartX=0, EndX=9.9900000000000002,
      OutputNormalisedCovarianceMatrix='background_NormalisedCovarianceMatrix', OutputParameters='background_Parameters', OutputWorkspace='background_Workspace')

  outparws = mtd["background_Parameters"]
  print "Input workspace has %d data points; Output workspace has %d data points." % (len(ws.readX(0)), len(outputs[0].readX(0)))
  print "Fitted background parameters: A0 = %.5e, A1 = %.5e, Chi-square = %.5f" % (outparws.cell(0, 1), outparws.cell(1,1), outparws.cell(2,1))

.. testcleanup:: testRmPeaks

  DeleteWorkspace(Workspace=ws)
  for i in xrange(3):
      DeleteWorkspace(Workspace=outputs[i])
  DeleteWorkspace(Workspace="background_NormalisedCovarianceMatrix")
  DeleteWorkspace(Workspace="background_Parameters")
  DeleteWorkspace(Workspace="background_Workspace")
  DeleteWorkspace(Workspace="peaktb")

Output:

.. testoutput:: testRmPeaks

  Input workspace has 1000 data points; Output workspace has 741 data points.
  Fitted background parameters: A0 = 3.90254e+00, A1 = 1.09284e-02, Chi-square = 0.08237

.. categories::
