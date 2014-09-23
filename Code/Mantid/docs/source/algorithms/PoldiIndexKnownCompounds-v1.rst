
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

In many experiments at POLDI it is well known which compounds are present in a sample, as well as their unit cells. For some algorithms it is necessary to index the peaks found in the correlation spectrum obtained by :ref:`algm-PoldiAutoCorrelation`. This algorithm tries to index those peaks using lists of calculated reflections of one or more compounds (created with :ref:`alg-PoldiCreatePeaksFromCell`), allowing for a certain tolerance in peak positions.

The major difficulty arises from the fact that typical POLDI experiments involve lattice deformations, which causes a shift in peak positions. For a single phase material with few peaks, this can be handled by making the tolerance larger - but only to a certain degree. If cell parameters are too large and consequently lattice plane spacings at low d-values too narrow, the indexing becomes ambiguous. Of course, introducing additional phases makes this situation even worse, leading to even more ambiguity.

To resolve these ambiguities, this algorithm assumes that a peak must occur somewhere in the vicinity of the theoretical position. As the distance between measured and calculated position grows, it becomes less and less likely that the measured peak corresponds to the theoretical one. That behavior is modeled by assuming a Gaussian distribution around the position :math:`d_0`of the theoretical peak:

.. math::
    f(d) = \frac{A}{\sigma\sqrt(2\pi)}\cdot\exp\left[-\frac{1}{2}\left(\frac{d - d_0}{\sigma}\right)^2\right]

This is illustrated by the plot in Figure 1. The standard deviation :math:`\sigma` of the distribution is a parameter that can be specified by the user, the larger it is, the larger becomes the interval in which a reflection may be observed.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - PoldiIndexKnownCompounds**

.. testcode:: PoldiIndexKnownCompoundsExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = PoldiIndexKnownCompounds()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: PoldiIndexKnownCompoundsExample

  The output workspace has ?? spectra

.. categories::

