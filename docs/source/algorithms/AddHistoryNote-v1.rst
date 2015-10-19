
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This simple algorithm just adds a record to the history of a workspace and is a simple way of annotating your workflow.

It does not change the data within a workspace in any way.


Usage
-----

**Example - AddHistoryNote**

.. testcode:: AddHistoryNoteExample

   ws = CreateSampleWorkspace()

   AddHistoryNote(ws,"The next algorithm is doing 1/ws")

   # Print the result
   print ws.getHistory().lastAlgorithm().getPropertyValue("Note")

Output:

.. testoutput:: AddHistoryNoteExample

  The next algorithm is doing 1/ws

.. categories::

.. sourcelink::

