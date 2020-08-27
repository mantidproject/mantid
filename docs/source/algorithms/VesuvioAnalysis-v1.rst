.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm has been developed by Giovanni Romanelli for Vesuvio experiments.

The algorithm can can load, reduce and analyse Vesuvio data. 
Vesuvio is used for Deep Inelastic Neutron Scattering (DINS), which is also refered to as Neutron Compton Scattering (NCS).
This allows direct measurments of nuclear kinetic energies and momentum distributions. 
From these it is possible to examine the nuclear quantum effects and anhominicity. 
The spectroscopic result are different depending on the masses of the elements.

The analysed data represents the kinetic energies of the nuclei.
The analysis will map all of the peak centers onto the same position.
This done by using West Scaling. 


Warning
#######

This algorithm is still in development. 
If you encounter any problems please contact the Mantid team and the Vesuvio scientists.

Usage:
------

**Example: Simple mean of two workspaces**

.. testcode::

   # Create two  2-spectrum workspaces with Y values 1->8 & 3->10
   dataX = [0,1,2,3,4,
            0,1,2,3,4]
   dataY = [1,2,3,4,
            5,6,7,8]
   ws_1 = CreateWorkspace(dataX, dataY, NSpec=2)
   dataY = [3,4,5,6,
            7,8,9,10]
   ws_2 = CreateWorkspace(dataX, dataY, NSpec=2)

   result = Mean("ws_1, ws_2") # note the comma-separate strings
   print("Mean of y values in first spectrum: {}".format(result.readY(0)))
   print("Mean of y values in second spectrum: {}".format(result.readY(1)))

Output:

.. testoutput::

   Mean of y values in first spectrum: [ 2.  3.  4.  5.]
   Mean of y values in second spectrum: [ 6.  7.  8.  9.]

.. categories::

.. sourcelink::
