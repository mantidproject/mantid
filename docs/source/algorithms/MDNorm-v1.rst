
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Starting from events in the momentum transfer space , this algorithm puts the data on a regular grid
in either reciprocal space of the sample (if `RLU` is selected) or in the goniometer frame,
then it normalizes to get the scattering cross section.
For diffraction data, the output workspace contains the differential cross section :math:`\frac{d\sigma}{d\Omega}`, while
for direct geometry inelastic data one obtains the double differential cross section  :math:`\frac{d^2 \sigma}{dE d\Omega}`.
One can choose any orientation for the momentum axes (to get the first axis to be `[-H,H,0]`, set `QDimension0` to `-1,1,0`.

**Note:** In order to calculate the trajectories, the algorithm relies on finding information about detector
trajectories stored in the workspace. The algorithm :ref:`CropWorkspaceForMDNorm <algm-CropWorkspaceForMDNorm>` must
be run *before* converting to multidimensional workspace. Optionally, in aaddition, one can recalculate the extents of the trajectories
using the :ref:`RecalculateTrajectoriesExtents  <algm-RecalculateTrajectoriesExtents>` algorithm after convering to the
multidimensional workspace.

The solid angle workspace is a :ref:`MatrixWorkspace <MatrixWorkspace>` that contains the solid angle/efficiency of the detectors.
One can just integrate a vanadium file between some appropriate limits. For diffraction measurements, the flux workspace
is a mandatory input. It is a :ref:`MatrixWorkspace <MatrixWorkspace>`
that contains the indefinite integral of the incident flux. It contains one or more spectra, each of them corresponding to
detectors that have the same energy response. The algorithm :ref:`MDNormSCDPreprocessIncoherent
<algm-MDNormSCDPreprocessIncoherent>` can be used to process Vanadium
data for the Solid Angle and Flux workspaces.

The input workspace is binned using the :ref:`BinMD <algm-BinMD>` algorithm, to account for the selected momentum
dimensions. For each dimension to be binned, we specify a name (for example `Dimension0Name='QDimension0'`). For any momentum dimension
the name is one of `QDimension0`, `QDimension1`, or `QDimension2` along the axes specified in the algorithm. All three momentum
dimensions must be present in the parameter list of the algorithm. Any other dimension name, such as `DeltaE`, is optional,
and must be identical to a dimension in the input workspace. If a dimension name is present in the input workspace but not within the
parameters of this algorithm, the corresponding data will be integrated. The semnification of binning parameters is as following:

+----------------------------------+-------------------------------------------------------+
| Format                           |                                                       |
+==================================+=======================================================+
|  minimum, stepsize, maximum      | The dimension in the output workspace will extend     |
|                                  | from 'minimum' to 'maximum' with bins of width        |
|                                  | 'stepsize'.                                           |
+----------------------------------+-------------------------------------------------------+
|  minimum, maximum                | A single bin will be created between 'minimum' and    |
|                                  | 'maximum'.                                            |
+----------------------------------+-------------------------------------------------------+
|  stepsize                        | The 'minimum' and 'maximum' are set to the dimension  |
|                                  | limits; the workspace is not cut in this dimension.   |
+----------------------------------+-------------------------------------------------------+

The binned workspace is stored in the `OutputDataWorkspace`.

Trajectories of each detector in reciprocal space are omputed, and the normalization is calculated between intersections with each
MDBox. A brief introduction to the multi-dimensional data normalization can be found :ref:`here <MDNorm>`. The
`OutputNormalizationWorkspace` contains the denominator of equations (2) or (3). In the :ref:`normalization document <MDNorm>`.

The `OutputWorkspace` contains the ratio of the `OutputDataWorkspace` and `OutputNormalizationWorkspace`.

One can accumulate multiple inputs. The correct way to do it is to add the counts together, add the normalizations
together, then divide. For user convenience, one can provide these accumulation workspaces as `TemporaryDataWorkspace`
and `TemporaryNormalizationWorkspace`.

There are symmetrization options for the data. To achieve this option, one can use the `SymmetryOperations` parameter. It can accept
a space group name, a point group name, or a list of symmetry operations. More information about symmetry operations can be found
:ref:`here <Symmetry groups>` and :ref:`here <Point and space groups>`


**Example - MDNorm**

For diffraction measurements a sample code is found below:

.. code-block:: python

   Load(Filename='CORELLI_29782.nxs', OutputWorkspace='data')
   Load(Filename='SingleCrystalDiffuseReduction_SA.nxs', OutputWorkspace='SolidAngle')
   Load(Filename='SingleCrystalDiffuseReduction_Flux.nxs', OutputWorkspace= 'Flux')
   MaskDetectors(Workspace='data', MaskedWorkspace='SolidAngle')
   ConvertUnits(InputWorkspace='data',OutputWorkspace='data',Target='Momentum')
   CropWorkspaceForMDNorm(InputWorkspace='data',
                          XMin=2.5,
                          XMax=10,
                          OutputWorkspace='data')
   LoadIsawUB(InputWorkspace='data',Filename='SingleCrystalDiffuseReduction_UB.mat')
   SetGoniometer(Workspace='data',Axis0='BL9:Mot:Sample:Axis1,0,1,0,1')
   min_vals,max_vals=ConvertToMDMinMaxGlobal(InputWorkspace='data',
                                             QDimensions='Q3D',
                                             dEAnalysisMode='Elastic',
                                             Q3DFrames='Q')
   ConvertToMD(InputWorkspace='data',
               QDimensions='Q3D',
               dEAnalysisMode='Elastic',
               Q3DFrames='Q_sample',
               OutputWorkspace='md',
               MinValues=min_vals,
               MaxValues=max_vals)
   RecalculateTrajectoriesExtents(InputWorkspace= 'md', OutputWorkspace='md')

   MDNorm(InputWorkspace='md',
          SolidAngleWorkspace='SolidAngle',
          FluxWorkspace='Flux',
          QDimension0='1,1,0',
          QDimension1='1,-1,0',
          QDimension2='0,0,1',
          Dimension0Name='QDimension0',
          Dimension0Binning='-10.0,0.1,10.0',
          Dimension1Name='QDimension1',
          Dimension1Binning='-10.0,0.1,10.0',
          Dimension2Name='QDimension2',
          Dimension2Binning='-0.1,0.1',
          SymmetryOperations='P 31 2 1',
          OutputWorkspace='result',
          OutputDataWorkspace='dataMD',
          OutputNormalizationWorkspace='normMD')

The output would look like:

.. figure:: /images/MDNorm_elastic_sym.png


Here is a sample code for inelastic data:

.. code-block:: python

   Load(Filename='HYS_13656-13658',OutputWorkspace='sum')
   SetGoniometer(Workspace='sum', Axis0='s1,0,1,0,1')
   GenerateEventsFilter(InputWorkspace='sum',
                        OutputWorkspace='splboth',
                        InformationWorkspace='info',
                        UnitOfTime='Nanoseconds',
                        LogName='s1',
                        MaximumLogValue=90,
                        LogValueInterval=1)
   FilterEvents(InputWorkspace='sum',
                SplitterWorkspace='splboth',
                InformationWorkspace='info',
                FilterByPulseTime=True,
                GroupWorkspaces=True,
                OutputWorkspaceIndexedFrom1=True,
                OutputWorkspaceBaseName='split')
   DgsReduction(SampleInputWorkspace='split',
                SampleInputMonitorWorkspace='split_1',
                IncidentEnergyGuess=50,
                SofPhiEIsDistribution=False,
                TimeIndepBackgroundSub=True,
                TibTofRangeStart=10400,
                TibTofRangeEnd=12400,
                OutputWorkspace='reduced')
   SetUB(Workspace='reduced',
         a=5.823,
         b=6.475,
         c=3.186,
         u='0,1,0',
         v='0,0,1')
   CropWorkspaceForMDNorm(InputWorkspace='reduced',
                          XMin=-25,
                          XMax=49,
                          OutputWorkspace='reduced')
   ConvertToMD(InputWorkspace='reduced',
               QDimensions='Q3D',
               Q3DFrames='Q_sample',
               OutputWorkspace='md',
               MinValues='-11,-11,-11,-25',
               MaxValues='11,11,11,49')
   MergeMD(InputWorkspaces='md', OutputWorkspace='merged')
   MDNorm(InputWorkspace='merged',
          Dimension0Name='QDimension1',
          Dimension0Binning='-5,0.05,5',
          Dimension1Name='QDimension2',
          Dimension1Binning='-5,0.05,5',
          Dimension2Name='DeltaE',
          Dimension2Binning='-2,2',
          Dimension3Name='QDimension0',
          Dimension3Binning='-0.5,0.5',
          SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
          OutputWorkspace='result',
          OutputDataWorkspace='dataMD',
          OutputNormalizationWorkspace='normMD')

and the corresponding output:

.. figure:: /images/MDNorm_inelastic_sym.png

.. categories::

.. sourcelink::

