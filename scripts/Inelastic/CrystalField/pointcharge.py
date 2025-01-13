# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from scipy import constants
import re
import warnings

CHARGES_PATTERN = re.compile(r"^[A-z]+")


class PointCharge(object):
    """
    Class for calculating crystal field parameters under the point-charge model.
    """

    # Expectation values of the radial wavefunction <r^n> for n=2, 4, 6 respectively. From McPhase/Cfield (P. Fabi)
    rns = {
        "Ce": [1.309, 3.964, 23.31],
        "Pr": [1.1963, 3.3335, 18.353],
        "Nd": [1.114, 2.910, 15.03],
        "Pm": [1.0353, 2.5390, 12.546],
        "Sm": [0.9743, 2.260, 10.55],
        "Eu": [0.9175, 2.020, 9.039],
        "Gd": [0.8671, 1.820, 7.831],
        "Tb": [0.8220, 1.651, 6.852],
        "Dy": [0.7814, 1.505, 6.048],
        "Ho": [0.7446, 1.379, 5.379],
        "Er": [0.7111, 1.270, 4.816],
        "Tm": [0.6804, 1.174, 4.340],
        "Yb": [0.6522, 1.089, 3.932],
    }

    # Normalisation factors (pre-factors) of the tesseral harmonics. From McPhase.
    Zlm = [
        [1 / np.sqrt(4 * np.pi)],
        [np.sqrt(3.0 / 4 / np.pi), np.sqrt(3.0 / 4 / np.pi)],
        [np.sqrt(5.0 / np.pi) / 4, np.sqrt(15.0 / np.pi) / 2, np.sqrt(15.0 / np.pi) / 4],
        [np.sqrt(7.0 / 16 / np.pi), np.sqrt(21.0 / 32 / np.pi), np.sqrt(105.0 / 16 / np.pi), np.sqrt(35.0 / 32 / np.pi)],
        [
            3.0 / np.sqrt(np.pi) / 16,
            3.0 * np.sqrt(5.0 / 2 / np.pi) / 4,
            3.0 * np.sqrt(5.0 / np.pi) / 8,
            3.0 * np.sqrt(70.0 / np.pi) / 8,
            3.0 * np.sqrt(35.0 / np.pi) / 16,
        ],
        [
            np.sqrt(11.0 / 256 / np.pi),
            np.sqrt(165.0 / 256 / np.pi),
            np.sqrt(1155.0 / 64 / np.pi),
            np.sqrt(385.0 / 512 / np.pi),
            np.sqrt(3465.0 / 256 / np.pi),
            np.sqrt(693.0 / 512 / np.pi),
        ],
        [
            np.sqrt(13 / np.pi) / 32,
            np.sqrt(273.0 / 4 / np.pi) / 8,
            np.sqrt(2730.0 / np.pi) / 64,
            np.sqrt(2730.0 / np.pi) / 32,
            21.0 * np.sqrt(13.0 / 7 / np.pi) / 32,
            np.sqrt(9009.0 / 512 / np.pi),
            231.0 * np.sqrt(26.0 / 231 / np.pi) / 64,
        ],
    ]

    # Stevens factor for trivalent rare-earths. From McPhase/Cfield (P. Fabi)
    theta = {
        "Ce": [-2.0 / 5 / 7, 2.0 / 3 / 3 / 5 / 7, 0],
        "Pr": [
            -2.0 * 2 * 13 / 3 / 3 / 5 / 5 / 11,
            -2.0 * 2 / 3 / 3 / 5 / 11 / 11,
            2.0 * 2 * 2 * 2 * 17 / 3 / 3 / 3 / 3 / 5 / 7 / 11 / 11 / 13,
        ],
        "Nd": [
            -7.0 / 3 / 3 / 11 / 11,
            -2.0 * 2 * 2 * 17 / 3 / 3 / 3 / 11 / 11 / 11 / 13,
            -5.0 * 17 * 19 / 3 / 3 / 3 / 7 / 11 / 11 / 11 / 13 / 13,
        ],
        "Pm": [
            2.0 * 7 / 3 / 5 / 11 / 11,
            2.0 * 2 * 2 * 7 * 17 / 3 / 3 / 3 / 5 / 11 / 11 / 11 / 13,
            2.0 * 2 * 2 * 17 * 19 / 3 / 3 / 3 / 7 / 11 / 11 / 11 / 13 / 13,
        ],
        "Sm": [13.0 / 3 / 3 / 5 / 7, 2.0 * 13 / 3 / 3 / 3 / 5 / 7 / 11, 0],
        "Eu": [0, 0, 0],
        "Gd": [0, 0, 0],
        "Tb": [-1.0 / 3 / 3 / 11, 2.0 / 3 / 3 / 3 / 5 / 11 / 11, -1.0 / 3 / 3 / 3 / 3 / 7 / 11 / 11 / 13],
        "Dy": [-2.0 / 3 / 3 / 5 / 7, -2.0 * 2 * 2 / 3 / 3 / 3 / 5 / 7 / 11 / 13, 2.0 * 2 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13],
        "Ho": [-1.0 / 2 / 3 / 3 / 5 / 5, -1.0 / 2 / 3 / 5 / 7 / 11 / 13, -5.0 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13],
        "Er": [2.0 * 2 / 3 / 3 / 5 / 5 / 7, 2.0 / 3 / 3 / 5 / 7 / 11 / 13, 2.0 * 2 * 2 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13],
        "Tm": [1.0 / 3 / 3 / 11, 2.0 * 2 * 2 / 3 / 3 / 3 / 3 / 5 / 11 / 11, -5.0 / 3 / 3 / 3 / 3 / 7 / 11 / 11 / 13],
        "Yb": [2.0 / 3 / 3 / 7, -2.0 / 3 / 5 / 7 / 11, 2.0 * 2 / 3 / 3 / 3 / 7 / 11 / 13],
    }

    def __init__(self, *args, **kwargs):
        """
        Constructor.
        @param Structure: either a Mantid CrystalStructure object (from mantid.geometry)
                          or workspace with valid attached crystal structure
                          or a CIF file name (string)
                          or an explicit list of ligands, each element being: [charge, xpos, ypos, zpos]
        @param IonLabel: the magnetic ion name in the structure.
        @param Charges: a dictionary of ion names and associated charges in units of |e|.
        @param Ion: (optional) the rare earth ion (if different [apart from index] from IonLabel)
        @param MaxDistance: (optional) the maximum distance to calculate charges up to (in Angstrom) [default: 5.0]
        @param Neighbour: (optional) an integer indicating the nth neighbour to calculate charges up to

        The above parameters can be given as arguments or keyword arguments.
        For the 'Structure' argument, Mantid workspaces may be specified as either references or string names
        If a CIF file name is give, LoadCIF will be used to create a workspace with a CrystalStructure. Thus,
            IonLabel, and Charges must also be specified (any valence / charge information in the CIF will be ignored)
        The 'IonLabel' parameter indicates which ion within the given crystal structure the crystal field parameters
            may be calculated for. This is only used to determine the origin of the coordinates of the ligand, and
            need not be a rare-earth ion. If it is *not* a rare earth ion, then the 'Ion' parameter which should
            be a string (name of rare earth ion) *must* be given.
        If an explicit list of ligands is given then all other parameters will be ignored.
        Only one of 'MaxDistance' or 'Neighbour' should be given. If both are given, 'Neighbour' will be ignored
            (priority given to 'MaxDistance'). If neither are given, MaxDistance=5 Angstrom will be used.

        Only the first argument ('Structure') is required to initialise the PointCharge object. All other
            arguments may be set later using the "dot" notation: e.g.
            pc = PointCharge('file.cif'); pc.IonLabel = 'Yb'; pc.Charges = {'Yb':3, 'O':-2}
        """
        self._cryst = None  # Mantid CrystalStructure object, from either LoadCIF, a workspace or direct input
        self._ligands = None  # List of ligands. Calculated by .getLigands() or entered directly
        self._charges = None  # Dictionary of ion labels and associated charges
        self._ionlabel = None  # Label of the magnetic ion
        self._ion = None  # Name of the magnetic ion (if different from the label, e.g. different element)
        self._maxdistance = None  # Outer distance of shell within which to compute charges
        self._neighbour = None  # Nth level of nearest neighbour ions within which to compute charges
        self._atoms = None  # A list of all the inequivalent sites by their unique labels and coordinates
        # Parse args / kwargs
        argname = ["Structure", "IonLabel", "Charges", "Ion", "MaxDistance", "Neighbour"]
        argdict = {"MaxDistance": 5.0}
        for ind in range(0, (len(argname) if len(args) > len(argname) else len(args))):
            argdict[argname[ind]] = args[ind]
        for ind in kwargs.keys():
            if ind in argname:
                argdict[ind] = kwargs[ind]
        if "Structure" not in argdict.keys():
            raise ValueError("PointCharge must be initialised with either a CIF file or a workspace with a valid crystal structure")
        # Make sure only one of MaxDistance or Neighbour is set
        if "Neighbour" in kwargs.keys():
            if "MaxDistance" in kwargs.keys():
                del argdict["Neighbour"]
                warnings.warn(
                    "Both Neighbour and MaxDistance arguments given. Using MaxDistance, ignoring Neighbour",
                    RuntimeWarning,
                )
            else:
                del argdict["MaxDistance"]
        # Determines if user gave CIF file or workspace or ligands, and populates other attributes.
        self._parseStructure(argdict.pop("Structure"))
        # If the user gives the ligands directly, we don't need the default MaxDistance parameter
        if self._ligands is not None and "MaxDistance" not in kwargs.keys():
            del argdict["MaxDistance"]
        # Parses all other parameters.
        for parname, value in argdict.items():
            setattr(self, parname, value)

    def _copyCrystalStructure(self, cryst):
        from mantid.geometry import CrystalStructure

        cell = cryst.getUnitCell()
        cellstr = " ".join([str(v) for v in [cell.a(), cell.b(), cell.c(), cell.alpha(), cell.beta(), cell.gamma()]])
        return CrystalStructure(cellstr, cryst.getSpaceGroup().getHMSymbol(), ";".join(cryst.getScatterers()))

    def _parseStructure(self, structure):
        from mantid.simpleapi import mtd, LoadCIF, CreateWorkspace, DeleteWorkspace
        import uuid

        self._fromCIF = False
        if isinstance(structure, str):
            if mtd.doesExist(structure):
                try:
                    self._cryst = self._copyCrystalStructure(mtd[structure].sample().getCrystalStructure())
                    self._getUniqueAtoms()
                except RuntimeError:
                    raise ValueError("Workspace %s has no valid CrystalStructure" % (structure))
            else:
                tmpws = CreateWorkspace(1, 1, OutputWorkspace="_tempPointCharge_" + str(uuid.uuid4())[:8])
                try:
                    LoadCIF(tmpws, structure)
                    # Attached CrystalStructure object gets destroyed when workspace is deleted
                    self._cryst = self._copyCrystalStructure(tmpws.sample().getCrystalStructure())
                except:
                    DeleteWorkspace(tmpws)
                    raise
                else:
                    DeleteWorkspace(tmpws)
                    self._getUniqueAtoms()
        elif isinstance(structure, list):
            if len(structure) == 4 and all([isinstance(x, (int, float)) for x in structure]):
                structure = [structure]
            if all([isinstance(x, list) and (len(x) == 4) and all([isinstance(y, (int, float)) for y in x]) for x in structure]):
                self._ligands = structure
            else:
                raise ValueError(
                    "Incorrect ligands direct input. Must be a 4-element list or a list "
                    "of 4-element list. Each ligand must be of the form [charge, x, y, z]"
                )
        elif hasattr(structure, "getScatterers"):
            self._cryst = structure
            self._getUniqueAtoms()
        else:
            if not hasattr(structure, "sample"):
                raise ValueError(
                    "First input must be a Mantid CrystalStructure object, workspace or string (name of CIF file or workspace)"
                )
            try:
                self._cryst = self._copyCrystalStructure(structure.sample().getCrystalStructure())
                self._getUniqueAtoms()
            except RuntimeError:
                raise ValueError("Workspace %s has no valid CrystalStructure" % (structure.name()))

    def _getUniqueAtoms(self):
        """Gets a list of unique ion names for each non-equivalent ion in the unit cell"""
        self._atoms = {}
        index = {}
        for atom in [entry.split() for entry in self._cryst.getScatterers()]:
            name = atom[0]
            if name in self._atoms.keys():
                index[name] = (index[name] + 1) if name in index.keys() else 2
                name = name + str(index[name])
            self._atoms[name] = [float(val) for val in atom[1:4]]
        for duplicate in index.keys():
            self._atoms[duplicate + "1"] = self._atoms.pop(duplicate)

    def _checkHasStructure(self, warnName=None):
        if self._atoms is None:
            parwarn = ("Ignoring parameter %s." % (warnName)) if warnName else ""
            warnings.warn("No structure has been set. %s" % (parwarn), RuntimeWarning)
            return False
        return True

    @property
    def Charges(self):
        """The current list of charges as a dictionary"""
        return self._charges

    @Charges.setter
    def Charges(self, charge, value=None):
        """
        Sets the value of a ion's charge in units of |e|.
        @param charge - either a dictionary mapping ionnames to charges e.g. {'Ce':3, 'O1':-2, 'O2':-2}
                        or a particular ion's name, with its value in the next argument
        @param value - the value of the charge on the ion defined in 'charge' in |e|.
        Note that ions which have no charges set will be considered to have zero charge (ignored in calculation)
        """
        if self._checkHasStructure(warnName="Charges"):
            if value is None:
                if not isinstance(charge, dict):
                    raise ValueError("Argument charge must be a dictionary")
                self._charges = charge
            else:
                if self._charges is None:
                    self._charges = {}
                self._charges[charge] = float(value)

    @property
    def MaxDistance(self):
        """The maximum distance to calculate ligands away from the magnetic atom"""
        return self._maxdistance

    @MaxDistance.setter
    def MaxDistance(self, value):
        if self._checkHasStructure(warnName="MaxDistance"):
            try:
                self._maxdistance = float(value)
            except ValueError:
                raise ValueError("MaxDistance (in Angstrom) must be a floating point number")
            if self._neighbour is not None:
                self._neighbour = None

    @property
    def Neighbour(self):
        """The nth neighbour of the magnetic ion to calculate up to"""
        return self._neighbour

    @Neighbour.setter
    def Neighbour(self, value):
        if self._checkHasStructure(warnName="Neighbour"):
            try:
                self._neighbour = int(value)
                if np.abs(self._neighbour - float(value)) > 1.0e-6:
                    warnings.warn("Neighbour value %s not an integer, rounded to %s" % (value, self._neighbour), RuntimeWarning)
                if self._maxdistance is not None:
                    self._maxdistance = None
            except ValueError:
                raise ValueError("Neighbour must be an integer")

    @property
    def IonLabel(self):
        """The text label (name) of the magnetic ion in case it is not the same as the Ion property"""
        return self._ionlabel

    @IonLabel.setter
    def IonLabel(self, value):
        if (self._atoms is not None) and (value not in self._atoms.keys()) and (self._ligands is None):
            raise ValueError("IonLabel %s not in list of atoms: %s." % (value, ",".join(self._atoms.keys())))
        self._ionlabel = value

    @property
    def Ion(self):
        """The magnetic ion type / label"""
        return self._ion

    @Ion.setter
    def Ion(self, value):
        ion = value[0].upper() + value[1].lower()
        if ion not in self.theta.keys():
            raise ValueError("Ion %s not in list of known ions: %s." % (value, ",".join(self.theta.keys())))
        self._ion = ion

    def getIons(self):
        """Returns the current structure as a dictionary of ion names and coordinates"""
        return self._atoms

    def _getBlm(self, q, x, y, z, r, PreFact, rn, thetak):
        # Converts from Cartesian to polars: c==cos, s==sin, t==theta, fi==phi
        xy = x * x + y * y
        ct = z / r
        ct2 = ct * ct
        st = np.sqrt(xy) / r
        st2 = st * st
        if xy == 0:
            sfi = 0
            cfi = 0
        else:
            sfi = y / np.sqrt(xy)
            cfi = x / np.sqrt(xy)

        Blm = [[0 for _ in range(4 * i + 5)] for i in range(3)]

        # These are the tesseral harmonics
        #  - see e.g. https://en.wikipedia.org/wiki/Table_of_spherical_harmonics#Real_spherical_harmonics
        Blm[0][0] = PreFact[2][2] ** 2 * 2 * st2 * sfi * cfi
        Blm[0][1] = PreFact[2][1] ** 2 * st * sfi * ct
        Blm[0][2] = PreFact[2][0] ** 2 * (3 * ct2 - 1)
        Blm[0][3] = PreFact[2][1] ** 2 * st * cfi * ct
        Blm[0][4] = PreFact[2][2] ** 2 * st2 * (cfi * cfi - sfi * sfi)

        Blm[1][0] = PreFact[4][4] ** 2 * st2 * st2 * 4 * (cfi * cfi * cfi * sfi - cfi * sfi * sfi * sfi)
        Blm[1][1] = PreFact[4][3] ** 2 * ct * st * st2 * (3 * cfi * cfi * sfi - sfi * sfi * sfi)
        Blm[1][2] = PreFact[4][2] ** 2 * (7 * ct2 - 1) * 2 * st2 * cfi * sfi
        Blm[1][3] = PreFact[4][1] ** 2 * st * sfi * ct * (7 * ct2 - 3)
        Blm[1][4] = PreFact[4][0] ** 2 * (35 * ct2 * ct2 - 30 * ct2 + 3)
        Blm[1][5] = PreFact[4][1] ** 2 * st * cfi * ct * (7 * ct2 - 3)
        Blm[1][6] = PreFact[4][2] ** 2 * (7 * ct2 - 1) * st2 * (cfi * cfi - sfi * sfi)
        Blm[1][7] = PreFact[4][3] ** 2 * ct * st * st2 * (cfi * cfi * cfi - 3 * cfi * sfi * sfi)
        Blm[1][8] = PreFact[4][4] ** 2 * st2 * st2 * (cfi * cfi * cfi * cfi - 6 * cfi * cfi * sfi * sfi + sfi * sfi * sfi * sfi)

        Blm[2][0] = PreFact[6][6] ** 2 * st2 * st2 * st2 * (6 * pow(cfi, 5) * sfi - 20 * pow(cfi, 3) * pow(sfi, 3) + 6 * cfi * pow(sfi, 5))
        Blm[2][1] = (
            PreFact[6][5] ** 2 * ct * st * st2 * st2 * (5 * cfi * cfi * cfi * cfi * sfi - 10 * cfi * cfi * pow(sfi, 3) + pow(sfi, 5))
        )
        Blm[2][2] = PreFact[6][4] ** 2 * (11 * ct2 - 1) * 4 * st2 * st2 * (cfi * cfi * cfi * sfi - cfi * sfi * sfi * sfi)
        Blm[2][3] = PreFact[6][3] ** 2 * (11 * ct * ct2 - 3 * ct) * st2 * st * (3 * cfi * cfi * sfi - sfi * sfi * sfi)
        Blm[2][4] = PreFact[6][2] ** 2 * 2 * st2 * sfi * cfi * (16 * ct2 * ct2 - 16 * ct2 * st2 + st2 * st2)
        Blm[2][5] = PreFact[6][1] ** 2 * ct * st * sfi * (33 * ct2 * ct2 - 30 * ct2 + 5)
        Blm[2][6] = PreFact[6][0] ** 2 * (231 * ct2 * ct2 * ct2 - 315 * ct2 * ct2 + 105 * ct2 - 5)
        Blm[2][7] = PreFact[6][1] ** 2 * ct * st * cfi * (33 * ct2 * ct2 - 30 * ct2 + 5)
        Blm[2][8] = PreFact[6][2] ** 2 * (16 * ct2 * ct2 - 16 * ct2 * st2 + st2 * st2) * st2 * (cfi * cfi - sfi * sfi)
        Blm[2][9] = PreFact[6][3] ** 2 * (11 * ct * ct2 - 3 * ct) * st2 * st * (cfi * cfi * cfi - 3 * cfi * sfi * sfi)
        Blm[2][10] = PreFact[6][4] ** 2 * (11 * ct2 - 1) * st2 * st2 * (cfi * cfi * cfi * cfi - 6 * cfi * cfi * sfi * sfi + pow(sfi, 4))
        Blm[2][11] = PreFact[6][5] ** 2 * ct * st * st2 * st2 * (pow(cfi, 5) - 10 * pow(cfi, 3) * sfi * sfi + 5 * cfi * pow(sfi, 4))
        Blm[2][12] = PreFact[6][6] ** 2 * pow(st2, 3) * (pow(cfi, 6) - 15 * pow(cfi, 4) * sfi**2 + 15 * cfi**2 * pow(sfi, 4) - pow(sfi, 6))

        Q_e = constants.physical_constants["atomic unit of charge"][0]
        eps0 = constants.physical_constants["electric constant"][0]
        a0 = constants.physical_constants["Bohr radius"][0] * 1e10  # Want a0 in Angstrom
        # Factor including e^2 factor for potential energy and Coulomb's constant and conversion from J to meV
        # Note that q below is the charge on the ligand ion in units of |e|, so we need the e^2 factor.
        energyfactor = pow(Q_e, 2) * (1.0 / 4 / np.pi / eps0) / (Q_e / 1000.0)

        # The expression for the point charge crystal field parameter is:
        #
        #  m    4pi  [      1        q|e|   a0^l ]       l
        # B  = ----  [ ------------ ------ ----- ] |e| <r >  theta   Z  (theta, phi)
        #  l   2l+1  [ 4pi.epsilon0   r     r^l  ]                l   lm
        #
        # where the term in the square brackets is the electric potential due to the point charge neighbour.

        for i in range(3):
            rl = pow(r, 2 * i + 2) / pow(a0, 2 * i + 2)
            for m in range(4 * (i + 1) + 1):
                Blm[i][m] *= -(4.0 * np.pi / (i * 4 + 5)) * q / (r * 1.0e-10) / rl * rn[i] * thetak[i] * energyfactor

        return Blm

    def _calculateLigands(self, dist):
        """
        Parses the stored crystal structure and returns a set of ligands
        @param dist - the maximum distance of ligands (if dist > 0) or the nth neighbour shell (if dist < 0)
        """
        # Check that the charges have been defined:
        if self._charges is None:
            raise RuntimeError("No charges have been defined for this model.")
        # Determine the transformation matrix to convert from fractional to Cartesian coordinates and back.
        cell = self._cryst.getUnitCell()
        rtoijk = cell.getBinv()
        invrtoijk = cell.getB()
        # Make a list of all atomic positions
        sg = self._cryst.getSpaceGroup()
        pos = {}
        charges = self._charges
        for name, coords in self._atoms.items():
            pos[name] = [c for c in sg.getEquivalentPositions(coords)]
            if name not in self._charges.keys():
                try:
                    charges[name] = self._charges[re.match(CHARGES_PATTERN, name).group(0)]
                except (KeyError, AttributeError):
                    warnstr = "Atom type %s in structure not in list of charges. Assuming q=0" % (name)
                    warnings.warn(warnstr, RuntimeWarning)
                    charges[name] = 0.0
        if self._ionlabel not in pos.keys():
            raise RuntimeError("Magnetic ion %s not found in structure" % (self._ionlabel))
        # Construct a large enough supercell such that we can be sure to find all neighbours of the magnetic
        # ion up to the desired shell / distance.
        if dist < 0:
            nn = -dist
            # Estimate number of nearest-neighbours in the cell, and then scale the search distance by the
            # nearest-neighbour separation weighted by the number of neighbours in the cell
            r0 = np.array(pos[self._ionlabel][0])
            dist_in_cell = [np.linalg.norm(np.inner(rtoijk, r0 - r)) for r in [rs for rs in pos.values()]]
            nn_in_cell = len(dist_in_cell)
            dist = float(dist / nn_in_cell) * np.min(dist_in_cell)
        # Supercell size is distance (with 50% fudge factor) times unit cell dimensions in each orthongal direction
        nmax = [int(val) for val in np.ceil(np.abs(dist) * 1.5 * np.sqrt(np.sum(invrtoijk**2, 1)))]
        for r0 in pos[self._ionlabel]:
            entries = []
            for i1 in range(-nmax[0], nmax[0] + 1):
                for i2 in range(-nmax[1], nmax[1] + 1):
                    for i3 in range(-nmax[2], nmax[2] + 1):
                        r1 = r0 + np.array([i1, i2, i3])
                        for name, rns in pos.items():
                            for rn in rns:
                                rvec = np.array(np.matrix(r1 - rn) * rtoijk)[0]
                                r = np.linalg.norm(rvec)
                                if (r > 0) and (r < (dist if dist > 0 else r + 1)):
                                    entries.append([r, charges[name], rvec[0], rvec[1], rvec[2]])
        if dist < 0:
            rlist = np.sort(np.unique([ent[0] for ent in entries]))
            entries = [ent for ent in entries if ent[0] < rlist[nn]]  # Truncates the entries to nnth neighbours
        idx = np.argsort([ent[0] for ent in entries])
        return [entries[i][1:] for i in idx]

    def _getIon(self):
        ion = self._ion if self._ion else self._ionlabel
        try:
            return ion[0].upper() + ion[1].lower()
        except (TypeError, IndexError):
            raise ValueError("Invalid value of IonLabel or Ion %s." % (ion))

    def _getDist(self):
        return self._maxdistance if self._maxdistance else -self._neighbour

    def calculate(self):
        """Calculates the crystal field parameters in the point charge model for the defined structure"""
        ion = self._getIon()
        ligands = self._calculateLigands(self._getDist()) if (self._ligands is None) else self._ligands
        # Zeros all parameters
        Blms = [
            ["IB22", "IB21", "B20", "B21", "B22"],
            ["IB44", "IB43", "IB42", "IB41", "B40", "B41", "B42", "B43", "B44"],
            ["IB66", "IB65", "IB64", "IB63", "IB62", "IB61", "B60", "B61", "B62", "B63", "B64", "B65", "B66"],
        ]
        Blm = {lm: 0 for sublist in Blms for lm in sublist}
        for n in range(len(ligands)):
            q = ligands[n][0]
            x = ligands[n][1]
            y = ligands[n][2]
            z = ligands[n][3]
            r = np.sqrt(x * x + y * y + z * z)
            nBlm = self._getBlm(q, x, y, z, r, self.Zlm, self.rns[ion], self.theta[ion])
            for i in range(3):
                for m in range(4 * (i + 1) + 1):
                    Blm[Blms[i][m]] += nBlm[i][m]
        # Removes parameters which are zero
        for lm in [key for key in Blm.keys() if np.abs(Blm[key]) < 1.0e-10]:
            del Blm[lm]
        return Blm
