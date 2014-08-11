.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves a HDF5 file to the ZODS (Zurich Oak Ridge Disorder Simulation
program) format. This format consists of a slice of a
:ref:`MDHistoWorkspace <MDHistoWorkspace>` and some information about its
location.

**You must be in HKL space for the output of this algorithm to make
sense!**

From http://www.crystal.mat.ethz.ch/research/DiffuseXrayScattering:

-  In the frame of an international cooperation between our group, the
   University of Zurich and Oak Ridge National Laboratories, we are
   further developing Monte Carlo simulation techniques for modeling
   disordered crystal structures. A major goal of this project is to
   develop the Monte Carlo simulation computer program ZODS (Zurich Oak
   Ridge Disorder Simulation program), which is designed to be not only
   user friendly, but also fast and flexible. Special focus is on the
   efficient use of modern super-computer architectures and of
   multi-core desktop computers to take full advantage of current trends
   in computing technologies.

Summary of data format
######################

In general it contains collection of grids with intensities and each
grid is described by specifying origin, size of grid (in each direction)
and grid base vectors (a1,a2,a3) so a point at node (i,j,k) of grid has
coordinates r = origin+i\*a1+j\*a2+k\*a3.

Please contact Michal Chodkiewicz (michal.chodkiewicz@gmail.com); Vickie
Lynch (lynchve@ornl.gov) for more details.

Description of data fields
##########################

-  The CoordinateSystem data object has the attribute "isLocal".

   -  If **isLocal**\ =1, then we are in HKL space:

      -  The **direction\_1** vector (0.04,0,0) represents a step of
         0.04 in the (1,0,0) direction (a\*) in reciprocal space.
      -  **This is currently the only mode in which SaveZODS saves**.

   -  If **isLocal**\ =0, then we are in Q-space.

      -  The **direction\_1** vector (0.04,0,0) represents a step of
         0.04 Angstrom^(-1) in X direction of Cartesian coordinate
         system (with X colinear with a and Z with c\*)
      -  In this case CoordinateSystem has additional attribute
         UnitCell, which is a vector with 6 components
         (a,b,c,alpha,beta,gamma) a,b,c in angstroms and angles in
         degrees.

-  The **origin** field gives the origin of the center of the (0,0,0)
   cell; in HKL space for our case of isLocal=1.
-  The **size** field gives the number of bins in each direction.
-  The **data** field contains the actual intensity at each bin.

   -  The grid of points (r = origin+i\*a1+j\*a2+k\*a3) specifies the
      centers of histogram, not the corners.

Usage
-----

This algorithm can be run on a pre-existing `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_
or a newly created one. The example below will be done with newly created one
using :ref:`CreateMDWorkspace <algm-CreateMDWorkspace>`.

**SaveZODS HKL MDHisto Example**

.. testcode:: SaveZODSEx

    ws = CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8', ErrorInput='1,1,1,1,1,1,1,1', 
        Dimensionality='3', Extents='-2, 2, -2, 2, -2, 2', Names=['[H,0,0]','[0,K,0]','[0,0,L]'],
        NumberOfBins='2,2,2', Units='lattice,lattice,lattice')
    import os
    savefile = os.path.join(config["defaultsave.directory"], "ZODS.h5")
    SaveZODS(InputWorkspace=ws, FileName=savefile)
    print "File created:", os.path.exists(savefile)

Output:

.. testoutput:: SaveZODSEx

   File created: True 


.. categories::
