.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compute the cross correlation function for a range of spectra with
respect to a reference spectrum.

This is use in powder diffraction experiments when trying to estimate
the offset of one spectra with respect to another one. The spectra are
converted in d-spacing and then interpolate on the X-axis of the
reference. The cross correlation function is computed in the range
[-N/2,N/2] where N is the number of points.

More details can be found
`here. <http://en.wikipedia.org/wiki/Cross-correlation>`__

Usage
-----
**Example - Crosscorrelate 2 spectra**

.. testcode:: ExCrossCorrelate

   
   #Create a workspace with 2 spectra with five bins of width 0.5
   ws = CreateSampleWorkspace(BankPixelWidth=1, XUnit='dSpacing', XMax=5, BinWidth=0.5)
   ws = ScaleX(InputWorkspace='ws', Factor=0.5, Operation='Add', IndexMin=1, IndexMax=1)
   # Run algorithm  CrossCorrelate
   OutputWorkspace = CrossCorrelate(InputWorkspace='ws', WorkspaceIndexMax=1, XMin=2, XMax=4)

   # Show workspaces
   print "AutoCorrelation",OutputWorkspace.readY(0)
   print "CrossCorrelation",OutputWorkspace.readY(1)

.. testoutput:: ExCrossCorrelate

   AutoCorrelation [-0.01890212  1.         -0.01890212]
   CrossCorrelation [-0.68136257  0.16838401  0.45685055]

.. categories::
