.. _02_workspace_validators:

====================
Workspace Validators
====================

As with the basic types (int, float, etc.) there are some special validators for checking input workspaces.

Those currently available are:

* :class:`~mantid.api.WorkspaceUnitValidator` - Checks that the workspace unit
  matches a given value. Code to create: ``WorkspaceUnitValidator("UnitName")``
* :class:`~mantid.api.HistogramValidator` - Checks if the workspace is a
  histogram. Code to create: ``HistogramValidator(True/False)``
* :class:`~mantid.api.CommonBinsValidator` - Checks that if bins in the first and
  last spectrum are the same, i.e. a tentative check that the workspace's
  spectra have common bins. Code to create: ``CommonBinsValidator()``
* :class:`~mantid.api.RawCountValidator` - Checks if the workspace contains raw
  counts. Code to create: ``RawCountsValidator(True/False)``
* :class:`~mantid.api.InstrumentValidator` - Checks if the input workspace has a
  defined instrument. Code to create: ``InstrumentValidator()``

To use a specific one pass the created validator as the validator argument of the ``declareProperty`` function, e.g.

.. code-block:: python

    def PyInit(self):
        # Requires the input workspace to have x-axis units of Wavelength
        self.declareProperty(WorkspaceProperty(name="InputWorkspace",
                                               defaultValue="",
                                               direction=Direction.Input,
                                               validator=WorkspaceUnitValidator("Wavelength")))
