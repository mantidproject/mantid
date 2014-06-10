.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Beam finder workflow algorithm for SANS instruments. 
This algorithm is rarely called directly. It is called by 
`SANSReduction <http://www.mantidproject.org/SANSReduction>`_ or 
`HFIRSANSReduction <http://www.mantidproject.org/HFIRSANSReduction>`_ .

This algorithm abstracts out the details of how to call the 
`FindCenterOfMassPosition <http://www.mantidproject.org/FindCenterOfMassPosition>`_
algorithm. By using the *ReductionProperties* property, the algorithm
will store previously calculated results so that the center doesn't have
to be recalculated every time we load a new file.

.. categories::
