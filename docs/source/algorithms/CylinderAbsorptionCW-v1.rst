.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm implements two simple analytical absorption corrections and a lookup table for multiple-scattering corrections for constant wavelength diffractometers.
The absorption and multiple scattering are calculated assuming a cylindrical sample geometry, constant wavelength and in-plane scattering only.

Two methods are available for calculating the absorption correction: Using Equation 26 and Table 3 from Sears [1]_ and using Equations 1-6 from Sabine [2]_.

Multiple scattering is calculated using Equation 16 and Table 1 from Blech and Averbach [3]_, using a bilinear interpolation of the tabulated values. Equation 16 gives :math:`\sigma_m` but the output from this algorithm is the multiple scattering factor, :math:`\delta` = :math:`\sigma_m/(\sigma_s + \sigma_m)`.


Usage
-----

**Example:**

.. testcode:: SimpleExample

   ws = CreateSampleWorkspace()
   # Using values for vanadium
   result = CylinderAbsorptionCW(
            InputWorkspace=ws,
            Radius=0.5,  # cm
            Height=4.0,  # cm
            Wavelength=1.7982,
            AttenuationXSection=5.08,  # barn at 1.798 Å
            ScatteringXSection=5.1,  # barn
            SampleNumberDensity=0.0723,  # atoms/Å^3
            AbsorptionWorkspace="Absorption",
            MultipleScatteringWorkspace="MultipleScattering"
        )
   print(f"Absorption correction calculated value is {result.AbsorptionWorkspace.readY(0)[0]:.2f} and multiple scattering value is {result.MultipleScatteringWorkspace.readY(0)[0]:.2f}")

Output:

.. testoutput:: SimpleExample

   Absorption correction calculated value is 0.54 and multiple scattering value is 0.15


**Example using SetSample:**

.. testcode:: SetSampleExample

   ws = CreateSampleWorkspace()
   # Using values for Vanadium
   SetSample(
        ws,
        Geometry={"Shape": "Cylinder", "Height": 4.0, "Radius": 0.5},
        Material = {"ChemicalFormula": "V", "SampleNumberDensity": 0.0723}
    )
   result = CylinderAbsorptionCW(
            InputWorkspace=ws,
            Wavelength=1.7982,
            AbsorptionWorkspace="Absorption",
            MultipleScatteringWorkspace="MultipleScattering"
        )
   print(f"Absorption correction calculated value is {result.AbsorptionWorkspace.readY(0)[0]:.2f} and multiple scattering value is {result.MultipleScatteringWorkspace.readY(0)[0]:.2f}")

Output:

.. testoutput:: SetSampleExample

   Absorption correction calculated value is 0.54 and multiple scattering value is 0.15

.. categories::

.. sourcelink::


References
----------

.. [1] Sears, V. F. (1984). Absorption factor for cylindrical samples. J. Appl. Cryst. 17, 226-230. `<https://doi.org/10.1107/S0021889884011420>`__

.. [2] Sabine, T. M., Hunter, B. A., Sabine, W. R. & Ball, C. J. (1998). Analytical Expressions for the Transmission Factor and Peak Shift in Absorbing Cylindrical Specimens. J. Appl. Cryst. 31, 47-51. `<https://doi.org/10.1107/S0021889897006961>`__

.. [3] Blech, I. A. & Averbach, B. L. (1965). Multiple Scattering of Neutrons in Vanadium and Copper. Phys. Rev. 137, A1113. `<https://doi.org/10.1103/PhysRev.137.A1113>`__
