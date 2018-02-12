.. _TrainingAlgorithms:

==========
Algorithms
==========

From the main MantidPlot menu, select

View->Algorithms

For example, ILL reduction workflows can be easily found

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms8.png
   :align: center

Algorithms can take care of error propagation. For more details it is recommended to check the corresponding documentation, click on the ? sign in the left lower corner of the specific algorithm graphical user interface. Arithmetic operations propagate errors (Plus, Minus, ...). As well in Python.

Input validation
----------------

Some algorithms have requirements that will be checked before execution, e.g. units, common binning, etc.
This way, the user will be quickly and early guided to correct inputs rather than motivated reading error messages.

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms0.png
   :align: center

Results Log
-----------

Supports the following log levels:

Error:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms7.png
   :align: center

Warning:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms6.png
   :align: center

Notice:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms5.png
   :align: center

Information:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms3.png
   :align: center

Debug:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Algorithms2.png
   :align: center

The default log level is notice and can be changed by: right click on Result log window and select 'Log level'

History
-------

Mantid keeps the entire history of all algorithms applied to workspaces. Not only does this allow you to audit the data reduction and analysis, it also provides the means to extract a re-executable python script from the GUI.

- Right click on workspace 592724 and select 'Show History'. This will open up the Algorihm History window. In the left-hand side Algorithms panel click on the arrow in front of LoadILLReflectometry v.1:

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/History1.png
   :align: center

More information is given on this in the :ref:`TrainingPythonAndMantid` section.