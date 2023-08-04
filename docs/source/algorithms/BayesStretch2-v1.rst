.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a variation of the stretched exponential option of
:ref:`Quasi <algm-BayesQuasi2>`. For each spectrum a fit is performed
for a grid of :math:`\beta` a FWHM values. The unnormalised probability
is given for the :math:`z` values.

This routine replaces the :ref:`BayesStretch <algm-BayesStretch>` algorithm.

Usage
-----

**Example - BayesStretch2**

.. code::


    sample_ws = Load('irs26176_graphite002_red.nxs')
    resolution_ws = Load('irs26173_graphite002_red.nxs')

    # Run BayesStretch2 algorithm
    fit_group, contour_group = BayesStretch2(NumberProcessors='8', NumberFWHM='10', NumberBeta='10', StartBeta=0.8,EndBeta=1.0,
                                             StartFWHM=0.01, EndFWHM=0.1, SampleWorkspace='irs26176_graphite002_red',
                                             ResolutionWorkspace='iris26173_graphite002_res',
                                             Background='Linear', OutputWorkspaceFit='irs26176_graphite002_red_Stretch_Fit_workspaces',
                                             OutputWorkspaceContour='irs26176_graphite002_red_Stretch_Contour_workspaces')


.. categories::

.. sourcelink::
