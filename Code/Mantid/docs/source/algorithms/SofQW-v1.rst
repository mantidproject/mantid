.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is for use by inelastic instruments and takes as its
input a workspace where the data's been reduced to be in `units <http://www.mantidproject.org/Units>`_ 
of **energy transfer** against spectrum number (which can be seen as equivalent to
angle, with the angle being taken from the detector(s) to which the
spectrum pertains).

This algorithm can operate in one of three modes. Each mode simply runs a different algorithm to perform the computation:

- *Centre*: performs a basic centre-point rebinning, see :ref:`algm-SofQWCentre`
- *Polygon*: performs a parallel-piped rebin, taking into account the curvature of the output bins see :ref:`algm-SofQWPolygon`
- *NormalisedPolygon*: performs the same rebin as *Polygon* but the output bins normalised by the contributing overlap area see :ref:`algm-SofQWNormalisedPolygon`

The energy binning will not be changed by this algorithm, so the input
workspace should already have the desired bins (though this axis can be
rebinned afterwards if desired). The EMode and EFixed parameters are
used for the calculation of :math:`Q`.


If the input workspace is a distribution (i.e. **counts/meV** ) then the
output workspace will similarly be divided by the bin width in both
directions (i.e. will contain **counts/meV/(1/Angstrom)** ).

Usage
-----

**Example - simple transformation of inelastic workspace:**

.. testcode:: SofQW

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra values
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into MD workspace 
   ws=SofQW(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
   
   print "The converted X values are:"
   print ws.readX(59)[0:10]
   print ws.readX(59)[10:21]   
  
   print "The converted Y values are:"
   print ws.readY(59)[0:10]
   print ws.readY(59)[10:21]   


.. testcleanup:: SofQW

   DeleteWorkspace(ws)
   
**Output:**


.. testoutput:: SofQW

   The converted X values are: 
   [-10.  -9.  -8.  -7.  -6.  -5.  -4.  -3.  -2.  -1.]
   [  0.   1.   2.   3.   4.   5.   6.   7.   8.   9.  10.]   
   The converted Y values are: 
   [ 12.  18.  18.  18.  18.  21.  18.  18.  21.  12.]
   [ 18.  21.  24.  24.  24.  21.  24.  33.  39.  45.]


.. categories::
