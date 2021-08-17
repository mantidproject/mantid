
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to mask detector pixels showing deficient or excessive neutron counts. The deficit and excess is
referenced to the median calculated over intensities in a single tube, an eight-pack, or a panel of eight-packs.
The prescription for the median calculation depends on the collimation level of the eigh-pack to which a
particular detector belongs to.

The full-collimated and half-collimated eight-packs are presented in picture below, respectively.
Those grey-colored tubes are full-collimated (in the first picture) or half-collimated (in the second picture).

.. figure:: /images/NOMADMedianDetectorTest_1.png
   :alt: ClusterImage.png

.. figure:: /images/NOMADMedianDetectorTest_2.png
   :alt: ClusterImage.png

The following workflow presents the prescription to mask a detector pixel according to the median taken
over the pixel intensity in a tube. For tubes in eight-packs with a Half-collimation level, the median
is taken over the first half of the tube to test the pixels in this half. Analogously, the median is
taken over the second half of the tube to test the pixels in the second half.

.. figure:: /images/NOMADMedianDetectorTest_3.png
   :alt: ClusterImage.png

In the inequalities below, :math:`I` is the pixel intensity, :math:`m` is the  median over the tube,
and :math:`f_{lp} < 1` and :math:`f_{up} > 1` are multiplicative factors to set the minimum and maximum
intensity thresholds.

Condition [2] :math:`I < f_{lp} \cdot m` or :math:`I > f_{up} \cdot m`

Condition [3] :math:`I < (1 + 3(f_{lp} - 1)) m` or :math:`I > (1 + 3(f_{up} - 1)) m`

In the inequalities below, :math:`m_1` and :math:`m_1` are the median taken over the first and second half
of the tube.

Condition [4] :math:`I < (1 + 3(f_{lp} - 1)) m_1` or :math:`I > (1 + 3(f_{up} - 1)) m_1`

Condition [5] :math:`I < (1 + 3(f_{lp} - 1)) m_2` or :math:`I > (1 + 3(f_{up} - 1)) m_2`

Pixels can also be masked if they fail the median test where the median is calculated from the intensities of
their corresponding eight-pack (if the eight-pack is fully collimated) or the median of their corresponding
panel (if the eight-pack is not fully collimated).

the following workflow prescribes the calculation of the median for each of the instrument panels (termed
as "banks" in the workflow picture)

.. figure:: /images/NOMADMedianDetectorTest_4.png
   :alt: ClusterImage.png

Finally, the following workflow prescribes the test using the eight-pack mediam or the panel median criterium

.. figure:: /images/NOMADMedianDetectorTest_5.png
   :alt: ClusterImage.png

Reference [6] The first four Panels (a.k.a Banks) are "non-flat", and the last two are "flat".

In the inequalities below, :math:`I` is the pixel intensity and :math:`m_B` is the  median over the Panel
(a.k.a "Bank"), :math:`m_{8P}` is the  median over the eight-pack. Multiplicative factors :math:`f_{lb} < 1` and
:math:`f_{ub} > 1` set the minimum and maximum intensity thresholds.

Condition [7]  :math:`I < f_{lB} \cdot m_B` or :math:`I > f_{pB} \cdot m_B`

Condition [8] :math:`I < f_{lB} \cdot m_{8P}` or :math:`I > f_{pB} \cdot m_{8P}`

Condition [9] :math:`I < 0.1 \cdot m_B`

Eight-Packs in use, their collimation level, as well as multiplicative factors  :math:`f_{lp}` ("low_pixel"),
:math:`f_{up}` ("high_pixel"), :math:`f_{lb}` ("low_tube"), and :math:`f_{ub}` ("high_tube") are specified
in a configuration file in YML format. Here's the relevant portion of one such file:

   #

   # This block specifies the threshold (relative to median integrated

   # intensity of either pixel or tube) for masking out pixels.

   #

   threshold:

   low_pixel: 0.9


   high_pixel: 1.2

   low_tube: 0.7

   high_tube: 1.3

   #

   # Indexes of eight-packs in use.

   #
   eight_packs: [3,7,8,9,10,11,19,20,26,28,30,34,38,39,40,41,44,45,46,47,48,49,50,54,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,89,90,93,94,95]

   #

   # This block specifies the full and half collimated eight-packs.

   # Notice: Indexes represent indexes of the "eight_packs" list

   #

   collimation:

   full_col: [1, 8, 16, 25, 27, 28, 29]

   half_col: [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46]

Usage
-----

.. code-block:: python

    LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
    NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                            ConfigurationFile='NOMAD_mask_gen_config.yml',
                            SolidAngleNorm=True,
                            OutputMaskXML='NOM_144974_mask.xml')
