.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm uses the Monte Carlo simulation (originally designed for calculating absorption corrections) to
calculate the absorption weighted path length for each peak in a peaks workspace. The absorption weighted path
length is used to calculate extinction corrections in single crystal diffraction.

The absorption weighted path length is also referred to as "t-bar" in some literature.

The definition of the absorption weighted path length (in cm) for each peak is:

:math:`\bar{t}= -100 * \frac{log{A}}{\mu(\lambda)}`

Where A is the average attenuation factor for all simulated tracks and :math:`\mu` is the attenuation coefficient (in :math:`m^{-1}`) for the sample material.

The algorithm requires an input workspace with a sample defined but no sample environment. The sample must have a material and a shape defined.

By default the beam is assumed to be the a slit with width and height matching
the width and height of the sample. This can be overridden using :ref:`SetBeam <algm-SetBeam>`.

The algorithm generates some statistics on the scattering angle for the simulated tracks when the log level is set to debug.

There is an option to apply the absorption correction (`ApplyCorrection=True`) to the peak intensities.

Method
######

The material for the sample defines the values of the cross section used to compute the attenuation coefficient and will
include contributions from both the total scattering cross section & absorption cross section.
This follows the Hamilton-Darwin [#DAR]_, [#HAM]_ approach as described by T. M. Sabine in the International Tables of Crystallography Vol. C [#SAB]_.

The algorithm for calculating the attenuation factor A proceeds as follows. For each peak:

#. look up the associated detector position and wavelength in the peaks workspace

#. for each event in `NEvents`

   * generate a random point on the beam face defined by the input height & width. If the point is outside of the
     area defined by the face of the sample then it is pulled to the boundary of this area

   * generate a random point within the sample object as the scatter point and create a `Track`
     from the selected position on the beam face to the scatter point

   * test for intersections of the track & sample object, giving the number of subsections
     and corresponding distances within the object for each section, call them :math:`l_{1i}`. There will typically
     be a single intersection and subsection but for complex sample shapes it could in theory be more

   * form a second `Track` with the scatter position as the starting point and the direction defined by
     `detPos - scatterPos`

   * test for intersections of the track & sample object, giving the number of subsections
     and corresponding distances within the object for each section, call them :math:`l_{2i}`

   * assume elastic scattering so that the wavelength is the same before and after scattering

   * compute the self-attenuation factor for all intersections as
       :math:`\prod\limits_{i} \exp(-(\mu(\lambda)l_{1i} + \mu(\lambda)l_{2i}))`
       where :math:`\mu` is the attenuation coefficient of the sample material

   * accumulate this wavelength-specific factor across all `NEvents`

#. average the accumulated attentuation factors over `NEvents` and assign this as the correction factor for
   this :math:`\lambda`.


Usage
-----

**Example: A simple cylindrical sample**

.. testcode:: ExSimpleCylinder

    # load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')

    SetSample(peaks,Geometry={'Shape': 'Cylinder','Height': 5.0,'Radius': 1.0,'Center': [0.,0.,0.]},
                              Material={'ChemicalFormula': 'V'})

    # populate the t bar column in the peaks workspace
    AddAbsorptionWeightedPathLengths(peaks)

    print("Tbar for first peak {:.11f} cm".format(peaks.getPeak(0).getAbsorptionWeightedPathLength()))

Output:

.. testoutput:: ExSimpleCylinder

    Tbar for first peak 1.56626404089 cm

References
----------

.. [#DAR] Darwin, C. G., *Philos. Mag.*, **43** 800 (1922)
          `doi: 10.1080/10448639208218770 <http://dx.doi.org/10.1080/10448639208218770>`_
.. [#HAM] Hamilton, W.C., *Acta Cryst*, **10**, 629 (1957)
          `doi: 10.1107/S0365110X57002212 <http://dx.doi.org/10.1107/S0365110X57002212>`_
.. [#SAB] Sabine, T. M., *International Tables for Crystallography*, Vol. C, Page 609, Ed. Wilson, A. J. C and Prince, E. Kluwer Publishers (2004)
          `doi: 10.1107/97809553602060000103 <http://dx.doi.org/10.1107/97809553602060000103>`_

.. categories::

.. sourcelink::