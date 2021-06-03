.. _04_loading_extensions_on_startup:

=============================
Loading Extensions On Startup
=============================

So far we have had to load our new algorithm in to Mantid each time. Mantid
can be configured to look in directories for Python extensions and load them
automatically:

.. figure:: /images/Training/ExtendingMantidWithPython/python_extensions.png
   :alt: Python Extensions
   :align: center
   :width: 800
   :height: 500

The image above shows the configuration item that controls where Mantid
looks for Python extensions. This is a semi-colon (**;**) separated list of
directories that should be search on startup.

**Warning**: The search is recursive so be careful with directories you put
in here.
