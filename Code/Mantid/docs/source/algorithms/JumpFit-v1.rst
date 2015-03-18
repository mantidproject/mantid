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

**Example - Chudley-Elliott fit**

.. testcode:: exChudleyElliottFit

    data = Load(Filename='irs26176_graphite002_conv_2LFixF_s0_to_9_Result.nxs')

    JumpFit(InputWorkspace=data,
            QMin=0.6,
            QMax=1.8)

    fit = mtd['irs26176_graphite002_conv_2LFixF_s0_to_9_ChudleyElliot_fit_Workspace']
    params = mtd['irs26176_graphite002_conv_2LFixF_s0_to_9_ChudleyElliot_fit_Parameters']

    print 'Fit parameters: %s' % params.column(0)

**Output:**

.. testoutput:: exChudleyElliottFit

    Tau, L, Cost function value

.. categories::
