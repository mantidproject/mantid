.. _mantid.utils.logging:

===============
 log_to_python
===============

``mantid.utils.logging.log_to_python`` forwards Mantid's log messages to Python's standard
`logging <https://docs.python.org/3/library/logging.html>`_ framework instead of letting Mantid write them itself.
Messages are sent to the Python logger named ``'Mantid'``, so they can be filtered, formatted and written
to files or other handlers together with log messages from other Python packages.

``log_to_python`` changes Mantid's logging configuration for the current session.
It replaces the console channel with ``PythonLoggingChannel``; see the logging section of
:ref:`Properties File` for the underlying configuration options.
It does not configure Python's logging system. You must still add handlers and set levels on the Python side,
for example with :py:func:`logging.basicConfig`.

Mantid message priorities map to Python logging levels as follows:

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

A message must pass Mantid's logging level before it is forwarded, and then pass the level of the
Python logger and its handlers before it is shown.
To let Python decide which messages to keep, set a low Mantid level with the ``level`` argument.

.. autofunction:: mantid.utils.logging.log_to_python

Usage
-----

**Example - Send Mantid messages to Python's logging framework**

.. code-block:: python

    import logging
    from mantid.kernel import logger
    from mantid.utils.logging import log_to_python

    # Configure Python's logging: show messages of level INFO and above on the console
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(levelname)s: %(message)s")

    # Forward all Mantid messages of priority 'debug' and above to Python
    log_to_python(level="debug")

    logger.information("This message is shown by Python's logging framework.")
    logger.debug("This message is forwarded, but filtered out by the Python logger.")

**Example - Write Mantid messages to a file with a custom format**

.. code-block:: python

    import logging
    from mantid.utils.logging import log_to_python

    handler = logging.FileHandler("reduction.log")
    mantid_log = logging.getLogger("Mantid")
    mantid_log.addHandler(handler)
    mantid_log.setLevel(logging.WARNING)

    # Prefix each message with the name of the Mantid logger that produced it
    log_to_python(level="information", pattern="[%s] %t")

Messages are queued before they are passed to Python, so they can appear shortly after the call that produced them.
If more than ``logging.python.queueSize`` messages are waiting, further messages are dropped
and a warning reports how many were lost.

See Also
--------
:ref:`Properties File`, :class:`mantid.kernel.Logger`
