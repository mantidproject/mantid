
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This simple algorithm just adds a comment record to the history of a workspace and is a simple way of annotating your workflow.

It does not change the data within a workspace in any way.


Usage
-----

**Example - Comment**

.. testcode:: CommentExample

   ws = CreateSampleWorkspace()

   Comment(ws,"The next algorithm is doing 1/ws")

   # Print the result
   print ws.getHistory().lastAlgorithm().getPropertyValue("Text")

Output:

.. testoutput:: CommentExample

  The next algorithm is doing 1/ws

.. categories::

.. sourcelink::

