
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

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - MaxEnt**

.. testcode:: MaxEntExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = MaxEnt()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: MaxEntExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

