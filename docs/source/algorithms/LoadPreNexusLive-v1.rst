
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This loads a 'live' file that is periodically saved by the legacy DAS
at SNS. If the ``LoadLogs`` option is specified, it will attempt to
load logs from the most recent saved run. Specifying ``LogFilename``
will turn off the search capability.

.. warning::

    This only works at ORNL.

Usage
-----

.. code-block:: python

      LoadPreNexusLive(Instrument='SNAP', OutputWorkspace='SNAP_live')


.. categories::

.. sourcelink::
