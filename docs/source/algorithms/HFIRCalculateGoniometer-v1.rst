.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the goniometer of peaks from the q_sample
and wavelength. It makes use of the
:meth:`mantid.geometry.Goniometer.calcFromQSampleAndWavelength`
method. It was created to work with WAND² (HB2C) and DEMAND (HB3A) at
HFIR but may be applied to other constant wavelength instruments.

The wavelength will be read from the workspace property `wavelength`
unless one is provided as input. The peaks with have their wavelength
set to this value.

The goniometer is calculated by the following method. It only works
for a constant wavelength source. It also assumes the goniometer
rotation is around the y-axis only.

You can specify if the goniometer to calculated is omega or phi, outer
or inner respectively, by setting the option InnerGoniometer. The
existing goniometer on the workspace will be used as the starting
goniometer allowing only the scanning goniometer to be
calculated. This won't change anything for HB2C as they only have an
omega axis but for HB3A you need to make sure that the goniometer is
correctly set from the logs with `SetGoniometer(Workspace=scan,
Axis0='omega,0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2='phi,0,1,0,-1')`
and that you can only process peak workspaces containing peaks from a
single scan at a time.

The goniometer (:math:`G`) is calculated from
:math:`\textbf{Q}_{sample}` for a given wavelength (:math:`\lambda`)
by:

First calculate the :math:`\textbf{Q}_{lab}` using
:math:`\textbf{Q}_{sample}` and :math:`\lambda`.

.. math:: k = \frac{2 \pi}{\lambda}

.. math:: G \textbf{Q}_{sample} = \textbf{Q}_{lab} = \left(\begin{array}{c}
          -k\sin(\theta)\cos(\phi) \\
          -k\sin(\theta)\sin(\phi) \\
          k (1-\cos(\theta))
          \end{array}\right) (1)

.. math:: |\textbf{Q}_{sample}|^2 = |\textbf{Q}_{lab}|^2 = 2 k^2 (1-\cos(\theta))

:math:`\therefore`

.. math:: \theta = \cos^{-1}(1-\frac{|\textbf{Q}_{sample}|^2}{2k^2})

.. math:: \phi = \sin^{-1}(-\frac{\textbf{Q}_{sample}^y}{k \sin(\theta)})

where :math:`\theta` is from 0 to :math:`\pi` and :math:`\phi` is from
:math:`-\pi/2` to :math:`\pi/2`. This means that it will assume your
detector position is on the left of the beam even if it's not, unless
the option FlipX is set to True then the :math:`\textbf{Q}_{lab}^x = -\textbf{Q}_{lab}^x`.

Now you have :math:`\theta`, :math:`\phi` and k you can get :math:`\textbf{Q}_{lab}` using (1).

We need to now solve :math:`G \textbf{Q}_{sample} =
\textbf{Q}_{lab}`. For a rotation around y-axis only we want to find
:math:`\psi` for:

.. math:: G = \begin{bmatrix}
	  \cos(\psi)  & 0 & \sin(\psi) \\
	  0           & 1 & 0 \\
	  -\sin(\psi) & 0 & \cos(\psi)
	  \end{bmatrix} (2)

which gives two equations

.. math:: \cos(\psi)\textbf{Q}_{sample}^x+\sin(\psi)\textbf{Q}_{sample}^z = \textbf{Q}_{lab}^x
.. math:: -\sin(\psi)\textbf{Q}_{sample}^x+\cos(\psi)\textbf{Q}_{sample}^z = \textbf{Q}_{lab}^z

make

.. math:: A = \begin{bmatrix}
          \textbf{Q}_{sample}^x & \textbf{Q}_{sample}^z \\
          \textbf{Q}_{sample}^z & -\textbf{Q}_{sample}^x
          \end{bmatrix}

.. math:: B = \begin{bmatrix}
	  \textbf{Q}_{lab}^x \\
	  \textbf{Q}_{lab}^z
	  \end{bmatrix}

Then we need to solve :math:`A X = B` for :math:`X` where

.. math:: X = \begin{bmatrix}
              \cos{\psi} \\
              \sin{\psi}
              \end{bmatrix} = A^{-1} B

then

.. math:: \psi = \tan^{-1}\left(\frac{\sin{\psi}}{\cos{\psi}}\right)

Put :math:`\psi` into (2) and you have the goniometer for that peak.

.. note::

   If the instrument is HB3A and the `InnerGoniometer` and `FlipX`
   properties are left empty then these values will be set correctly
   automatically.

Usage
-----

**Example: omega calculation**

.. testsetup:: example

   from mantid.geometry import Goniometer

.. testcode:: example

    peaks = CreatePeaksWorkspace(OutputType="LeanElasticPeak", NumberOfPeaks=0)
    peaks.addPeak(peaks.createPeakQSample([0.5, 0, np.sqrt(3)/2]))   # omega = -90
    peaks.addPeak(peaks.createPeakQSample([0, 0, 1]))                # omega = -60
    peaks.addPeak(peaks.createPeakQSample([-0.5, 0, np.sqrt(3)/2]))  # omega = -30
    peaks.addPeak(peaks.createPeakQSample([-np.sqrt(3)/2, 0, 0.5]))  # omega = 0
    peaks.addPeak(peaks.createPeakQSample([-1, 0, 0]))               # omega = 30
    peaks.addPeak(peaks.createPeakQSample([-np.sqrt(3)/2, 0, -0.5])) # omega = 60
    peaks.addPeak(peaks.createPeakQSample([-0.5, 0, -np.sqrt(3)/2])) # omega = 90
    HFIRCalculateGoniometer(peaks, 2*np.pi)
    for n in range(7):
        g = Goniometer()
        g.setR(peaks.getPeak(n).getGoniometerMatrix())
        print(f"Peak {n} - omega = {g.getEulerAngles('YZY')[0]:.1f}")

Output:

.. testoutput:: example

    Peak 0 - omega = -90.0
    Peak 1 - omega = -60.0
    Peak 2 - omega = -30.0
    Peak 3 - omega = 0.0
    Peak 4 - omega = 30.0
    Peak 5 - omega = 60.0
    Peak 6 - omega = 90.0

.. categories::

.. sourcelink::
