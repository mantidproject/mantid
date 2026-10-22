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


Memory Allocator on Linux
=========================

On Linux, MantidWorkbench uses Intel TBB's scalable memory allocator (``tbbmalloc``),
which makes many multithreaded algorithms, such as :ref:`algm-LoadEventNexus`, faster than with the system allocator.
When ``mantid`` is installed with conda, every ``python``, ``ipython`` or Jupyter kernel started from that environment
uses the same allocator:
at startup, before any of your code runs, Python restarts itself once with
``LD_PRELOAD`` set to the environment's ``libtbbmalloc_proxy.so.2``.
Programs started from that Python process inherit ``LD_PRELOAD`` and use it too.

``tbbmalloc`` can use more memory than the system allocator, because it keeps freed memory for reuse.
To use the system allocator instead, set the environment variable ``MANTID_DISABLE_TBBMALLOC=1``
before starting Python, or start Python with ``-S``.

Script and Notebook Generation
==============================

After processing a workspace in Mantid, the history of how this data was manipulated can be converted to a script
or a notebook, with the use of :ref:`algm-GeneratePythonScript` or :ref:`algm-GenerateIPythonNotebook` respectively.

.. categories:: Concepts
