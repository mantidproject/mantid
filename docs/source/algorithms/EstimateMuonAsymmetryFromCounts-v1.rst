.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm estimates the asymmetry from the specified muon
spectra. By default, all of the spectra
in a workspace will be corrected.

The formula for estimating the asymmetry is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times e^\frac{t}{\tau})/(F N_0) - 1.0,

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds), :math:`F` is the number of good frames and :math:`N_0` is a
fitted normalisation constant. The normalisation is given by

.. math:: N_0= \frac{\Delta t\sum_j(\textrm{OldData}_j)}{\tau F \left( \exp(-\frac{t_0}{\tau})-\exp(-\frac{t_N}{\tau})\right)  },

where the summation only includes the data with times bewtween :math:`t_0` and :math:`t_N` and :math:`\Delta t` is the time step. 

Usage
-----

**Example - Removing exponential decay:**

.. testcode:: ExSimple

   import math
   import numpy as np
   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)
   run = input.getRun()
   run.addProperty("goodfrm","10","None",True)
   output,norm=EstimateMuonAsymmetryFromCounts(InputWorkspace=input,spectra=0,StartX=1,EndX=5)
   print  "Asymmetry: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   print "Normalization constant: {0:.2f}".format(norm[0])
   
Output:

.. testoutput:: ExSimple

   Asymmetry:  ['-0.12', '1.07', '0.09', '-0.66', '-0.73']   
   Normalization constant: 17.98

.. categories::

.. sourcelink::
