.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates solid angles from the sample position of the
input workspace for all of the spectra selected. If several detectors
have been mapped to the same spectrum then the solid angles of this
detectors will be summed to provide the solid angle for the spectrum.
The solid angle of a detector that has been masked or marked as dead is
considered to be 0 steradians. For grouped detectors, the solid angles will be summed.

This algorithms can happily accept :ref:`ragged workspace <Ragged_Workspace>`
as an input workspace. The result would
be a ragged output workspace whose X axis values match the lowest and
highest of each the input spectra.

Note: The Solid angle calculation assumes that the path between the
sample and detector is unobstructed by another other instrument
components.

``Method`` Property
-------------------

The method property changes how the solid angle calculation is
performed.
``GenericShape`` uses the ray-tracing methods of :ref:`Instrument`.

All of the others have special analytical forms taken from small angle scattering literature.
Those are fast analytical approximations that are valid in large detector distance and small pixel area limit.
For those equations :math:`2\theta` is the scattering angle (from direct beam rather than in-plane), :math:`\alpha` is the scattering angle constrained in either the x (horizontal) or y (vertical) plane.
The methods below also assume that the instrument has properties ``x-pixel-size`` and ``y-pixel-size`` which define the size of the pixels in millimetres.
A difference between this implementation and what is in literature, is that rather than using the detector elements distance as :math:`L_{2i} = \frac{D}{\cos(2\theta_i)}`, this uses the actual :math:`L_{2i}` and in the equation after proper substitution.

* ``Rectangular``: :math:`d\Omega_i = p_x p_y cos(2\theta_i) / L_{2i}^2`

* ``VerticalTube`` and ``HorizontalTube``: :math:`d\Omega_i = p_x p_y \cos(\alpha_i) / L_2^2`

* ``VerticalWing`` and ``HorizontalWing``: :math:`d\Omega_i = p_x p_y \cos^3(\alpha_i) / (L_2 \cos(2\theta_i))^2`


Usage
-----

**Example:**

.. testcode:: solidAngle

    ws = CreateSampleWorkspace()
    wsOut = SolidAngle(ws)

    print('Solid angle of Spectra 1 in Bank 1: %.2e' % wsOut.readY(0)[0])
    print('Solid angle of Spectra 101 in Bank 2: %.2e' % wsOut.readY(100)[0])


Output:

.. testoutput:: solidAngle

    Solid angle of Spectra 1 in Bank 1: 6.40e-08
    Solid angle of Spectra 101 in Bank 2: 1.60e-08

**Example: BIOSANS**

.. testcode:: biosans

   LoadEmptyInstrument(InstrumentName='BIOSANS', OutputWorkspace='BIOSANS')
   mainDet = SolidAngle(InputWorkspace='BIOSANS', OutputWorkspace='main_detector',
                        Method='VerticalTube', StartWorkspaceIndex=2, EndWorkspaceIndex=49153)
   wingDet = SolidAngle(InputWorkspace='BIOSANS', OutputWorkspace='wing_detector',
                        Method='VerticalWing', StartWorkspaceIndex=49154, EndWorkspaceIndex=90113)
   # both are zero where nothing was calculated
   print('Solid angle where main was not calculated: %.2e' % mainDet.readY(1)[0])
   print('Solid angle where wing was not calculated: %.2e' % wingDet.readY(1)[0])
   # both have values where they were calculated
   print('Solid angle where main was calculated: %.2e' % mainDet.readY(2)[0])
   print('Solid angle where wing was calculated: %.2e' % wingDet.readY(49155)[0])


Output:

.. testoutput:: biosans

    Solid angle where main was not calculated: 0.00e+00
    Solid angle where wing was not calculated: 0.00e+00
    Solid angle where main was calculated: 2.82e-05
    Solid angle where wing was calculated: 1.30e-05


References
----------

The specialzed ``Method`` calculations are based on work summarized in:

#. I. Grillo, *Small-angle neutron scattering and applications in soft condensed matter.* Soft matter characterization (2008): 723-782.

#. Annie Brûlet, *Improvement of data treatment in small-angle neutron scattering.* J. of Appl. Cryst. 40.1 (2007): 165-177 `doi: 10.1107/S0021889806051442 <https://doi.org/10.1107/S0021889806051442>`_


.. categories::

.. sourcelink::
