
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Measurements using polarized neutrons can be used to identify magnetic and nuclear contribution.
In the case of coherent scattering, nuclear spins generally do not change the spin polarization 
of the neutrons (non spin flip scattering).
For magnetic scattering, the magnetization parallel to the neutron spin direction produces
non spin flip scattering, while magnetization perpendicular to the spin direction will flip
the neutron spin.

When measuring spin flip and non spin flip cross sections, the finite capabilities of the instrument 
will allow a certain percentage of the other component to leak through. This is denoted by the 
flipping ratio :math:`F`. Using subscript "m" for measured and "c" for corrected, the corrected 
intensities are given by [1]_:

.. math::

    I_{NSFc}=\frac{F}{F-1}I_{NSFm}-\frac{1}{F-1}I_{SFm}\\
    I_{SFc}=\frac{F}{F-1}I_{SFm}-\frac{1}{F-1}I_{NSFm}


Given a multidimensional event workspace, this algorithm will output the workspace where events
are multiplied by  :math:`F/(F-1)` (in OutputWorkspace1) and by :math:`1/(F-1)` (in OutputWorkspace2).
Note however that the flipping ratio might be angle dependent. For example, in the case of
type II superconductors, the flux lattice will depolarize the neutrons differently, depending 
on the orientation of the neutron beam with respect to the superconducting planes. In this case
one must use a formula that would use a different flipping ratio for each neutron, that
depends on a goniometer angle. Assuming that we have a log value "omega" for the sample rotation,
the flipping ratio might be described by something like [2]_:

.. math::

    F=6.5+2.8\cos(\pi(omega+3.7)/180)



.. [1] O. Schärpf, *Experience with spin analysis on a time-of-flight multidetectorscatteringinstrument*, In Neutron scattering in the ‘nineties’, Proceedings of the IAEA Conference, Jülich 85-97 (1985)

.. [2] Igor A Zaliznyak et al, *Polarized neutron scattering on HYSPEC: the HYbrid SPECtrometer at SNS*, J. Phys.: Conf. Ser. 862 012030 (2017) `doi:10.1088/1742-6596/862/1/012030 <https://doi.org/10.1088/1742-6596/862/1/012030>`__


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - FlippingRatioCorrectionMD**

.. testcode:: FlippingRatioCorrectionMDExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = FlippingRatioCorrectionMD()

   # Print the result
   print "The output workspace has %%i spectra" %% wsOut.getNumberHistograms()

Output:

.. testoutput:: FlippingRatioCorrectionMDExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

