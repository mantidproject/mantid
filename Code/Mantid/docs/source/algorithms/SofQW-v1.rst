.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is for use by inelastic instruments and takes as its
input a workspace where the data's been reduced to be in `units <Unit_Factory>`__ 
of **energy transfer** against spectrum number (which can be seen as equivalent to
angle, with the angle being taken from the detector(s) to which the
spectrum pertains). For each bin the value of **momentum transfer**
(:math:`\\Delta q`) is calculated, and the counts for that bin are assigned to
the appropriate :math:`\\Delta q` bin.

The energy binning will not be changed by this algorithm, so the input
workspace should already have the desired bins (though this axis can be
rebinned afterwards if desired). The EMode and EFixed parameters are
required for the calculation of :math:`\\Delta q`.

If the input workspace is a distribution (i.e. **counts/meV** ) then the
output workspace will similarly be divided by the bin width in both
directions (i.e. will contain **counts/meV/(1/Angstrom)** ).

Usage
-----

**Example - simple transformation of a histogram workspace:**

.. testcode:: SofQHistSimple

   import numpy as np
   # create histogram workspace which looks like inelastic workspace
   dataX=range(-10,10,1)*10;
   dataY= np.exp(-(np.array(xx)/2.)**2)
   ws=CreateWorkspace(xx,yy,DataE=None,Nspec=10,UnitX='DeltaE')

   # convert workspace into MD workspace 
   wsE=SofQW(InputWorkspace='ws',QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
      

   print "The rebinned X values are: " + str(wsE.readX(0))
   print "The rebinned Y values are: " + str(wsE.readY(0))

.. testcleanup:: SofQHistSimple

   DeleteWorkspace(ws)
   DeleteWorkspace(wsE)
   
.. categories::
