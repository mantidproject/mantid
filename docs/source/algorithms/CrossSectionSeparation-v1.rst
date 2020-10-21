
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that performs magnetic, nuclear coherent, and spin-incoherent cross-section separation for polarised diffraction and spectroscopy data. 
Three types of analysis are supported: `uniaxial`, `XYZ`, and `10p`, for which 2, 6, and 10 distributions with spin-flip and non-spin-flip cross-sections
are required. The expected input is a workspace group containing spin-flip and non-spin-flip cross-sections, with the following order of axis directions:
Z, Y, X, X-Y, X+Y.

To achieve the best results, the input data should be fully corrected for detector effects and efficiencies.

Cross-section separation method
###############################

Below are presented formulae used to separate magnetic (M), nuclear coherent (N), and spin-incoherent (I) cross-sections using
spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{sf}}` and non-spin-flip :math:`\left(\frac{d\sigma}{d\Omega}\right)_{\text{nsf}}`
cross-sections from the provided input :ref:`WorkspaceGroup <WorkspaceGroup>`. 

1. Uniaxial

.. math::

      N = 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}}

      I = 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} - \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}}

      
In this case, the magnetic cross-section cannot be separated from data.

2. XYZ

.. math::

      M = 2 \cdot \left(2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{d\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} - \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} \right)

      N = \frac{1}{6} \cdot \left(2 \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{d\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} \right)

      - \left( \left( \frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left( \frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} \right) \right)

      I = \frac{1}{2} \cdot \left(\left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} - M \right)

3. 10-point

.. math::

   \alpha = \theta - \theta_{0} - \frac{\pi}{2},

where :math:`\alpha` is the Sharpf angle, which for elastic scattering is equal to half of the (signed) in-plane scattering angle and :math:`\theta_{0}` is an experimentally fixed offset (see more in Ref. [3]).



.. math:: M_{1} = (2 c_{0} - 4) \cdot \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + (2c_{0} + 2) \cdot \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2-4c_{0}) \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}}

.. math:: M_{2} = (2 c_{4} - 4) \cdot \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2c_{4} + 2) \cdot \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{nsf}} + (2-4c_{4}) \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}}

.. math:: M = M_{1} \cdot \text{cos}(2\alpha) + M_{2} \cdot \text{sin}(2\alpha),

where :math:`c_{0} = \text{cos}^{2} \alpha` and :math:`c_{4} = \text{cos}^{2} (\alpha - \frac{\pi}{4})`

.. math:: N = \frac{1}{12} \cdot \left(2 \cdot \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{nsf}} + 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{nsf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{nsf}} \right)

      - \left( \left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}d\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{sf}} \right) \right)

.. math:: I = \frac{1}{4} \cdot \left(\left(\frac{\text{d}\sigma_{x}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{y}}{\text{d}d\Omega}\right)_{\text{sf}} + 2 \cdot \left(\frac{\text{d}\sigma_{z}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x+y}}{\text{d}\Omega}\right)_{\text{sf}} + \left(\frac{\text{d}\sigma_{x-y}}{\text{d}\Omega}\right)_{\text{sf}} - M \right)


Usage
-----
.. include:: ../usagedata-note.txt

**Example - CrossSectionSeparation - XYZ component separation of vanadium data**

.. testcode:: ExCrossSectionSeparationXYZ

      Load('vanadium_xyz.nxs', OutputWorkspace='vanadium_xyz')
      CrossSectionSeparation(InputWorkspace='vanadium_xyz', CrossSectionSeparationMethod='XYZ',
	      OutputWorkspace='xyz')
      print("Number of separated cross-sections: {}".format(mtd['xyz'].getNumberOfEntries())
      sum_incoherent = SumSpectra(InputWorkspace=mtd['xyz'][0], EndWorkspaceIndex=mtd['xyz'][1].getNumberHistograms()-1)
      sum_coherent = SumSpectra(InputWorkspace=mtd['xyz'][0], EndWorkspaceIndex=mtd['xyz'][0].getNumberHistograms()-1)
      Divide(LHSWorkspace=sum_incoherent, RHSWorkspace=sum_coherent, OutputWorkspace='ratio')
      print("Ratio of nuclear coherent to spin-incoherent cross-sections measured for vanadium is equal to:{0:.1f}".format(mtd['ratio'].readY(0)[0])

Output:

.. testoutput:: ExCrossSectionSeparationXYZ
	Number of separated cross-sections: 3
        Ratio of nuclear coherent to spin-incoherent cross-sections measured for vanadium is equal to: 11.8

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
