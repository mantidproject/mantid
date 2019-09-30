.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
Compute the detector efficiency of SANS data. This algorithm is used by
the EQSANS and HFIR SANS reduction.

The relative detector efficiency is computed the following way

:math:`S(x,y)=\frac{I_{flood}(x,y)}{1/N_{pixels}\sum_{i,j}I_{flood}(i,j)}`

where :math:`I_{flood}(x,y)` is the pixel count of the flood data in pixel (x,y).
If a minimum and/or maximum sensitivity is given, the pixels having an
efficiency outside the given limits are masked and the efficiency is recomputed
without using those pixels.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Compute the detector efficiency from a BioSANS data file:**

.. testcode:: ExEff

   # Load your data file
   workspace = LoadSpice2D('BioSANS_empty_cell.xml')

   # Compute the detector efficiency
   efficiency = CalculateEfficiency('workspace', MinEfficiency=0.5, MaxEfficiency=1.5, Version=1)

.. categories::

.. sourcelink::
