.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs unary SANS reduction for the instruments D11, D22, D33 at the ILL.
Unary in this context means that each call of this algorithm processes one type of data which is a part of the whole experiment.
The logic is resolved by the property **ProcessAs**, which governs the reduction steps based on the requested type.
It can be one of the 6: absorber, beam, transmission, container, reference and sample.
The full data treatment of the complete experiment should be build up as a chain with multiple calls of this algorithm over various types of acquisitions.
The sequence should be logical, typically as listed above, since the later processes need the outputs of earlier processes as input.
The common mandatory input is a run file (numor), or a list of them, in which case they will be summed at raw level, so right after loading.
The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

.. note::
    This is a dynamic algorithm; it takes different inputs and produces different outputs based on the **ProcessAs** selected.
    Furthermore, some properties are bi-directional; for example, `BeamFluxValue` is output, when processing as `Beam`, and is input, when processing as `Sample`.

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

.. note::
    For a technical reason, when processing as **Beam** or **Transmission**, one has to set a flag **ReturnAll=True** in the algorithm call.
    This is needed when there is more than one primitive typed `InOut` property to be returned; which are not returned by default in `mantid.simpleapi`.
    This is invented as a workaround to not to break the backwards compatibility with `mantid.simpleapi` interface.

.. categories::

.. sourcelink::
