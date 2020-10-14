.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
------------

The workflow proceeds as follows:

1. Using the algorithm :ref:`LiquidsReflectometryReduction <algm-LiquidsReflectometryReduction>`, reduce the normalization run for a standard using normalization input parameters: :math:`I^{standard}(Q)`

2. With the input `Refl1DModelParameters` JSON string, calculate the model reflectivity for the normalization run to produce the theoretical reflectivity of the standard. Uses the `refl1d` [1]_ package: :math:`R_{theory}^{standard}(Q)`

3. The reduced normalization run from step (1), :math:`I^{standard}(Q)`, is then divided by the model reflectivity of the same material from step (2), :math:`R_{theory}^{standard}(Q)`, to produce the incident flux for normalzing the sample run: :math:`I_{norm}(Q) = I^{standard}(Q) / R_{theory}^{standard}(Q)`.

4. Using the algorithm :ref:`LiquidsReflectometryReduction <algm-LiquidsReflectometryReduction>`, reduce the sample run with the normalization turned OFF (i.e. `NormFlag` set to `False`): :math:`I^{sample}(Q)`

5. Calculate the sample reflectivity by dividing the sample reduction of step (4), :math:`I^{sample}(Q)`, by the the normalization in step (3), thus :math:`R^{sample}(Q) = I^{sample}(Q) / I_{norm}(Q)`.

References
------------

.. [1] Refl1D is a program for analyzing 1D reflectometry measurements made with X-ray and neutron beamlines. `Refl1D GitHub repo <https://github.com/reflectometry/refl1d>`__ and `Refl1D Docs <https://refl1d.readthedocs.io/en/latest/>`__

.. categories::

.. sourcelink::
