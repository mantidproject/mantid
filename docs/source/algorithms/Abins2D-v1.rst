.. algorithm::

Credits
-------

Author:
    Adam Jackson

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Abins2D is a plugin for Mantid which allows scientists to generate theoretical inelastic neutron scattering spectra (INS) in 2-D :math:`(|\mathbf{q}|,\omega)` space, accounting for the limitations of real-world measurements.

The intensity is calculated in the "almost-isotropic" incoherent powder-averaging approximation, as used in the 1-D :ref:`Abins <algm-Abins>` algorithm.
Different 2-D INS instruments may be selected, along with instrument parameters (incident energy, chopper settings) that determine the accessible :math:`(|\mathbf{q}|,\omega)` region and energy resolution.
More information about the implemented working equations can be found :ref:`here <DynamicalStructureFactorFromAbInitio>`.

Abins requires data from existing lattice dynamics calculations to determine the phonon mode frequencies and displacements. These are used to compute INS intensities; higher-order phonons may be approximated as combinations of fundamental modes.


Several file formats are supported for frequency and displacement data at given :math:`\mathbf{q}`-points. Alternatively, force-constants data can be provided and a fine :math:`\mathbf{q}`-point mesh will be sampled automatically.

Supported codes include CASTEP (.phonon), CRYSTAL (.out), DMOL3 (.outmol), GAUSSIAN (.log), PHONOPY (.yaml), VASP (.xml) as well as some pre-processed JSON (.json) formats. For more information about the files, features and quirks of theskee codes see :ref:`AbinsDataFormats`.

For VASP XML inputs, it is also permitted to use a "selective dynamics" in which some atoms are frozen.
In this case, data is only available for the moving atoms and contributions from the other atoms are omitted from the calculated spectrum.
This may be a suitable model for systems where the frozen atoms form a rigid substrate (e.g. a noble metal surface) for an adsorbed molecule of lightweight atoms.
It is not recommended where there is significant vibrational coupling between frozen and moving atoms, or where substrate modes would occupy a similar frequency range to the adsorbate.

In this case the supported formats are CASTEP (.castep_bin generated with ``phonon_write_force_constants = True``)
and Phonopy (*phonopy.yaml* file generated with ``INCLUDE_ALL = .TRUE.``)
It is also permitted to use a phonopy YAML file which does not contain the force constants, if force constants are stored in a ``FORCE_CONSTANTS`` or ``force_constants.hdf5`` file in the same directory. This is less convenient but may result in significantly faster loading if the force constant matrix is large.

After a successful run, results are written to a Mantid Workspace Group; by default this is divided by element and quantum order.
The user can also request contributions from individual atoms (e.g. 'atom_1', the first atom listed in the input
data) or types of atom (for example for benzene two element symbols: C, H).

Abins2D is under active development; please direct feature requests and bug reports to Dr. Sanghamitra Mukhopadhyay and Dr. Adam Jackson.
If you are developing or modifying Abins(2D), see the :ref:`AbinsImplementation` notes which outline some of the key conventions, practices and pitfalls.

If Abins is used as part of your data analysis routines, please cite the relevant reference [1]_.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading CASTEP phonon data:**

A minimal example, relying heavily on default parameters:

.. testsetup:: Abins2DCastepSimple

    from tempfile import TemporaryDirectory
    from mantid.kernel import ConfigService

    test_dir = TemporaryDirectory()

    initial_defaultsave = ConfigService.getString("defaultsave.directory")
    ConfigService.setString("defaultsave.directory", test_dir.name)

.. testcode:: Abins2DCastepSimple

    benzene_wrk = Abins2D(AbInitioProgram="CASTEP", VibrationalOrPhononFile="benzene.phonon")


    for name in benzene_wrk.getNames():
        print(name)

Output: (note that only the fundamental excitations are included)

.. testoutput:: Abins2DCastepSimple

    benzene_wrk_C_total
    benzene_wrk_C
    benzene_wrk_H_total
    benzene_wrk_H

.. testcleanup:: Abins2DCastepSimple

    test_dir.cleanup()
    ConfigService.setString("defaultsave.directory", initial_defaultsave)

**Example - using more arguments:**

In practice we would usually select an instrument, incident energy,
and enable all available quantum orders (2 + autoconvolution). Setting
"Total" cross-section applies the "incoherent approximation"; although
Abins only (so far) calculates the incoherent contribution, coherent
weights can be added for a slight improvement to the predicted
spectrum.  (If the spectrum is dominated by coherent scattering, this
approximation may not be the appropriate tool.)

.. testsetup:: Abins2DExplicitParameters

    from tempfile import TemporaryDirectory
    from mantid.kernel import ConfigService

    test_dir = TemporaryDirectory()

    initial_defaultsave = ConfigService.getString("defaultsave.directory")
    ConfigService.setString("defaultsave.directory", test_dir.name)

.. testcode:: Abins2DExplicitParameters

    wrk_verbose=Abins2D(AbInitioProgram="CASTEP", VibrationalOrPhononFile="benzene.phonon",
                        TemperatureInKelvin=10, Instrument="MAPS",
                        Atoms="H, atom1, atom2", SumContributions=True,
                        QuantumOrderEventsNumber="2", Autoconvolution=True,
                        ScaleByCrossSection="Total")

    print(f"Created {wrk_verbose.size()} workspaces")
    print(f"including {wrk_verbose[12].name()}")

Output:

.. testoutput:: Abins2DExplicitParameters

   Created 34 workspaces
   including wrk_verbose_atom_1_total

.. testcleanup:: Abins2DExplicitParameters

    test_dir.cleanup()
    ConfigService.setString("defaultsave.directory", initial_defaultsave)

.. categories::

.. sourcelink::
  :cpp: None
  :h: None


References
----------

.. [1] K. Dymkowski, S. F. Parker, F. Fernandez-Alonso and S. Mukhopadhyay,  “AbINS: The modern software for INS interpretation” , Physica B, doi:10.1016/j.physb.2018.02.034 (2018).
