# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-branches,too-many-locals, invalid-name
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction
from mantid.simpleapi import CreateWorkspace

from scipy.io import netcdf
import numpy as np
import re
import time


class VelocityAutoCorrelations(PythonAlgorithm):
    def category(self):
        return "Simulation"

    def summary(self):
        return (
            "Imports trajectory data from an nMoldyn-generated .nc file and calculates velocity auto-correlations."
            "Analogous in some respects to the algorithm found in nMoldyn. Timestep must be specified in femtoseconds."
        )

    def PyInit(self):
        self.declareProperty(FileProperty("InputFile", "", action=FileAction.Load), doc="Input .nc file with an MMTK trajectory")

        self.declareProperty(
            "Timestep", "1.0", direction=Direction.Input, doc="Specify the timestep between trajectory points in the simulation in fs"
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace name")

    def PyExec(self):
        # Get file path
        file_name = self.getPropertyValue("InputFile")

        # Load trajectory file
        trajectory = netcdf.netcdf_file(file_name, mode="r")

        logger.information("Loading particle id's and coordinate array...")
        start_time = time.time()

        # netcdf object containing the particle id numbers
        description = (trajectory.variables["description"])[:]
        # Convert description object to string via for loop. The original object has strange formatting
        particleID = ""
        for i in description:
            particleID += i.decode("UTF-8")

        # Extract particle id's from string using regular expressions
        p_atoms = re.findall(r"A\('[a-z]+\d+',\d+", particleID)

        # Many-to-one structures. Identify the set of atomic species present (list structure 'elements') in the
        # simulation and repackage particles into a dictionary 'particles_to_species' with structure id number -> species
        atoms_to_species = {}
        species_to_atoms = {}
        elements = []

        # Populate the particles_to_species dictionary and the elements list
        for j in p_atoms:
            key = re.findall(r"',\d+", j)[0]
            key = int(re.findall(r"\d+", key)[0])
            element = re.findall(r"[a-z]+", j)[0]
            if element not in elements:
                elements.append(str(element))
            atoms_to_species[key] = str(element)

        # Initialise lists in the species_to_particles dictionary
        for j in elements:
            species_to_atoms[j] = []

        # Populate the species_to_particles dictionary
        for j in p_atoms:
            key = re.findall(r"',\d+", j)[0]
            key = int(re.findall(r"\d+", key)[0])
            element = re.findall(r"[a-z]+", j)[0]
            species_to_atoms[element].append(key)

        # Coordinate array. Shape: timesteps x (# of particles) x (# of spatial dimensions)
        configuration = trajectory.variables["configuration"]

        # Extract useful simulation parameters
        # Number of species present in the simulation
        n_species = len(elements)
        # Number of particles present in the simulation
        n_particles = len(atoms_to_species)
        # Number of timesteps in the simulation
        n_timesteps = int(configuration.shape[0])
        # Number of spatial dimensions
        n_dimensions = int(configuration.shape[2])

        logger.information(str(time.time() - start_time) + " s")

        logger.information("Transforming coordinates...")
        start_time = time.time()

        # Box size for each timestep. Shape: timesteps x (3 consecutive 3-vectors)
        box_size = trajectory.variables["box_size"]

        # Reshape the paralellepipeds into 3x3 tensors for coordinate transformations.
        # Shape: timesteps x 3 vectors x (# of spatial dimensions)
        box_size_tensors = np.array([box_size[j].reshape((3, 3)) for j in range(n_timesteps)])

        # Copy the configuration object into a numpy array
        configuration_copy = np.array([configuration[i] for i in range(n_timesteps)])

        # Swap the time and particle axes
        configuration_copy = np.swapaxes(configuration_copy, 0, 1)  # /1.12484770

        # Get scaled coordinates (assumes orthogonal simulation box)
        scaled_coords = np.zeros(np.shape(configuration_copy))
        for i in range(n_particles):
            for j in range(n_timesteps):
                for k in range(n_dimensions):
                    scaled_coords[i, j, k] = configuration_copy[i, j, k] / box_size_tensors[j, k, k]

        # # Transform particle trajectories (configuration array) to Cartesian coordinates at each time step

        logger.information(str(time.time() - start_time) + " s")

        logger.information("Calculating velocities...")
        start_time = time.time()

        # Initialise velocity arrray. Note that the first dimension is 2 elements shorter than the coordinate array.
        # Use finite difference methods to evaluate the time-derivative to 1st order
        # Shape: (# of particles) x (timesteps-2) x (# of spatial dimensions)
        velocities = np.zeros((n_particles, n_timesteps - 1, n_dimensions))

        for i in range(n_particles):
            for j in range(n_timesteps - 2):
                # Unwrapping coordinates
                v_temp1 = scaled_coords[i, j + 1] - scaled_coords[i, j] - np.round(scaled_coords[i, j + 1] - scaled_coords[i, j])
                v_temp2 = scaled_coords[i, j + 2] - scaled_coords[i, j + 1] - np.round(scaled_coords[i, j + 2] - scaled_coords[i, j + 1])
                velocities[i, j] = (v_temp1 + v_temp2) / (2.0)

        # Transform velocities (configuration array) back to Cartesian coordinates at each time step
        velocities = np.array(
            [[np.dot(box_size_tensors[j + 1], np.transpose(velocities[i, j])) for j in range(n_timesteps - 1)] for i in range(n_particles)]
        )
        logger.information(str(time.time() - start_time) + " s")

        logger.information("Calculating velocity auto-correlations (resource intensive calculation)...")
        start_time = time.time()

        correlation_length = n_timesteps - 1
        correlations = np.zeros((n_species, n_species, correlation_length))
        # Array for counting particle pairings
        correlation_count = np.zeros((n_species, n_species))

        # Compute cross-correlations for each pair of particles in each spatial coordinate
        for i in range(n_particles):
            # Retrieve particle indices from the 'particles' dictionary and determine the relevant position in the 'correlations' matrices
            k = elements.index(atoms_to_species[i])
            # Check for the order of elements (ensures upper triangular matrix form & consistent order of operations)
            correlation_temp = self.auto_correlation(velocities[i])
            correlations[k, k] += correlation_temp
            correlation_count[k, k] += 1

        logger.information(str(time.time() - start_time) + " s")

        # Neutron incoherent scattering lengths (fm) weighted by isotope abundancies
        # Sources:
        # https://www.ncnr.nist.gov/resources/n-lengths/list.html
        # http://www.ati.ac.at/~neutropt/scattering/RecommendedScatteringLengthsOfElements.PDF
        Coh_b = {
            "h": 1.0,
            "he": 1.0,
            "li": 1.0,
            "be": 1.0,
            "b": 1.0,
            "c": 1.0,
            "n": 1.0,
            "o": 1.0,
            "f": 1.0,
            "ne": 1.0,
            "na": 1.0,
            "mg": 1.0,
            "al": 1.0,
            "si": 1.0,
            "p": 1.0,
            "s": 1.0,
            "cl": 1.0,
            "ar": 1.0,
            "k": 1.0,
            "ca": 1.0,
            "sc": 1.0,
            "ti": 1.0,
            "v": 1.0,
            "cr": 1.0,
            "mn": 1.0,
            "fe": 1.0,
            "co": 1.0,
            "ni": 1.0,
            "cu": 1.0,
            "zn": 1.0,
            "ga": 1.0,
            "ge": 1.0,
            "as": 1.0,
            "se": 1.0,
            "br": 1.0,
            "kr": 1.0,
            "rb": 1.0,
            "sr": 1.0,
            "y": 1.0,
            "zr": 1.0,
            "nb": 1.0,
            "mo": 1.0,
            "tc": 1.0,
            "ru": 1.0,
            "rh": 1.0,
            "pd": 1.0,
            "ag": 1.0,
            "cd": 1.0,
            "in": 1.0,
            "sn": 1.0,
            "sb": 1.0,
            "te": 1.0,
            "i": 1.0,
            "xe": 1.0,
            "cs": 1.0,
            "ba": 1.0,
            "la": 1.0,
            "ce": 1.0,
            "pr": 1.0,
            "nd": 1.0,
            "pm": 1.0,
            "sm": 1.0,
            "eu": 1.0,
            "gd": 1.0,
            "tb": 1.0,
            "dy": 1.0,
            "ho": 1.0,
            "er": 1.0,
            "tm": 1.0,
            "yb": 1.0,
            "lu": 1.0,
            "hf": 1.0,
            "ta": 1.0,
            "w": 1.0,
            "re": 1.0,
            "os": 1.0,
            "ir": 1.0,
            "pt": 1.0,
            "au": 1.0,
            "hg": 1.0,
            "tl": 1.0,
            "pb": 1.0,
            "bi": 1.0,
            "po": 1.0,
            "at": 1.0,
            "rn": 1.0,
            "fr": 1.0,
            "ra": 1.0,
            "ac": 1.0,
            "th": 1.0,
            "pa": 1.0,
            "u": 1.0,
            "np": 1.0,
            "pu": 1.0,
            "am": 1.0,
            "cm": 1.0,
        }

        logger.information("Averaging auto-correlations...")
        start_time = time.time()

        # Scaling cross-correlations with the scattering lengths
        for i in range(n_species):
            for j in range(i, n_species):
                correlations[i, j] = correlations[i, j] * Coh_b[elements[i]] * Coh_b[elements[j]] / correlation_count[i, j]

        logger.information(str(time.time() - start_time) + " s")

        # Generate a list of row names according to the atomic species present in the simulation
        row_names = []
        for i in range(n_species):
            row_names.append(elements[i].capitalize())

        # Initialise & populate the output_ws workspace
        nrows = n_species
        # (np.shape(correlations)[2])
        yvals = np.empty(0)
        for i in range(n_species):
            # Add folded correlations to the array passed to the workspace
            yvals = np.append(yvals, self.fold_correlation(correlations[i, i]))

        # Timesteps between coordinate positions
        step = float(self.getPropertyValue("Timestep"))

        xvals = np.arange(0, np.ceil((n_timesteps - 1) / 2.0)) * step / 1000.0
        evals = np.zeros(np.shape(yvals))

        output_name = self.getPropertyValue("OutputWorkspace")
        output_ws = CreateWorkspace(
            OutputWorkspace=output_name,
            DataX=xvals,
            DataY=yvals,
            DataE=evals,
            NSpec=nrows,
            VerticalAxisUnit="Text",
            VerticalAxisValues=row_names,
            UnitX="ps",
        )

        # Set output workspace to output_ws
        self.setProperty("OutputWorkspace", output_ws)

    def auto_correlation(self, u):
        # Returns auto-correlation of a 3-vectors
        n = np.shape(u)[0]
        norm = np.arange(np.ceil(n / 2.0), n + 1)
        norm = np.append(norm, (np.arange(n / 2 + 1, n)[::-1]))
        # Correlation in x
        C = np.divide(np.correlate(u[:, 0], u[:, 0], "same"), norm)
        # Correlation in y
        C += np.divide(np.correlate(u[:, 1], u[:, 1], "same"), norm)
        # Correlation in z
        C += np.divide(np.correlate(u[:, 2], u[:, 2], "same"), norm)

        return C

    def fold_correlation(self, w):
        # Folds an array with symmetrical values into half by averaging values around the centre
        right_half = w[len(w) // 2 :]
        left_half = w[: int(np.ceil(len(w) / 2.0))][::-1]

        return (left_half + right_half) / 2.0


# Subscribe algorithm to Mantid software
AlgorithmFactory.subscribe(VelocityAutoCorrelations)
