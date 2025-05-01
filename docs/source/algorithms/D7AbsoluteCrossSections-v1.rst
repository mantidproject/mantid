
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that performs cross-section separation and allows for sample data normalisation to absolute scale.
The cross-section separation provides information about magnetic, nuclear coherent, and spin-incoherent contributions to the measured cross-section.
The absolute scale normalisation uses either the output from cross-section separation or a vanadium reference sample for polarised diffraction and spectroscopy data measured
by D7 instrument at the ILL.

Three types of cross-section separation are supported: `Z`, `XYZ`, and `10p`, for which 2, 6, and 10 distributions with spin-flip and non-spin-flip cross-sections
are required. In addition, the XYZ cross-section separation can be executed either assuming isotropic magnetism, such as in Ref. [#Ehlers]_, or when the magnetism cannot
be assumed to be isotropic, for example in single crystals, as described in Ref. [#Schweika]_. Which set of equations is used is controlled by setting `IsotropicMagnetism`
to either `True` or `False`. The expected input for the cross-section separation is a workspace group containing spin-flip and non-spin-flip
cross-sections, with the following order of axis directions: Z, Y, X, X-Y, X+Y. This step can be skipped by setting `CrossSectionSeparationMethod` parameter to `'None'`.

Three ways of sample data normalisation are supported: `Vanadium`, `Paramagnetic`, and `Incoherent`, for which either the output from vanadium data reduction,
or from the cross-section separation (magnetic and spin-incoherent respectively) is used. This step can also be skipped by setting `NormalisationMethod` parameter
to `'None'`.

`MeasurementTechnique` property allows to distinguish between data reduced as `Powder`, `SingleCrystal`, or `TOF` by :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>`.
`Paramagnetic` and `Incoherent` data normalisation approaches are not possible when the was data reduced in the `TOF` mode.

The choice for the output units are set with `OutputUnits` property. The options are: `TwoTheta`, `Q`, `Qxy`, `Qw`, `Default`, and `Input`. The first two are the most relevant
for `Powder` measurement technique. Single crystal data would be best shown on the `Qx` - `Qy` plane, which can be achieved by setting `OutputUnit` to `Qxy`. The `TOF`-mode
specific output unit is `Qw`, which allows to obtain data on the momentum exchange (Q) - energy exchange plane (`w`, which stands for :math:`\omega`). The `Input` setting allows for
preserving the input unit without undergoing any conversions, which can be useful when trying to find a source of issue in the reduction. The `Default` mode will show the most
relevant output, depending on the technique. It will be equivalent to `Q` option for `Powder` data, `Qxy` for single crystal, and in the TOF mode, it will be a group of data
as a function of spectrum number versus energy exchange (equivalent to `Input` in TOF), :math:`2\theta` versus energy exchange, and :math:`S (Q, \omega)` distribution (as in `Qw`).

For the TOF-mode, it is possible to provide user-defined binning for the momentum-exchange axis, used by :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` algorithm. If this
property is not defined, the algorithm will automatically provide non-equidistant binning that is suitable for the data distribution.

This algorithm is intended to be invoked on sample data that is fully corrected and needs to be normalised to an appropriate scale.

SampleAndEnvironmentProperties
##############################

This property is a dictionary containing all of the information about the sample and its environment, in the same fashion as in :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>`.
This information is used for proper normalisation of the given sample.

The following keys need to be defined:

- *SampleMass*
- *FormulaUnitMass*
- *SampleSpin* if the `NormalisationMethod` is set to `Paramagnetic`
- *IncoherentCrossSection* if the `NormalisationMethod` is set to `Incoherent` and `AbsoluteUnitsNormalisation` is *True*

Keys required when the `MeasurementTechnique` is `SingleCrystal`:

- *KiXAngle* - angle between the incident momentum and X-axis
- *OmegaShift* - omega offset

Optional keys when the `MeasurementTechnique` is `SingleCrystal`:

- *fld*
- *nQ*

`nQ` allows the user to decide how many bins should the `Q_{x}` and `Q_{y}` axes be split into, 80 by default. `fld` is used to decide whether to expand the distribution symmetrically
to the negative side of `Q_{x}` and `Q_{y}` axes, equal to 1 (`True`) by default.

Cross-section separation method
###############################

Below are presented formulae used to separate magnetic (M), nuclear coherent (N), and spin-incoherent (I) cross-sections using
spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{sf}}` and non-spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{nsf}}`
cross-sections from the provided input :ref:`WorkspaceGroup <WorkspaceGroup>`.

1. Z

At least two separate measurements along the same axis with opposite spin orientations are needed for this method to work. Usually, the measured axis is the 'Z' axis that is colinear with the beam axis, thus the spin-flip and non-spin-flip cross-sections are measured along the longitudinal axis. This method does not allow for separation of magnetic cross-section.

.. math::

      N &= \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \frac{1}{2} \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}

      I &= \frac{3}{2} \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}


In this case, the magnetic cross-section cannot be separated from data.

2. XYZ

This method is an expansion of the Z method, that requires measurements of spin-flip and non-spin-flip cross-sections along three orthogonal axes. This method allows for separation of magnetic cross-section from nuclear coherent and spin-incoherent.

The following set of equations applies to the isotropic magnetism case, such as in powder samples, and are based on Ref. [#Steward]_ and [#Ehlers]_.

.. math::

      T &= \frac{1}{3} \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}  \right)

      N &= \frac{1}{6} \cdot \left(2 \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} \right) - \left( \left( \frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right) \right)

      M_{\text{nsf}} &= 2 \cdot \left(2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{d\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} \right)

      M_{\text{sf}} &= 2 \cdot \left(\left(\frac{d\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} -2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right)

      M_{\text{av}} &= \frac{1}{2} \left( M_{\text{nsf}} + M_{\text{sf}} \right)

      I &= \frac{1}{2} \cdot \left(\left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right) - M_{\text{av}}

The equations below apply to the case of anisotropic magnetism, such as in single crystals, and are based on Ref. [#Schweika]_. The total cross-section is calculated the same way in both cases.

.. math::

   N &= \frac{1}{2} \left( \left( \frac{\text{d}\sigma_{x}}{\text{d}\Omega} \right)_{\text{nsf}} + \left( \frac{\text{d}\sigma_{y}}{\text{d}\Omega} \right)_{\text{nsf}} - \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega} \right)_{\text{sf}} \right)

   I &= \frac{3}{2} \left( \frac{ \left( \frac{\text{d}\sigma_{x}}{\text{d}\Omega} \right)_{\text{nsf}} - \left( \frac{\text{d}\sigma_{y}}{\text{d}\Omega} \right)_{\text{nsf}}}{\text{cos}^{2}{\alpha} - \text{sin}^{2}{\alpha}} + \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega} \right)_{\text{sf}} \right)

   M_{\perp \text{y}} &= \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega} \right)_{\text{sf}} - \frac{2}{3} I

   M_{\perp \text{z}} &= \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega} \right)_{\text{nsf}} - \frac{1}{3} I - N

   M_{\text{av}} &= \frac{1}{2} \left( M_{\perp \text{y}} + M_{\perp \text{z}} \right),

where :math:`\alpha` is the Sharpf angle, which for elastic scattering is equal to half of the (signed) in-plane scattering angle and :math:`\theta_{0}` is an experimentally fixed offset (see more in Ref. [#Sharpf]_).


3. 10-point

The 10-point method is an expansion of the XYZ method, that requires measurements of spin-flip and non-spin-flip cross-sections along three orthogonal axes as in the XYZ and two additional axes that are rotated by 45 degrees around the Z axis, labelled `'x-y'` and `'x+y'`. Similarly to the XYZ method, it is possible to separate magnetic cross-section from nuclear coherent and spin-incoherent ones, and additionally the method offers a possibility to separate the magnetic cross-section and the term dependent on azimuthal angle.

.. math::

   \alpha = \theta - \theta_{0} - \frac{\pi}{2},

where :math:`\alpha` is the Sharpf angle, which for elastic scattering is equal to half of the (signed) in-plane scattering angle and :math:`\theta_{0}` is an experimentally fixed offset (see more in Ref. [#Sharpf]_).

.. math::

   M_{1} &= (2 c_{0} - 4) \cdot \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + (2c_{0} + 2) \cdot \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2-4c_{0}) \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}}

   M_{2} &= (2 c_{4} - 4) \cdot \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2c_{4} + 2) \cdot \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2-4c_{4}) \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}}

   M &= M_{1} \cdot \text{cos}(2\alpha) + M_{2} \cdot \text{sin}(2\alpha),

where :math:`c_{0} = \text{cos}^{2} \alpha` and :math:`c_{4} = \text{cos}^{2} (\alpha - \frac{\pi}{4})`

.. math::

   N = \frac{1}{12} \cdot \left(2 \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{nsf}} \right) \right. \\ - \left. \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}d\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{sf}} \right) \right)

.. math::

   I = \frac{1}{4} \cdot \left(\left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}d\Omega}\right)_{\text{sf}} + 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{sf}} - M \right)


Sample data normalisation
#########################

The sample data normalisation is the final step of data reduction of D7 sample, and allows to simultaneously correct for detector efficiency and set the output to the absolute scale.

There are three options for the normalisation; it uses either the input from a reference sample with a well-known cross-section, namely vanadium, or the output from the cross-section separation, either magnetic or spin-incoherent cross-sections. A relative normalisation of the sample workspace to the detector with the highest counts is always performed.

.. math::

   S (\#, \pm \text{, chn/t/meV)} = I (\#, \pm \text{, chn/t/meV)} \cdot D (\#, \pm \text{, chn/t/meV)},

where `I` is the sample intensity distribution corrected for all effects, and `D` is the normalisation factor.

1. Vanadium

If the data is to be expressed in absolute units, the normalisation factor is the reduced vanadium data, normalised by the number of moles of the sample material :math:`N_{S}`:

.. math:: D (\#, \pm \text{, chn/t/meV)} = \frac{1}{N_{S}} \cdot V^{-1} (\#, \pm \text{, chn/t/meV)}

If data is not to be expressed in absolute units, the normalisation factor depends only on the vanadium input:

.. math:: D (\#, \pm \text{, chn/t/meV)} = V^{-1} (\#, \pm \text{, chn/t/meV)}

2. Paramagnetic

This normalisation is not valid for TOF data, and requires input from XYZ or 10-point cross-section separation. The paramagnetic measurement does not need to have background subtracted, as the background is self-subtracted in an XYZ measurement.

.. math:: D (\#) = \frac{2}{3} \frac{(\gamma r_{0})^{2} S(S+1)}{P (\#)},

where :math:`\gamma` is the neutron gyromagnetic ratio, :math:`r_{0}` is the electron's classical radius, and S is the spin of the sample.

3. Spin-incoherent

Similarly to the paramagnetic normalisation, it is also not valid for TOF data, and requires input from XYZ or 10-point cross-section separation. This normalisation assumes that the spin-incoherent contribution is isotropic and flat in :math:`Q`.

The data can be put on absolute scale if the nuclear-spin-incoherent (NSI) cross-section for the sample is known, then:

.. math:: D (\#) = \frac{\text{d} \sigma_{NSI}}{\text{d} \Omega} \frac{1}{NSI (\#)}.

If only the detector efficiency is to be corrected, then it is sufficient to use only the nuclear-spin-incoherent cross-section input:

.. math:: D (\#) = \frac{\text{max}(NSI (\#))}{NSI (\#)}.


Usage
-----
.. include:: ../usagedata-note.txt

**Example - D7AbsoluteCrossSections - XYZ cross-section separation of vanadium data**

.. testcode:: ExD7AbsoluteCrossSections_XYZ_separation

   sampleProperties = {'SampleMass': 2.932, 'FormulaUnitMass': 50.942}

   Load('ILL/D7/vanadium_xyz.nxs', OutputWorkspace='vanadium_xyz') # loads already reduced data
   D7AbsoluteCrossSections(
	      InputWorkspace='vanadium_xyz',
	      CrossSectionSeparationMethod='XYZ',
              SampleAndEnvironmentProperties=sampleProperties,
	      OutputTreatment='Merge',
	      OutputWorkspace='xyz',
	      OutputUnits='TwoTheta')
   print("Number of separated cross-sections: {}".format(mtd['xyz'].getNumberOfEntries()))
   Integration(InputWorkspace=mtd['xyz'][1], OutputWorkspace='sum_coherent')
   Integration(InputWorkspace=mtd['xyz'][2], OutputWorkspace='sum_incoherent')
   Divide(LHSWorkspace='sum_incoherent', RHSWorkspace='sum_coherent', OutputWorkspace='ratio')
   print("Ratio of spin-incoherent to nuclear coherent cross-sections measured for vanadium is equal to: {0:.0f}".format(mtd['ratio'].readY(0)[0]))

Output:

.. testoutput:: ExD7AbsoluteCrossSections_XYZ_separation

   Number of separated cross-sections: 6
   Ratio of spin-incoherent to nuclear coherent cross-sections measured for vanadium is equal to: 170

.. testcleanup:: ExD7AbsoluteCrossSections_XYZ_separation

   mtd.clear()


**Example - D7AbsoluteCrossSections - Sample normalisation to vanadium data**

.. testcode:: ExD7AbsoluteCrossSections_vanadium_normalisation

   sampleProperties = {'SampleMass': 2.932, 'FormulaUnitMass': 182.54}

   Load('ILL/D7/396993_reduced.nxs', OutputWorkspace='vanadium_input')
   GroupWorkspaces(InputWorkspaces='vanadium_input', OutputWorkspace='vanadium_data')
   Load('ILL/D7/397004_reduced.nxs', OutputWorkspace='sample_data')
   D7AbsoluteCrossSections(InputWorkspace='sample_data', OutputWorkspace='normalised_sample_vanadium',
                           CrossSectionSeparationMethod='XYZ', NormalisationMethod='Vanadium',
			   SampleAndEnvironmentProperties=sampleProperties,
                           VanadiumInputWorkspace='vanadium_data', AbsoluteUnitsNormalisation=False)
   print("The number of entries in the normalised data is: {}".format(mtd['normalised_sample_vanadium'].getNumberOfEntries()))


Output:

.. testoutput:: ExD7AbsoluteCrossSections_vanadium_normalisation

   The number of entries in the normalised data is: 6

.. testcleanup:: ExD7AbsoluteCrossSections_vanadium_normalisation

   mtd.clear()


**Example - D7D7AbsoluteCrossSections - Sample normalisation to paramagnetic cross-section**

.. testcode:: ExD7AbsoluteCrossSections_paramagnetic_normalisation

   sampleProperties = {'SampleMass': 2.932, 'FormulaUnitMass': 182.54, 'SampleSpin':0.5}

   Load('ILL/D7/397004_reduced.nxs', OutputWorkspace='sample_data')
   D7AbsoluteCrossSections(InputWorkspace='sample_data', OutputWorkspace='normalised_sample_magnetic',
                           CrossSectionSeparationMethod='XYZ', NormalisationMethod='Paramagnetic',
                           SampleAndEnvironmentProperties=sampleProperties, AbsoluteUnitsNormalisation=False)
   print("The number of entries in the normalised data is: {}".format(mtd['normalised_sample_magnetic'].getNumberOfEntries()))

Output:

.. testoutput:: ExD7AbsoluteCrossSections_paramagnetic_normalisation

   The number of entries in the normalised data is: 6

.. testcleanup:: ExD7AbsoluteCrossSections_paramagnetic_normalisation

   mtd.clear()


**Example - D7D7AbsoluteCrossSections - Single crystal sample XYZ cross-section separation**

.. testcode:: ExD7AbsoluteCrossSections_single_crystal

   sampleProperties = {'SampleMass': 2.932, 'FormulaUnitMass': 182.54, 'OmegaShift': 0.0, 'KiXAngle': 45.0}

   Load('ILL/D7/399870_400288_by_25.nxs', OutputWorkspace='sample_data')
   D7AbsoluteCrossSections(
     InputWorkspace='sample_data',
     OutputWorkspace='sample_data_qxy',
     CrossSectionSeparationMethod='XYZ',
     NormalisationMethod='None',
     OutputUnits='Qxy',
     SampleAndEnvironmentProperties=sampleProperties,
     AbsoluteUnitsNormalisation=False,
     IsotropicMagnetism=True,
     MeasurementTechnique='SingleCrystal',
     ClearCache=True
   )
   print("The number of entries in the output data is: {}".format(mtd['sample_data_qxy'].getNumberOfEntries()))

Output:

.. testoutput:: ExD7AbsoluteCrossSections_single_crystal

   The number of entries in the output data is: 6

.. testcleanup:: ExD7AbsoluteCrossSections_single_crystal

   mtd.clear()


**Example - D7D7AbsoluteCrossSections - Time-of-flight measurement with Z-only cross-section separation**

.. testcode:: ExD7AbsoluteCrossSections_tof_z-only

   sample_dictionary_H2O = {'SampleMass':0.874, 'FormulaUnitMass':18.0, 'SampleChemicalFormula':'H2O'}

   Load('ILL/D7/395639_reduced.nxs', OutputWorkspace='h2O_ws')
   D7AbsoluteCrossSections(
     InputWorkspace='h2O_ws',
     OutputWorkspace='h2O_reduced',
     CrossSectionSeparationMethod='Z',
     NormalisationMethod='None',
     OutputUnits='Default',
     SampleAndEnvironmentProperties=sample_dictionary_H2O,
     AbsoluteUnitsNormalisation=True,
     MeasurementTechnique='TOF',
     ClearCache=True,
     QBinning='-5,0.04,1.5'
   )
   print("The number of entries in the output data is: {}".format(mtd['h2O_reduced'].getNumberOfEntries()))

Output:

.. testoutput:: ExD7AbsoluteCrossSections_tof_z-only

   The number of entries in the output data is: 9

.. testcleanup:: ExD7AbsoluteCrossSections_single_crystal

   mtd.clear()

References
----------

.. [#Sharpf] Scharpf, O. and Capellmann, H.
   *The XYZ‐Difference Method with Polarized Neutrons and the Separation of Coherent, Spin Incoherent, and Magnetic Scattering Cross Sections in a Multidetector*
   Physica Status Solidi (A) **135** (1993) 359-379
   `doi: 10.1002/pssa.2211350204 <https://doi.org/10.1002/pssa.2211350204>`_

.. [#Steward] Stewart, J. R. and Deen, P. P. and Andersen, K. H. and Schober, H. and Barthelemy, J.-F. and Hillier, J. M. and Murani, A. P. and Hayes, T. and Lindenau, B.
   *Disordered materials studied using neutron polarization analysis on the multi-detector spectrometer, D7*
   Journal of Applied Crystallography **42** (2009) 69-84
   `doi: 10.1107/S0021889808039162 <https://doi.org/10.1107/S0021889808039162>`_

.. [#Ehlers] Ehlers G., Stewart J. R., Wildes A. R., Deen P. P., and Andersen K. H.
   *Generalization of the classical xyz-polarization analysis technique to out-of-plane and inelastic scattering*
   Review of Scientific Instruments **84** (2013), 093901
   `doi: 10.1063/1.4819739 <https://doi.org/10.1063/1.4819739>`_

.. [#Schweika] Schweika W.
    *XYZ-polarisation analysis of diffuse magnetic neutron scattering from single crystals*
    J. Phys.: Conf. Ser. **211** (2010) 012026
    `doi: 10.1088/1742-6596/211/1/012026 <https://doi.org/10.1088/1742-6596/211/1/012026>`_

.. categories::

.. sourcelink::
