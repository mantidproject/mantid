.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Use this algorithm to remove cache files.

As described in algorithm CreateCacheFilename, cache files are named
in the form of <prefix>_<sha1>.nxs or <sha1>.nxs.
This algorithm delete all such files from the default
cache directory or the user-defined cache directory, if supplied.

The name matching is done using regex (40 charaters of numbers plus
a-f letters).
Therefore if a user created a file in the designated directory
that happens to have the same pattern, it will be deleted.

The algorithm also take parameter "AgeInDays", which allow
users to preserve cache files that are newer.
For example, if AgeInDays is 5, the latest 5 days of cache files will
be preserved.
By default, AgeInDays is 14 days or two weeks.


Usage
-----

**Example:**

.. testcode:: ExCleanFileCache

  # Execute
  CleanFileCache(
      CacheDir = "/path/to/mycache",
      AgeInDays = 5,
      )

Related Algorithms
------------------

:ref:`CreateCacheFilename <algm-CreateCacheFilename>` will create
filenames that this will delete.

.. categories::

.. sourcelink::
