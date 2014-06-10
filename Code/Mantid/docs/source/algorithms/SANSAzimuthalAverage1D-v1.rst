.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compute I(q) for reduced SANS data. 
This algorithm is rarely called directly. It is called by 
`HFIRSANSReduction <http://www.mantidproject.org/HFIRSANSReduction>`_ or
`EQSANSAzimuthalAverage1D <http://www.mantidproject.org/EQSANSAzimuthalAverage1D>`_.

This algorithm will gather the relevant information necessary to perform the
I(Q) calculation. This information includes:

- The Q binning, which is either taken from the inputs (if supplied) or computed.
- The number of pixels in X and Y, which is an input of `Q1DWeighted <http://www.mantidproject.org/Q1DWeighted>`_.

After this information is gathered, the 
`Q1DWeighted <http://www.mantidproject.org/Q1DWeighted>`_
algorithm is called.

If *ComputeResolution* was set to True, the
`ReactorSANSResolution <http://www.mantidproject.org/ReactorSANSResolution>`_ is called.

.. categories::
