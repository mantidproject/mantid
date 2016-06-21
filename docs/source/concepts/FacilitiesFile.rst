.. _Facilities File:

.. role:: xml(literal)
   :class: highlight

Facilities File
===============

Summary
-------

The facilities file, called **facilities.xml**, contains properties of
facilities and instruments that Mantid is aware of. In order for Mantid
to function correctly for your facility then the facilities file should
contain the appropriate definitions as defined below.

File syntax
-----------

Each facility is described using XML with an instrument defined as a sub
component of a facility. A simple facility definition would be

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <facilities>

     <facility name="BrandNew" delimiter="_" zeropadding="8" FileExtensions=".nxs,.n*">

      <instrument name="ABCDEF"/>

     </facility>

    </facilities>

which would define a facility called *BrandNew* with an instrument
called *ABCDEF*. The facilities attributes have the following meanings:

-  ``delimiter`` gives the delimiter that is inserted between the
   instrument name and the run number when constructing a file name;
-  ``zeroPadding`` gives the number of digits that a run number is
   padded to when constructing a file name;
-  ``FileExtensions`` should list the extensions of the data files for
   the facility. The first is taken as the preferred extension.

An instrument can have further attributes which define properties of the
that instrument rather than the facility as a whole, e.g.

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <facilities>

     <facility name="BrandNew" zeropadding="8" FileExtensions=".nxs,.n*">

      <instrument name="ABCDEF" shortName="ABC">
      <technique>Tech 1</technique>
      <technique>Tech 2</technique>
      <zeropadding size="12" startRunNumber="12345" prefix="FEDCBA"></zeropadding>
      <zeropadding size="15" startRunNumber="54321" prefix="ZYXWUV"></zeropadding>
      </instrument>

     </facility>

    </facilities>

where the attributes are defined as:

-  ``shortName`` gives a shortened version of the instrument name
   sometimes used in run filename generation;
-  ``<technique>NAME<\technique>`` tags give a named technique supported
   by the instrument. Mantid uses this to retrieve lists of instruments
   based on a particular technique. Multiple techniques can be
   specified.
-  ``<zeropadding size="12" startRunNumber="12345" prefix="FEDCBA"\>``
   is an optional tag to specify zero padding different from the default
   for the facility. ``startRunNumber`` is an optional attribute
   specifying the smallest run number at which this zero padding must be
   used. If ``startRunNumber`` is omitted this zero padding is applied
   from run number 0. The optional ``prefix`` attribute allows a
   filename to have a different prefix. An ``instrument`` tag can have
   multiple ``zeropadding`` tags.

Location
--------

The file should be located in the directory pointed to by the
**instrumentDefinition.directory** key in the
:ref:`.properties <Properties File>` file.



.. categories:: Concepts