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


class VelocityCrossCorrelations(PythonAlgorithm):
    def category(self):
        return "Simulation"

    def summary(self):
        return (
            "Imports trajectory data from an nMoldyn-generated .nc file and calculates velocity "
            "cross-correlations between atomic species. The algorithm calculates the velocity cross-correlation "
            "of every pair of particles and averages the correlations according to the particles' atomic species. "
            "Timestep must be specified in femtoseconds."
        )

    def PyInit(self):
        self.declareProperty(FileProperty("InputFile", "", action=FileAction.Load), doc="Input .nc file with an MMTK trajectory")

        self.declareProperty(
            "Timestep", "1.0", direction=Direction.Input, doc="Specify the timestep between trajectory points in the simulation, fs"
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

        # Many-to-one structures. Identify the set of atomic species present (list structure 'elements')
        # in the simulation and repackage particles into a dictionary
        # 'particles_to_species' with structure id number -> species
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
        v1 = scaled_coords[:, 1:-1, :] - scaled_coords[:, :-2, :] - np.round(scaled_coords[:, 1:-1, :] - scaled_coords[:, :-2, :])
        v2 = scaled_coords[:, 2:, :] - scaled_coords[:, 1:-1, :] - np.round(scaled_coords[:, 2:, :] - scaled_coords[:, 1:-1, :])
        velocities[:, :-1, :] = (v1 + v2) / 2.0

        # Transform velocities (configuration array) back to Cartesian coordinates at each time step
        velocities = np.array(
            [[np.dot(box_size_tensors[j + 1], np.transpose(velocities[i, j])) for j in range(n_timesteps - 1)] for i in range(n_particles)]
        )
        logger.information(str(time.time() - start_time) + " s")

        logger.information("Calculating velocity cross-correlations (resource intensive calculation)...")
        start_time = time.time()
        correlation_length = n_timesteps - 1
        correlations = np.zeros((n_species, n_species, correlation_length))
        # Array for counting particle pairings
        correlation_count = np.zeros((n_species, n_species))

        # Compute cross-correlations for each pair of particles in each spatial coordinate
        for i in range(n_particles):
            for j in range(i + 1, n_particles):
                # Retrieve particle indices from the 'particles' dictionary and
                # determine the relevant position in the 'correlations' matrices
                k = elements.index(atoms_to_species[i])
                l = elements.index(atoms_to_species[j])
                # Check for the order of elements (ensures upper triangular matrix form & consistent order of operations)
                if k <= l:
                    correlation_temp = self.cross_correlation(velocities[i], velocities[j])
                    correlations[k, l] += correlation_temp
                    correlation_count[k, l] += 1
                else:
                    correlation_temp = self.cross_correlation(velocities[j], velocities[i])
                    correlations[l, k] += correlation_temp
                    correlation_count[l, k] += 1

        logger.information(str(time.time() - start_time) + " s")

        # Neutron coherent scattering lengths (femtometres)
        # Sources:
        # https://www.ncnr.nist.gov/resources/n-lengths/list.html
        # http://www.ati.ac.at/~neutropt/scattering/RecommendedScatteringLengthsOfElements.PDF
        Coh_b = {
            "h": -3.7409,
            "he": 3.26,
            "li": -1.90,
            "be": 7.79,
            "b": 5.30,
            "c": 6.6484,
            "n": 9.36,
            "o": 5.805,
            "f": 5.654,
            "ne": 4.60,
            "na": 3.63,
            "mg": 5.375,
            "al": 3.449,
            "si": 4.15071,
            "p": 5.13,
            "s": 2.847,
            "cl": 9.5792,
            "ar": 1.909,
            "k": 3.67,
            "ca": 4.70,
            "sc": 12.29,
            "ti": -3.37,
            "v": -0.443,
            "cr": 3.635,
            "mn": -3.73,
            "fe": 9.45,
            "co": 2.49,
            "ni": 10.3,
            "cu": 7.718,
            "zn": 5.680,
            "ga": 7.288,
            "ge": 8.185,
            "as": 6.58,
            "se": 7.970,
            "br": 6.79,
            "kr": 7.81,
            "rb": 7.08,
            "sr": 7.02,
            "y": 7.75,
            "zr": 7.16,
            "nb": 7.054,
            "mo": 6.715,
            "tc": 6.8,
            "ru": 7.02,
            "rh": 5.90,
            "pd": 5.91,
            "ag": 5.922,
            "cd": 4.83,
            "in": 4.065,
            "sn": 6.225,
            "sb": 5.57,
            "te": 5.68,
            "i": 5.28,
            "xe": 4.69,
            "cs": 5.42,
            "ba": 5.07,
            "la": 8.24,
            "ce": 4.84,
            "pr": 4.58,
            "nd": 7.69,
            "pm": 12.6,
            "sm": 0.00,
            "eu": 5.3,
            "gd": 9.5,
            "tb": 7.34,
            "dy": 16.9,
            "ho": 8.44,
            "er": 7.79,
            "tm": 7.07,
            "yb": 12.41,
            "lu": 7.21,
            "hf": 7.77,
            "ta": 6.91,
            "w": 4.755,
            "re": 9.2,
            "os": 10.7,
            "ir": 10.6,
            "pt": 9.60,
            "au": 7.63,
            "hg": 12.66,
            "tl": 8.776,
            "pb": 9.401,
            "bi": 8.532,
            "po": None,
            "at": None,
            "rn": None,
            "fr": None,
            "ra": 10.0,
            "ac": None,
            "th": 10.31,
            "pa": 9.1,
            "u": 8.417,
            "np": 10.55,
            "pu": None,
            "am": 8.3,
            "cm": 9.5,
        }

        logger.information("Averaging correlation Fourier transforms & scaling with the coherent neutron scattering lenghts...")
        start_time = time.time()

        # Scaling cross-correlations with the scattering lengths
        for i in range(n_species):
            for j in range(i, n_species):
                correlations[i, j] = correlations[i, j] * Coh_b[elements[i]] * Coh_b[elements[j]] / correlation_count[i, j]

        logger.information(str(time.time() - start_time) + " s")

        # Generate a list of row names according to the atomic species present in the simulation
        row_names = []
        for i in range(n_species):
            for j in range(i, n_species):
                row_names.append(elements[i].capitalize() + " and " + elements[j].capitalize())

        # Initialise & populate the output_ws workspace
        nrows = int((n_species * n_species - n_species) / 2 + n_species)
        # nbins=(np.shape(correlations)[2])
        yvals = np.empty(0)
        for i in range(n_species):
            for j in range(i, n_species):
                # Add folded correlations to the array passed to the workspace
                yvals = np.append(yvals, self.fold_correlation(correlations[i, j]))

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

    def cross_correlation(self, u, v):
        # Returns cross-correlation of two 3-vectors
        n = np.shape(v)[0]
        norm = np.arange(np.ceil(n / 2.0), n + 1)
        norm = np.append(norm, (np.arange(n / 2 + 1, n)[::-1]))
        # Correlation in x
        C = np.divide(np.correlate(u[:, 0], v[:, 0], "same"), norm)
        # Correlation in y
        C += np.divide(np.correlate(u[:, 1], v[:, 1], "same"), norm)
        # Correlation in z
        C += np.divide(np.correlate(u[:, 2], v[:, 2], "same"), norm)

        return C

    def fold_correlation(self, w):
        # Folds an array with symmetrical values into half by averaging values around the centre
        right_half = w[int(len(w) / 2) :]
        left_half = w[: int(np.ceil(len(w) / 2.0))][::-1]

        return (left_half + right_half) / 2.0


# Subscribe algorithm to Mantid software
AlgorithmFactory.subscribe(VelocityCrossCorrelations)
