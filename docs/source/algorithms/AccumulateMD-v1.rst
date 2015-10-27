.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This workflow algorithm appends new data to an existing multidimensional workspace. It allows accumalating data in an MDWorkspace as you go, during an experiment.

For more information on the input properties, see :ref:`algm-CreateMD`.

DataSources
#######
These can be workspace names, filenames or full file paths. Not all of the data needs to exist when the algorithm is called.

Clean
#######
It is possible to get confused about what data has been included in an MDWorkspace if it is built up slowly over an experiment. Use this option to start afresh; it creates a new workspace using all of the data in DataSources which are available rather then appending to the existing workspace.

**SOMETHING Example**
##########################################

.. code-block:: python

   print "Hello World!"
  
Output
^^^^^^

.. code-block:: python

   Hello World!

.. categories::

.. sourcelink::
