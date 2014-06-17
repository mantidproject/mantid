.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm creates an XML Grouping file of the form:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <detector-grouping>
    <group name="fwd"><detids val="1,2,17,32"/></group>
    <group name="bwd"><detids val="33,36,38,60,64"/> </group>
    </detector-grouping>

Based on information retrieved from the
`Nearest Neighbours <http://www.mantidproject.org/Nearest_Neighbours>`_
class in Mantid Geometry.

.. categories::
