.. _Crystal structure and reflections:

Crystal structure and reflections
=================================

This document describes how crystal structure data can be processed and used in Mantid. For the understanding of the concepts of :ref:` <Symmetry groups>` and :ref:`space groups <Point and space groups>` in Mantid it may be useful to read those introductory articles before proceeding with this document. While there is a short introduction into theoretical aspects this page is not a replacement for proper text books on the subject.

Theoretical aspects
~~~~~~~~~~~~~~~~~~~

Crystal structures
------------------

A crystal is modelled as an infinitely repeating three-dimensional arrangement of scatterers, usually atoms. Due to the periodic nature of crystals, the entire object can be described by specifying the repeated unit and how it is repeated. These information are called "crystal structure" and comprise three components:

  1. Lattice (describes the periodicity of the structure)
  2. Basis (distribution of scatterers in the unit cell)
  3. Space group (describes the symmetry of the arrangement of scatterers)
  
The description of the basis depends on the model's level of detail. In the simplest case it could be a list of point scatterers that are fully characterized by three coordinates (x, y and z in terms of the lattice vectors) and a scattering length. In reality however, the scatterers are usually atoms that fluctuate about their average position due to thermal agitation. A basic way to model this motion is to assume it to be an isotropic phenomenon, allowing the description with one single parameter that quantifies the radius of a sphere in which the scatterer oscillates harmonically. This so called Debye-Waller-factor will be introduced later on.

Another important parameter for a basic description of the basis is the so called occupancy. It describes the fraction of the total number of scatterer-positions that is actually occupied. A common example where this is required are unordered binary metal mixtures where the same position in the crystal structure is partly filled with two different atoms in a randomly distributed manner.

To summarize, a very basic model of the basis comprises a list of scatterers that are in turn characterized by six parameters:

  1. Scattering length (known for each element)
  2. Fractional coordinate x
  3. Fractional coordinate y
  4. Fractional coordinate z
  5. Occupancy of the site
  6. Isotropic thermal displacement parameter
  
Knowledge of the space group makes it possible to derive all scatterers in the entire unit cell. The symmetry operations of the space group map each scatterer-position onto a set of equivalent positions that are consequently occupied by the same type of scatterer as well. Since the unit cell is repeated infinitely in all three directions of space, it is enough to describe one unit cell. Finally, the lattice is described by six parameters as well, the lengths of the three lattice vectors (usually given in Angström) and the three angles (in degree) between these vectors.

Reflections and structure factors
---------------------------------

In a diffraction experiment the periodic arrangement of atoms is probed using radiation, in this case in the form of neutrons, of an appropriate wavelength (on the same scale of interatomic distances, typically between 0.5 and 5 Angström). The incident beam interacts with the scatterers and in certain orientations the beam is "reflected" by a flock of lattice planes, a phenomenon which is described by Bragg's law:

  2d sin theta = lambda
  
In this equation d is the spacing between the lattice planes, theta is the angle of the incoming beam and the lattice plane normal and lambda is the wavelength of the radiation. In an experiment theta and lambda are usually limited, thus they are limiting the range of interplanar spacings that can be probed. In Bragg's law the lattice plane families are only described by one parameter, the interplanar distance. But each lattice plane family also has an orientation in space which can be described by the plane's normal vector. Usually the vector is given in terms of the reciprocal lattice of the structure, where it is reduced to three integers H, K, L, the so called Miller indices. With knowledge of the unit cell (and therefor the reciprocal lattice metric tensor), the interplanar spacing can also be computed like this:

  1/d = sqrt(h^T * G* * h)
  
The parameters taken into account so far determine the geometric characteristics of Bragg-reflections, i.e. their position on a detector and their time of flight. But besides these, each reflection also has an intensity. The intensity is proportional to the squared structure factor, which depends on the kind and arrangement of scatterers in the unit cell.
