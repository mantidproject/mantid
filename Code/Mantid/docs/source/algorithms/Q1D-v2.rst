.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Q Unit Conversion
#################

The equation for :math:`Q` as function of wavelength, :math:`\lambda`,
and neglecting gravity, is

.. math:: Q = \frac{4\pi}{\lambda} sin(\theta)

where :math:`2 \theta` is the particle's angle of deflection. If a
particle's measured deflection over the sample to the detector (pixel)
distance, :math:`L_2`, is :math:`x` along the x-axis and :math:`y` along
the y-axis then :math:`\theta` is

.. math:: \theta = \frac{1}{2} arcsin\left (\frac{\sqrt{x^2+y^2}}{L_2} \right )

Including gravity adds another term to this equation which becomes:

.. math:: \theta = \frac{1}{2} arcsin\left (\frac{ \sqrt{x^2+\left (y+\frac{gm^2}{2h^2} \lambda^2 L_2^2 \right)^2}}{L_2} \right )

where :math:`m` is the particle's mass, :math:`g` is the acceleration
due to gravity and :math:`h` is `plank's
constant <http://en.wikipedia.org/wiki/Planks_constant>`__ (this assumes
neutrons are all travelling in horizontal at sample, and that
:math:`x=y=0` would be beam centre at :math:`\lambda = 0`).

Normalized Intensity
####################

This :ref:`algorithm <Algorithm>` takes a workspace of number of neutron
counts against `wavelength <http://www.mantidproject.org/Units>`_ and creates a workspace of cross
section against Q. The output Q bins boundaries are defined by setting
the property OutputBinning.

Below is the formula used to calculate the cross section,
:math:`P_I(Q)`, for one bin in the output workspace whose bin number is
denoted by I, when the input workspace has just one detector. Each bin
is calculated from the sum of all input wavelength bins, n, that
evaluate to the same Q using the formula for Q at the top of this page.
In equations this relationship between the input bins and the output
bins is represented by :math:`n \supset I` and an example of a set of
two bins is shown diagrammatically below.  (Each Q bin contains the sum
of many, one, or no wavelength bins.)

.. figure:: /images/Wav_Q_bins.png
   :alt: DgsAbsoluteUnitsReductionWorkflow.png

In the equation the number of counts in the input spectrum number is
denoted by :math:`S(n)`, :math:`N(n)` is the wavelength dependent
correction and :math:`\Omega` is the `solid angle <http://www.mantidproject.org/SolidAngle>`_ of the
detector

.. math:: P_I(Q) = \frac{ \sum_{n \supset I} S(n)}{\Omega\sum_{n \supset I}N(n)}

The wavelength dependent correction is supplied to the algorithm through
the WavelengthAdj property and this workspace must have the same
wavelength binning as the input workspace and should be equal to the
following:

.. math:: N(n) = M(n)\eta(n)T(n)

where :math:`M`, :math:`\eta` and :math:`T` are the monitor counts,
detector efficiency and transmission fraction respectively.

Normally there will be many spectra each from a different pixel with a
row number :math:`i` and column number :math:`j`. Because the value of
:math:`\theta` varies between pixels corresponding input bins (n) from
different input spectra can contribute to different output bins (I) i.e.
:math:`n \supset I` will be different for different pixels. For multiple
spectra the sum for each output bin will be over the set of input bins
in each pixel that have the correct Q, that is
:math:`\{i, j, n\} \supset \{I\}` while :math:`\Omega_{i j}` is detector
dependent:

.. math:: P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}}

where :math:`F` is the detector dependent (e.g. flood) scaling specified
by the PixelAdj property, and where a :math:`\lambda` bin :math:`n`
spans more than one :math:`Q` bin :math:`I`, it is split assuming a
uniform distribution of the counts in :math:`\lambda`. The normalization
takes any `bin masking <http://www.mantidproject.org/MaskBins>`_ into account.

Although the units on the y-axis of the output workspace space are
quoted in 1/cm note that conversion to a cross section requires scaling
by an :ref:`instrument <instrument>` dependent absolute units constant.

Resolution and Cutoffs
######################

There are two sources of uncertainty in the intensity: the statistical
(counting) error and the finite size of the bins, i.e. both time bins
and the spatial extent of the detectors (pixels). The first error is
reducible by increasing the length of the experiment or bin sizes while
the second reduces with smaller bin sizes. The first is represented by
the errors on the output workspace but the second is not included in the
error calculation although it increases uncertainties and degrades the
effective resolution of the data none the less. This algorithm allows
the resolution to be improved by removing the bins with the worst
resolution.

Normally the bins that give the worst resolution are those near the beam
center and with short wavelengths. When the optional properties
:math:`RadiusCut` and :math:`WaveCut` are set bins from this region of
the input workspace are removed from the intensity calculation (both
from the numerator and denominator). For a pixel at distance R from the
beam center the wavelength cutoff, :math:`W_{low}`, is defined by the
input properties :math:`RadiusCut` and :math:`WaveCut` as:

.. math:: W_{low} = \frac{WaveCut (RadiusCut-R)}{RadiusCut}

The bin that contains the wavelength :math:`W_{low}` and all lower
indices are excluded from the summations for that detector pixel.

From the equation it is possible to see that for pixels in
:math:`R > RadiusCut` all (positive) wavelengths are included. Also
substituting :math:`WaveCut = W_{low}` we have that :math:`R = 0` and
hence all detectors contribute at wavelengths above :math:`WaveCut`.

*Practically, it is more likely to be necessary to implement
:math:`RadiusCut` and :math:`WaveCut` in situations where the scattering
near to the beamstop is weak and 'contaminated' by short wavelength
scatter. This might arise, for example, when running at long
sample-detector distances, or at short sample-detector distances with
large diameter beams, or where the sample generates Bragg peaks at
low-Q. The best recourse is to check the wavelength overlap. If it is
not too bad it may be possible to improve the data presentation simply
by altering :math:`Q{min}` and the binning scheme.*

**References**

`R.P. Hjelm Jr. *J. Appl. Cryst.* (1988), 21,
618-628 <http://scripts.iucr.org/cgi-bin/paper?gk0158>`__.

`P.A. Seeger & R.P. Hjelm Jr. *J. Appl. Cryst.* (1991), 24,
467-478 <http://scripts.iucr.org/cgi-bin/paper?gk0573>`__.

Variations on applying the normalization
########################################

It is possible to divide the input workspace by the WavelenghAdj and
PixelAdj workspaces prior to calling this algorithm. The results will be
same as if these workspaces were passed to Q1D instead when there are
high numbers of particle counts. However, in this scheme the
probabilities tend to converge on the true high count probabablities
more slowly with increasing number of counts and so the result is less
accuate.

Depending on the input and output bins there could be a significant
difference in CPU time required by these two methods.

References
##########

Calculation of Q is from Seeger, P. A. and Hjelm, R. P. Jr, "Small-Angle
Neutron Scattering at Pulsed Spallation Sources" (1991) J. Appl **24**
467-478

Previous Versions
-----------------

Version 1
#########

Before July 2011 the intensity was calculated with an equation like the
following:

.. math:: P_I(Q) = \frac{ \sum_{\{i, j, n\} \supset \{I\}}G(i,j,n) }{ \sum_{\{i, j, n\} \supset \{I\}} \Omega_{i j} }

where G is the input workspace normally related to the raw counts
workspace as:

.. math:: G(i,j,n) = S(i,j,n)/(M(n)\eta(n)T(n)F_{i j})

That is the normalization was performed before the Q calculation which
gives the same probilities at high numbers of particles counts but
weighted noisy, low count data too highly, giving more noise in
:math:`P_I(Q)`.

The error was calculation did not include the errors due the
normalization or any corrections.

Properties
##########

+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Order   | Name                | Direction   | Type              | Default     | Description                                                                                                                                               |
+=========+=====================+=============+===================+=============+===========================================================================================================================================================+
| 1       | InputWorkspace      | Input       | MatrixWorkspace   | Mandatory   | The (partly) corrected data in units of wavelength.                                                                                                       |
+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| 2       | InputForErrors      | Input       | MatrixWorkspace   | Mandatory   | The workspace containing the counts to use for the error calculation. Must also be in units of wavelength and have matching bins to the InputWorkspace.   |
+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| 3       | OutputWorkspace     | Output      | MatrixWorkspace   | Mandatory   | The workspace name under which to store the result histogram.                                                                                             |
+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| 4       | OutputBinning       | Input       | String            | Mandatory   | The bin parameters to use for the final result (in the format used by the :ref:`algm-Rebin` algorithm).                                                   |
+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| 5       | AccountForGravity   | Input       | Boolean           | False       | Whether to correct for the effects of gravity.                                                                                                            |
+---------+---------------------+-------------+-------------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+

.. categories::
