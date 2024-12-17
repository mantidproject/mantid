.. _Abins Data Formats:

Abins Data Formats
==================

.. list-table:: Title
   :header-rows: 1

  * - Code
    - Data type
    - File extension
    - AbInitioProgram
    - Frozen atoms
  * - CASTEP
    - .phonon
    - modes
    - CASTEP
    -
  * - CASTEP
    - force constants
    - .castep_bin
    - FORCECONSTANTS
    -
  * - CRYSTAL
    - modes
    - .out
    - CRYSTAL
    - N
  * - DMOL3
    - modes
    - .outmol
    - DMOL3
    -
  * - Euphonic
    - modes
    - .json
    - JSON
    - N
  * - Euphonic
    - force constants
    - .json
    - JSON
    - N
  * - Gaussian
    - modes
    - .log
    - GAUSSIAN
    - Y
  * - Phonopy
    - force constants
    - .yaml
    - FORCECONSTANTS
    -
  * - VASP
    - modes
    - .xml
    - VASP
    - Y

CASTEP
------

Writing force constants
~~~~~~~~~~~~~~~~~~~~~~~

To include force constants in a *.castep_bin* file,
set ``phonon_write_force_constants = True``, ``phonon_fine_method = interpolate``
and ensure the ``phonon_mp_grid`` includes the Gamma point (0, 0, 0).
This can be achieved by carefully setting ``phonon_kpoint_mp_offset`` to suitable divisions or,
in recent CASTEP versions, setting it to ``INCLUDE_GAMMA``.

Further information is available in the `castep docs <https://castep-docs.github.io/castep-docs/documentation/Phonons/Castep_Phonons/Running-phonon-calculations/#sec:interpolation-setup>`__.

Isotopes
~~~~~~~~

CASTEP allows arbitrary labels to be assigned to species with corresponding masses and pseudopotentials.
In order to match customised species to on-the-fly-generated pseudopotentials, it is recommended to use the ``EL:tag`` syntax, where e.g. deuterium would be specified as ``H:D`` or ``H:2`` and a corresponding species put in the SPECIES_MASS block.
Abins expects this syntax and uses it to identify appropriate neutron cross-section data.

Some examples are given in the `castep docs <https://castep-docs.github.io/castep-docs/documentation/Phonons/Castep_Phonons/Advanced-Topics/#sec:isotopes>`__.

Phonopy
-------

Abins expects to read force constants from a *phonopy.yaml* file generated with ``INCLUDE_ALL = .TRUE.``.
It is also permitted to use a phonopy YAML file which does not contain the force constants, if the force constants are stored in a *FORCE_CONSTANTS* or *force_constants.hdf5* file in the same directory.
We recommend using the *force_constants.hdf5* file where practical as the read/write performance is significantly better.
(With the power and flexibility of *.yaml* comes a lot of computational overhead to read simple arrays!)

VASP
----
Abins can read the frequencies and displacements from Gamma-point vibration calculations performed with the ``IBRION=5,6,7,8`` tags.
We do not (currently) support the Fourier-interpolated supercell calculations added in VASP 6.4.0 (i.e. using QPOINTS file and PHON_DOS).
We recommend that VASP users use Phonopy for such work, which makes it easier for large calculations to be "task farmed".

From VASP XML inputs, it is permitted to use "selective dynamics" in which some atoms are frozen.
In this case, data is only available for the moving atoms and contributions from the other atoms are omitted from the calculated spectrum.
This may be a suitable model for systems where the frozen atoms form a rigid substrate (e.g. a noble metal surface) for an adsorbed molecule of lightweight atoms.
It is not recommended where there is significant vibrational coupling between frozen and moving atoms, or where substrate modes would occupy a similar frequency range to the adsorbate.
