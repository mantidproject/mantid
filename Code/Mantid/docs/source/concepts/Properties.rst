.. _Properties:

Properties
==========

Properties in Mantid are the mechanism by which we pass parameters into
:ref:`algorithms <algorithm>`. There are a number of different types of
properties, and these are described below.

Types of Properties
-------------------

Single Value Properties
~~~~~~~~~~~~~~~~~~~~~~~

This is the simplest type of property, which is essentially a name-value
pair. Currently, single value properties of type integer (``int``),
floating point (``double``), string (``std::string``) and boolean
(``bool``) are supported. .

Array Properties
~~~~~~~~~~~~~~~~

Sometimes, a multi-element parameter may be required (a list of spectra
to process, for example). This is achieved using an
ArrayProperty
(which is actually a PropertyWithValue - see above - of type
std::vector). It can be created in a number of ways:

-  As an empty vector: ``ArrayProperty<double>("PropertyName");``
-  By passing in an existing vector:
   ``ArrayProperty<double>("PropertyName",myStdVecOfDoubles);``
-  By passing in a string of comma-separated values:
   ``ArrayProperty<int>("PropertyName","1,2,3,4");``

A validator (see below) argument can optionally be appended to all of
these options.

An ArrayProperty can be declared in a algorithm as follows:

``declareProperty(new ArrayProperty``\ \ ``(...));``

or, if creating using an already existing vector:

``declareProperty("PropertyName",myVector);``

File Properties
~~~~~~~~~~~~~~~

These properties are for capturing and holding the path and filename to
an external file. File properties have a FileAction property that
controls it's purpose and behaviour.

Save :to specify a file to write to, the file may or may not exist
OptionalSave :to specify a file to write to but an empty string is
allowed here which will be passed to the algorithm
Load :to specify a file to open for reading, the file must exist
OptionalLoad :to specify a file to read but the file doesn't have to
exist

If the file property is has a FileAction of Load as is given a relative
path (such as "input.txt" or "\\data\\input.txt" as its value it will
search for matching files in this order:

#. The current directory
#. The entries in order from the datasearch.directories entry in the
   :ref:`Properties File <Properties File>`

If the file property is has a FileAction of Save as is given a relative
path (such as "input.txt" or "\\data\\input.txt" as its value it will
assume that path starts from the location defined in the
defaultsave.directory entry in the :ref:`Properties File <Properties File>`.

A FileProperty can be declared in a algorithm as follows:

| ``declareProperty(new Kernel::FileProperty("Filename","",``
| ``  Kernel::FileProperty::Load), "Descriptive text");``

or for saving a file providing a suggested extension

| ``declareProperty(new Kernel::FileProperty("Filename","",``
| ``  Kernel::FileProperty::Save, ``
| ``  std::vector``\ \ ``(1,"cal")), "Descriptive text");``

Workspace Properties
~~~~~~~~~~~~~~~~~~~~

Properties for holding :ref:`workspaces <workspace>` are more complicated,
in that they need to hold links both to the workspace name (in the
:ref:`Analysis Data Service <Analysis Data Service>`) and the workspace
itself. When setting or retrieving the value as a string (i.e. using the
``setValue`` or ``value`` methods) you are interacting with the
workspace's name; other methods interact with a :ref:`shared
pointer <Shared Pointer>` to the workspace.

The syntax to declare a WorkspaceProperty
in an algorithm is:

``declareProperty(new WorkspaceProperty("PropertyName","WorkspaceName",direction));``

In this case, the direction (see below) must be explicitly declared. An
optional :ref:`validator <Properties Validators>` may also be appended to
the above declaration.

Other 'Property Properties'
---------------------------

Default values
~~~~~~~~~~~~~~

If a property is empty

-  in a GUI algorithm call, then the property's default value is used
   (if there is any)
-  in a Python API call, then the property's value is left empty.

Direction
~~~~~~~~~

All properties have a direction. They can be input or output properties,
or both. The default is always input. Technically, these are a C++ enum,
which can have the following values:

| ``Mantid::Kernel::Direction::Input``
| ``Mantid::Kernel::Direction::Output``
| ``Mantid::Kernel::Direction::InOut``

This is what should be passed in when a direction argument is required.
The InOut option is principally used by workspace properties, when a
single workspace is to be input and manipulated by as algorithm rather
than a new one created to store the result.

.. _Properties Validators:

Validators
~~~~~~~~~~

A validator is an external object that is used to verify that the value
of a property is suitable for a particular algorithm. If no validator is
given, then the property can have any value (of the correct type).
Validators are checked immediately before an algorithm is executed, when
the value of a property is set (which will fail if it doesn't pass the
validator) and through the MantidPlot interface to an algorithm.

The validators currently included in Mantid are:

-  BoundedValidator - restricts a numeric property to a particular
   range.
-  MandatoryValidator - requires that a string or array property not be
   empty.
-  ListValidator - restricts a string property to one of a particular
   set of values.
-  FileValidator - ensures that a file (given as a string property)
   exists (used internally by the FileProperty).

In addition, there are a number of validators specifically for use with
Workspace properties:

-  InstrumentValidator - checks that the workspace has an Instrument
   object.
-  WorkspaceUnitValidator - checks that the workspace has a specified
   unit.
-  HistogramValidator - requires that the workspace contains histogram
   data (or not).
-  RawCountValidator - requires that the workspace data is raw counts.
-  CommonBinsValidator - checks that all spectra in a workspace have the
   same bins.
-  SpectraAxisValidator - checks that the axis of the workspace contains
   spectra numbers.
-  NumericAxisValidator - checks that the axis of the workspace contains
   numeric data.
-  CompositeValidator - enables combination of more that one of the
   above validators for the same WorkspaceProperty.

In addition to the above, if used, Workspace properties also have a
built in validator that requires that input workspaces exist and are of
the correct type and that output workspaces have a name set.

For more details on using validators, see the
`PropertyAlgorithm <https://github.com/mantidproject/mantid/blob/master/Code/Mantid/Framework/UserAlgorithms/PropertyAlgorithm.cpp>`__
example or the full documentation for the individual validators (linked
above).

Writing your own validator is relatively straightforward - it simply has
to implement the IValidator interface.



.. categories:: Concepts