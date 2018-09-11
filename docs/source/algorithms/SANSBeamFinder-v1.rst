.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Beam finder workflow algorithm for SANS instruments. 
This algorithm is rarely called directly. It is called by 
:ref:`SANSReduction <algm-SANSReduction>` or :ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

This algorithm abstracts out the details of how to call the :ref:`FindCenterOfMassPosition <algm-FindCenterOfMassPosition>`
algorithm. By using the *ReductionProperties* property, the algorithm
will store previously calculated results so that the center doesn't have
to be recalculated every time we load a new file.

.. categories::

.. sourcelink::
