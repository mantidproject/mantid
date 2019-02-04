.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs powder diffraction data reduction for WAND²
with calibration, monitor normalization and background subtraction.
The CalibrationWorkspace will most likely be a vanadium and will
correct for the detector sensitivity. The data can be normalized by
monitor count or time.  The output workspace can be saved to various
formats with :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>`.

It is recommenced to load WAND data with :ref:`LoadWAND
<algm-LoadWAND>` as the wavelength/energy will be set correctly and
monitor counts correctly taken into account for normalization. This
algorithm will work on data loaded with :ref:`LoadEventNexus
<algm-LoadEventNexus>` or the grouped output from :ref:`FilterEvents
<algm-FilterEvents>` but you will need to specify `EFixed` if
converting to anything except `Theta`.


MaskAngle
#########

The MaskAngle option will mask any out-of-plane (phi angle) detector
larger than this value using :ref:`MaskAngle <algm-MaskAngle>`. This
can help with peak sharpness by removing regions where there is large
peak broadening due to the divergent beam. The example below using
MaskAngle of 10.

.. figure:: /images/WANDPowderReduction_MaskAngle.png

Usage
-----

**Silicon powder**

.. code-block:: python

   silicon =LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5')
   vanadium=LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26509.nxs.h5')

   WANDPowderReduction(InputWorkspace=silicon,
                       CalibrationWorkspace=vanadium,
                       Target='Theta',
                       NumberBins=1000,
                       OutputWorkspace='silicon_powder')

.. figure:: /images/WANDPowderReduction_silicon_powder.png

**Silicon powder to Q over limited range**

.. code-block:: python

   silicon =LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5')
   vanadium=LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26509.nxs.h5')

   WANDPowderReduction(InputWorkspace=silicon,
                       CalibrationWorkspace=vanadium,
                       Target='ElasticQ',
                       XMin=4.5,
                       Xmax=6.25,
                       NumberBins=500,
                       OutputWorkspace='silicon_powder_q')

.. figure:: /images/WANDPowderReduction_silicon_powder_q.png

**Silicon powder to D spacing**

.. code-block:: python

   silicon2=LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26507.nxs.h5')
   vanadium=LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26509.nxs.h5')

   WANDPowderReduction(InputWorkspace=silicon2,
                       CalibrationWorkspace=vanadium,
                       Target='ElasticDSpacing',
                       NumberBins=1000,
                       OutputWorkspace='silicon_powder_d_spacing')

.. figure:: /images/WANDPowderReduction_silicon_powder_d.png

**Background subtraction**

.. code-block:: python

   silicon =LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5')
   vanadium=LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26509.nxs.h5')

   # Create fake flat background, constant 10
   bkg=CreateWorkspace(DataX=[1.487,1.489]*silicon.getNumberHistograms(),
                       DataY=[10]*silicon.getNumberHistograms(),
                       NSpec=silicon.getNumberHistograms(),
                       UnitX="Wavelength",ParentWorkspace=silicon)

   WANDPowderReduction(InputWorkspace=silicon,
                       CalibrationWorkspace=vanadium,
                       BackgroundWorkspace=bkg,
                       Target='Theta',
                       NumberBins=1000,
                       OutputWorkspace='silicon_powder_background')

   # Scale background by 50%
   WANDPowderReduction(InputWorkspace=silicon,
                       CalibrationWorkspace=vanadium,
                       BackgroundWorkspace=bkg,
                       BackgroundScale=0.5,
                       Target='Theta',
                       NumberBins=1000,
                       OutputWorkspace='silicon_powder_background_0.5')

.. figure:: /images/WANDPowderReduction_silicon_powder_bkg.png

.. categories::

.. sourcelink::
