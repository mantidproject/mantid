
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is applying a polarization correction for single crystal
inelastic experiments. If one measures scattering intensity with polarization
along the momentum transfer  :math:`Q`, perpendicular to it in the horizontal
plane, and in the vertical direction, one can write the spin incoherent
contribution as:

.. math::

    I_{SI}=\frac{3}{2}\left(\Sigma_x^{nsf}-\Sigma_y^{nsf}+\Sigma_z^{sf}\right)

where the *sf* and *nsf* subscripts stand for spin flip and non-spin flip.
The *x* direction is parallel to :math:`Q`, *y* is perpendicular in the horizontal
plane, while *z* is vertically up. **NOTE**: this is not the Mantid convention.

However, for instruments with multiple detectors and using time of flight
neutrons, one has a constantly varying angle between :math:`Q` and polarization.
If we call this angle  :math:`\alpha` (Scharpf angle), the above equation can
be written as:

.. math::

    I_{SI}=\frac{3}{2}\left(\frac{\Sigma_{x'}^{nsf}-\Sigma_{y'}^{nsf}}{\cos^2\alpha-\sin^2\alpha}\right)+\frac{3}{2}\Sigma_z^{sf}

This algorithm calculates the Scharpf angle for every event or energy transfer bin,
then divides the intensity by :math:`F=\cos^2\alpha-\sin^2\alpha=\cos(2\alpha)`.
In places where *F* is less than the `Precision`, the intensity of the output workspace is set to 0.

For a detector at angle :math:`\theta` in the horizontal plane, the angle
between :math:`Q` and :math:`k_i` is given by

.. math::

    \gamma=\arctan2\left(-\frac{k_f}{k_i}\sin\theta, 1-\frac{k_f}{k_i}\cos\theta\right)

The Scharpf angle is then:

.. math::
    \alpha = \gamma- PolarizationAngle

.. Note::

    This algorithm assumes that all scattering is in the horizontal plane (good enough approximation
    for Hyspec instrument, in polarized mode).

For more information, see

#. Werner Schweika - *XYZ-polarisation analysis of diffuse magnetic neutron scattering from single crystals*, Journal of Physics: Conference Series, **211**,012026, (2010) doi: `10.1088/1742-6596/211/1/012026 <http://dx.doi.org/10.1088/1742-6596/211/1/012026>`_



Usage
-----

**Example - HyspecScharpfCorrection**

.. testcode:: HyspecScharpfCorrectionExample

   # Create a workspace (NXSPE equivalent)
   w = CreateSampleWorkspace(Function='Flat background', NumBanks=1,
                             BankPixelWidth=1, XUnit='DeltaE',
                             XMin=-10.25, XMax=20, BinWidth=0.5)
   MoveInstrumentComponent(Workspace=w, ComponentName='bank1', X=3, Z=3, RelativePosition=False)
   AddSampleLog(Workspace=w,LogName='Ei', LogText='17.1', LogType='Number')

   wsOut = HyspecScharpfCorrection(InputWorkspace=w,
                                   PolarizationAngle=-10,
                                   Precision=0.2)

   # Get the data
   intensity = wsOut.readY(0)
   bin_boundaries = wsOut.readX(0)
   energy_transfer = 0.5*(bin_boundaries[1:]+bin_boundaries[:-1])
   # at DeltaE=5meV, Q makes an angle of 55.7 degrees with incident beam
   # If polarization angle is -10 degrees, the intensity should be 0
   # Below this energy, the Scharpf angle correction is negative, above
   # is positive. If energy transfer is greater than Ei, intensity is
   # set to 0
   print('Intensity at DeltaE= 0meV: {0:.2f}'.format((intensity[energy_transfer==0])[0]))
   print('Intensity at DeltaE= 5meV: {0:.2f}'.format((intensity[energy_transfer==5])[0]))
   print('Intensity at DeltaE=10meV: {0:.2f}'.format((intensity[energy_transfer==10])[0]))
   print('Intensity at DeltaE=19meV: {0:.2f}'.format((intensity[energy_transfer==19])[0]))


Output:

.. testoutput:: HyspecScharpfCorrectionExample

  Intensity at DeltaE= 0meV: -2.37
  Intensity at DeltaE= 5meV: 0.00
  Intensity at DeltaE=10meV: 1.99
  Intensity at DeltaE=19meV: 0.00

.. categories::

.. sourcelink::
