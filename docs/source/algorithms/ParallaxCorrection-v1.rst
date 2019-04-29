
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a geometrical correction for the so-called parallax effect in tube based SANS instruments.

The correction formula must be specified in the :ref:`IPF <InstrumentParameterFile>` as follows:

- A string parameter named **direction** must hold **x** or **y** which is the direction of the tubes in the detector.

- A string parameter named **parallax** must hold the `muparser <http://beltoforion.de/article.php?a=muparser>`_ expression, where **t** is reserved for the parallax angle:

:math:`t = \arctan(\frac{x}{z})` if direction is **y**, and :math:`t = \arctan(\frac{y}{z})` if direction is **x**.

:math:`x, y, z` are the coordinates of the detector pixel in the system where sample is at :math:`0,0,0` and :math:`z` is the beam axis.
:math:`t \in (0,\frac{\pi}{2})` in radians.

The correction will be calculated for each pixel and the input data will be divided by the correction and stored in the output.

The instrument parameters must be defined for detector components and without loss of generality, different components can have different formulae.

At least one component name must be given as input.

Example of adding parameters in the IPF
---------------------------------------

.. code-block:: xml

  <component-link name="some_bank">

    <parameter name="parallax" type="string">
      <!-- Normally, the function would be increasing with t -->
      <value val="1 + 0.01 * t"/>
    </parameter>

    <parameter name="direction" type="string">
      <value val="x"/>
    </parameter>

  </component-link>

Usage
-----

**Example - ParallaxCorrection**

.. testcode:: ParallaxCorrectionExample

    CreateSampleWorkspace(NumBanks=1, XMin=1, XMax=2, BinWidth=1, BankPixelWidth=100, Function="One Peak", XUnit="Wavelength", OutputWorkspace="in")
    SetInstrumentParameter(Workspace="in", ParameterName="direction", ComponentName="bank1", ParameterType="String", Value="y")
    SetInstrumentParameter(Workspace="in", ParameterName="parallax", ComponentName="bank1", ParameterType="String", Value="1+0.1*t")
    ParallaxCorrection(InputWorkspace="in", ComponentNames="bank1", OutputWorkspace="out")
    Divide(LHSWorkspace="in", RHSWorkspace="out", OutputWorkspace="corr")
    print("The correction is {0:.4f} for the spectrum {1}".format(mtd["corr"].readY(1000)[0], 1000))

Output:

.. testoutput:: ParallaxCorrectionExample

  The correction is 1.0016 for the spectrum 1000

Example of correction
---------------------

Below is an example of the corrections for the instrument D22. The tubes are vertical, the magnitude depends only on the horizontal coordinate.
The correction increases with increasing angle from the beam, reaching about 10% for the outmost tube for detector distance of 1.5 meters.

.. figure:: /images/parallax.png

.. categories::

.. sourcelink::
