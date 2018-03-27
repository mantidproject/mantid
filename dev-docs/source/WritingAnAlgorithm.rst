.. _WritingAnAlgorithm:

Writing An Algorithm
====================

.. contents::
  :local:

Introduction
############

Mantid's `plugin <https://www.mantidproject.org/Plugin>`__ architecture has been engineered so that it is easy for a user 
to write their own algorithm. This page is a primer for the user about to write their first algorithm and assumes no 
great knowledge of C++. 
It covers the basics, with links to more advanced options where appropriate. Note if you are looking to add a 
`plugin <https://www.mantidproject.org/Plugin>`__ fit function rather than an algorithm then see 
`Writing a Fit Function <https://www.mantidproject.org/Writing_a_Fit_Function>`__. 
There is special description for the case when you are looking to add a custom `MD conversion plugin <WritingCustomConvertToMDTransformation>`__.

Alternatively, you can implement your algorithm in `Python <https://www.mantidproject.org/Extending_Mantid_With_Python>`__. 
See `Python Vs C++ Algorithms <https://www.mantidproject.org/Python_Vs_C%2B%2B_Algorithms>`__ for a comparison of Mantid's 
two programming languages.

All `algorithms <https://www.mantidproject.org/Algorithm>`__ in Mantid `inherit <http://en.wikipedia.org/wiki/Inheritance_(computer_science)>`__ 
from a base ``Algorithm`` class, which provides the support and services required for running a specific 
algorithm and greatly simplifies the process of writing a new one.

Getting Started
###############
The first step is to create a new directory, with any name of your choice, under your MantidInstall directory
(on Windows, probably located at ``C:\\MantidInstall``). Alternatively, you can just do everything in the 
UserAlgorithms directory. The UserAlgorithms directory contains a simple Python script called ``createAlg.py``.
This can be used to create a new 'empty' algorithm - to create one called 'MyAlg' you should type ``python 
createAlg.py myAlg category``, where category is an optional argument to set the algorithm's category. 
To do the same thing 'by hand', create files called ``MyAlg.h`` and ``MyAlg.cpp`` and paste in the following 
boilerplate C++ code (changing each occurrence of 'MyAlg' to your chosen algorithm name):

**Header file (MyAlg.h)**:

.. code-block:: cpp

    #ifndef MYALG_H_
    #define MYALG_H_
    
    #include "MantidAPI/Algorithm.h"
    
    class MyAlg : public Mantid::API::Algorithm
    {
    public:
      /// (Empty) Constructor
      MyAlg() : Mantid::API::Algorithm() {}
      /// Virtual destructor
      virtual ~MyAlg() {}
      /// Algorithm's name
      virtual const std::string name() const { return "MyAlg"; }
      /// Algorithm's version
      virtual const int version() const { return (1); }
      /// Algorithm's category for identification
      virtual const std::string category() const { return "UserDefined"; }
    
    private:
      /// Initialisation code
      void init();
      /// Execution code
      void exec();
    };
 
    #endif /*MYALG_H_*/

**Source file (MyAlg.cpp)**:

.. code-block:: cpp

    #include "MyAlg.h"
    
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(MyAlg);
    
    void MyAlg::init()
    {
    }
    
    void MyAlg::exec() 
    { 
    }

At this point you will already have something that will compile and run. To do so (on Windows), copy the files 
``build.bat`` & ``SConstruct`` from ``UserAlgorithms`` into the directory containing your code and execute ``build.bat``. 
If you then start MantidPlot your algorithm will appear in the list of available algorithms and could be run. 
But, of course, it won't do anything of interest until you have written some algorithm code...

Coding the Algorithm
####################

You will see that the algorithm skeletons set up in the last section contain two methods/functions/subroutines
called ``init`` and ``exec``. It will be no surprise to discover that these will, respectively, contain the code to 
initialise and execute the algorithm, which goes in the ``.cpp`` file between the curly brackets of each method. 
Note that these are private methods (i.e. cannot be called directly); an algorithm is run by calling the base 
class's ``initialize()`` and ``execute()`` methods, which provide additional services such as the validation of properties, 
fetching workspaces from the ``AnalysisDataService``, handling errors and filling the workspace histories.

Initialization
--------------

The initialization (init) method is executed by the ``FrameworkManager`` when an algorithm is requested and must
contain the declaration of the properties required by the algorithm. Atypically, it can also contain other 
initialization code such as the calculation of constants used by the algorithm, so long as this does not 
rely on the values of any of the properties.

Calls to the ``declareProperty`` method are used to add a property to this algorithm. See the properties page
for more information on the types of properties supported and the example algorithms in ``UserAlgorithms`` 
(especially `PropertyAlgorithm <http://svn.mantidproject.org/mantid/trunk/Code/Mantid/UserAlgorithms/PropertyAlgorithm.cpp>`__
and `WorkspaceAlgorithm <http://svn.mantidproject.org/mantid/trunk/Code/Mantid/UserAlgorithms/WorkspaceAlgorithm.cpp>`__) 
for further guidance on how to use them.

For the simple types (integer, double or string), the basic syntax is::

   declareProperty("UniquePropertyName",value);

An optional `validator <https://www.mantidproject.org/Properties#Validators>`__ or 
`directional argument <https://www.mantidproject.org/Properties#Direction>`__ (input, output or both)
can also be appended. The syntax for other property types (``WorkspaceProperty`` & ``ArrayProperty``) is more 
complex - see the `properties <https://www.mantidproject.org/Properties#Direction>`__ page or the 
example algorithms in `UserAlgorithms <https://www.mantidproject.org/UserAlgorithms>`__ for further details.

Execution
#########

Fetching properties
-------------------

Before the data can be processed, the first task is likely to be to fetch the values of the input properties. 
This uses the ``getProperty`` method as follows::

    TYPE myProperty = getProperty("PropertyName");

where ``TYPE`` is the type of the property (``int``, ``double``, ``std::string``, ``std::vector``...). Note that the 
value of a ``WorkspaceProperty`` is a `shared pointer <https://www.mantidproject.org/Shared_Pointer>`__
to the workspace, which is referred to as ``Mantid::API::Workspace_sptr`` or ``Mantid::API::Workspace_const_sptr``. 
The latter should be used for input workspaces that will not need to be changed in the course of the algorithm.

If a handle is required on the property itself, rather than just its value, then the same method is used as follows::

    Mantid::Kernel::Property* myProperty = getProperty("PropertyName");

This is useful, for example, for checking whether or not an optional property has been set (using Property's 
``isDefault()`` method).

Creating the output workspace
-----------------------------

Usually, the result of an algorithm will be stored in another new workspace and the algorithm 
will need to create that new workspace through a call to the ``WorkspaceFactory``. For the (common) 
example where the output workspace should be of the same type and size as the input one, the code 
would read as follows::

   Mantid::API::Workspace_sptr outputWorkspace = Mantid::API::WorkspaceFactory::Instance().create(inputWorkspace);

where ``inputWorkspace`` is a shared pointer to the input workspace.

It is also important to, at some point, set the output workspace property to point at this workspace. 
This is achieved through a call to the ``setProperty`` method as follows::

  setProperty("OutputWorkspacePropertyName",outputWorkspace);

where ``outputWorkspace`` is a shared pointer to the created output workspace.

Using workspaces
----------------

The bulk of most algorithms will involve the manipulation of the data contained in workspaces 
and information on how to interact with these is given `here <https://www.mantidproject.org/Interacting_with_Workspaces>`__. 
The more advanced user may also want to refer to the full 
`workspace documentation <http://doxygen.mantidproject.org/nightly/d3/de9/classMantid_1_1API_1_1Workspace.html>`__.

Those familiar with C++ should make use of private methods and data members to break up the execution code into
more manageable and readable sections.

Further Features
################

The advanced user is referred to the `full documentation page <http://doxygen.mantidproject.org/nightly/d3/de9/classMantid_1_1API_1_1Workspace.html>`__
for the ``Algorithm`` base class to explore the full range of methods available for use within an algorithm. 
A few aspects are highlighted below.

Child Algorithms
----------------

Algorithms may wish to make use of the functionality of other algorithms as part of their execution. 
For example, if a units change is required the ``ConvertUnits`` algorithm could be used. Mantid therefore 
has the concept of a child algorithm and this is accessed through a call to the 
``createChildAlgorithm`` method as follows::

    Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("AlgorithmName");

This call will also initialise the algorithm, so the algorithm's properties can then be set and it can be executed::

     childAlg->setPropertyValue("number", 0);
     childAlg->setProperty<Workspace_sptr>("Workspace",workspacePointer);
     childAlg->execute();

Logging
-------

The ``g_log`` object enables access to the `logging <Logging>`__ facilities of Mantid, and is an invaluable 
tool in understanding the running of your algorithms.

Enhancing asynchronous running
------------------------------

Any algorithm can be run asynchronously (e.g. by MantidPlot) without modification. However, some features 
are only enabled if code is added within the ``exec()`` method. ``Algorithm::interruption_point()`` should 
be called at appropriate intervals so that the algorithm's execution can be interrupted. 
``Algorithm::progress(double p)`` reports the progress of the algorithm. ``p`` must be between 
0 (start) and 1 (finish).

Exceptions
----------

It is fine to throw exceptions in your algorithms in the event of an unrecoverable failure. 
These will be caught in the base Algorithm class, which will report the failure of the algorithm.

Validation of inputs
--------------------

`Validators <https://www.mantidproject.org/Properties#Validators>`__ allow you to give feedback 
to the user if the input of a property is incorrect (for example, typing non-numeric characters 
in a number field).

For more advanced validation, override the ``Algorithm::validateInputs()`` method. This is a 
method that returns a map where:

- The key is the name of the property that is in error.

- The value is a string describing the error.

This method allows you to provide validation that depends on several property values at once 
(something that cannot be done with ``IValidator``). Its default implementation returns an empty map, 
signifying no errors.

It will be called in dialogs **after** parsing all inputs and setting the properties, but **before** executing. 
It is also called again in the ``execute()`` call, which will throw if this returns something.

In the MantidPlot GUI, this will set a "star" ``*`` label next to each property that is reporting an error. 
This makes it easier for users to find where they went wrong.

If your ``validateInputs()`` method validates an input workspace property, bear in mind that the user 
could provide a ``WorkspaceGroup`` (or an unexpected type of workspace) - when retrieving the property, 
check that casting it to its intended type succeeded before attempting to use it.
