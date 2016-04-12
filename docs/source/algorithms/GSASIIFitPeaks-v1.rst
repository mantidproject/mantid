.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is experimental and at the moemtn is being developed
   for a specific technique. It might be changed, renamed or even
   removed without a notification, should instrument scientists decide to do so.


Uses `GSAS-II <https://subversion.xray.aps.anl.gov/trac/pyGSAS>`_
(Toby & Von Dreele, 2013) as external software to fit peaks to a
powder / engineering diffraction pattern. Here the process of peak
fitting is in the sense of Rietveld fitting or Rietveld refinement.
This algorithm uses the peak fitting (or "Peaks List") functionality
included in the `powder calculation module
<https://subversion.xray.aps.anl.gov/pyGSAS/sphinxdocs/build/html/GSASIIpwd.html>`_
of GSAS-II. The use of this algorithm is very close to the examples
described in these two GSAS-II tutorials: `Getting started / Fitting
individual peaks & autoindexing
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/FitPeaks/Fit%20Peaks.htm>`_,
and `Rietveld fitting / CW Neutron Powder fit for Yttrium-Iron Garnet
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/CWNeutron/Neutron%20CW%20Powder%20Data.htm>`_

To run this algorithm GSAS-II must be installed and it must be
available for importing from the Mantid Python interpreter. This
algorithm requires a modified version of GSAS-II. Please contact the
developers for details.

The main inputs required by this algorithm are a instrument definition
parameter (in GSAS format, readable by GSAS-II), histogram data, and
various parameters for the fitting/refinement process.  The algorithm
only supports peaks with shape of type back-to-back exponential
convoluted with pseudo-voigt (BackToBackExponentialPV). It is possible
to enable the refinement of the different function parameters via
several properties (RefineAlpha, RefineSigma, etc.).

This algorithm produces an output table with as many rows as peaks
have been found. The columns of the table give the parameters fitted,
similarly to the information found in the "Peaks List" window of the
GSAS-II GUI. These results are printed in the log messages as well.

The algorithm also provides goodness-of-fit estimates in the outputs
"GoF" and "Rwp" (Toby 2008).

For fitting single peaks, one at a time, see also :ref:`EnggFitPeaks
<algm-EnggFitPeaks>`.

References:

Toby, B. H., & Von Dreele, R. B. (2013). "GSAS-II: the genesis of a
modern open-source all purpose crystallography software
package". Journal of Applied Crystallography, 46(2), 544-549.

Toby, B. H. (2008). "R factors in Rietveld analysis: How good is good
enough?". Powder Diffraction, 21(1), 67-70.

Usage
-----

**Example - Fit several peaks in a spectrum**

.. code-block:: python

   print 'example'

Output:

.. code-block:: none

    example

.. categories::

.. sourcelink::
