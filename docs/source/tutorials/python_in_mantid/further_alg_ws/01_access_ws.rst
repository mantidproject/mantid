.. _01_access_ws:

============================
Basic Workspace Manipulation
============================


Access a Workspace in Python
============================

Access a workspace, loaded in the Workspaces Toolbox, inside a script:

.. code-block:: python

    from mantid.api import AnalysisDataService as ADS
    ws = ADS.retrieve('ws')

Or simply:

.. code-block:: python

    ws = mtd['ws']
    # ADS import not needed


Workspace algebra
=================

MatrixWorkspaces can undergo basic algebra using an algorithm: :ref:`algm-Plus`, :ref:`algm-Minus`, :ref:`algm-Multiply`, :ref:`algm-Divide`.

As a shorthand, use +,-,*,/ with either number or another workspace as the second argument

.. code-block:: python

    w1 = mtd['workspace1']
    w2 = mtd['workspace2']

    # Sum the two workspaces and place the output into a third
    w3 = w1 + w2

    # Multiply the new workspace by 2 and place the output into a new workspace
    w4 = w3 * 2

Replace an input workspaces using +=,-=,*=,/= e.g.

.. code-block:: python

    # Multiply a workspace by 2 and replace w1 with the output
    w1 *= 2.0

    # Add 'workspace2' to 'workspace1' and replace 'workspace1' with the output
    w1 += w2
