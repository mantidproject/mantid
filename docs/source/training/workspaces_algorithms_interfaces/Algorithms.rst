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

To replay the history do:

- From the main MantidPlot menu select View->Script Window, this opens the 'MantidPlot: Python Window'
- Go back to Algorithm History window and press the 'Script to Clipboard' button and close the Algorithm History window
- Flip to the script window ('MantidPlot: Python Window') and paste what was copied to the clipboard into the script window
- Delete the 592724 workspace from the Algorithms panel
- To recreate the work you have just done script window execute the script via Execute->Execute All on the script window.

Content Clipboard (Python):

.. code-block:: python

   LoadILLReflectometry(Filename='/net4/serdon/illdata/171/figaro/internalUse/rawdata/592724.nxs', OutputWorkspace='592724', XUnit='TimeOfFlight')
   GravityCorrection(InputWorkspace='592724', OutputWorkspace='592724_gc', FirstSlitName='slit3')
   Logarithm(InputWorkspace='592724_gc', OutputWorkspace='592724_gc')
   ConvertUnits(InputWorkspace='592724_gc', OutputWorkspace='592724_gc', Target='Wavelength', ConvertFromPointData=False)
