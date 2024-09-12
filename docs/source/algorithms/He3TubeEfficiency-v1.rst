.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm corrects the detection efficiency of He3 tubes using an
exponential function and certain detector properties. The correction
scheme is given by the following:

:math:`\epsilon = \frac{A}{1-e^{\frac{-\alpha P (L - 2W) \lambda}{T \sin(\theta)}}}`

where *A* is a dimensionless scaling factor, :math:`\alpha` is a
constant with units :math:`(\text{Kelvin} / (\text{metres} \: \AA\: \text{atm}))`, *P* is
pressure in units of *atm*, *L* is the tube diameter in units of
metres, *W* is the tube thickness in units of metres, *T* is the
temperature in units of Kelvin, :math:`\sin(\theta)` is the angle
of the neutron trajectory with respect to the long axis of the He3 tube
and :math:`\lambda` is in units of :math:`\AA`.

The Optional properties that are of num list type can be used in the
following manner. If no input value is given, the detector parameter is
pulled from the detector itself. If a single value is used as input,
that value is applied to all detectors. If an array of values is used,
that array *must* be the same size as the number of spectra in the
workspace. If it is not, the spectra indices that do not have values
will be zeroed in the output workspace.

Restrictions on Input Workspace
###############################

The input workspace must be in units of wavelength.

Usage
-----

The correction applies equally well to histogram and event workspaces. An example
for a histogram workspace will be given.

.. testcode:: ExSampleWorkspace

    # create workspace with correct units
    ws = CreateSampleWorkspace(Function='Flat background',NumBanks='1',BankPixelWidth='2',
                               XUnit='Wavelength',XMax='10',BinWidth='0.01')

    # workspace does not have correct tube parameters, so we need to provide some
    # parameters taken from the ARCS instrument at the SNS
    ws1 = He3TubeEfficiency(ws, TubePressure='10',TubeThickness='0.0008',TubeTemperature='290')

    # show the before and after for the first detector for the first 5 bins
    print('Original counts: {}'.format(ws.readY(0)[:5]))
    print('Corrected counts: {}'.format(ws1.readY(0)[:5]))

Output:

.. testoutput:: ExSampleWorkspace

    Original counts: [1. 1. 1. 1. 1.]
    Corrected counts: [417.07353051 139.35837696  83.81566631  60.01187609  46.787726  ]

.. categories::

.. sourcelink::
