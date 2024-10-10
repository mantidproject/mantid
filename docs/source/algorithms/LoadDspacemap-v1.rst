.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format)
into an OffsetsWorkspace.

The resulting workspace can then be used with, e.g.
:ref:`ConvertUnits <algm-ConvertUnits>` to perform calibration.

Usage
-----

This algorithm is SNS specific in its use.

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadDspacemap(InstrumentFilename="VULCAN_Definition_2006-01-31.xml", Filename="pid_offset_vulcan_new.dat",
                       FileType="VULCAN-ASCII")
    print("Workspace type = {}".format(ws.id()))

Output:

.. testoutput:: Ex

    Workspace type = OffsetsWorkspace

.. categories::

.. sourcelink::
