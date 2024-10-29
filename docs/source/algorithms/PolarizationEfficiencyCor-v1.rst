.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a wrapper around the algorithms
:ref:`algm-PolarizationCorrectionWildes` and :ref:`algm-PolarizationCorrectionFredrikze`. Use `CorrectionMethod` property
to select between the two. The default is Wildes.

The input workspaces can be passed in either via `InputWorkspaces` or
`InputWorkspaceGroup` property but not both. An attempt to set both properties will result in an error.

The default values for the ``Flippers``, ``SpinStates``, and ``PolarizationAnalysis`` properties are empty strings and correspond to the actual defaults of the child algorithms.

The default value for the ``AddSpinStateToLog`` property is ``False`` and corresponds to the actual default of the Wildes algorithm. This property has no effect if used with the Fredrikze algorithm.

.. categories::

.. sourcelink::
