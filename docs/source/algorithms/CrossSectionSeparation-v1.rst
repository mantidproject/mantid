
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that performs magnetic, nuclear coherent, and incoherent component separation for ILL polarised diffraction and spectroscopy data. 
Three types of measurement are supported: uniaxial (2-point), XYZ (6-point), and 10p (10-point). The expected input is a workspace group containing
spin-flip and non-spin-flip cross-sections, with the assumed following order of axis directions: Z, Y, X, X-Y, X+Y.

Currently the algorithm is focused on D7 data structure.

Component separation method
###########################

1. Uniaxial

2. XYZ

3. 10-point


Usage
-----
.. include:: ../usagedata-note.txt

**Example - CrossSectionSeparation - XYZ component separation of vanadium data**


#. Stewart, J. R. and Deen, P. P. and Andersen, K. H. and Schober, H. and Barthelemy, J.-F. and Hillier, J. M. and Murani, A. P. and Hayes, T. and Lindenau, B.
   *Disordered materials studied using neutron polarization analysis on the multi-detector spectrometer, D7*
   Journal of Applied Crystallography **42** (2009) 69-84
   `doi: 10.1107/S0021889808039162 <https://doi.org/10.1107/S0021889808039162>`_


.. categories::

.. sourcelink::
