.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to calculate the asymmetry for a muon workspace.
The asymmetry is given by:

.. math:: Asymmetry = \frac{F-\alpha B}{F+\alpha B}

where F is the front spectra, B is the back spectra and a is alpha.

The errors in F-aB and F+aB are calculated by adding the errors in F and
B in quadrature; any errors in alpha are ignored. The errors for the
asymmetry are then calculated using the fractional error method with the
values for the errors in F-aB and F+aB.

The output workspace contains one set of data for the time of flight,
the asymmetry and the asymmetry errors.

Note: this algorithm does not perform any grouping; the grouping must be
done via the GroupDetectors algorithm or when the NeXus file is loaded
auto\_group must be set to true.

.. categories::
