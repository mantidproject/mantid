.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compute I(q) for reduced EQSANS data. 
This algorithm is rarely called directly. It is called by 
:ref:`SANSReduction <algm-SANSReduction>`.

This workflow algorithm takes into account whether the data was 
acquired in frame-skipping mode or not. 

- In normal mode, the algorithm will simply call 
  :ref:`SANSAzimuthalAverage1D <algm-SANSAzimuthalAverage1D>`,
  followed by a call to :ref:`EQSANSResolution <algm-EQSANSResolution>` 
  to compute the resolution in Q.

- In frame-skipping mode, the algorithm will compute the Q binning according to 
  the wavelength range of the acquired data in each frame. The
  :ref:`SANSAzimuthalAverage1D <algm-SANSAzimuthalAverage1D>` and 
  :ref:`EQSANSResolution <algm-EQSANSResolution>`
  algorithms are then called for each frame. An I(Q) workspace will be produced
  for each of the two frames. When called by the 
  :ref:`SANSReduction <algm-SANSReduction>`
  algorithm, the name of those output workspaces will end with
  *_frame_1_Iq* or *_frame_2_Iq*.

.. categories::

.. sourcelink::
