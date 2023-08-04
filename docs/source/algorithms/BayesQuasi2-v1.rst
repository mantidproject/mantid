
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
