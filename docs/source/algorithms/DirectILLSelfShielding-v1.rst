.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates self shielding correction factors for the input workspace. It is part of :ref:`ILL's direct geometry reduction suite <DirectILL>`. *InputWorkspace* should have a sample defined using :ref:`SetSample <algm-SetSample>`. Beam profile can be optionally set using :ref:`SetBeam <algm-SetBeam>`. The algorithm uses :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` as its backend.

To speed up the simulation, the sparse instrument option of :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` is used by default. The number of detectors to simulate can be given by *SparseInstrumentRows* and *SparseInstrumentColumns*.

By default the correction factors are calculated for each bin. By specifying *NumberOfSimulatedWavelengths*, one can restrict the number of points at which the calculation is done thus reducing the execution time. In this case CSplines are used to interpolate the correction factor over all bins.

The correction factor contained within the *OutputWorkspace* can be further fed to :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

Usage
-----

For usage of this algorithm, check the examples :ref:`here <DirectILL>`.

.. categories::

.. sourcelink::
