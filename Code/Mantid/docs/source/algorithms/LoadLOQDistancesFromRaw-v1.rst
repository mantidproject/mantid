.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The ISIS TS1 instrument
`LOQ <http://www.isis.stfc.ac.uk/instruments/loq/>`__ writes values for
the moderator-sample and sample-detector distances to the RAW data file.
These distances are required for correct data reduction. This algorithm
extracts the information from the ``i_l1`` and ``i_sddist`` variables of
the IVPB struct respectively and moves the appropriate components so
that the Mantid instrument satisfies these values.

.. categories::
