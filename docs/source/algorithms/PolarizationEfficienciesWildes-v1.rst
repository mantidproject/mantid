
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


This algorithm calculates the instrument component efficiencies in a polarized analysis experiment by implementing the Wildes [#WILDES]_ approach for calibrating an instrument with two flippers.

A non-magnetic transmission run should be provided via the ``InputNonMagWorkspace`` property. This can be used to calculate both flipper efficiencies and phi, as follows:

.. math::

   \phi = \frac{(I^{00}_{1} - I^{01}_{1})(I^{00}_{1} - I^{10}_{1})}{I^{00}_{1}I^{11}_{1} - I^{01}_{1}I^{10}_{1}}

.. math::

   f_p = \frac{I^{00}_{1} - I^{01}_{1} - I^{10}_{1} + I^{11}_{1}}{2(I^{00}_{1} - I^{01}_{1})}

.. math::

   f_a = \frac{I^{00}_{1} - I^{01}_{1} - I^{10}_{1} + I^{11}_{1}}{2(I^{00}_{1} - I^{10}_{1})}


The algorithm will only be able to calculate polarizer and/or analyser efficiencies if you also provide a magnetic transmission run via the ``InputMagWorkspace`` property.
This can used to calculate the polarizer and analyser efficiencies as follows:

.. math::

   (2p-1)^2 = \phi\Bigg(\frac{(1-2f_a)I^{00}_{2} + (2f_a-1)I^{10}_{2} - I^{01}_{2} + I^{11}_{2}}{(1-2f_p)I^{00}_{2} + (2f_p-1)I^{01}_{2} - I^{10}_{2} + I^{11}_{2}}\Bigg)

From which you can solve for the polarizer efficiency (:math:`p`) and then solve for the analyser efficiency (:math:`a`) from the following relationship:

.. math::

   \phi = (2p-1)(2a-1)

Alternatively, previously calculated polarizer and/or analyser efficiency workspaces can be passed to the ``InputPolarizerEfficiency`` and ``InputAnalyserEfficiency`` properties respectively.
If workspaces are provided for both then they are used directly as the output polarizer and analyser efficiencies. If only one is provided then it is used to solve for the other efficiency.

Both types of input workspace group should contain four child workspaces. A flipper configuration must be passed to the ``Flippers`` property to identify which child workspaces in the input group(s) correspond to which instrument configurations.
The ``Flippers`` property takes a string in the form :literal:`'00, 01, 10, 11'`, which would indicate both flippers off, analyzer flipper on, polarizer flipper on, both flippers on. The flipper configuration can be provided in any order that matches the child workspaces in the input group(s).

Outputs
#######

As a minimum, the algorithm calculates the polarizing and analysing flipper efficiencies, producing two output workspaces that give these values as a function of wavelength.

If the ``OutputPolarizerEfficiency`` property is set then an output workspace will be produced giving the polarizer efficiency as a function of wavelength.

If the ``OutputAnalyserEfficiency`` property is set then an output workspace will be produced giving the analyser efficiency as a function of wavelength.

If property ``IncludeDiagnosticOutputs`` is set to ``True`` then the following diagnostic output workspace properties will be set, again with values calculated as a function of wavelength:

- ``OutputPhi`` - outputs the value that was calculated for :math:`\phi`.
- ``OutputRho`` - calculates :math:`2f_p - 1`.
- ``OutputAlpha`` - calculates :math:`2f_a - 1`.
- ``OutputTwoPMinusOne`` - calculates :math:`2p-1`. This will only be included in the diagnostic output if the ``OutputPolarizerEfficiency`` property has also been set.
- ``OutputTwoAMinusOne`` - calculates :math:`2a-1`. This will only be included in the diagnostic output if the ``OutputAnalyserEfficiency`` property has also been set.

Workspace names are automatically provided for each of the diagnostic outputs (see the property default values), but can be overwritten if desired.

Usage
-----
**Example - PolarizationEfficienciesWildes**

.. testcode:: PolarizationEfficienciesWildesExample

   ws00 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+12")
   ws01 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+1")
   ws10 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+2")
   ws11 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+10")

   nonMag = GroupWorkspaces([ws00, ws01, ws10, ws11])

   wsM00 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+6")
   wsM01 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+0.2")
   wsM10 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+0.3")
   wsM11 = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, BankPixelWidth=1, Function="User Defined", UserDefinedFunction="name=UserFunction, Formula=x*0+1")

   mag = GroupWorkspaces([wsM00, wsM01, wsM10, wsM11])

   PolarizationEfficienciesWildes('nonMag', 'mag', Flippers='00,01,10,11', IncludeDiagnosticOutputs=False, OutputFpEfficiency="fp", OutputFaEfficiency="fa", OutputPolarizerEfficiency="p", OutputAnalyserEfficiency="a")
   fp = AnalysisDataService.retrieve("fp")
   print("Polarizing flipper efficiency is: {:.4}".format(fp.readY(0)[0]))

Output:

.. testoutput:: PolarizationEfficienciesWildesExample

   Polarizing flipper efficiency is: 0.8636

References
----------

.. [#WILDES] A. R. Wildes, *Neutron News*, **17** 17 (2006)
             `doi: 10.1080/10448630600668738 <https://doi.org/10.1080/10448630600668738>`_

.. categories::

.. sourcelink::
