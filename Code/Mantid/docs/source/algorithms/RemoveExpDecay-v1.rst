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

.. include:: ../usagedata-note.txt

**Example - Removing exponential decay from a MUSR run:**

.. testcode:: ExMUSR

   # Load first period of a MUSR run
   input = LoadMuonNexus('MUSR0015189.nxs', EntryNumber=1)

   # Remove uninteresting bins
   input = CropWorkspace('input', XMin=0.55, XMax=12)

   # Remove exp. decay
   output = RemoveExpDecay('input')

   # Bins to compare
   bins_to_compare = [0, 150, 300, 450, 600]

   # Get values before&after and format them as strings for output
   init_values = ["{:.3f}".format(input.readY(0)[bin]) for bin in bins_to_compare]
   exp_dec_removed = ["{:.3f}".format(output.readY(0)[bin]) for bin in bins_to_compare]

   print "Initial values", ", ".join(init_values)
   print "Exp. decay removed:", ", ".join(exp_dec_removed)

Output:

.. testoutput:: ExMUSR

   Initial values 49.000, 17.000, 2.000, 3.000, 0.000
   Exp. decay removed: 0.090, 0.127, -0.605, 0.768, -0.824

.. categories::
