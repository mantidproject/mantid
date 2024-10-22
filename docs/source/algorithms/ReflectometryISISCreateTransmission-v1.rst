
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates a transmission workspace for ISIS reflectometry data. It is intended primarily for creating input transmission workspaces that are compatible with the :ref:`algm-PolarizationEfficienciesWildes` algorithm.

First the ``InputRun`` is loaded using :ref:`algm-LoadNexus`. The ``InputRun`` can be prefixed with an instrument name to load runs that are not from the user's default instrument.

If a ``FloodWorkspace`` is provided then a flood correction is performed using the :ref:`algm-ApplyFloodWorkspace` algorithm.

If workspace indices are provided to the ``BackgroundProcessingInstructions`` property then a background subtraction is performed using the :ref:`algm-ReflectometryBackgroundSubtraction` algorithm.
The subtraction uses ``PerDetectorAverage`` as the calculation method.

Finally, the transmission workspace is created by running :ref:`algm-CreateTransmissionWorkspaceAuto`.

Usage
-----
**Example - ReflectometryISISCreateTransmission**

.. testcode:: ReflectometryISISCreateTransmissionExample

   transmission_grp = ReflectometryISISCreateTransmission(InputRun="POLREF4699", ProcessingInstructions="5-244")
   print(f"Output workspace group contains {transmission_grp.size()} single spectrum transmission workspaces.")

Output:

.. testoutput:: ReflectometryISISCreateTransmissionExample

   Output workspace group contains 2 single spectrum transmission workspaces.

.. categories::

.. sourcelink::
