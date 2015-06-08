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
spectrum pertains). For each detector the value of **momentum transfer**
(:math:`Q`) is calculated, and the counts for detectors and each input 
**energy transfer** bin are added to the appropriate output :math:`(Q ;\Delta E)` bin.


The momentum transfer (:math:`Q`-values) obtained for each detector are calculated
for the detector center, so the binning algorithm uses center-point binning.
Use :ref:`algm-SofQWPolygon` and :ref:`algm-SofQWNormalisedPolygon` for more complex and precise (but slower)
binning strategies.

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

.. testcode:: SofQWCentre

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra values
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into MD workspace 
   ws=SofQWCentre(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
   
   print "The converted X values are:"
   print ws.readX(59)[0:10]
   print ws.readX(59)[10:21]   
  
   print "The converted Y values are:"
   print ws.readY(59)[0:10]
   print ws.readY(59)[10:21]   


.. testcleanup:: SofQWCentre

   DeleteWorkspace(ws)
   
**Output:**


.. testoutput:: SofQWCentre

   The converted X values are: 
   [-10.  -9.  -8.  -7.  -6.  -5.  -4.  -3.  -2.  -1.]
   [  0.   1.   2.   3.   4.   5.   6.   7.   8.   9.  10.]   
   The converted Y values are: 
   [ 12.  18.  18.  18.  18.  21.  18.  18.  21.  12.]
   [ 18.  21.  24.  24.  24.  21.  24.  33.  39.  45.]


.. categories::
