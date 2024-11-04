.. _pim_alg_and_ws:

=====================================
Algorithms and Workspaces with Python
=====================================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   01_running_algorithms
   02_ws_types
   03_matrix_ws_py
   04_table_ws_py
   05_group_ws_py
   06_alg_help
   07_pim_ex_1


Much of Mantid can be scripted using `Python <https://www.python.org/>`_. There are currently three ways of running Mantid related Python code:

#. inside Mantid Workbench: Script Editor (View > Editor) - For writing & executing long multi-line scripts;
#. inside Mantid Workbench: IPython (View > IPython) - For executing single-line commands that are evaluated when you press return;
#. outside Mantid Workbench: in a command prompt / terminal like any regular python script. Note that Mantid functionality is not available (plots, instrument views, etc.). Mantid libraries need to be included in the Python search path, either through the PYTHONPATH environment variable, or explicitly in the script.

Scopes
======

Each of the above 'environments' has a separate scope. What this means is that each environment (including each tab on the script window) is separated when it comes to running code. For example, writing the following code in a tab within the script window

.. code-block:: python

    x = 1
    print(x)

and executing it in the same tab then it will print "1" to the output window. However, opening the script interpreter and executing

.. code-block:: python

    print(x)

will result in the following error: `NameError on line 1: "name 'x' is not defined"`. This is because x is only defined in the place where x=1 was executed.

**Contents**

* :ref:`01_running_algorithms`
* :ref:`02_ws_types`
* :ref:`03_matrix_ws_py`
* :ref:`04_table_ws_py`
* :ref:`05_group_ws_py`
* :ref:`06_alg_help`
* :ref:`07_pim_ex_1`
