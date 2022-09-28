.. _func-DecoupAsymPowderMagLong:

=======================
DecoupAsymPowderMagLong
=======================

.. index:: DecoupAsymPowderMagLong

Description
-----------

The Decoupling of asymmetry in the ordered state of a powdered magnet fit function for longitudinal polarization can be written as:

.. math:: y = a*A_z({\frac{x}{B0}})

where:

- :math:`a` - a scaling parameter for the overall asymmetry
- :math:`B0` - the characteristic field
- :math:`A_z(b) = {\frac{3}{4}} - {\frac{1}{4b^2}} + {\frac{(b^2-1)^2}{8b^3}}log|{{\frac{b+1}{b-1}}}|`


For a rotating asymmetry use :ref:`DecoupAsymPowderMagRot<func-DecoupAsymPowderMagRot>` .

Examples
--------

An example of when this might be used is for examining field dependence of :math:`\mu` SR signals in a polycrystalline magnet[1].



.. attributes::

.. properties::

References
----------
[1] Pratt, F.L. (2007). Field dependence of :math:`\mu` SR signals in a polycrystalline magnet. Journal of Physics:Condensed Matter, Vol. 19 No. 45, doi `10.1088/0953-8984/19/45/456207 <https://iopscience.iop.org/article/10.1088/0953-8984/19/45/456207>`_ .

.. categories::

.. sourcelink::
