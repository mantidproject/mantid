# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-branches,too-many-locals, invalid-name
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction
from mantid.simpleapi import logger, CreateWorkspace

from scipy.io import netcdf
import numpy as np
import re
import time


class AngularAutoCorrelationsTwoAxes(PythonAlgorithm):
    def category(self):
        return "Simulation"

    def summary(self):
        return (
            "Calculates the angular auto-correlations of molecules in a simulation along two user-defined axes. "
            "The first axis is defined by the vector connecting the average position of species two and the average position "
            "of species one (user input). The second axis is perpendicular to axis 1 and is constructed by considering one "
            "arbitrary atom of species 3 (user input). Timestep must be specified in femtoseconds."
        )

    def PyInit(self):
        self.declareProperty(FileProperty("InputFile", "", action=FileAction.Load), doc="Input .nc file with an MMTK trajectory")

        self.declareProperty(
            "Timestep", "1.0", direction=Direction.Input, doc="Time step between two coordinates in the trajectory in femtoseconds"
        )

        self.declareProperty("SpeciesOne", "", direction=Direction.Input, doc="Specify the first species, e.g. H, He, Li...")
        self.declareProperty("SpeciesTwo", "", direction=Direction.Input, doc="Specify the second species, e.g. H, He, Li...")
        self.declareProperty("SpeciesThree", "", direction=Direction.Input, doc="Specify the third species, e.g. H, He, Li...")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace name")
        self.declareProperty(WorkspaceProperty("OutputWorkspaceFT", "FT", direction=Direction.Output), doc="FT Output workspace name")

    def PyExec(self):
        # Get file path
        file_name = self.getPropertyValue("InputFile")

        # Get the user-specified species
        types = [
            self.getPropertyValue("SpeciesOne").lower(),
            self.getPropertyValue("SpeciesTwo").lower(),
            self.getPropertyValue("SpeciesThree").lower(),
        ]

        # Load trajectory file
        trajectory = netcdf.netcdf_file(file_name, mode="r")

        logger.information("Loading particle id's, molecule id's and coordinate array...")
        start_time = time.time()

        # netcdf object containing the particle id numbers
        description = (trajectory.variables["description"])[:]

        # Convert description object to string via for loop. The original object has strange formatting
        particleID = ""
        for i in description:
            particleID += i.decode("UTF-8")

        # Extract particle id's from string using regular expressions
        p_atoms = re.findall(r"A\('[a-z]+\d+',\d+", particleID)

        # Split the string s by molecules
        molecules = particleID.split("AC")

        # Remove first item of molecule list (contains initialisation of variable 'description')
        del molecules[0]

        # Many-to-one structures. Identify the set of atomic species present (list structure 'elements')
        # in the simulation and repackage particles into a dictionary 'particles_to_species' with structure id number -> species
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

        # Check wether user-specified species present in the trajectory file
        for i in range(3):
            if types[i] not in elements:
                raise RuntimeError("Species " + ["one", "two", "three"][i] + " not found in the trajectory file. Please try again...")

        # Initialise lists in the species_to_particles dictionary
        for j in elements:
            species_to_atoms[j] = []

        # Populate the species_to_particles dictionary
        for j in p_atoms:
            key = re.findall(r"',\d+", j)[0]
            key = int(re.findall(r"\d+", key)[0])
            element = re.findall(r"[a-z]+", j)[0]
            species_to_atoms[element].append(key)

        # Many-to-one structures. Assign atom indices to molecule indices using a dictionaries
        # with structures atom id -> molecule id and molecule id -> list of atoms ids
        atoms_to_molecules = {}
        molecules_to_atoms = {}

        # Initialise lists in the molecule_to_atom dictionary
        for k in range(len(molecules)):
            molecules_to_atoms[k] = []

        # Populate the dictionaries with atoms & molecules
        for k in range(len(molecules)):
            r_atoms = re.findall(r"A\('[a-z]+\d+',\d+", molecules[k])
            for i in r_atoms:
                key = re.findall(r"',\d+", i)[0]
                key = int(re.findall(r"\d+", key)[0])
                atoms_to_molecules[key] = k
                molecules_to_atoms[k].append(key)

        # Coordinate array. Shape: timesteps x (# of particles) x (# of spatial dimensions)
        configuration = trajectory.variables["configuration"]

        # Extract useful simulation parameters
        # Number of species present in the simulation
        # n_species=len(elements)
        # Number of particles present in the simulation
        n_particles = len(p_atoms)
        # Number of molecules present in the simulation
        n_molecules = len(molecules)
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
        # Shape: (# of timesteps) x (3-vectors) x (# of spatial dimensions)
        box_size_tensors = 10.0 * np.array([box_size[j].reshape((3, 3)) for j in range(n_timesteps)])

        # Extract box dimensions (assuming orthorhombic simulation cell, diagonal matrix)
        box_size_x = np.array([box_size_tensors[i, 0, 0] for i in range(n_timesteps)])
        box_size_y = np.array([box_size_tensors[i, 1, 1] for i in range(n_timesteps)])
        box_size_z = np.array([box_size_tensors[i, 2, 2] for i in range(n_timesteps)])

        # Copy the configuration object into a numpy array
        configuration_copy = np.array([configuration[i] for i in range(n_timesteps)])

        # Swap the time and particle axes
        configuration_copy = np.swapaxes(configuration_copy, 0, 1)

        # Transform particle trajectories (configuration array) to Cartesian coordinates at each time step.
        cartesian_configuration = np.array(
            [[np.dot(box_size_tensors[j], np.transpose(configuration_copy[i, j])) for j in range(n_timesteps)] for i in range(n_particles)]
        )

        logger.information(str(time.time() - start_time) + " s")

        logger.information("Calculating orientation vectors...")
        start_time = time.time()

        # Initialise orientation vector array. Shape: (# of molecules) x (# of timesteps) x (# of dimensions)
        orientation_vectors1 = np.zeros((n_molecules, n_timesteps, n_dimensions))
        orientation_vectors2 = np.zeros((n_molecules, n_timesteps, n_dimensions))

        for i in range(n_molecules):
            # Retrieve constituents of the ith molecule
            temp = molecules_to_atoms[i]
            # Find which constituents belong to species one, species two and species three
            species_one = []
            species_two = []
            species_three = []
            for j in temp:
                if atoms_to_species[j] == types[0]:
                    species_one.append(j)
                if atoms_to_species[j] == types[1]:
                    species_two.append(j)
                if atoms_to_species[j] == types[2]:
                    species_three.append(j)
            # Find the average positions of species one and two
            sum_position_species_one = np.zeros((n_timesteps, n_dimensions))
            sum_position_species_two = np.zeros((n_timesteps, n_dimensions))
            for k in species_one:
                sum_position_species_one += cartesian_configuration[k]
            for L in species_two:
                sum_position_species_two += cartesian_configuration[L]
            avg_position_species_one = 1.0 * sum_position_species_one / float(len(species_one))
            avg_position_species_two = 1.0 * sum_position_species_two / float(len(species_two))
            # Choose the 1st element of species_three to build the 2nd vector
            position_species_three = 1.0 * cartesian_configuration[species_three[0]]

            # Find the vectors connecting average positions of species one and species two
            vectors1 = avg_position_species_two - avg_position_species_one

            # Find the vector to the third atom
            vectors2 = position_species_three - avg_position_species_two

            diffX1 = np.divide(vectors1[:, 0], box_size_x)
            diffY1 = np.divide(vectors1[:, 1], box_size_y)
            diffZ1 = np.divide(vectors1[:, 2], box_size_z)

            diffX2 = np.divide(vectors2[:, 0], box_size_x)
            diffY2 = np.divide(vectors2[:, 1], box_size_y)
            diffZ2 = np.divide(vectors2[:, 2], box_size_z)

            # Wrapping the vectors
            vectorX1 = np.array([(diffX1[k] - round(diffX1[k])) * box_size_x[k] for k in range(n_timesteps)])
            vectorY1 = np.array([(diffY1[k] - round(diffY1[k])) * box_size_y[k] for k in range(n_timesteps)])
            vectorZ1 = np.array([(diffZ1[k] - round(diffZ1[k])) * box_size_z[k] for k in range(n_timesteps)])

            vectorX2 = np.array([(diffX2[k] - round(diffX2[k])) * box_size_x[k] for k in range(n_timesteps)])
            vectorY2 = np.array([(diffY2[k] - round(diffY2[k])) * box_size_y[k] for k in range(n_timesteps)])
            vectorZ2 = np.array([(diffZ2[k] - round(diffZ2[k])) * box_size_z[k] for k in range(n_timesteps)])

            # Normalisation
            norm1 = np.sqrt(vectorX1 * vectorX1 + vectorY1 * vectorY1 + vectorZ1 * vectorZ1)
            vectorX1 = np.divide(vectorX1, norm1)
            vectorY1 = np.divide(vectorY1, norm1)
            vectorZ1 = np.divide(vectorZ1, norm1)

            norm2 = np.sqrt(vectorX2 * vectorX2 + vectorY2 * vectorY2 + vectorZ2 * vectorZ2)
            vectorX2 = np.divide(vectorX2, norm2)
            vectorY2 = np.divide(vectorY2, norm2)
            vectorZ2 = np.divide(vectorZ2, norm2)

            # Dot product
            cosine = np.multiply(vectorX1, vectorX2) + np.multiply(vectorY1, vectorY2) + np.multiply(vectorZ1, vectorZ2)

            # Gram-Schmidt orthogonalisation process
            vectorX2 = vectorX2 - np.divide(vectorX1, cosine)
            vectorY2 = vectorY2 - np.divide(vectorY1, cosine)
            vectorZ2 = vectorZ2 - np.divide(vectorZ1, cosine)

            # Renormalisation of the 2nd vector
            norm2 = np.sqrt(vectorX2 * vectorX2 + vectorY2 * vectorY2 + vectorZ2 * vectorZ2)
            vectorX2 = np.divide(vectorX2, norm2)
            vectorY2 = np.divide(vectorY2, norm2)
            vectorZ2 = np.divide(vectorZ2, norm2)

            # Store calculations in the orientation_vectors1 and orientation_vectors2 arrays
            orientation_vectors1[i] = np.swapaxes(np.array([vectorX1, vectorY1, vectorZ1]), 0, 1)
            orientation_vectors2[i] = np.swapaxes(np.array([vectorX2, vectorY2, vectorZ2]), 0, 1)

        logger.information(str(time.time() - start_time) + " s")

        logger.information("Calculating angular auto-correlations...")
        start_time = time.time()

        # First axis
        R_avg_axis1 = np.zeros(n_timesteps)
        for i in range(n_molecules):
            R_avg_axis1 += self.auto_correlation(orientation_vectors1[i])

        R_avg_axis1 = 1.0 * R_avg_axis1 / n_molecules

        # Second axis
        R_avg_axis2 = np.zeros(n_timesteps)
        for i in range(n_molecules):
            R_avg_axis2 += self.auto_correlation(orientation_vectors2[i])

        R_avg_axis2 = 1.0 * R_avg_axis2 / n_molecules

        logger.information(str(time.time() - start_time) + " s")

        # Initialise & populate the output_ws workspace
        nrows = 2
        step = float(self.getPropertyValue("Timestep"))
        xvals = np.arange(0, np.ceil((n_timesteps) / 2.0)) * step / 1000.0
        yvals = np.empty(0)
        yvals = np.append(yvals, self.fold_correlation(R_avg_axis1))
        yvals = np.append(yvals, self.fold_correlation(R_avg_axis2))
        evals = np.zeros(np.shape(yvals))

        output_name = self.getPropertyValue("OutputWorkspace")
        output_ws = CreateWorkspace(
            OutputWorkspace=output_name,
            DataX=xvals,
            UnitX="ps",
            DataY=yvals,
            DataE=evals,
            NSpec=nrows,
            VerticalAxisUnit="Text",
            VerticalAxisValues=["Axis 1", "Axis 2"],
        )
        # Set output workspace to output_ws
        self.setProperty("OutputWorkspace", output_ws)

        # Fourier transform output to workspace
        nrows = 2
        yvals = np.empty(0)
        yvals = np.append(yvals, np.fft.rfft(R_avg_axis1))
        yvals = np.append(yvals, np.fft.rfft(R_avg_axis2))
        evals = np.zeros(np.shape(yvals))
        xvals = np.arange(0, np.shape(yvals)[0])

        FT_output_name = self.getPropertyValue("OutputWorkspaceFT")
        FT_output_ws = CreateWorkspace(
            OutputWorkspace=FT_output_name,
            DataX=xvals,
            UnitX="1/fs",
            DataY=yvals,
            DataE=evals,
            NSpec=nrows,
            VerticalAxisUnit="Text",
            VerticalAxisValues=["FT Axis 1", "FT Axis 2"],
        )
        self.setProperty("OutputWorkspaceFT", FT_output_ws)

    def auto_correlation(self, vector):
        # Returns angular auto-correlation of a normalised time-dependent 3-vector
        num = np.shape(vector)[0]
        norm = np.arange(np.ceil(num / 2.0), num + 1)
        norm = np.append(norm, (np.arange(int(num / 2) + 1, num)[::-1]))

        # x dimension
        autoCorr = np.divide(np.correlate(vector[:, 0], vector[:, 0], "same"), norm)
        # y dimension
        autoCorr += np.divide(np.correlate(vector[:, 1], vector[:, 1], "same"), norm)
        # z dimension
        autoCorr += np.divide(np.correlate(vector[:, 2], vector[:, 2], "same"), norm)

        return autoCorr

    def fold_correlation(self, omega):
        # Folds an array with symmetrical values into half by averaging values around the centre
        right_half = omega[int(len(omega)) // 2 :]
        left_half = omega[: int(np.ceil(len(omega) / 2.0))][::-1]

        return (left_half + right_half) / 2.0


# Subscribe algorithm to Mantid software
AlgorithmFactory.subscribe(AngularAutoCorrelationsTwoAxes)
