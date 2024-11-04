
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm integrates a workspace around the peak positions given in an elastic peak position (EPP) table. The integration is done using the :ref:`algm-Integration` algorithm. *EPPWorkspace* should be a table workspace in the format returned by :ref:`algm-FindEPP`. The integration limits for workspace index :math:`i` in *InputWorkspace* are given by

    :math:`d_i = c_i \pm w \sigma_i`,

where :math:`-` is for the lower and :math:`+` for the upper limit, :math:`c_i` the value from the 'PeakCentre' column, :math:`w` the *HalfWidthInSigmas* property and :math:`\sigma_i` the value from the 'Sigma' column.

If a workspace index is missing from *EPPWorkspace*, the integration limits will be set to zero.

Usage
-----
**Example - IntegrateEPP**

.. testcode:: IntegrateEPPExample

    gaussian = 'name=Gaussian, PeakCentre=7000, Height=230, Sigma=680'
    ws = CreateSampleWorkspace('Histogram', 'User Defined', gaussian)

    epps = FindEPP(ws)

    integrated = IntegrateEPP(ws, epps, 3)

    xs = integrated.readX(0)
    ys = integrated.readY(0)
    print('Integral from {:.6} to {:.6} yields {:.5}'.format(xs[0], xs[1], ys[0]))

Output:

.. testoutput:: IntegrateEPPExample

    Integral from ... to ... yields ...

.. categories::

.. sourcelink::
