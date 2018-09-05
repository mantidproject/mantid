.. _Workbench:

=====================
Workbench Development
=====================

.. contents::
  :local:


Overview
########

The workbench is the new PyQt-based GUI that will be the primary interface for
interacting with the mantid framework. The plotting is provided by
`matplotlib <https://matplotlib.org/>`_. It will eventually replace MantidPlot.

Building
########

The following build instructions assume you have follwed the instructions on the :ref:`GettingStarted` pages and can build mantid and MantidPlot. To enable the
build of the workbench simply set the cmake flag ``ENABLE_WORKBENCH=ON`` and
build as normal. A ``workbench`` startup script (Linux/macOS) or executable (Windows) will appear in the ``bin`` folder. For Windows the executable will appear in the configuration subdirectory of ``bin``.

Packaging
#########

Packaging is currently only supported on Linux platforms and must be enabled
using the cmake flag ``PACKAGE_WORKBENCH=ON``.
