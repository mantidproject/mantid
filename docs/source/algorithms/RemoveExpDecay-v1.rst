.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm removes the exponential time decay from the specified muon
spectra, leaving the rest unchanged. By default, all the spectra
in a workspace will be corrected.

The formula for removing the exponential decay is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times{e^\frac{t}{\tau}})/N_0 - 1.0

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds). :math:`N_0` is a
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

   Exp. decay removed: [-0.24271431  0.79072482 -0.05900907 -0.70331658 -0.76614798]

.. categories::

.. sourcelink::
