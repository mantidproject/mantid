.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Load algorithm is a more intelligent algorithm than most other load
algorithms. When passed a filename it attempts to search the existing
load `algorithms <:Category:Algorithms>`__ and find the most appropriate
to load the given file. The specific load algorithm is then run as a
child algorithm with the exception that it logs messages to the Mantid
logger.

Specific Load Algorithm Properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each specific loader will have its own properties that are appropriate
to it: SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTof\_Min
and FilterByTof\_Max for Event data. The Load algorithm cannot know
about these properties until it has been told the filename and found the
correct loader. Once this has happened the properties of the specific
Load algorithm are redeclared on to that copy of Load.

.. categories::
