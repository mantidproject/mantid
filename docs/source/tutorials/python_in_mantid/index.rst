.. _pim_intro:

===================
Python Introduction
===================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 2

   algorithms_and_workspaces
   script_generation
   controlling_mantid
   further_alg_ws
   pim_solutions


**Other Plotting Documentation**

* :ref:`pim_alg_and_ws`
* :ref:`pim_script_generation`
* :ref:`pim_controlling_mantid`
* :ref:`pim_further_alg_ws`
* :ref:`pim_solutions`




Much of Mantid can be scripted using Python. There are currently three ways of running Mantid related Python code:
from MantidPlot: Script window (View->Script Window) - For writing & executing long multi-line scripts;
from MantidPlot: Script interpreter (View->Toggle Script Interpreter) - For executing single-line commands that are run when you press return;
from outside MantidPlot: run a like any regular python script. Note that MantidPlot functionality is not available (plots, instrument views, etc.). Mantid libraries would need to be included in the Python search path, either through PYTHONPATH environment variable, or explicitly in the script.
Scopes
Each of the above 'environments' defines its own scope. What this means is that each environment (including each tab on the script window) is separated when it comes to running code. For example, writing the following code in a tab within the script window

.. code-block:: python

    x = 1
    print(x)

and executing it in the same tab then it will print "1" to the output window. However, opening the script interpreter, typing

.. code-block:: python

    print(x)

and pressing return will result in the following error: `NameError on line 1: "name 'x' is not defined"`. This is because x is only defined in the place where x=1 was executed.