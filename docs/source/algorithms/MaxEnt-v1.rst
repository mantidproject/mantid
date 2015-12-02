
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The maximum entropy method (MEM) is used as a signal processing technique for reconstructing
images from noisy data. Is often regarded as the only consistent way of selecting a single
image from the many images which fit the data with the same value of the goodness-of-fit statistic,
:math:`\chi^2`. The maximum entropy method selects from this *feasible set* of images, the one which
has minimum information, so that there must be enough evidence in the data from any observed structure.
In this way, the algorithm maximizes the entropy :math:`S\left(\rho\right)` subject to the constraint:

.. math:: \chi^2 = \sum_m \frac{\left(d_m - d_m^c\right)^2}{\sigma_m^2} \leq C_{target}

where :math:`d_m` are the experimental data, :math:`\sigma_m` the associated errors, and :math:`d_m^c`
the calculated or reconstructed data. The image can be regarded a set of numbers
:math:`\{\rho_0, \rho_1, \dots, \rho_N\}` related to the measured data as:

.. math:: d_m = \sum_j A_{mj} \rho_j

where the measurement kernel matrix :math:`\mathbf{A}` represents a Fourier transform,
:math:`A_{mj} = \exp\left(-ik_mj\right)`. At present, nothing is assumed about :math:`\rho_j`:
it can be either positive or negative, and real or complex, and the entropy is thus defined as

.. math:: S = \sum_j \left(\rho_j/b\right) \sinh^{-1} \left(\rho_j/b\right)

where :math:`b` is a constant background above which the image is significant.

The current implementation follows the approach by Skilling & Bryan, in which the entropy is maximized
subject to the constraint in :math:`\chi^2` without using explicitly a Lagrange multiplier. Instead, they
construct a subspace from a set of *search directions* to approach the maximum entropy solution. Initially,
the image :math:`\rho` is set to the flat background math:`b` and the search directions are constructed
using the gradients of :math:`S` and :math:`\chi^2`:

.. math:: \mathbf{e}_1 = f\left(\nabla S\right)
.. math:: \mathbf{e}_2 = f\left(\nabla \chi^2\right)

where :math:`f\left(\nabla S\right)` stands for a componentwise multiplication. The algorithm next uses
a quadratic approximation to determine the increment :math:`\delta \mathbf{\rho}` that *moves* the image
one step closer to the solution:

.. math:: \mathbf{\rho} = \mathbf{\rho} + \delta \mathbf{\rho}

Output Workspaces
-----------------

There are four output workspaces: *ReconstructedImage* and *ReconstructedData* contain the image and
calculated data respectively. The latter can be used as a test to check that the reconstructed data
approaches the experimental, measured data. The first one corresponds to its Fourier transform, and
so it containes twice the original number of spectra, as the Fourier transform will be a complex signal
in general. The real and imaginary parts are organized as follows: assuming your input workspace has
:math:`N` spectra, the real part of the reconstructed image for spectrum :math:`i` corresponds to
spectrum :math:`i` in *ReconstructedImage*, while the imaginary part can be found in spectrum :math:`N+i`.

At present, the algorithm runs until a solution was found. An image is considered to be a maximum entropy
solution when the following two conditions are met simultaneously:

.. math:: \chi^2_{Target} - \chi^2 < \epsilon_1, \qquad \frac{1}{2} \left| \frac{\nabla S}{\left|\nabla S\right|} - \frac{\nabla \chi^2}{\left|\nabla \chi^2\right|} \right| < \epsilon_2

While one of this conditions is not satisfied the algorithm keeps running until it reaches the maximum
number of iterations. When the maximum number of iteration is reached, the algorithm returns the last
reconstructed image and its corresponding calculated data. The user must ensure that the result is a
true solution by checking manually that the above conditions are satisfied. This can be done by inspecting
the output workspaces *EvolChi* and *EvolAngle*. They record the evolution of :math:`\chi^2` and the
angle between :math:`\nabla S` and :math:`\nabla \chi^2` at each iteration, and are set to zero when
a solution was found. This means that, if in these output workspaces the last value is zero, an image
satisfying the above conditions was found.

Usage
-----

**Example - Reconstruct a cosine function**

.. testcode:: MaxEntCosine

   from math import pi, cos

   # Create a workspace
   X = []
   Y = []
   E = []
   N = 50
   w = 1.6

   for i in range(0,N):
       x = 2*pi*i/N
       X.append(x)
       Y.append(cos(w*2*pi*i/N))
       E.append(0.1)

   ws = CreateWorkspace(DataX=X, DataY=Y, DataE=E)
   evolChi, evolAngle, image, data = MaxEnt(InputWorkspace='ws', Background=0.01, ChiTarget=50)

   print "Original data %.4f" % (ws.readY(0)[25])
   print "Reconstructed data %.4f" % (data.readY(0)[25])

Output:

.. testoutput:: MaxEntCosine

  Original data 0.3090
  Reconstructed data 0.3112

.. categories::

.. sourcelink::

