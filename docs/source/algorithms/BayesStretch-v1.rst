.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a variation of the stretched exponential option of
`Quasi <http://www.mantidproject.org/IndirectBayes:Quasi>`__. For each spectrum a fit is performed
for a grid of :math:`\beta` and :math:`\sigma` values. The distribution of goodness of fit values
is plotted.

This routine was originally part of the MODES package. Note that this algorithm
uses F2Py and is currently only supported on windows.

Usage
-----

**Example - BayesStretch**

.. testcode:: BayesStretchExample

    # Check OS support for F2Py (Windows only)
    from IndirectImport import is_supported_f2py_platform
    if is_supported_f2py_platform():
        # Load in test data
        sample_ws = Load('irs26176_graphite002_red.nxs')
        resolution_ws = Load('irs26173_graphite002_red.nxs')

        # Run BayesStretch algorithm
        fit_group, contour_group = BayesStretch(SampleWorkspace=sample_ws, ResolutionWorkspace=resolution_ws,
                                                EMin=-0.2, EMax=0.2, Background='Sloping', Loop=True)

.. categories::

.. sourcelink::