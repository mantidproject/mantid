
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates an S(Q, w) from the fitted results of a ConvFit: one
or two Lorentzians. The Q, w range is that for the input *_red* workspace. A
resolution *_res* is also needed for the elastic/delta-function peak.

The object is to create a S(Q, w) with an energy range greater than that of the
measured Q, w to provide a better calculation for multiple scattering.

The *ParameterWorkspace* workspace must contain the following sample logs
present when using ConvFit to fit either one or two Lorentzians:

- program="ConvFit"
- 'lorenztians'

Workflow
--------

.. diagram:: MuscatSofQW-v1_wkflw.dot


Usage
-----

**Example - MuscatSofQW**

.. testcode:: exMuscatSofQW

    sample = Load('irs26176_graphite002_red.nxs')
    resolution = Load('irs26173_graphite002_res.nxs')
    parameters = Load('irs26176_graphite002_conv_1LFixF_s0_to_9_Result.nxs')

    sqw = MuscatSofQW(SampleWorkspace=sample,
                      ResolutionWorkspace=resolution,
                      ParameterWorkspace=parameters)

    print('S(Q, w) workspace is intensity as a function of {0} and {1}'.format(
          sqw.getAxis(0).getUnit().unitID(),
          sqw.getAxis(1).getUnit().unitID()))

Output:

.. testoutput:: exMuscatSofQW

    S(Q, w) workspace is intensity as a function of Energy and MomentumTransfer

.. categories::

.. sourcelink::

