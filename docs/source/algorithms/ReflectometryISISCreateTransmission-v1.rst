
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates a transmission workspace for ISIS reflectometry data. It is intended primarily for creating input transmission workspaces that are compatible with the :ref:`algm-PolarizationEfficienciesWildes` algorithm.

The ``InputRuns`` property takes a list of one or more run numbers. These are loaded and summed (if more than one) using :ref:`algm-LoadAndMerge`.

If a ``FloodWorkspace`` is provided then a flood correction is performed using the :ref:`algm-ApplyFloodWorkspace` algorithm.

If workspace indices are provided to the ``BackgroundProcessingInstructions`` property then a background subtraction is performed using the :ref:`algm-ReflectometryBackgroundSubtraction` algorithm.
The subtraction uses ``PerDetectorAverage`` as the calculation method.

Finally, the transmission workspace is created by running :ref:`algm-CreateTransmissionWorkspaceAuto`.

Usage
-----
**Example - ReflectometryISISCreateTransmission**

.. testcode:: ReflectometryISISCreateTransmissionExample

   transmission_grp = ReflectometryISISCreateTransmission(InputRuns="POLREF4699", ProcessingInstructions="5-244")
   print(f"Output workspace group contains {transmission_grp.size()} single spectrum transmission workspaces.")

Output:

.. testoutput:: ReflectometryISISCreateTransmissionExample

   Output workspace group contains 2 single spectrum transmission workspaces.

.. categories::

.. sourcelink::
