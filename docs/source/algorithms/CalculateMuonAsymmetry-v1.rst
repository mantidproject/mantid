.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the asymmetry from the specified muon
spectra. By default, all of the spectra
in a workspace will be corrected.

The formula for calculating the asymmetry (from counts) is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times e^\frac{t}{\tau})/(F N_0) - 1.0,

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds), :math:'F' is the number of good frames and :math:`N_0` is a
fitted normalisation constant. The normalisation is calculated by fitting to the normalised counts which is given by

.. math:: \textrm{normalisedCounts}=(\textrm{OldData}\times e^\frac{t}{\tau})/F

and the fitting function is given by

.. math:: N_0[1+f(t)] 

where :math:`f(t)` is a user defined function. 

It is also possible to calculate the asymmetry from an estimated asymmetry. 

Usage
-----



.. categories::

.. sourcelink::
