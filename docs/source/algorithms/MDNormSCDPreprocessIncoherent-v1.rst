.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm preprocesses an incoherent scattering sample (usually
Vanadium) for use with :ref:`MDNormSCD <algm-MDNormSCD>`. The input
filename follows the syntax from :py:obj:`MultipleFileProperty
<mantid.api.MultipleFileProperty>`. The resulting workspaces can be
saved with :ref:`SaveNexus <algm-SaveNexus>`.

Usage
-----

**Corelli example using multiple files with masking**

.. code-block:: python

   # Create a simple mask file
   MaskFilename=mantid.config.getString("defaultsave.directory")+"MDNormSCDPreprocessIncoherent_mask.xml"
   LoadEmptyInstrument(InstrumentName='CORELLI', OutputWorkspace='CORELLI')
   # Missing banks
   MaskBTP('CORELLI', Bank='1-6,29,30,62,63-68,91')
   # End of tubes
   MaskBTP('CORELLI', Pixel='1-15,242-256')
   SaveMask('CORELLI', MaskFilename)

   SA, Flux = MDNormSCDPreprocessIncoherent(Filename='CORELLI_28119-28123',
                                            MomentumMin=2.5,
                                            MomentumMax=10,
                                            MaskFile=MaskFilename)

Solid Angle instrument view:

.. figure:: /images/MDNormSCDPreprocessIncoherent_SA.png

Flux spectrum plot:

.. figure:: /images/MDNormSCDPreprocessIncoherent_Flux.png

**TOPAZ example with background subtraction, grouping, masking and Spherical Absorption Correction**

.. code-block:: python

   SA, Flux = MDNormSCDPreprocessIncoherent(Filename='TOPAZ_15670',
                                            Background='TOPAZ_15671',
                                            FilterByTofMin=500,
                                            FilterByTofMax=16000,
                                            MomentumMin=1.8,
                                            MomentumMax=12.5,
                                            GroupingFile='/SNS/TOPAZ/IPTS-15376/shared/calibration/TOPAZ_grouping_2016A.xml',
                                            DetCal='/SNS/TOPAZ/IPTS-15376/shared/calibration/TOPAZ_2016A.DetCal',
                                            MaskFile='/SNS/TOPAZ/IPTS-15376/shared/calibration/TOPAZ_masking_2016A.xml',
                                            SphericalAbsorptionCorrection=True,
                                            LinearScatteringCoef=0.367,
                                            LinearAbsorptionCoef=0.366,
                                            Radius=0.2)
   print(SA.getNumberHistograms())
   print(SA.blocksize())
   print(Flux.getNumberHistograms())
   print(Flux.blocksize())

Output:

.. code-block:: none

   1507328
   1
   23
   10000


Related Algorithms
------------------

:ref:`MDNormSCD <algm-MDNormSCD>` uses the output of this algorithm

.. categories::

.. sourcelink::
