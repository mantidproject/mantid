
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This returns a string the full path to the IPTS shared folder to allow
for saving of files in accessible user folders (e.g. ``shared``).

The algorithm has a cache of runnumbers that is stores while mantid is running.
This cache can be reset using the ``ClearCache`` argument or by restarting mantid.

.. warning::

    This only works at ORNL.

Usage
-----

**Example - IPTS directory for default instrument**

This will vary based on your default instrument.

.. code-block:: python

      print(GetIPTS(12345))

Output:

.. code-block:: none

    /SNS/PG3/IPTS-8111/

**Example - IPTS directory for specific instrument**

Being explicit about the instrument gives the same
result for everyone.

.. code-block:: python

      print(GetIPTS(Instrument='NOM', RunNumber=12345))

Output:

.. code-block:: none

      /SNS/NOM/IPTS-8687/

.. categories::

.. sourcelink::
