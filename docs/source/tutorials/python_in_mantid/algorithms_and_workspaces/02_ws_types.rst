.. _02_ws_types:

==========================
Workspace Types Via Python
==========================


A Workspace object will always be one of these Python types :ref:`ITableWorkspace <04_table_ws_py>`, :ref:`MatrixWorkspace <03_matrix_ws_py>`, :ref:`WorkspaceGroup <05_group_ws_py>` or :ref:`MDWorkspace`. The MatrixWorkspace is the most commonly used and is often referred to simply as a :ref:`workspace`. Usage of the MDWorkspace is not required for this course.

All Workspace types have at least these methods:

`getComment()`
Returns a string with any comment that was loaded into the workspace, usually when it was initially created from a data file.

`getMemorySize()`
Returns the number of kilobytes of RAM that the workspace is currently using.

`getName()`
Returns the name of the workspace as a string.

`getTitle()`
Returns a string containing the title that was loaded into the workspace.

Example
-------

.. code-block:: python

    ws = Load(Filename="GEM38370_Focussed.nxs", OutputWorkspace="myWS")
    print(ws.getComment())
    print(ws.getMemorySize())
    print(ws.getName())
    print(ws.getTitle())
    print(ws)
