.. _NewStarterC++:

=====================
New Starter C++
=====================

.. contents::
   :local:

-------------
Prerequisites
-------------

-  A copy of the book `Modern C++ for Absolute Beginners <https://www.amazon.co.uk/Modern-Absolute-Beginners-Introduction-Programming/dp/1484260465>`__ by Slobodan Dmitrovic. The source code used in the examples and exercises in this book can be found at: https://github.com/Apress/modern-cpp-for-absolute-beginners
-  You will also need a computer with a C++ development environment.
-  Git: *Windows/Mac* - Download the latest version from http://git-scm.com/; *Linux* - Install from package manager (ubuntu pkg=git-gui)
-  CMake: *Windows/Mac* - Download from http://www.cmake.org/download/; *Linux* - Install from package manager (ubuntu pkg=cmake-qt-gui)

---------------
Version Control
---------------

You will be using the `git <http://git-scm.com/documentation>`__ version control system with Mantid. Here we aim to get you started using git while working through the new exercises. Our code is stored on `github <https://www.github.com/>`__ so to get started you will need an account.

#. Navigate to `github <https://github.com/>`__, fill in the details: username (e.g. firstnamelastname), email & password and click sign up.
#. Go to https://help.github.com/articles/set-up-git and follow the instructions to set up git for your environment (for windows do NOT use the native app)

Before you start the exercises below it is a good idea to read `this <http://git-scm.com/book/en/Git-Basics-Recording-Changes-to-the-Repository>`__ page that discusses the basic operations of working with git.

The idea of version control is that snapshots of the development history can be recorded and these states returned to if necessary. As you go through the exercises and get to a point where something is working it is a good idea to *commit* the changes to your new repository. On Mantid we use what are known as branches within the repository to keep track of a single piece of work. The idea is that each feature/bugfix is developed independently on a separate branch within the repository. When the work is complete, it is tested by another developer and merged within a special branch, called **main**. This branch is reserved code that will form part of that distributed to users. More details on Mantid's workflow with git can be found `here <https://developer.mantidproject.org/GitWorkflow.html>`__.

While developing the code for your exercises you will work in a separate repository `here <https://github.com/mantidproject/newstarter>`__ but the intention is that you will follow the workflow described in the document above and in particular using the commands described in `this <https://developer.mantidproject.org/GitWorkflow.html#Workflow_Git_Commands>`__ section. Do not use the macros yet, the aim is to understand the process by using the real commands.

Before starting the exercises, there are some setup steps:

-  Clone this repository: https://github.com/mantidproject/newstarter
-  Make a new branch ``git checkout --no-track -b firstname_lastname_exercises origin/main``, where *firstname*, *lastname* should be replaced as appropriate.
-  Make a copy of the *exercises-cpp/template* directory and name it *firstname_lastname*. The directory should be in the exercises directory.
-  Make a directory called *builds* in the root of the *newstarter* repository. Git is setup to ignore this directory.
-  Start the CMake Gui
-  Point the source directory at *exercises-cpp/firstname_lastname* and build directory at *builds*
-  Click *Configure* and select *Unix Makefiles* on *Linux/OS X or Visual Studio 2019* and click *Finish*
-  Assuming there are no errors click *Generate*

Now you are ready to code the solution to the exercise in your chosen native build environment.

-  As you work use the git commands to commit to your branch and push to GitHub.
-  When you think you have completed the exercise you can use the continuous integration build servers to check your work. To do this you first need to create a pull-request. See `create a pull request <https://help.github.com/articles/creating-a-pull-request/>`__ for your branch so that it can be reviewed by a senior developer.
-  The pull request will kick-off builds on Red Hat and Windows platforms and GitHub will mark up the results of these builds on the pull requests. Try and get each build to a green status before saying it is ready for review. As you push further commits to your branch the PR will update and new builds will kick off to check your work. Continue in this pattern until the builds pass. If you're not sure how to resolve some errors check with another member of the team.

----------
C++ Basics
----------

Feel free to skim read sections that you understand, just pay attention to anything that is new to you.

Mantid uses C++17. Comments relating to useful additional features have been added below the relevant sections to introduce further useful concepts in modern C++.

Reading
^^^^^^^

-  Chapter 13 Introduction to Strings
-  Chapter 14 Automatic type deduction
-  Chapter 19 Functions

   -  also look up online methods for returning more than one value (eg std::tuple)

-  Chapter 31 Organizing Code
-  Chapter 38 C++ Standard Library and Friends

   -  std::vector and std::map
   -  Range based for loops
   -  Lambda expressions
   -  Algorithms. Suggest also looking up:

      -  std::remove (and use with std::erase)
      -  std::transform

Exercise
^^^^^^^^

The code should be placed in *exercises-cpp/firstname_lastname/ex01_basics/src"*

The Visual Studio solution is placed in builds/ex01_basics. On Unix you can type make in *builds/ex01_basics*. The executable will be place in *builds/ex01_basics/bin*.

Write a command line program that will:

#. Take a filename of an ascii file as an argument (you can use the example file `here <https://github.com/martyngigg/cpp-examples/raw/master/Holmes.txt>`__)
#. Load that ascii file.
#. Count the number of occurrences of unique words (longer than 4 characters and split hyphenated words, treating each part as different words). It should be case and punctuation insensitive. You only need to consider the following punctuation characters ``.,?'"!():`` (hint: you will need a backslash escape character for the double-quote)
#. Consider handling of common error cases, such as the wrong file name specified. Return error and status information to the user of the command line tool.
#. Write out a results file containing the unique words and the number of uses in descending order of usage, e.g.

::

   Word    Usage

   which           55
   holmes          49
   there           32
   could           25
   photograph      21
   ...

--------------------------
Object Oriented C++ Basics
--------------------------

Reading
^^^^^^^

-  Chapter 23 Classes - Introduction

   -  Member initialization

      -  also worth looking at this on approaches where an argument is copied in the constructor: `Modernize Pass By Value <https://clang.llvm.org/extra/clang-tidy/checks/modernize-pass-by-value.html>`__

-  Chapter 25 Classes - Inheritance and Polymorphism
-  Chapter 26 Exercises

   -  const modifier
   -  calling base class constructor

-  Chapter 33 Conversions
-  Chapter 35 Smart Pointers
-  Chapter 36 Exercises

Exercise
^^^^^^^^

The code should be placed in *exercises-cpp/firstname_lastname/ex02_oo_basics/src"*

The Visual Studio solution is place in builds/ex02_oo_basics. On Unix you can type make in *builds/ex02_oo_basics*. The executable will be place in *builds/ex02_oo_basics/bin*.

Write a command line program that:

#. Has classes to allow number of shapes to be defined: square (side1), rectangle(side1, side2), circle(radius), triangle(height, base).

   #. Each shape class should know it's type ("Square"), how many sides it has.
   #. Each shape needs to be able to calculate it's perimeter and area. For the triangle you can assume it is isoceles and the perimeter can be computed using :math:`p = b + 2\sqrt{h^2+(b^2/4)}`, where :math:`b` is the base and :math:`h` is the height.

#. Within the Main method create a variety of the shapes and put them in a std::vector
#. Create a class ShapeSorter which should contain four methods

   #. Print out the Shapes that match a chosen type
   #. Print out the Shapes that match a chosen number of sides
   #. Print out the Shapes in order of area descending
   #. Print out the Shapes in order of perimeter descending

---------------
Further reading
---------------

Further modern C++:

-  `nullptr <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#nullptr>`__
-  `Strongly Typed Enums <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#strongly-typed-enums>`__
-  `Constexpr <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#constexpr>`__
-  `Lambdas <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#lambda-expressions>`__
-  `Type Aliases <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#type-aliases>`__
-  `Move Semantics/R-Value References <https://github.com/AnthonyCalandra/modern-cpp-features/blob/master/CPP11.md#move-semantics>`__
