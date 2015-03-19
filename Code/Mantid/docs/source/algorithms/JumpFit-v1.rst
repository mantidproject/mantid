.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Fits the Q variation of the fitted widths  from either ConvFit or Quasi to one
of 4 models (the fit function used is given in brackets):

- Chudley-Elliott model (ChudelyElliott)
- Hall-Ross model (HallRoss)
- Simple Fick Diffusion (FickDiffusion)
- Teixeira's model for water (TeixeiraWater)

Usage
-----

**Example - Chudley-Elliot fit**

.. testcode:: exChudleyElliotFit

    data = Load(Filename='irs26176_graphite002_conv_2LFixF_s0_to_9_Result.nxs')

    JumpFit(InputWorkspace=data,
            FUnction='ChudleyElliot',
            QMin=0.6,
            QMax=1.8)

    fit = mtd['data_ChudleyElliot_fit_Workspace']
    params = mtd['data_ChudleyElliot_fit_Parameters']

    print 'Fit parameters: %s' % ', '.join(params.column(0))

**Output:**

.. testoutput:: exChudleyElliotFit

    Fit parameters: Tau, L, Cost function value

.. categories::
