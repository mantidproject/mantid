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

.. _mantid.kernel.Logger.priorities:

Logging priorities
------------------

Each message is logged with a priority. A message is only shown if its priority is at or above the logging
level set for the logger and channel (see the logging section of :ref:`Properties File`).
The table lists the priorities from highest to lowest, together with the Python
`logging <https://docs.python.org/3/library/logging.html>`_ level they map to when messages are forwarded
to Python with :ref:`log_to_python <mantid.utils.logging>`.

+-------------------------------+-----------------------+
| Mantid priority               | Python level          |
+===============================+=======================+
| ``fatal``, ``critical``       | ``CRITICAL``          |
+-------------------------------+-----------------------+
| ``error``                     | ``ERROR``             |
+-------------------------------+-----------------------+
| ``warning``                   | ``WARNING``           |
+-------------------------------+-----------------------+
| ``notice``, ``information``   | ``INFO``              |
+-------------------------------+-----------------------+
| ``debug``, ``trace``          | ``DEBUG``             |
+-------------------------------+-----------------------+


.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.Logger
    :members:
    :undoc-members:
    :inherited-members:
