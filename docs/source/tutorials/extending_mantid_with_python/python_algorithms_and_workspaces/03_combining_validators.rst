.. _03_combining_validators:

====================
Combining Validators
====================

A special validator, :class:`~mantid.kernel.CompositeValidator`, exists so that many validators can be combined together.
This then requires that each validator is satisfied for the input value to be accepted.

A composite validator can be constructed by:

.. code-block:: python

    def PyInit(self):
        validation = CompositeValidator()
        validation.add(WorkspaceUnitValidator("Wavelength"))
        validation.add(InstrumentValidator())

        # or create validator from list
        # validation = CompositeValidator([
        #                  WorkspaceUnitValidator("Wavelength"),
        #                  InstrumentValidator()
        #              ])
        self.declareProperty(WorkspaceProperty("InputWorkspace",
                                               "",
                                               Direction.Input,
                                               validation))
