.. _PythonNotebook:

==========================
MantidPython and Notebook
==========================

MantidPython can be used to access all of the functionality of the Mantid Framework outside of the Workbench interface.

The IPython Notebook is a web-based interactive computational environment where you can combine code execution, text, mathematics, plots and rich media into a single document.

Launching on different platforms
================================

Windows
-------

Open a Command Prompt.


Launch MantidPython:

.. code-block:: python

   C:\MantidInstall\bin\mantidpython.bat --classic

Launch iPython Notebook:

.. code-block:: python

   C:\MantidInstall\bin\mantidpython.bat notebook


MacOS 
-----

Open Terminal.


Launch MantidPython:

.. code-block:: python

   /Applications/MantidWorkbench.app/Contents/MacOS/mantidpython --classic

Launch iPython Notebook:

.. code-block:: python

   /Applications/MantidWorkbench.app/Contents/MacOS/mantidpython notebook


Ubuntu
------

Open Terminal.

Launch MantidPython:

.. code-block:: python

   /opt/Mantid/bin/mantidpython --classic

Launch iPython Notebook:

.. code-block:: python

   /opt/Mantid/bin/mantidpython notebook --notebook-dir="~"


Accessing Algorithms
====================

In MantidPython or an IPython Notebook, accessing the Mantid Framework should be as simple as:

.. code-block:: python

   from mantid.simpleapi import *

If this doesn't work, the path to the Mantid Installation directory should be set:

.. code-block:: python

   import sys
   sys.path.append('PATH')

Where the 'PATH' should be:

* Windows: ``[INSTALL_PATH]\bin`` where ``INSTALL_PATH`` eg. ``'C:\MantidInstall\bin\'``
* Linux: ``/opt/Mantid/bin`` for an official release and ``/opt/mantidnightly/bin`` for a nightly development build
* MacOS: ``/Applications/MantidWorkbench.app/Contents/MacOS``



Running a Script
================

Simply call the script after the MantidPython or Notebook launch command:

Run a python script on MacOS:

.. code-block:: python

   /Applications/MantidWorkbench.app/Contents/MacOS/mantidpython --classic script.py

Run a notebook on Windows:

.. code-block:: python

   C:\MantidInstall\bin\mantidpython.bat notebook "Downloads\Introduction to using Mantid with IPython Notebook.ipynb"

`Download this Example Notebook <http://sourceforge.net/projects/mantid/files/IPython%20Notebook/Introduction%20to%20using%20Mantid%20with%20IPython%20Notebook.ipynb/download>`_


Script and Notebook Generation
==============================

After processing a workspace in Mantid, the history of how this data was manipulated can be converted to a script or a notebook, with the use of :ref:`algm-GeneratePythonScript` or :ref:`algm-GenerateIPythonNotebook` respectively.
