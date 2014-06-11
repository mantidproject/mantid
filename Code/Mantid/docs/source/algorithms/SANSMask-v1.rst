.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Apply a mask to a SANS detector. This workflow algorithm will apply any mask that was saved 
in the logs by the reduction UI. It can also apply a mask to the edge of the detectors
according to whether we are dealing with EQSANS or a HFIR SANS.

This algorithm is rarely called directly. It is called by 
`SANSReduction <http://www.mantidproject.org/SANSReduction>`_ or
`HFIRSANSReduction <http://www.mantidproject.org/HFIRSANSReduction>`_.

.. categories::
