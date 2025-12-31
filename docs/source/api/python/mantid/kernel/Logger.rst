========
 Logger
========

This is a Python binding to the C++ class Mantid::Kernel::Logger.

By default, ``Logger.notice()`` will log at the root level (mantid).
To get a named ``Logger`` you must create an instance with the name

.. code-block:: python

    from Mantid.kernel import Logger
    mylogger = Logger("mine")
    mylogger.notice("this is a test")

will generate the text ``this is a test`` in the logging console and ``mine-[Notice] this is a test`` in the terminal.
This assumes you are using the default settings.


.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.Logger
    :members:
    :undoc-members:
    :inherited-members:
