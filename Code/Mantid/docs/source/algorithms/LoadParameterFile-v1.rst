.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows instrument parameters to be specified in a
separate file from the `IDF <InstrumentDefinitionFile>`__. The required
format for this file is identical to that used for defining parameters
through s in an IDF. Below is an example of how to define a parameter
named 'test' to be associated with a component named 'bank\_90degnew'
defined in the IDF of the HRPD instrument:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <parameter-file instrument="HRPD" valid-from="YYYY-MM-DD HH:MM:SS">

    <component-link name="bank_90degnew" >
      <parameter name="test"> <value val="50.0" /> </parameter>
    </component-link>

    </parameter-file>

.. categories::
