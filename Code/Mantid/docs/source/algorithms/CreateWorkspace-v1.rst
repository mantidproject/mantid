.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Example of use in Python for create a simple histogram workspace and
automatically populating the VerticalAxis with SpectraNumber values.

.. code-block:: python

     dataX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
     dataY = [1,2,3,4,5,6,7,8,9,10,11,12]
     dataE = [1,2,3,4,5,6,7,8,9,10,11,12]
     
     # The workspace will be named "dataWS"
     dataWS = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4,UnitX="Wavelength")

.. categories::
