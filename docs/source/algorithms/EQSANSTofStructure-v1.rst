.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Corrects the TOF of each event of an EQSANS data workspace according to
the chopper settings. The algorithm determines whether the data was taken
in frame skipping mode in order to perform the proper frame correction.

This algorithm will trim the edges of the TOF distribution according to the input properties.
It will also correct the TOF values for the flight path of the neutrons.

This algorithm is rarely called directly. It is called by 
:ref:`EQSANSLoad <algm-EQSANSLoad>`.


.. categories::

.. sourcelink::
