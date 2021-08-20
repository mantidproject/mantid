
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

Three types of cross-section separation are supported: `Uniaxial`, `XYZ`, and `10p`, for which 2, 6, and 10 distributions with spin-flip and non-spin-flip cross-sections
are required. The expected input is a workspace group containing spin-flip and non-spin-flip cross-sections, with the following order of axis directions:
Z, Y, X, X-Y, X+Y. This step can be skipped by setting `CrossSectionSeparationMethod` parameter to `'None'`.

Three ways of sample data normalisation are supported: `Vanadium`, `Paramagnetic`, and `Incoherent`, for which either the output from vanadium data reduction,
or from the cross-section separation (magnetic and spin-incoherent respectively) is used. This step can also be skipped by setting `NormalisationMethod` parameter
to `'None'`.

This algorithm is indended to be invoked on sample data that is fully corrected and needs to be normalised to the absolute scale.

SampleAndEnvironmentProperties
##############################

This property is a dictionary containing all of the information about the sample and its environment, in the same fashion as in :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>`.
This information is used for proper normalisation of the given sample.

The following keys need to be defined:

- *SampleMass*
- *FormulaUnitMass*
- *SampleSpin* if the `NormalisationMethod` is set to `Paramagnetic`
- *IncoherentCrossSection* if the `NormalisationMethod` is set to `Incoherent` and `AbsoluteUnitsNormalisation` is *True*

Cross-section separation method
###############################

Below are presented formulae used to separate magnetic (M), nuclear coherent (N), and spin-incoherent (I) cross-sections using
spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{sf}}` and non-spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{nsf}}`
cross-sections from the provided input :ref:`WorkspaceGroup <WorkspaceGroup>`.

1. Uniaxial

At least two separate measurements along the same axis with opposite spin orientations are needed for this method to work. Usually, the measured axis is the 'Z' axis that is colinear with the beam axis, thus the spin-flip and non-spin-flip cross-sections are measured along the longitudinal axis. This method does not allow for separation of magnetic cross-section.

.. math::

      N &= \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \frac{1}{2} \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}

      I &= \frac{3}{2} \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}


In this case, the magnetic cross-section cannot be separated from data.

2. XYZ

This method is an expansion of the Uniaxial method, that requires measurements of spin-flip and non-spin-flip cross-sections along three orthogonal axes. This method allows for separation of magnetic cross-section from nuclear coherent and spin-incoherent.

.. math::

      T &= \frac{1}{3} \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}  \right)

      M_{\text{nsf}} &= 2 \cdot \left(2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{d\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} \right)

      M_{\text{sf}} &= 2 \cdot \left(\left(\frac{d\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} -2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right)

      M_{\text{av}} &= \frac{1}{2} \cdot \left( M_{\text{nsf}} + M_{\text{sf}} \right)

      N &= \frac{1}{6} \cdot \left(2 \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} \right) - \left( \left( \frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right) \right)

      I &= \frac{1}{2} \cdot \left(\left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right) - M_{\text{av}}

3. 10-point

The 10-point method is an expansion of the XYZ method, that requires measurements of spin-flip and non-spin-flip cross-sections along three orthogonal axes as in the XYZ and two additional axes that are rotated by 45 degrees around the Z axis, labelled `'x-y'` and `'x+y'`. Similarly to the XYZ method, it is possible to separate magnetic cross-section from nuclear coherent and spin-incoherent ones, and additionally the method offers a possibility to separate the magnetic cross-section and the term dependent on azimuthal angle.

.. math::

   \alpha = \theta - \theta_{0} - \frac{\pi}{2},

where :math:`\alpha` is the Sharpf angle, which for elastic scattering is equal to half of the (signed) in-plane scattering angle and :math:`\theta_{0}` is an experimentally fixed offset (see more in Ref. [3]).

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

.. math:: D (\#, \pm \text{, chn/t/meV)} = \frac{1}{N_{S}} \cdot \frac{1}{V (\#, \pm \text{, chn/t/meV)}}

If data is not to be expressed in absolute units, the normalisation factor depends only on the vanadium input:

.. math:: D (\#, \pm \text{, chn/t/meV)} = \frac{\text{max}(V (\#, \pm \text{, chn/t/meV)})}{V (\#, \pm \text{, chn/t/meV)}}

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
   D7AbsoluteCrossSections(InputWorkspace='vanadium_xyz', CrossSectionSeparationMethod='XYZ',
                           SampleAndEnvironmentProperties=sampleProperties,
                           OutputTreatment='Merge', OutputWorkspace='xyz')
   print("Number of separated cross-sections: {}".format(mtd['xyz'].getNumberOfEntries()))
   Integration(InputWorkspace=mtd['xyz'][1], OutputWorkspace='sum_coherent')
   Integration(InputWorkspace=mtd['xyz'][2], OutputWorkspace='sum_incoherent')
   Divide(LHSWorkspace='sum_incoherent', RHSWorkspace='sum_coherent', OutputWorkspace='ratio')
   print("Ratio of spin-incoherent to nuclear coherent cross-sections measured for vanadium is equal to: {0:.1f}".format(mtd['ratio'].readY(0)[0]))

Output:

.. testoutput:: ExD7AbsoluteCrossSections_XYZ_separation

   Number of separated cross-sections: 6
   Ratio of spin-incoherent to nuclear coherent cross-sections measured for vanadium is equal to: 11.9

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


References
----------

#. Scharpf, O. and Capellmann, H.
   *The XYZ‐Difference Method with Polarized Neutrons and the Separation of Coherent, Spin Incoherent, and Magnetic Scattering Cross Sections in a Multidetector*
   Physica Status Solidi (A) **135** (1993) 359-379
   `doi: 10.1002/pssa.2211350204 <https://doi.org/10.1002/pssa.2211350204>`_

#. Stewart, J. R. and Deen, P. P. and Andersen, K. H. and Schober, H. and Barthelemy, J.-F. and Hillier, J. M. and Murani, A. P. and Hayes, T. and Lindenau, B.
   *Disordered materials studied using neutron polarization analysis on the multi-detector spectrometer, D7*
   Journal of Applied Crystallography **42** (2009) 69-84
   `doi: 10.1107/S0021889808039162 <https://doi.org/10.1107/S0021889808039162>`_

#. G. Ehlers, J. R. Stewart, A. R. Wildes, P. P. Deen, and K. H. Andersen
   *Generalization of the classical xyz-polarization analysis technique to out-of-plane and inelastic scattering*
   Review of Scientific Instruments **84** (2013), 093901
   `doi: 10.1063/1.4819739 <https://doi.org/10.1063/1.4819739>`_

.. categories::

.. sourcelink::
