.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Extracts a 'block' from a workspace and places it in a new workspace
(or, to look at it another way, lops bins or spectra off a workspace).

CropWorkspace works on workspaces with common X arrays/bin boundaries or
on so-called `ragged workspaces <http://www.mantidproject.org/Ragged_Workspace>`__. If the input
workspace has common bin boundaries/X values then cropping in X will
lead to an output workspace with fewer bins than the input one. If the
boundaries are not common then the output workspace will have the same
number of bins as the input one, but with data values outside the X
range given set to zero.

If an X value given for XMin/XMax does not match a bin boundary (within
a small tolerance to account for rounding errors), then the closest bin
boundary within the range will be used. Note that if none of the
optional properties are given, then the output workspace will be a copy
of the input one.

Usage
-----
**Example - Crop a 5-bin workspace**

.. testcode:: ExCropWorkspace

   # Create a workspace with 1 spectrum with five bins of width 10
   ws = CreateSampleWorkspace(WorkspaceType="Histogram",  NumBanks=1, BankPixelWidth=1, BinWidth=10, Xmax=50)

   # Run algorithm  removing first and last bins
   OutputWorkspace = CropWorkspace(InputWorkspace=ws,XMin=10.0,XMax=40.0)

   # Show workspaces
   print "TOF Before CropWorkspace",ws.readX(0)
   print "TOF After CropWorkspace",OutputWorkspace.readX(0)
   
Output:

.. testoutput:: ExCropWorkspace

   TOF Before CropWorkspace [  0.  10.  20.  30.  40.  50.]
   TOF After CropWorkspace [ 10.  20.  30.  40.]

.. categories::
