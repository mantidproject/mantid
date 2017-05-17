
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm can be used to clear several areas of cached files or
in memory caches within Mantid.  The various boolean options give the
choice of which caches to clear.


Usage
-----

**Example - ClearCache**

.. testcode:: ClearCacheExample

   filesRemoved = ClearCache(DownloadedInstrumentFileCache=True)

   # Print the result
   print "%i files were removed" % filesRemoved

   # This will repopulate the cache you have just cleared
   DownloadInstrument()

Output:

.. testoutput:: ClearCacheExample
   :options: +ELLIPSIS

   ... files were removed

.. categories::

.. sourcelink::
