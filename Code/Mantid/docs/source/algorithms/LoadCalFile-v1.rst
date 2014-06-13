.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads an ARIEL-style 5-column ASCII .cal file into up to
3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.

The format is

-  Number: ignored.\* UDET: detector ID.\* Offset: calibration offset.
   Goes to the OffsetsWorkspace.
-  Select: 1 if selected (not masked out). Goes to the MaskWorkspace.
-  Group: group number. Goes to the GroupingWorkspace.

Usage
-----

.. include:: ../usagedata-note.txt

The following provides a simple example that uses just the instrument geometry to
create the necessary workspaces.

.. testcode:: ExInstrumentBase

    # Grouping, offsets and masking workspaces are all made by default.
    # WorkspaceName parameter is required inspite of docs not saying so.
    ws = LoadCalFile(InstrumentName="GEM", CalFilename="offsets_2006_cycle064.cal",
                     WorkspaceName="ws")
    print "Total number of workspaces =", len(ws)
    print "Workspace 1 type =", ws[0].id()
    print "Workspace 2 type =", ws[1].id()
    print "Workspace 3 type =", ws[2].id()

Output:

.. testoutput:: ExInstrumentBase

    Total number of workspaces = 3
    Workspace 1 type = GroupingWorkspace
    Workspace 2 type = OffsetsWorkspace
    Workspace 3 type = MaskWorkspace

.. categories::
