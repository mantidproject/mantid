
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is part of the new workflow for :ref:`normalizing <MDNorm>` multi-dimensional event workspaces.

Once the ends of detector trajectories are stored in the original :ref:`EventWorkspace <EventWorkspace>` using
the :ref:`CropWorkspaceForMDNorm <algm-CropWorkspaceForMDNorm>` algorithm, one has to run the
:ref:`ConvertToMD <algm-ConvertToMD>` algorithm and convert it to **Q_sample**. During this conversion
some of the trajectories might be truncated. This recalculates the ends such as all the trajectory is
completely contained within the outside box of the :ref:`MDEventWorkspace <MDWorkspace>`

The master equation for transforming from lab coordinate system to HKL units is given by

.. math::

    Q_l = 2 \pi R \cdot U \cdot B \left(\begin{array}{c}
                                        h \\
                                        k \\
                                        l
                                 \end{array}\right)

We define the sample frame as

.. math::

    Q_s=R^{-1}Q_l=R^{-1} \left(\begin{array}{c}
                              -k_f\sin(\theta)\cos(\phi) \\
                              -k_f\sin(\theta)\sin(\phi) \\
                           k_i-k_f\cos(\theta)
                          \end{array}\right)

For elasic scattering  :math:`k_i=k_f`.
For given extents of the input workspace, one can now recalculate the minimum and maximum :math:`k_f`
such as the trajectory is completely contained inside the box.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - RecalculateTrajectoriesExtents**

.. testcode:: RecalculateTrajectoriesExtentsExample

   # Create a host workspace
   event = Load(Filename='CNCS_7860_event.nxs')
   event = ConvertUnits(InputWorkspace=event, Target='DeltaE', EMode='Direct', EFixed=3)
   event = CropWorkspaceForMDNorm(InputWorkspace=event, XMin=-2, XMax=3)
   SetGoniometer(Workspace=event, Axis0='0,0,1,0,1')
   md = ConvertToMD(InputWorkspace=event,
                    QDimensions='Q3D',
                    Q3DFrames='Q_sample',
                    OtherDimensions='SampleTemp',
                    MinValues='0.8,-2,-2,-2,-2',
                    MaxValues='1.,0,2,2,500')
   recalculated = RecalculateTrajectoriesExtents(InputWorkspace=md)

   import numpy as np
   #Original workspace
   high=np.array(md.getExperimentInfo(0).run()['MDNorm_high'].value)
   low=np.array(md.getExperimentInfo(0).run()['MDNorm_low'].value)
   n=len(high[high-low>1])
   print("Number of trajectories in original workspace with length of more than 1meV: {}".format(n))
   #Recalculated workspace
   high=np.array(recalculated.getExperimentInfo(0).run()['MDNorm_high'].value)
   low=np.array(recalculated.getExperimentInfo(0).run()['MDNorm_low'].value)
   n=len(high[high-low>1])
   print("Number of trajectories in recalculated workspace with length of more than 1meV: {}".format(n))

.. testcleanup:: RecalculateTrajectoriesExtentsExample

    DeleteWorkspace('event')
    DeleteWorkspace('md')
    DeleteWorkspace('recalculated')

Output:

.. testoutput:: RecalculateTrajectoriesExtentsExample

  Number of trajectories in original workspace with length of more than 1meV: 51200
  Number of trajectories in recalculated workspace with length of more than 1meV: 2590

.. categories::

.. sourcelink::
