
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can calculate the 1st and 2nd order Placzek inelastic scattering correction [1]_ [2]_ .
For this particular algorithm:

* The input workspace must

  * contain a sample with proper chemical formula as the correction calculation relies on it.
  * have a valid instrument geometry attached to it as the correction factors are calculated on a per spectrum (i.e. detector) basis.

* A workspace containing the incident spectrum extracted from the monitor is needed.

  * For the first order correction, only the incident spectrum and its first order derivative is needed.
  * For the second order correction, the incident spectrum along with its first and second derivate are needed.
  * It is implicitly assumed that

    * ``IncidentSpectra.readY(0)`` returns the incident spectrum.
    * ``IncidentSpectra.readY(1)`` returns the first order derivative.
    * ``IncidentSpectra.readY(2)`` returns the second order derivative.

* The algorithm will try to extract temperature from the sample log if it is not provided. However, this will be a simple average without any additional consideration about outliers or bad reading. Therefore, it is recommended to provide a sample temperature in Kelvin explicitly.

* The Placzek correction calculation requires a detector efficiency curve and its derivatives. This algorithm will prioritize the use of input ``EfficiencySpectra``. However, when ``EfficiencySpectra`` is not provided:

  * The algorithm will can generate a theoretical detector efficiency curve (see :ref:`He3TubeEfficiency <algm-He3TubeEfficiency>` for details) using the input Parameter ``LambdaD``.
  * When no ``LambdaD`` is provided, the default value 1.44 will be used, which is also the implicit value used in the original :ref:`CalculatePlaczekSelfScattering <algm-CalculatePlaczekSelfScattering-v1>`.
  * Generally speaking it is better to measure the detector efficiency instead of relying on a theoretical one.

* The calculated Placzek correction factor will be scaled by the packing fraction if ``ScaleByPackingFraction=True`` (default).

.. math::
   :label: ScaleByPackingFraction

   P_\text{scaled} = (1 + P) * p_\text{packingFraction}

where :math:`P` is the Placzek correction factor, and :math:`p` is the packing fraction.

Physics
-------

This section provides a brief description of the formula used to calculate the Placzek correction.
In the original work [1]_ , the formula to compute the first order Placzek correction, :math:`P_1` is given as:

.. math::
   :label: Placzek1

   P_1  = 2 \sin^2(\dfrac{\theta}{2})
          \left[ (f-1)\phi_1 - f \epsilon_1 + f - 3 \right]
          \sum_\alpha c_\alpha \bar{b_\alpha^2} \dfrac{m}{M_\alpha}

where

* :math:`\theta` is the scattering angle.
* :math:`f = \frac{L_1}{L_1+L_2}` with :math:`L_1` being the distance between moderator and the sample and :math:`L_2` being the distance between the sample and the detector.
* :math:`\phi_1` is the first order incident flux coefficient.
* :math:`\epsilon_1` is the first order detector efficiency coefficient.
* :math:`c_\alpha` is the number proportion of species :math:`\alpha`.
* :math:`b_\alpha` is the total scattering length of species :math:`\alpha`.
* :math:`m` is the mass of neutron.
* :math:`M_\alpha` refers to the atomic mass of species :math:`\alpha`.

When the incident flux :math:`\phi` is available from monitor, the first order incident flux coefficient, :math:`\phi_1` can be calculated with

.. math::
   :label: incidentFluxCoff1

   \phi_1 = \lambda_i \dfrac{\phi'(\lambda_i)}{\phi(\lambda_i)}

where :math:`\phi'(\lambda)` is defined as

.. math::
   :label: phiprime

   \phi'(\lambda) = \dfrac{\ln(\phi(\lambda))}{\ln(\lambda)}

When the detector efficiency :math:`\epsilon` is measured as a function of wave vector :math:`k = 2\pi / \lambda`, the first order detector efficiency coefficient, :math:`\epsilon_1` can be calculated with

.. math::
   :label: detectorEffCoff1

   \epsilon_1 = k_i \dfrac{\epsilon'(k_i)}{\epsilon(k_i)}

However, if the detector efficiency is never measured, one can still use an approximated detector efficiency curve

.. math::
   :label: detectorEffCurve

   \epsilon(k) \approx 1 - \exp(\dfrac{-\lambda}{\lambda_d})

where :math:`\lambda_d` is the reference wavelength for the detector.
Therefore, the approximate first order detector efficiency coefficient, :math:`\epsilon_1` simplified to

.. math::
   :label: idealDetectorEffCoff1

   \epsilon_1 = \dfrac{x e^x}{1 - e^x}

where :math:`x = -\lambda / \lambda_d`.
It is worth points out that the derivative of the detector efficiency is computed with respect to :math:`\ln(k)`, namely

.. math::

   \epsilon' = \dfrac{\ln(\epsilon(k))}{\ln(k)}

The detailed explanation can be found in [2]_ .

.. plot:: algorithms/CalculatePlaczekPlotP1.py

The second order Placzek correction, :math:`P_2` is similar to the first order, just with some new components

.. math::

   P_2 &= \sum_\alpha c_\alpha \bar{b_\alpha^2} \dfrac{m}{M_\alpha}
          \{\dfrac{k_B T}{2E}
            + \dfrac{k_B T}{E} \sin^2(\dfrac{\theta}{2})
              \left[
                 (8f - 9)(f-1)\phi_1
                -3f(2f-3)\epsilon_1
                +2f(1-f)\phi_1\epsilon_1
                +(1-f)^2\phi_2
                +f^2\epsilon_2
                +3(4f-5)(f-1)
              \right]
          \} \\
       &+ 2 \sin^2(\dfrac{\theta}{2})
            \sum_\alpha c_\alpha \bar{b_\alpha^2} (\dfrac{m}{M_\alpha})^2
            \{ 1 + \sin^2(\dfrac{\theta}{2})
                   \left[(4f-7)(f-1)\phi_1
                        +f(7-2f)\epsilon_1
                        +2f(1-f)\phi_1\epsilon_1
                        +(1-f)^2\phi_2
                        +f^2\epsilon_2
                        +(2f^2 -7f +8)
                   \right]
            \}

where

* :math:`k_B` is the Boltzmann constant.
* :math:`T` is the temperature in Kelvin.
* :math:`E` is the energy of the incident neutron as :math:`E = h^2/(2m\lambda^2_i)`.
* :math:`\phi_2` is the second order incident flux coefficient.
* :math:`\epsilon_2` is the second order detector efficiency coefficient.

Similar to :math:`\phi_1`, :math:`\phi_2` can be calculated when incident flux is measured by the monitor,

.. math::
   :label: incidentFluxCoff2

   \phi_2 = \lambda_i \dfrac{\phi''(\lambda_i)}{\phi(\lambda_i)}

and :math:`\epsilon_2` can be calculated directly from measured detector efficiency,

.. math::
   :label: detectorEffCoff2

   \epsilon_2 = k_i \dfrac{\epsilon''(k_i)}{\epsilon(k_i)}

If no detector efficiency is measured, :math:`\epsilon_2` can also be approximated with the theoretical detector efficiency formula, namely

.. math::
   :label: idealDetectorEffCoff2

   \epsilon_2 = \dfrac{-x (x+2) e^x}{1 - e^x} = -(x+2)\epsilon_1

where :math:`x = -\lambda / \lambda_d`.

.. plot:: algorithms/CalculatePlaczekPlotP2.py

Usage
-----

**Example - CalculatePlaczek**

The data files used in the following example code are available as part of the Mantid testing data.

.. code-block:: python

   # Load the incident flux from file
   Load(
     Filename='fluxSmoothedNOM161959.nxs',
     OutputWorkspace='influx',
     )
   # Load the input workspace
   # - must have instrument
   # - must have sample with valid chemical formula
   Load(Filename='inputwsNOM_164109.nxs', OutputWorkspace='NOM_164109')

   # Calculate first order
   # NOTE: temperature is not needed for first order calculation
   CalculatePlaczek(
    InputWorkspace="NOM_164109",
    IncidentSpectra="influx",
    LambdaD=1.44,
    Order=1,
    ScaleByPackingFraction=False,
    CrystalDensity=0.01,
    OutputWorkspace="NOM_P1",
    )

   # Calculate first and second order
   CalculatePlaczek(
    InputWorkspace="NOM_164109",
    IncidentSpectra="influx",
    LambdaD=1.44,
    Order=2,
    SampleTemperature=943.15,  # in Kelvin
    ScaleByPackingFraction=False,
    CrystalDensity=0.01,
    OutputWorkspace="NOM_P2",
   )


References
----------

.. [1] Howe, McGreevy, and Howells, J., (1989), *The analysis of liquid structure data from time-of-flight neutron diffractometry*, Journal of Physics: Condensed Matter, Volume 1, Issue 22, pp. 3433-3451, `doi: 10.1088/0953-8984/1/22/005 <https://doi.org/10.1088/0953-8984/1/22/005>`__
.. [2] Howells, W.S. 1984. *On the Choice of Moderator for a Liquids Diffractometer on a Pulsed Neutron Source.*, Nuclear Instruments and Methods in Physics Research 223 (1): 141–46. `doi: 10.1016/0167-5087(84)90256-4 <https://doi.org/10.1016/0167-5087(84)90256-4>`__


.. categories::

.. sourcelink::
