.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compute I(q) for reduced SANS data. 
This algorithm is rarely called directly. It is called by 
:ref:`HFIRSANSReduction <algm-HFIRSANSReduction>` or 
:ref:`EQSANSAzimuthalAverage1D <algm-EQSANSAzimuthalAverage1D>`.

This algorithm will gather the relevant information necessary to perform the
I(Q) calculation. This information includes:

- The Q binning, which is either taken from the inputs (if supplied) or computed.
- The number of pixels in X and Y, which is an input of :ref:`Q1DWeighted <algm-Q1DWeighted>`.

After this information is gathered, the 
:ref:`Q1DWeighted <algm-Q1DWeighted>` algorithm is called.

If *ComputeResolution* was set to True, the 
:ref:`ReactorSANSResolution <algm-ReactorSANSResolution>` is called.

.. categories::

.. sourcelink::
