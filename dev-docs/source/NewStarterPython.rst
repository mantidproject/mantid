.. _NewStarterPython:

=====================
New Starter Python
=====================

.. contents::
   :local:

-------------
Prerequisites
-------------

-  Access to the `Python documentation <https://docs.python.org/3/>`__
-  You will also need a computer with a Python development environment.
-  Git: *Windows/Mac* - Download the latest version from http://git-scm.com/; *Linux* - Install from package manager (ubuntu pkg=git-gui)


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
-  Make a copy of the *exercises-python/template* directory and name it *firstname_lastname*. The directory should be in the exercises directory.
-  Make a directory called *builds* in the root of the *newstarter* repository. Git is setup to ignore this directory.


Now you are ready to code the solution to the exercise in your chosen native build environment.

-  As you work use the git commands to commit to your branch and push to GitHub.
-  When you think you have completed the exercise you can use the continuous integration build servers to check your work. To do this you first need to create a pull-request. See `create a pull request <https://help.github.com/articles/creating-a-pull-request/>`__ for your branch so that it can be reviewed by a senior developer.
-  The pull request will kick-off builds on Red Hat and Windows platforms and GitHub will mark up the results of these builds on the pull requests. Try and get each build to a green status before saying it is ready for review. As you push further commits to your branch the PR will update and new builds will kick off to check your work. Continue in this pattern until the builds pass. If you're not sure how to resolve some errors check with another member of the team.

----------
Python Basics
----------

Feel free to skim read sections that you understand, just pay attention to anything that is new to you.

Mantid uses Python 3.

Reading
^^^^^^^

-  Python library: `Strings <https://docs.python.org/3/library/stdtypes.html#text-sequence-type-str>`__
-  Python tutorial: `Defining Functions <https://docs.python.org/3/tutorial/controlflow.html#defining-functions>`__
-  Python tutorial: `Dictionaries <https://docs.python.org/3/tutorial/datastructures.html#dictionaries>`__
-  Python tutorial: `Command Line Arguments <https://docs.python.org/3/tutorial/stdlib.html#command-line-arguments>`__


Exercise
^^^^^^^^

The code should be placed in *exercises-python/firstname_lastname/ex01_basics/"*

Write a command line program that will:

#. Take a filename of an ascii file as an argument (you can use the example file `here <https://github.com/martyngigg/cpp-examples/raw/master/Holmes.txt>`__)
#. Load that ascii file.
#. Count the number of occurrences of unique words (longer than 4 characters and split hyphenated words, treating each part as different words). It should be case and punctuation insensitive. You only need to consider the following punctuation characters ``.,?'"!():`` (hint: you will need a backslash escape character for the double-quote)
#. Consider handling of common error cases, such as the wrong file name specified. Return error and status information to the user of the command line tool.
#. Print the results to screen showing the unique words and the number of uses in descending order of usage, e.g.

::

   Word    Usage

   which           55
   holmes          49
   there           32
   could           25
   photograph      21
   ...

--------------------------
Object Oriented Python Basics
--------------------------

Reading
^^^^^^^

-  Python tutorial: `Classes <https://docs.python.org/3/tutorial/classes.html>`__
-  Python HowTo: `Sorting <https://docs.python.org/3/howto/sorting.html>`__


Exercise
^^^^^^^^

The code should be placed in *exercises-python/firstname_lastname/ex02_oo_basics"*

Write a command line program that:

#. Has classes to allow number of shapes to be defined: square (side1), rectangle(side1, side2), circle(radius), triangle(height, base).

   #. Each shape class should know it's type ("Square"), how many sides it has.
   #. Each shape needs to be able to calculate it's perimeter and area. For the triangle you can assume it is isoceles and the perimeter can be computed using :math:`p = b + 2\sqrt{h^2+(b^2/4)}`, where :math:`b` is the base and :math:`h` is the height.

#. Within the Main method create a variety of the shapes and put them in a list
#. Create a class ShapeSorter which should contain four methods

   #. Print out the Shapes that match a chosen type
   #. Print out the Shapes that match a chosen number of sides
   #. Print out the Shapes in order of area descending
   #. Print out the Shapes in order of perimeter descending

---------------
Further reading
---------------

Further Python:

- `Think Python <https://greenteapress.com/wp/think-python-2e/>`__
- Python style guide `PEP8 <https://www.python.org/dev/peps/pep-0008/>`__
