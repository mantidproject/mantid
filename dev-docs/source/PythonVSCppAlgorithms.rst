.. _PythonVSCppAlgorithms:

========================
Python vs C++ Algorithms
========================

.. contents::
  :local:

Overview
--------

Mantid can be extended both with python and C++ algorithms as plug-ins. There are a number of considerations to take into account when deciding which language to choose.
These are summarised in the table and discussed below. Generally, it is recommended to implement **atomic** operations in C++, and **workflows** in python.

Further documentation for implementing algorithms:

* :ref:`User tutorial for writing a python algorithm <emwp_intro>`
* :ref:`Developer docs for writing a C++ algorithm <WritingAnAlgorithm>`
* :ref:`PythonAlgorithmsInExternalProjects`


Algorithm Language Comparison
-----------------------------

+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| Consideration         | C++                                                                                                    | Python                                                                     |
+=======================+========================================================================================================+============================================================================+
| **Execution Speed**   | Generally much faster (order of magnitude, and beyond), since compiled.                                | Generally slower. Numpy should be used wherever possible.                  |
|                       | Lots of optimisations can be made. OpenMP parallelisation for trivial loops (e.g. loops over spectra). | Large loops should be avoided, especially the nested ones.                 |
|                       |                                                                                                        | Provides no means for trivial parallelisation.                             |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **Creation**          | Generally slower and more complicated, but you do get the advantage of compile-time type checking.     | Generally easier and faster.                                               |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **Workflow**          | Large overhead when setting and getting the properties of child algorithms.                            | Very convenient and concise for workflows thanks to the python SimpleAPI.  |
|                       | Can quickly grow cumbersome, if many child algorithms have to be run.                                  |                                                                            |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **Testability**       | Easy in C++                                                                                            | Easy in python                                                             |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **API Accessibility** | Full                                                                                                   | Some of the framework functionality is not exposed to python.              |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **User Readability**  | Users are not generally expected to understand C++ code.                                               | Better readability. Power users are expected to understand python code.    |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+
| **User Modifiability**| Users can not change C++ algorithms, since they are shipped in the compiled form.                      | Users can play with python algorithms, since they are shipped as source.   |
|                       |                                                                                                        | It is not advised, of course, do to so, but in case of a spurious result,  |
|                       |                                                                                                        | they have the opportunity to play with the algorithm before contacting us. |
+-----------------------+--------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------+

Multiple Possibilities in Mantid
--------------------------------

There are many ways to extend Mantid to add new features not present out-of-the-box.

The easiest way to extend Mantid, and possibly the best starting place for any improvements, is a python script. Mantid provides a very high level of scriptable control, including plotting and visualisation as well as the execution of core routines. The scripting manual for Mantid provides an overview of the possibilities offered for scripting, including automatic generation via the graphical user interface for Mantid.

:ref:`Algorithms <Algorithm>` and :ref:`workspaces <Workspace>` are core concepts in Mantid. Generally, an Algorithm does something based on input workspaces, either to create a new one from the results, or to modify the input in-place. One reason to create an Algorithm, is because you have a script containing a well-defined and useful procedure, that you would like to share with a wider audience. It usually requires a low amount of effort to adapt a python script into one or more Python Algorithms.

Core Mantid Algorithms as well as some user defined Algorithms, are generally written in C++. There are a number of advantages to doing this (which are covered later), but also some serious disadvantages. When thinking about writing new functionality against Mantid, C++ does not need to be the default option.

Should I Write a C++ Algorithm?
-------------------------------
The main reason to write algorithms in C++ is that you can often significantly reduce the run-time processing time over code written in Python. Looping over large volumes of data tends to be fastest in C++. Mantid also has facilities for running your Algorithm in a multi-threaded context when assembled in C++.

Writing an algorithm in C++ gives you all the advantages associated with a compiled language including compile-time type checking. Some developers find this advantage significant enough to make writing C++ algorithms faster than Python equivalents.

Writing clean Mantid Algorithm code in C++ is sometimes not a good idea. Here are some brief reasons why:

For workflow Algorithms consisting mainly of child-algorithms execution, child-algorithm set-up in C++ can be long-winded and more fragile than Python.
There are many different ways to do the same thing in C++, and therefore more ways to get it wrong.
You are responsible for all the heap-allocated memory, as well as other resources you create.
Our target platforms have different compilers, with different interpretations of the same C++ standard, this can make writing cross-platform code tricky.
Compiler and linker outputs can be verbose, particularly in the presence of templated code.

Should I Write a Python Algorithm?
----------------------------------
Python algorithms are generally easier to put together than those assembled in C++. Because python has a limited dictionary, the barriers to writing effective code are much lower. Furthermore, not all algorithms need to run at the speed of light. For Algorithms that are only fed small volumes of data from small instruments, the user will not notice the difference between it running in half a second in Python or a tenth of a second in C++.

It's more natural to convert a python script into a python Algorithm than directly into a C++ algorithm. In many cases, the algorithm functionality is best assembled by procedural execution of existing algorithms. For this, the python API provides the best means of executing an algorithm in a single line, using well defined, named variables. An algorithm of this nature will take up only a few lines in Python and therefore be very easy to read and maintain.

Python algorithms also benefit from automatic GUI creation when they are registered with Mantid, so they can be used equally well through the command line, or through a GUI.

Python algorithms are great for editing and re-registering. Users can tweak existing Python algorithms or generate their own, without the complication of setting up a build environment. They can also more easily be re-issued to fix particular issues than C++ algorithms.

Note for Mantid Developers
--------------------------
Developers creating new algorithms in python must still generate unit tests for them. When an algorithm breaks, users do not care what language they are written in. The developer test suites allow you to create the same level of test coverage in python as you would in C++. Developers should also take care to ensure that the test exercises all of the code, as Python provides no compile-time type checking.
