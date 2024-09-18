.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Developed for CORELLI but should work on any instrument. This workflow
algorithm loops over a series of runs combining them with correct
normalisation, subtract the background and apply symmetry. The
resulting workspace is a :ref:`MDHistoWorkspace <MDHistoWorkspace>`
containing a volume of scattering.

The input filename follows the syntax from
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`

This workflow makes use of :ref:`ConvertToMD <algm-ConvertToMD>` and
:ref:`MDNorm <algm-MDNorm>` so these should be reviewed to better
understand all the options. An example of creating the Solid Angle and
Flux workspaces are included in :ref:`MDNormSCD
<algm-MDNormSCD>`. :ref:`MDNormSCDPreprocessIncoherent
<algm-MDNormSCDPreprocessIncoherent>` can be used to process Vanadium
data for the Solid Angle and Flux workspaces.

The resulting workspaces can be saved and loaded with :ref:`SaveMD
<algm-SaveMD>` and :ref:`LoadMD <algm-LoadMD>` respectively.

Masking
#######

The mask from the solid angle workspace is copied to the
data. Additional masking is provided by a masking file. A masking file
can be created my masking a data file then saving it using
:ref:`SaveMask <algm-SaveMask>`.

Background
##########

The background is processed the same as the data except that the
Goniometer is copied from the data before setting the :ref:`UB matrix
<Lattice>`. If a background is included three workspaces are
create. If "OutputWorkspace" is set to "ws" you will get the
following.

"ws_normalizedBackground" containing the normalised background.

"ws_normalizedData" containing the normalised data.

And "ws" where

.. math:: ws = ws\_normalizedData - ws\_normalizedBackground * BackgroundScale

Should the background scale not be correct this allows you to redo the
background subtraction without rerunning the reduction.

If no background is used then the "ws" is just the normalised data.

Symmetries
##########

The SymmetryOperations parameters can either be defined as a
:ref:`point or space group <Point and space groups>` were all the
symmetries for that group is applied, or you can specify individual
:ref:`symmetries <Symmetry groups>` to apply.

For example setting SymmetryOperations to space group "P 31 2 1",
point group "321" or "x,y,z; -y,x-y,z; -x+y,-x,z; y,x,-z; x-y,-y,-z;
-x,-x+y,-z" are equivalent. Note these symmetries are on the real
lattice not reciprocal.

Temporary Workspaces
####################

If the KeepTemporaryWorkspaces option is True the data and the
normalization in addition to the nomalized data will be
outputted. This allows you to run separate instances of
SingleCrystalDiffuseReduction and combine the results. They will have
names "ws_data" and "ws_normalization"
respectively.

Where

.. math:: ws\_normalizedData = \frac{ws\_data}{ws\_normalization}

If background is subtracted there will be similar
"ws_background_data" and
"ws_background_normalization" for the background.

Where

.. math:: ws\_normalizedBackground = \frac{ws\_background\_data}{ws\_backgournd\_normalization}

Workflow
--------

.. diagram:: SingleCrystalDiffuseReduction-v1.dot


Usage
-----

**Single file**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782',
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,0.1,10.05',
                                 Dimension1Binning='-10.05,0.1,10.05',
                                 Dimension2Binning='-0.1,0.1')

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_single.png

**Multiple files**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782:29817:10',
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,0.1,10.05',
                                 Dimension1Binning='-10.05,0.1,10.05',
                                 Dimension2Binning='-0.1,0.1')

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_multiple.png

**Single file with symmetry**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782',
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,0.1,10.05',
                                 Dimension1Binning='-10.05,0.1,10.05',
                                 Dimension2Binning='-0.1,0.1',
                                 SymmetryOperations="P 31 2 1")

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_single_sym.png

**Multiple files with symmetry**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782:29817:10',
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,0.1,10.05',
                                 Dimension1Binning='-10.05,0.1,10.05',
                                 Dimension2Binning='-0.1,0.1',
                                 SymmetryOperations="P 31 2 1")


.. figure:: /images/SingleCrystalDiffuseReduction_corelli_multiple_sym.png

**Multiple files with symmetry and background substraction**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782:29817:10',
                                 Background='CORELLI_28124',
                                 BackgroundScale=0.95,
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,0.1,10.05',
                                 Dimension1Binning='-10.05,0.1,10.05',
                                 Dimension2Binning='-0.1,0.1',
                                 SymmetryOperations="P 31 2 1")

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_multiple_sym_bkg.png

**Reading in elastic Corelli autoreduced data**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename=','.join('/SNS/CORELLI/IPTS-15526/shared/autoreduce/CORELLI_'+str(run)+'_elastic.nxs' for run in range(29782,29818,10)),
                                 Background='/SNS/CORELLI/IPTS-15796/shared/autoreduce/CORELLI_28124_elastic.nxs',
                                 BackgroundScale=0.95,
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 Dimension0Binning='-10.05,10.05,0.1',
                                 Dimension1Binning='-10.05,10.05,0.1',
                                 Dimension2Binning='-0.1,0.1',
                                 SymmetryOperations="P 31 2 1")

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_multiple_sym_bkg_elastic.png

**Defining the axis to be [H,H,0], [H,-H,0], [0,0,L]**

.. code-block:: python

   SingleCrystalDiffuseReduction(Filename='CORELLI_29782:29817:10',
                                 Background='CORELLI_28124',
                                 BackgroundScale=0.95,
                                 SolidAngle='/SNS/CORELLI/shared/Vanadium/2016B/SolidAngle20160720NoCC.nxs',
                                 Flux='/SNS/CORELLI/shared/Vanadium/2016B/Spectrum20160720NoCC.nxs',
                                 UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                 OutputWorkspace='output',
                                 SetGoniometer=True,
                                 Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                 QDimension0='1,1,0',
                                 QDimension1='1,-1,0',
                                 QDimension2='0,0,1',
                                 Dimension0Binning='-7.5375,0.075,7.5375',
                                 Dimension1Binning='-13.165625,0.13100125,13.165625',
                                 Dimension2Binning='-0.1,0.1',
                                 SymmetryOperations="P 31 2 1")

   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   c = ax.pcolormesh(mtd['output'],vmin=0, vmax=1e-5)
   fig.colorbar(c)
   fig.show()

.. figure:: /images/SingleCrystalDiffuseReduction_corelli_multiple_sym_bkg_HH0.png

Related Algorithms
------------------

:ref:`MDNormSCD <algm-MDNormSCD>` is the algorithm performing the normalisation of a single file.

:ref:`DeltaPDF3D <algm-DeltaPDF3D>` calculates the 3D-ΔPDF from the resulting workspace of this algorithm.

.. categories::

.. sourcelink::
