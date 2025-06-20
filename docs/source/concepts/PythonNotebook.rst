.. _PythonNotebook:

====================================
Access Mantid in Python and Notebook
====================================

All the functionality of Mantid can be accessed outside of the Workbench interface.

To access Mantid in python, simply install the
mantid framework alongside python (we recommend using `conda <https://download.mantidproject.org/conda.html>`_).
Alternatively, the python packaged with MantidWorkbench can be launched from the command line,
using the correct path detailed below.

A Jupyter Notebook is a web-based interactive computational environment where you can combine code execution, text,
mathematics, plots and rich media into a single document.
To access this, you need to install ``notebook`` onto the Python version that has ``mantid`` installed.
Again, we recommend using a conda environment that contains both ``notebook`` and ``mantid``.
If you want to launch a Jupyter Notebook from MantidWorkbench, then you will have to use conda or pip to install
``notebook`` onto the packaged python version.

Packaged Python Path
====================

Find the packaged python within MantidWorkbench on each OS:

* Windows: ``[INSTALL_PATH]\bin\python.exe`` where ``INSTALL_PATH`` eg. ``'C:\MantidInstall\bin\python.exe'``
* Linux: ``/opt/mantidworkbench/bin/python3``
* MacOS: ``/Applications/MantidWorkbench.app/Contents/Resources/bin/python3``

Commands
========

In Python or a Jupyter Notebook, accessing parts of the Mantid framework should be as simple as:

.. code-block:: python

   from mantid.simpleapi import *

Run a python script:

.. code-block:: none

   /path/to/python script.py

Run a notebook:

.. code-block:: none

   /path/to/python -m notebook "Downloads\Introduction to using Mantid with IPython Notebook.ipynb"

Download an Example Notebook `here <http://sourceforge.net/projects/mantid/files/IPython%20Notebook/Introduction%20to%20using%20Mantid%20with%20IPython%20Notebook.ipynb/download>`_.


Script and Notebook Generation
==============================

After processing a workspace in Mantid, the history of how this data was manipulated can be converted to a script
or a notebook, with the use of :ref:`algm-GeneratePythonScript` or :ref:`algm-GenerateIPythonNotebook` respectively.

.. categories:: Concepts
