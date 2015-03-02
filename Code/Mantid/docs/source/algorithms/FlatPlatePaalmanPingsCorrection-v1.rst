.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates absorption corrections for a flat plate sample giving output in the
Paalman & Pings absorption factors: :math:`A_{s,s}` (correction factor for
scattering and absorption in sample), :math:`A_{s,sc}` (scattering in sample and
absorption in sample and container), :math:`A_{c,sc}` (scattering in container
and absorption in sample and container) and  :math:`A_{c,c}` (scattering and
absorption in container).

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength.

Usage
-----

**Example:**

.. testcode:: exSampleAndCan

    # Create a sample workspace
    sample = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1,
                                   XUnit='Wavelength',
                                   XMin=6.8, XMax=7.9,
                                   BinWidth=0.1)

    # Copy and scale it to make a can workspace
    can = CloneWorkspace(InputWorkspace=sample)
    can = Scale(InputWorkspace=can, Factor=1.2)

    # Calculate absorption corrections
    abs = FlatPlatePaalmanPingsCorrection(SampleWorkspace=sample,
                                          SampleChemicalFormula='H2-O',
                                          SampleThickness=0.1,
                                          SampleAngle=45,
                                          CanWorkspace=can,
                                          CanChemicalFormula='V',
                                          CanFrontThickness=0.01,
                                          CanBackThickness=0.01,
                                          Emode='Indirect',
                                          Efixed=1.845)

    print 'Correction workspaces: %s' % ', '.join(abs.getNames())

Output:

.. testoutput:: exSampleAndCan

    Correction workspaces: abs_ass, abs_assc, abs_acsc, abs_acc

.. categories::
