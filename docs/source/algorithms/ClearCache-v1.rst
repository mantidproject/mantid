
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to clear several areas of cached files or
in memory caches within Mantid.  The various boolean options give the
choice of which caches to clear.


Usage
-----

**Example - ClearCache**

.. code-block:: python

   filesRemoved = ClearCache(DownloadedInstrumentFileCache=True)

   # Print the result
   print("{} files were removed".format(filesRemoved))

   # This will repopulate the cache you have just cleared
   DownloadInstrument()

Output:

.. code-block:: python

   ... files were removed

.. categories::

.. sourcelink::
