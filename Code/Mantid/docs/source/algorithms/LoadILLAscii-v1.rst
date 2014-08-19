.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL Ascii / Raw data file into an MDEventWorkspace with the given name.
To date this Loader is only compatible with non-TOF instruments.

Supported instruments : ILL D2B

Usage
-----

**Example - Load ILL D2B Ascii file:**
(see :ref:`algm-LoadILLAscii` for more options)

.. code-block:: python

   # Load ILL D2B data file into a Multi dimensional workspace.
   ws = Load('ILLD2B_121459.txt')

   print "This workspace has", ws.getNumDims(), "dimensions and", ws.getNEvents(), "events."

Output:

	This workspace has 3 dimensions and 409600 events.

.. categories::
