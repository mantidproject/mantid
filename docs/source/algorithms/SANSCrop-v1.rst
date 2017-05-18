.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows to crop a particular detector bank from a workspace. The supported configurations are *LAB* and *HAB*. Currently this crop mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.

Component setting: *LAB* and *HAB*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The mapping of this setting is:

+------------+---------------------+-------------------+
| Instrument | *LAB*               | *HAB*             |
+============+=====================+===================+
| SANS2D     | "rear-detector"     | front-detector"   |
+------------+---------------------+-------------------+
| LOQ        | "main-detector-bank"| "HAB"             |
+------------+---------------------+-------------------+
| LARMOR     | "DetectorBench"     | "DetectorBench"   |
+------------+---------------------+-------------------+


.. categories::

.. sourcelink::
