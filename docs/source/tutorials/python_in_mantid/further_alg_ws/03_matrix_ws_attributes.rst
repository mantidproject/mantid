.. _03_matrix_ws_attributes:

==========================
MatrixWorkspace Attributes
==========================


* Access the data accessed using `readX()`, `readY()` and `readE()`. These commands takes an integer workspace index and returns a list of the data i.e

Spectra and Bins
================

* A MatrixWorkspace is a 2D array of data :ref:`divided into spectra and bins <03_workspaces>`.

Get the number of histograms (spectra) in a workspace:

.. code-block:: python

	print(workspace_name.getNumberHistograms())

Get the number of bins:

.. code-block:: python

	print(workspace_name.blocksize())


Workspace algebra
=================

MatrixWorkspaces can be undergo basic algebra using an algorithm: :ref:<algm-Plus>, :ref:<algm-Minus>, :ref:<algm-Multiply>, :ref:<algm-Divide>.

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
