.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Reads an ISAW-style .peaks or .integrate file into a PeaksWorkspace. Any
detector calibration information is ignored.

NOTE: The instrument used is determined by reading the 'Instrument:' and
'Date:' tags at the start of the file.If the date is not present, the
latest `Instrument Definition File <Instrument Definition File>`__ is
used.

.. categories::
