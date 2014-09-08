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

.. categories::
