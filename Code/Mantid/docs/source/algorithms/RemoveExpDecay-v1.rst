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

where τ is the muon lifetime (2.197019e-6 seconds). :math:`N_0` is a
fitted normalisation constant.

.. categories::
