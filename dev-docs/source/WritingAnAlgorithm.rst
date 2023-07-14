.. _WritingAnAlgorithm:

Writing An Algorithm
====================

.. contents::
   :local:


Introduction
############

Mantid's :ref:`plugin <Plugin>` architecture has been engineered so that it is easy for a developer, or intrepid user, to write an algorithm.
This page is a primer for the user about to write their first algorithm and assumes no great knowledge of C++.
It covers the basics, with links to more advanced options where appropriate.
There is special description for the case when you are looking to add a custom :ref:`MD conversion plugin <WritingCustomConvertToMDTransformation>`.

Alternatively, you can implement your algorithm in :ref:`Python <emwp_intro>`.
See :ref:`Python Vs C++ Algorithms <PythonVSCppAlgorithms>` for a comparison of Mantid's two programming languages.

All :ref:`algorithms <Algorithms List>` in Mantid `inherit <http://en.wikipedia.org/wiki/Inheritance_(computer_science)>`__ from a base ``Algorithm`` class,
which provides the support and services required for running a specific algorithm and greatly simplifies the process of writing a new one.


Getting Started
###############

Creating a new algorithm is simplified using :ref:`class_maker.py <class_maker_py>`

.. code-block:: bash

  $ python buildconfig/class_maker.py --alg Algorithms MyAlg

which will give an amount of information about all that it did.
The intentionally failing unit test, ``MyAlgTest.h``, and user docs, ``MyAlg-v1.rst``, will not be described here, but need to be updated before contributing.
The excerpts below are the files created and formatted using the ``pre-commit`` hooks configured for c++ files.
The exact contents of the generated code may change, the methods required do not.

**Header file** :download:`MyAlg_initial.h <cppalgexample/MyAlg_initial.h>` from ``class_maker.py``

.. literalinclude:: cppalgexample/MyAlg_initial.h
   :language: cpp
   :lines: 7-
   :linenos:

**Source file** :download:`MyAlg_initial.cpp <cppalgexample/MyAlg_initial.cpp>` from ``class_maker.py``

.. literalinclude:: cppalgexample/MyAlg_initial.cpp
   :language: cpp
   :lineno-start: 8
   :lines: 8-
   :linenos:

At this point you will already have something that will compile and run.
If you then start MantidWorkbench your algorithm will appear in the list of available algorithms and could be run.
Of course, it won't do anything of interest until you have written some algorithm code.


Coding the Algorithm
####################

You will see that the algorithm has a variety of methods that need to be filled in.
The example linked here is used throughout the majority of this document.
The first few do not normally change very often

The full files described below are :download:`MyAlg.h <cppalgexample/MyAlg.h>` and :download:`MyAlg.cpp <cppalgexample/MyAlg.cpp>`.
Only ``MyAlg.cpp`` will be shown in detail, but both are functional code that can be viewed as "simple" examples.

.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 28
   :lines: 28-38
   :linenos:

The ``name()`` method should not be verified.

The ``version()`` method normally returns ``1``.
Mantid allows for having multiple versions of an algorithm registered which is facilitated by this method.
The most common use cases for multiple versions are when the signature or underly assumptions are radically changed.
Overall, few algorithms have more than one "version."

The ``category()`` method aids users in finding the algorithm.
The return can be a semilcolon (``;``) delimited string of categors/subcategories.
You are highly encouraged to stay to the list of :ref:`existing categories <Algorithms List>`.

The ``summary()`` method is a brief description of the algorithms functionality.
It is automatically re-used in the python docstring, and the generated help.
The first sentence of the summary (up to the first period, ``.``) is what appears in the generated gui for the algorithm.

You will see that the algorithm skeletons set up in the last section contain two larger methods called ``init`` and ``exec``.
These are described in sections below.


Logging
-------

The algorithm base class defines an object, ``g_log``.
The ``g_log`` object enables access to the :ref:`logging <Logging>` facilities of Mantid,
and is an invaluable tool in understanding the running of your algorithms.
When writing information to the logs, be aware of the various levels available and that the default level for users is set to ``notice``.

The logging framework does have facility for flushing logs.
However, logging ``std::endl`` will force the framework to flush at that time and developers should prever ``"\n"`` instead to not interfere with the framework.


Initialization
##############

The ``init()`` method written in your algorithm is called as part of the larger algorithm lifecycle.
It is a private method that cannot be called directly, but one can call an algorithm's inherited ``initialize()`` method for the desired effect.
For details, look at the source of the `API::Algorithm::initialize() <https://github.com/mantidproject/mantid/blob/main/Framework/API/src/Algorithm.cpp#L281>`_ .

The initialization (init) method is executed by the ``FrameworkManager`` when an algorithm is requested and must contain the declaration of the properties required by the algorithm.
Atypically, it can also contain other initialization code such as the calculation of constants used by the algorithm, so long as this does not rely on the values of any of the properties.

Calls to the inherited ``declareProperty()`` method are used to add a property to this algorithm.
See the properties page for more information on the types of properties supported and the example algorithms in ``UserAlgorithms`` (especially `PropertyAlgorithm <https://github.com/mantidproject/mantid/blob/main/Framework/Examples/PropertyAlgorithm.cpp>`__
and `WorkspaceAlgorithm <https://github.com/mantidproject/mantid/blob/main/Framework/Examples/WorkspaceAlgorithm.cpp>`__)
for further guidance on how to use them.
There are many overloaded signatures for ``declareProperty()`` which are intended to make things easier, but can often be confusing due to the many options available.

.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 43
   :lines: 43-57
   :linenos:

This example shows a couple of different ways of declaring properites.
They all have a couple of features in common:

* Every property has a name. Here they are ``InputWorkspace``, ``NumberToApply``, ``WayToApply``, and ``OutputWorkspace``
* Every property has a default value, many of which are effectively empty.
  Here they are empty workspace name, ``EMPTY_DBL()`` (special float value), ``"Y"``, and empty name.
  It is extremely unusual to specify a default workspace name
* Every property has documentation.
  Most of the ones here are very uniformative, but the document on ``NumberToApply`` was written to demonstrate a particular point.
  Everything up to the first period (``.``) appears in the mouseover in the generated algorithm dialogs.
  The whole string appears in the user docs and python help.

An optional :ref:`validator <Properties Validators>` or :ref:`directional argument <Properties Directions>` (input, output or both) can also be appended.
The syntax for other property types (``WorkspaceProperty`` and ``ArrayProperty``) is more complex - see the :ref:`properties <Properties>` page.

Validation of inputs
--------------------

While not part of the initialization, the optional ``validateInputs()`` method allows for cross-checking parameters.
This particular example shows how one would ban a particular value, but normally a validator would be placed on the property itself for this behavior
For more advanced validation, override the ``Algorithm::validateInputs()`` method.
This is a method that returns a map where:

- The key is the name of the property that is in error.

- The value is a string describing the error.

This method allows you to provide validation that depends on several property values at once (something that cannot be done with ``IValidator``).
Its default implementation returns an empty map, signifying no errors.

It will be called in dialogs **after** parsing all inputs and setting the properties,
but **before** executing.
It is also called again in the ``execute()`` call, which will throw if this returns something.

This will set a "star" ``*`` label next to each property that is reporting an error.
This makes it easier for users to find where they went wrong.

If your ``validateInputs()`` method validates an input workspace property,
bear in mind that the user could provide a ``WorkspaceGroup`` (or an unexpected type of workspace) - when retrieving the property,
check that casting it to its intended type succeeded before attempting to use it.

.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 58
   :lines: 58-70
   :linenos:


Execution
#########

Like initialization, the ``exec()`` method you write is not directly, but can be indirectly called via the ``execute()`` method of the algorith
There are other methods that will do the same thing in different modes.
All of them eventually call the `API::Algorithm::executeInternal() <https://github.com/mantidproject/mantid/blob/main/Framework/API/src/Algorithm.cpp#L509>`_ which does the following

* log a warning message if the algorithm is marked as deprecated
* validate all of the input parameters individually against what they are set to checking for type and any validators that were defined
* run the ``validateInputs()`` method
* run the ``exec()`` method
* add history to the Workspace annotating the parameters that the algorithm was called with

Additionally, the algorithm framework also handles things like sync/async calling, exception handling, and interaction with :ref:`AnalysisDataService <Analysis Data Service>`.


Fetching properties
-------------------

Before the data can be processed,
the first task is likely to be to fetch the values of the input properties.
This uses the ``getProperty()`` method as follows


.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 76
   :lines: 76-79
   :linenos:

where the type in the left hand side is the type of the property (``int``, ``double``, ``std::string``, ``std::vector``...) and must match what was defined in the ``init()``.
Note that the value of a ``WorkspaceProperty`` is a :ref:`shared pointer <Shared Pointer>` to the workspace,
which is referred to as ``Mantid::API::Workspace_sptr`` or ``Mantid::API::Workspace_const_sptr``.
The latter should be used for input workspaces that will not need to be changed in the course of the algorithm.
The output workspace is retrieved here to be used to detect if the algorithm is being performed in-place which will be discussed further down in this document.
This example also uses the ``getPropertyValue()`` method which returns a string representation of the requested property (e.g. the name of the workspace).

If a handle is required on the property itself, rather than just its value, then the same method is used as follows:

.. code-block:: cpp

    Mantid::Kernel::Property* myProperty = getProperty("PropertyName");

This is useful, for example, for checking whether or not an optional property has been set (using Property's ``isDefault()`` method).
However, the algorithm has a convenience method ``isDefault(const std::string &)`` as well.


Creating the output workspace
-----------------------------

Usually, the result of an algorithm will be stored in another new workspace and the algorithm
will need to create that new workspace through a call to the ``WorkspaceFactory``.
For the (common) example where the output workspace should be of the same type and size as the input one, the code would read as follows:

.. code-block:: cpp

   outputWS = Mantid::API::WorkspaceFactory::Instance().create(inputWS);

where ``inputWS`` is a shared pointer to the input workspace.
However, in many instances, one can simply clone the input workspace and work on the clone directly.

.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 81
   :lines: 81-85
   :linenos:

It is also important to, at some point, set the output workspace property to point at this workspace.
This is achieved through a call to the ``setProperty`` method as follows:

..   :lines: 109-110
.. literalinclude:: cppalgexample/MyAlg.cpp
   :language: cpp
   :lineno-start: 109
   :lines: 109-110
   :linenos:

where ``outputWorkspace`` is a shared pointer to the created output workspace.


Using workspaces
----------------

The bulk of most algorithms will involve the manipulation of the data contained in workspaces and information on how to interact with these is given :ref:`here <WorkingWithWorkspaces>`.
The more advanced user may also want to refer to the full
`workspace documentation <http://doxygen.mantidproject.org/nightly/d3/de9/classMantid_1_1API_1_1Workspace.html>`__.

Those familiar with C++ should make use of private methods and data members to break up the execution code into more manageable and readable sections.


Further Features
################

The advanced user is referred to the `full documentation page <http://doxygen.mantidproject.org/nightly/d3/de9/classMantid_1_1API_1_1Workspace.html>`__
for the ``Algorithm`` base class to explore the full range of methods available for use within an algorithm.
A few aspects are highlighted below.


Child Algorithms
----------------

Algorithms may wish to make use of the functionality of other algorithms as part of their execution.
For example, if a units change is required the ``ConvertUnits`` algorithm could be used. Mantid therefore has the concept of a child algorithm and this is accessed through a call to the
``createChildAlgorithm`` method as follows:

.. code-block:: cpp

   Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("AlgorithmName");

This call will also initialise the algorithm, so the algorithm's properties can then be set and it can be executed:

.. code-block:: cpp

   childAlg->setPropertyValue("number", 0);
   childAlg->setProperty<Workspace_sptr>("Workspace",workspacePointer);
   childAlg->execute();

Because ``Property`` is generic to input **and** output, the workspace pointers cannot be ``const`` when shared with a child algorithm.

Enhancing asynchronous running
------------------------------

Any algorithm can be run asynchronously without modification.
However, some features are only enabled if code is added within the ``exec()`` method.
``Algorithm::interruption_point()`` should be called at appropriate intervals so that the algorithm's execution can be interrupted.
``Algorithm::progress(double p)`` reports the progress of the algorithm.
``p`` must be between 0 (start) and 1 (finish).


Exceptions
----------

It is fine to throw exceptions in your algorithms in the event of an unrecoverable failure.
These will be caught in the base Algorithm class, which will report the failure of the algorithm.
