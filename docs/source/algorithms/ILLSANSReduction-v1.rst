.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs SANS reduction for the instruments at the ILL.
With each call, this algorithm processes one type of data which is a part of the whole experiment.
The logic is resolved by the property **ProcessAs**, which governs the reduction steps based on the requested type.
It can be one of the 6: absorber, beam, transmission, container, reference and sample.
The full data treatment of the complete experiment should be build up as a chain with multiple calls of this algorithm over various types of acquisitions.
The sequence should be logical, typically as enumerated above, since the later processes need the outputs of earlier processes as input.
The common mandatory input is a run file (numor), or a list of them, in which case they will be summed at raw level, so right after loading.
The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

ProcessAs
---------
In the flowcharts below the yellow ovals represent the inputs, the grey parallelograms are the outputs for each process type.

Absorber
~~~~~~~~

.. diagram:: ILLSANS-v1_absorber_wkflw.dot

Beam
~~~~

.. diagram:: ILLSANS-v1_beam_wkflw.dot

Transmission
~~~~~~~~~~~~

.. diagram:: ILLSANS-v1_transmission_wkflw.dot

Container
~~~~~~~~~

.. diagram:: ILLSANS-v1_container_wkflw.dot

Reference
~~~~~~~~~

.. diagram:: ILLSANS-v1_reference_wkflw.dot

Sample
~~~~~~

.. diagram:: ILLSANS-v1_sample_wkflw.dot

.. categories::

.. sourcelink::
