.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format)
into an OffsetsWorkspace.

The resulting workspace can then be used with, e.g.
:ref:`AlignDetectors <algm-AlignDetectors>` to perform calibration.

Usage
-----

This algorithm is SNS specific in its use.

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadDspacemap(InstrumentName="VULCAN", Filename="pid_offset_vulcan_new.dat",
                       FileType="VULCAN-ASCII")
    print("Workspace type = {}".format(ws.id()))

Output:

.. testoutput:: Ex

    Workspace type = OffsetsWorkspace

.. categories::

.. sourcelink::
