
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

**Example - ILLComponentSeparation - XYZ component separation of vanadium data**


.. categories::

.. sourcelink::
