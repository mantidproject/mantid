Writing An Algorithm
====================

Mantid's `plugin <https://www.mantidproject.org/Plugin>`__ architecture has been engineered so that it is easy for a user 
to write their own algorithm. This page is a primer for the user about to write their first algorithm and assumes no 
great knowledge of C++. 
It covers the basics, with links to more advanced options where appropriate. Note if you are looking to add a 
`plugin <https://www.mantidproject.org/Plugin>`__ fit function rather than an algorithm then see 
`Writing a Fit Function <https://www.mantidproject.org/Writing_a_Fit_Function>`__. 
There is special description for the case when you are looking to add a custom `MD conversion plugin <WritingCustomConvertToMDTransformation>`__

Alternatively, you can implement your algorithm in `Python <https://www.mantidproject.org/Extending_Mantid_With_Python>`__. 
See `Python Vs C++ Algorithms <https://www.mantidproject.org/Python_Vs_C%2B%2B_Algorithms>`__ for a comparison of Mantid's 
two programming languages.

All `algorithms <https://www.mantidproject.org/Algorithm>`__ in Mantid `inherit <http://en.wikipedia.org/wiki/Inheritance_(computer_science)>__` 
from a base Algorithm class, which provides the support and services required for running a specific 
algorithm and greatly simplifies the process of writing a new one.

.. contents::
  :local:

Getting Started
###############
The first step is to create a new directory, with any name of your choice, under your MantidInstall directory
(on Windows, probably located at C:\MantidInstall). Alternatively, you can just do everything in the 
UserAlgorithms directory. The UserAlgorithms directory contains a simple Python script called ``createAlg.py``.
This can be used to create a new 'empty' algorithm - to create one called 'MyAlg' you should type ``python 
createAlg.py myAlg category``, where category is an optional argument to set the algorithm's category. 
To do the same thing 'by hand', create files called MyAlg.h and MyAlg.cpp and paste in the following 
boilerplate C++ code (changing each occurrence of "MyAlg" to your chosen algorithm name):

**Header file (MyAlg.h)**

.. code-block:: python
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

**Source file (MyAlg.cpp)**

.. code-block:: python
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
initialise and execute the algorithm, which goes in the .cpp file between the curly brackets of each method. 
Note that these are private methods (i.e. cannot be called directly); an algorithm is run by calling the base 
class's ``initialize()`` and ``execute()`` methods, which provide additional services such as the validation of properties, 
fetching workspaces from the Analysis Data Service, handling errors and filling the workspace histories.

**Initialization**
The initialization (init) method is executed by the Framework Manager when an algorithm is requested and must
contain the declaration of the properties required by the algorithm. Atypically, it can also contain other 
initialization code such as the calculation of constants used by the algorithm, so long as this does not 
rely on the values of any of the properties.

Calls to the ``declareProperty`` method are used to add a property to this algorithm. See the properties page
for more information on the types of properties supported and the example algorithms in UserAlgorithms 
(especially `PropertyAlgorithm <http://svn.mantidproject.org/mantid/trunk/Code/Mantid/UserAlgorithms/PropertyAlgorithm.cpp>`__
and `WorkspaceAlgorithm <http://svn.mantidproject.org/mantid/trunk/Code/Mantid/UserAlgorithms/WorkspaceAlgorithm.cpp>`__) 
for further guidance on how to use them.

For the simple types (integer, double or string), the basic syntax is

.. code-block:: python
   declareProperty("UniquePropertyName",value);

An optional validator or directional argument (input, output or both) can also be appended. 
The syntax for other property types (WorkspaceProperty & ArrayProperty) is more complex - see the properties
page or the example algorithms in UserAlgorithms for further details.
