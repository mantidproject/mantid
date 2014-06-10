.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compute I(q) for reduced EQSANS data. 
This algorithm is rarely called directly. It is called by 
`SANSReduction <http://www.mantidproject.org/SANSReduction>`_.

This workflow algorithm takes into account whether the data was 
acquired in frame-skipping mode or not. 

- In normal mode, the algorithm will simply call 
  `SANSAzimuthalAverage1D <http://www.mantidproject.org/SANSAzimuthalAverage1D>`_,
  followed by a call to `EQSANSResolution <http://www.mantidproject.org/EQSANSResolution>`_ to compute the resolution in Q.

- In frame-skipping mode, the algorithm will compute the Q binning according to 
  the wavelength range of the acquired data in each frame. The
  `SANSAzimuthalAverage1D <http://www.mantidproject.org/SANSAzimuthalAverage1D>`_ and 
  `EQSANSResolution <http://www.mantidproject.org/EQSANSResolution>`_
  algorithms are then called for each frame. An I(Q) workspace will be produced
  for each of the two frames. When called by the 
  `SANSReduction <http://www.mantidproject.org/SANSReduction>`_
  algorithm, the name of those output workspaces will end with
  *_frame_1_Iq* or *_frame_2_Iq*.

.. categories::
