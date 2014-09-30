.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
 
.. role:: xml(literal)
   :class: highlight

This algorithm allows instrument parameters to be specified in a
separate file from the :ref:`IDF <InstrumentDefinitionFile>`. The required
format for this file is identical to that used for defining parameters
through :xml:`<component-link>` tags in an
:ref:`IDF <InstrumentDefinitionFile>`. Below is an example of how to define a parameter
named 'test' to be associated with a component named 'bank\_90degnew'
defined in the :ref:`IDF <InstrumentDefinitionFile>` of the HRPD instrument:

Components may also be identified by the id of a detector. The example below also demonstrates how to define a
parameter named 'test2' for a component whose detector has an id of '14'.

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <parameter-file instrument="HRPD" valid-from="YYYY-MM-DD HH:MM:SS">

    <component-link name="bank_90degnew" >
      <parameter name="test"> <value val="50.0" /> </parameter>
    </component-link>

    <component-link id="14" >
      <parameter name="test2"> <value val="50.0" /> </parameter>
    </component-link>

    </parameter-file>

.. categories::
