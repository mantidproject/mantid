.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs data normalisation for HFIR SANS. This algorithm is usually called by
:ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

According to the *NormalisationType* property, which can be set to "Monitor" or "Timer",
the algorithm will retrieve the corresponding value and scale the input workspace by it.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Normalise a BioSANS data set to time:**

.. testcode:: ExNorm

   workspace = HFIRLoad('BioSANS_empty_cell.xml')
   workspace = HFIRSANSNormalise(InputWorkspace='workspace', NormalisationType="Timer")


.. categories::

.. sourcelink::
