
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm replaces :ref:`BayesQuasi <algm-BayesQuasi>`.

The algorithm has two modes, `QL` and `QSe`.
These two modes are the same, except for the fitting function being used.
The sample and resolution are cropped and rebinned so that they have the same binning.
The fitting function is then generated with the user selected background plus a convolution of the resolution data with
a delta function (if elastic) and one of the following:

- one, two and three Lorentzians for the `QL` mode
- one stretched exponential for the `QSe` mode

The output includes a workspace of the fitting parameters, the loglikelihoods (the least negative is the most likely fit)
and the fits (interpolated back onto the original sample binning).

The stretched exponential results for the FWHM are different to :ref:`BayesQuasi <algm-BayesQuasi>`, as shown by the figure below.
However, the new results agree with the FWHM values for fitting a single Lorentzian.
The new method provides FWHM results that are comparable for all :math:`Q` values (green and black data), unlike the original code that has a divergence for low :math`Q` values.

.. figure:: /images/qse_cf.png
   :alt: qse_cf.png
   :width: 400px
   :align: center




Usage
-----

**Example - BayesQuasi2**

.. code::

    sampleWs = Load('irs26176_graphite002_red.nxs')
    resWs = Load('irs26173_graphite002_red.nxs')

    # Run BayesQuasi2 algorithm
    fit_ws, result_ws, prob_ws = BayesQuasi2(SampleWorkspace='irs26176_graphite002_red', ResolutionWorkspace='iris26173_graphite002_res',
                                             OutputWorkspaceFit='irs26176_graphite002_red_workspaces',
                                             OutputWorkspaceResult='irs26176_graphite002_red_results',
                                             OutputWorkspaceProb='irs26176_graphite002_red_prob')


.. categories::

.. sourcelink::
