.. _03_matrix_ws_attributes:

==========================
MatrixWorkspace Attributes
==========================

* A MatrixWorkspace is essentially a 2D list of binned data where a workspace index, starting at 0, gives access to the data fields in each spectra.

* The data is accessed using the `readX()`, `readY()` and `readE()` commands. Each of these commands takes a number that refers to the index on the workspace and returns a list of the data for that workspace index, i.e

.. code-block:: python

	# Get the Y vector for the second row of data
	y_data2 = raw_workspace.readY(1)
	for y in y_data2:
	    print(y)

# Or in loop access. Print the first value in all spectra

.. code-block:: python

	for index in range(0, raw_workspace.getNumberHistograms()):
	    #Note the round brackets followed by the square brackets
	    print(raw_workspace.readY(index)[0])

To find out the number of histograms on a workspace use the getNumberHistograms().

.. code-block:: python

	# raw_workspace from above
	print(raw_workspace.getNumberHistograms())

To find out the number of bins use blocksize().

.. code-block:: python

	# raw_workspace from above
	print(raw_workspace.blocksize())

Workspace algebra
=================

MatrixWorkspaces can have algebraic operations applied to them directly without the need to call a specific algorithm, e.g. Plus

The expected operations of +,-,*,/ are supported with either a single number or another workspace as the second argument, e.g.

.. code-block:: python

	w1 = mtd['workspace1']
	w2 = mtd['workspace2']

# Sum the two workspaces and place the output into a third

.. code-block:: python

	w3 = w1 + w2

# Multiply the new workspace by 2 and place the output into a new workspace

.. code-block:: python

	w4 = w3 * 2

It is also possible to replace one of the input workspaces using one of +=,-=,*=,/= e.g.
# Multiply a workspace by 2 and replace w1 with the output

.. code-block:: python

	w1 *= 2.0

# Add 'workspace2' to 'workspace1' and replace 'workspace1' with the output

.. code-block:: python

	w1 += w2