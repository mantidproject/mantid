.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The purpose of this algorithm is to create a unique filename for a
cache so that a workflow can reuse results from previous computations.

The algorithm will accept a prefix,
:class:`~mantid.kernel.PropertyManager`, list of properties to use
from the property manager (empty is use all), and a string array (or
List) of other properties to use, and a directory for cache files to
exist in (default described below).

The list of property names will be used to select which of the
properties in the :class:`~mantid.kernel.PropertyManager` will be used
to calculate the hash and will be interpreted as globbing.

The string array of other_properties will be key/value pairs of
properties that should be considered, but are not in the provided
:class:`~mantid.kernel.PropertyManager`.

If a directory is not specified, cache files will go into a cache
subdirectory of :class:`ConfigService.getUserPropertiesDir()
<mantid.kernel.ConfigServiceImpl.getUserPropertiesDir>`.  On unix this
will be ``~/.mantid/cache``.

The algorithm will convert all properties to strings as
``"%s=%s" % (property.name, property.valueAsStr)``, sort the list,
then convert it to a sha1.

A filename with the form ``<location>/<prefix>_<sha1>.nxs`` will be
returned as the output property.  If no prefix is specified then file
result will be ``<location>/<sha1>.nxs``.

* ``PropertyManager``: an instance of
  :class:`~mantid.kernel.PropertyManager` from which property values
  can be retrieved. None means we don't care about property manager --
  all properties will come from ``other_properties``
* ``Properties``: a list of strings. each string is a property managed by the
  given ``property_manager``, or it can be glob pattern to match prop
  names too. but empty list means taking all properties
  from the ``property_manager``
* ``OtherProperties``: a list of strings. each string is in the form of
  ``key=value`` for one property not managed by the ``property_manager``.
  no globbing here.
* ``Prefix``: prefix to the output hash name. when it is empty, just the hash.
  when it is not empty, it will be ``<prefix>_<sha1>``
* ``CacheDir``: the directory in which the cach file will be created.
  empty string means default as described above

Usage
-----

**Example:**

.. testcode:: ExCreateCacheFilename

  import mantid
  from mantid.kernel import PropertyManager
  pm = PropertyManager()
  aprops = ["a", "alibaba", "taa", "sa", "a75"]
  props = aprops + ['b', 'c', 'd']
  for p in props:
      pm.declareProperty(p, '')
      pm.setProperty(p, "fish")
      continue
  mantid.PropertyManagerDataService.add("excreatecachefilename", pm)
  other_props = ["A=1", "B=2"]
  # Execute
  cache_path, signature = CreateCacheFilename(
      PropertyManager = "excreatecachefilename",
      Properties = ['*a*'],
      OtherProperties = other_props,
      )

Related Algorithms
------------------

:ref:`CleanFileCache <algm-CleanFileCache>` will delete files using this naming scheme.

.. categories::

.. sourcelink::
