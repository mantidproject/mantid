==================
 IntArrayProperty
==================

This is a Python binding to the C++ class Mantid::Kernel::ArrayProperty.

Much of the syntax of this property is identical to :class:`mantid.api.MultipleFileProperty`.

+---------------------+-----------------------------+--------------------------------------------------+---------------+----------------+
| Name                | Usage                       | Description                                      | Example Input | Example Result |
+=====================+=============================+==================================================+===============+================+
| Range               | ``<start>-<stop>``          | Used to specify a range of values                | ``1-4``       | 1,2,3,4        |
+---------------------+-----------------------------+--------------------------------------------------+---------------+----------------+
| Range               | ``<start>:<stop>``          | Used to specify a range of values                | ``1:4``       | 1,2,3,4        |
+---------------------+-----------------------------+--------------------------------------------------+---------------+----------------+
| Stepped Range       | ``<start>:<stop>:<step>``   | Used to specify a ``stepped`` range of values    | ``1:5:2``     | 1,3,5          |
+---------------------+-----------------------------+--------------------------------------------------+---------------+----------------+
| List                | ``<item1>,<item2>``         | Used to list values, can be combined with ranges | ``1,3-5``     | 1,3,4,5        |
+---------------------+-----------------------------+--------------------------------------------------+---------------+----------------+


*bases:* :py:obj:`mantid.kernel.VectorLongPropertyWithValue`

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.IntArrayProperty
    :members:
    :undoc-members:
    :inherited-members:
