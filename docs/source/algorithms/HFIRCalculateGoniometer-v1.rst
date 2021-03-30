.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the goniometer of peaks from the q_sample
and wavelength. It makes use of the
:meth:`mantid.geometry.Goniometer.calcFromQSampleAndWavelength`
method.

The wavelength will be read from the workspace property `wavelength`
unless one is provided as input.

The goniometer is calculated by the following method. It only works
for a constant wavelength source. It also assumes the goniometer
rotation is around the y-axis only.

You can specify if the goniometer to calculated is omega or phi, outer
or inner respectively, by setting the option InnerGoniometer. The
existing goniometer on the peak will then be taken into account
correctly.

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
detector position is on the left of the beam even it it's not, unless
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

.. categories::

.. sourcelink::
