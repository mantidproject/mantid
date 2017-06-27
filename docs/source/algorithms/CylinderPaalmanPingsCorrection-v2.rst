.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates absorption corrections for a cylindrical or annular sample giving
output in the Paalman and Pings absorption factors: :math:`A_{s,s}` (correction
factor for scattering and absorption in sample), :math:`A_{s,sc}` (scattering in
sample and absorption in sample and container), :math:`A_{c,sc}` (scattering in
container and absorption in sample and container) and  :math:`A_{c,c}`
(scattering and absorption in container).

Restrictions on the input workspace
###################################

The input workspace must have a fully defined instrument.

Energy transfer modes
#####################

The algorithm operates in different energy transfer modes, where the incident (:math:`\lambda_1`) and the final (:math:`\lambda_2`)
wavelengths are defined as follows:

- **Elastic** : :math:`\lambda_1 = \lambda_2 = \lambda_{step}`

- **Direct**  : :math:`\lambda_1 = \lambda_{fixed}, \lambda_2 = \lambda_{step}`

- **Indirect** : :math:`\lambda_1 = \lambda_{step}, \lambda_2 = \lambda_{fixed}`

- **Efixed** : :math:`\lambda_1 = \lambda_2 = \lambda_{fixed}`,

where :math:`\lambda_{fixed}` is computed from the `Efixed` value corresponding to the monochromator or the analyser, and
:math:`\lambda_{step}` iterates equidistantly over the wavelength points in the input workspace x-axis, controlled by `NumberWavelengths` property.

Therefore, in all the modes except **Efixed**, the input workspaces must have the x-axis unit of `Wavelength`.
In all the modes except **Elastic**, `Efixed` value is needed. By default it will be attempted to be read
from the instrument parameters, but can be overridden by the homonym property.
In the **Efixed** mode the `NumberWavelengths` and `Interpolate` options will be ignored.

Usage
-----

**Example:**

.. testcode:: ExCylinderPaalmanPingsCorrection

    # Create a sample workspace
    sample = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1,
                                   XUnit='Wavelength',
                                   XMin=6.8, XMax=7.9,
                                   BinWidth=0.1)

    # Copy and scale it to make a can workspace
    can = CloneWorkspace(InputWorkspace=sample)
    can = Scale(InputWorkspace=can, Factor=1.2)

    # Calculate absorption corrections
    corr = CylinderPaalmanPingsCorrection(SampleWorkspace=sample,
                                          SampleChemicalFormula='H2-O',
                                          SampleInnerRadius=0.05,
                                          SampleOuterRadius=0.1,
                                          CanWorkspace=can,
                                          CanChemicalFormula='V',
                                          CanOuterRadius=0.15,
                                          BeamHeight=0.1,
                                          BeamWidth=0.1,
                                          StepSize=0.002,
                                          Emode='Indirect',
                                          Efixed=1.845)

    print 'Correction workspaces: %s' % (', '.join(corr.getNames()))

Output:

.. testoutput:: ExCylinderPaalmanPingsCorrection

    Correction workspaces: corr_ass, corr_assc, corr_acsc, corr_acc

Related Algorithms
------------------

:ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>`
will correctly apply the partial absorption factors

.. categories::

.. sourcelink::
