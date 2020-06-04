.. _01_access_ws:

================================
Accessing Workspaces From Python
================================

There are several ways to access a Mantid workspace from Python:
* As the return value of an algorithm function call - Captures this single workspace;
* Using `mtd['name']` - Captures the named workspace;
* Using `mtd.importAll()` - Captures all workspaces currently in Mantid.

Return from Algorithm Call
==========================

As described before, if running an algorithm via an algorithm call then the workspace will be returned as part of the outputs from the algorithm, e.g.

.. code-block:: python

	output_ws = ConvertUnits(input_ws, Target='dSpacing')

Using `mtd['name']`
===================

If a workspace was created via clicking in the GUI then there is no automatic generation of a Python variable for that workspace (but we are working on it). This single workspace can be retrieved by using its name

.. code-block:: python

sample = mtd['sample'] # Assumes a workspace was created called "sample"

If the name does not exist in Mantid then Python a KeyError is raised

.. code-block:: python

	mtd['not in mantid']

	#Traceback (most recent call last):
	#  File "<stdin>", line 1, in <module>
	#KeyError: "'not in mantid' does not exist."

Using `mtd.importAll()`
=======================

We currently have not automatic method for generating a Python variable for a workspace created via the GUI. We have therefore added a method, `mtd.importAll()`, that can be called manually to create Python variables for each workspace in Mantid. Assuming 2 workspaces have been created in Mantid called "ws1" and "ws2", running

.. code-block:: python

	print(ws1)
	#Traceback (most recent call last):
	#  File "<stdin>", line 1, in <module>
	#NameError: name 'ws1' is not defined

.. code-block:: python

	print~(ws2)
	#Traceback (most recent call last):
	#  File "<stdin>", line 1, in <module>
	#NameError: name 'ws2' is not defined

.. code-block:: python

	mtd.importAll()
	print(ws1)
	#ws1
	print(ws2)
	#ws2

shows that now two Python variables called ws1 and ws2 have been created where they didn't exist before (shown by the NameError at the top)

* A point to be aware of when using this function is that some names may not be valid Python variables, i.e. contain spaces or start with numbers. In these cases Mantid will import a "cleaned" up variable name and report this when it has been done. For example, if a workspace with the name "15 mev ei" exists in Mantid, running `mtd.importAll()` will produce

.. code-block:: python

	mtd.importAll()
	# Warning: "15 meV ei" is an invalid identifier, "_15_meV_ei" has been imported instead.
	print(_15_meV_ei)
	#15 meV ei

The printout shows that its name has not changed, it is only the Python variable that is cleaned up on import.
