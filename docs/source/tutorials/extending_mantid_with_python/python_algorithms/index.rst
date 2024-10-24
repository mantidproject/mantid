.. _emwp_py_algs:

=================
Python Algorithms
=================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   01_basic_algorithm_structure
   02_basic_properties
   03_validating_input
   04_main_algorithm_body
   05_logging
   06_exercise_2

Mantid ships with many predefined :ref:`Algorithms List`.

Python algorithms provide a mechanism to define extra algorithms at run time
that Mantid treats exactly the same as if they were shipped with the package.

Benefits of an algorithm over just writing in a script:

#. Create workspaces that are seen by Mantid - Improved performance for your
   specific task
#. History tracking - Python processing in a standard script isn't tracked in
   the workspace history but it is through a Python algorithm
#. Free GUI - All algorithms have a default auto-generated input GUI

**Contents**

* :ref:`01_basic_algorithm_structure`
* :ref:`02_basic_properties`
* :ref:`03_validating_input`
* :ref:`04_main_algorithm_body`
* :ref:`05_logging`
* :ref:`06_exercise_2`
