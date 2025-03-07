
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the instrument component efficiencies in a polarized analysis experiment. The algorithm is a wrapper algorithm around the :ref:`algm-ReflectometryISISCreateTransmission`,  :ref:`algm-PolarizationEfficienciesWildes`, and :ref:`algm-JoinISISPolarizationEfficiencies` algorithms.
See the linked documentation for the individual algorithms for detail on implementation.

The ``NonMagInputRuns`` property takes a list of one or more run numbers related to non-magnetic runs. This list is passed to :ref:`algm-ReflectometryISISCreateTransmission`, in which they are loaded and summed (if more than one) using :ref:`algm-LoadAndMerge`.

Likewise, the ``MagInputRuns`` property takes a list of one or more run numbers related to magnetic runs.

If a ``FloodWorkspace`` is provided then a flood correction is performed as part of :ref:`algm-ReflectometryISISCreateTransmission` for both magnetic and non-magnetic input runs.

If workspace indices are provided to the either of the ``BackgroundProcessingInstructions`` or ``MagBackgroundProcessingInstructions`` properties then a background subtraction is performed as part of :ref:`algm-ReflectometryISISCreateTransmission` for the relevant input runs.

If both non-magnetic and magnetic inputs runs are provided, they must have the same number of x-values (spectra). If this is not the case then a sub-section of the spectra can be specified for processing through the ``ProcessingInstructions`` and ``MagProcessingInstructions`` properties.

If diagnostic workspaces created as part of the :ref:`algm-PolarizationEfficienciesWildes` are required then the ``IncludeDiagnosticOutputs`` property can be used to enable output.

Usage
-----
**Example - ReflectometryISISCreateTransmission**

.. testcode:: ReflectometryISISCalculatePolEffExample

   pol_eff = ReflectometryISISCalculatePolEff(NonMagInputRuns="POLREF32130", ProcessingInstructions="270-292", MagInputRuns="POLREF32131", MagProcessingInstructions="270-292")
   print(f"Output workspace contains {pol_eff.getNumberHistograms()} spectrum related to the calculated efficiencies for the two polarizers, analyzers and flippers.")

Output:

.. testoutput:: ReflectometryISISCalculatePolEffExample

   Output workspace contains 4 spectrum related to the calculated efficiencies for the two polarizers, analyzers and flippers.

.. categories::

.. sourcelink::
