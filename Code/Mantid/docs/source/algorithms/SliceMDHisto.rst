.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

SliceMDHisto extracts a hyperslab of data from a MDHistoWorkspace. Beyond
the usual input and output workspace parameters, the start and end of the
hyperslabs dimensions are required. Both  as comma separated lists with an
entry for each dimension of the MDHistoWorkspace.

Example
#######

Consider an input MDHistoWorkspace with dimensions 100,100,100.
Running SliceMDHisto with parameters Start= 20,20,20 and End= 50,50,100
will copy all the data between x: 20-50, y: 20-50, z:20-100 into the
result MDHistoWorkspace with dimensions 30,30,80.

.. categories::
