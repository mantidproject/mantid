.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm removes the exponential time decay from a specified muon
spectra. By default, all the spectra in a workspace will be corrected.

The formula for removing the exponential decay is given by:

.. math:: NewData = (OldData\times{e^\frac{t}{\tau}})/N_0 - 1.0

where :math:`\tau` is the muon lifetime (2.197019e-6 seconds). :math:`N_0` is a
fitted normalisation constant.

Usage
-----

**Example - Removing exponential decay:**

.. testcode:: ExSimple

   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)

   output = RemoveExpDecay(input)

   print "Exp. decay removed:", output.readY(0)

Output:

.. testoutput:: ExSimple

   Exp. decay removed: [-0.24271091  0.79071878 -0.05901962 -0.70332224 -0.76615428]

.. categories::
