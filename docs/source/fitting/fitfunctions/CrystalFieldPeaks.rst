.. _func-CrystalFieldPeaks:

=================
CrystalFieldPeaks
=================

.. index:: CrystalFieldPeaks

Description
-----------

This function calculates energies and intensities of transitions between states of a crystal electric field acting upon a rare earth ion. It is a part of crystal field computation
in Mantid and under active development. More documentation will follow as the development progresses.

Here is an example of how the function can be evaluated from python. The output is a `TableWorkspace` with two columns: the first column with the energies (in meV) and the second one with the intensities (in milibarn per steradian). 
The function doesn't require an input workspace so `None` is passed for `InputWorkspace` property of `EvaluateFunction`.

.. code::

	fun = 'name=CrystalFieldPeaks,Ion=Ce,Temperature=25.0,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544'
	EvaluateFunction(fun, None, OutputWorkspace='out')

.. attributes::

   Ion;String;Mandatory;An element name for a rare earth ion. Possible values are: Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb.
   Symmetry;String;C1;A symbol for a symmetry group. Setting `Symmetry` automatically zeros and fixes all forbidden parameters. Possible values are: C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3, S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh
   Temperature;Double;1.0;A temperature in Kelvin.
   ToleranceEnergy;Double;:math:`10^{-10}`;Tolerance in energy in meV. If difference between two or more energy levels is smaller than this value they are considered degenerate.
   ToleranceIntensity;Double;:math:`10^{-3}`;Tolerance in intensity. If difference between intensities of two or more transitions is smaller than this value the transitions are considered degenerate.


.. properties::


.. categories::

.. sourcelink::
