from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

from scipy.io import netcdf
import numpy as np
import re
import time

class AngularAutoCorrelationsSingleAxis(PythonAlgorithm):

    def category(self):
        return "Simulation\\nMOLDYN"

    def summary(self):
        return "Calculates the angular auto-correlation of molecules in a simulation along a user-defined axis. The axis is defined by the vector connecting the average position of species two and the average position of species one (user input). Timestep must be specified in femtoseconds."

    def PyInit(self):
        self.declareProperty(FileProperty('InputFile','',action=FileAction.Load),doc="Input .nc file with an MMTK trajectory")

        self.declareProperty("Timestep",'1.0',direction=Direction.Input,doc="Time step between two coordinates in the trajectory, fs")

        self.declareProperty("SpeciesOne",'',direction=Direction.Input,doc="Specify the first species, e.g. H, He, Li...")
        self.declareProperty("SpeciesTwo",'',direction=Direction.Input,doc="Specify the second species, e.g. H, He, Li...")

        self.declareProperty(WorkspaceProperty('OutputWorkspace','',direction=Direction.Output),doc="Output workspace name")
        self.declareProperty(WorkspaceProperty('OutputWorkspaceFT','',direction=Direction.Output),doc="FT utput workspace name")

    def PyExec(self):
        # Get file path
        file_name=self.getPropertyValue("InputFile")

        # Get the two user-specified species
        type1=self.getPropertyValue("SpeciesOne")
        type2=self.getPropertyValue("SpeciesTwo")

        # Load trajectory file
        trajectory=netcdf.netcdf_file(file_name,mode="r")

        logger.information("Loading particle id's, molecule id's and coordinate array...")
        start_time=time.time()

        # netcdf object containing the particle id numbers
        description=(trajectory.variables["description"])[:]

        # Convert description object to string via for loop. The original object has strange formatting
        s=''
        for i in description:
            s+=i

        # Extract particle id's from string using regular expressions
        p_atoms=re.findall(r"A\('[a-z]+\d+',\d+",s)

        # Split the string s by molecules
        molecules=s.split("AC")

        # Remove first item of molecule list (contains initialisation of variable 'description')
        del molecules[0]

        # Many-to-one structures. Identify the set of atomic species present (list structure 'elements') in the simulation and repackage particles into a dictionary 'particles_to_species' with structure id number -> species
        atoms_to_species={}
        species_to_atoms={}
        elements=[]

        # Populate the particles_to_species dictionary and the elements list
        for j in p_atoms:
            key=re.findall(r"',\d+",j)[0]
            key=int(re.findall(r"\d+",key)[0])
            element=re.findall(r"[a-z]+",j)[0]
            if element not in elements:
                elements.append(str(element))
            atoms_to_species[key]=str(element)

        # Check wether user-specified species present in the trajectory file
        if type1.lower() not in elements:
            raise RuntimeError("Species one not found in the trajectory file. Please try again...")
        if type2.lower() not in elements:
            raise RuntimeError("Species two not found in the trajectory file. Please try again...")

        # Initialise lists in the species_to_particles dictionary
        for j in elements:
            species_to_atoms[j]=[]

        # Populate the species_to_particles dictionary
        for j in p_atoms:
            key=re.findall(r"',\d+",j)[0]
            key=int(re.findall(r"\d+",key)[0])
            element=re.findall(r"[a-z]+",j)[0]
            species_to_atoms[element].append(key)

        # Many-to-one structures. Assign atom indices to molecule indices using a dictionaries with structures atom id -> molecule id and molecule id -> list of atoms ids
        atoms_to_molecules={}
        molecules_to_atoms={}

        # Initialise lists in the molecule_to_atom dictionary
        for k in range(len(molecules)):
            molecules_to_atoms[k]=[]

        # Populate the dictionaries with atoms & molecules
        for k in range(len(molecules)):
            r_atoms=re.findall(r"A\('[a-z]+\d+',\d+",molecules[k])
            for l in r_atoms:
                key=re.findall(r"',\d+",l)[0]
                key=int(re.findall(r"\d+",key)[0])
                atoms_to_molecules[key]=k
                molecules_to_atoms[k].append(key)

        # Coordinate array. Shape: timesteps x (# of particles) x (# of spatial dimensions)
        configuration=trajectory.variables["configuration"]


        # Extract useful simulation parameters
        # Number of species present in the simulation
        n_species=len(elements)
        # Number of particles present in the simulation
        n_particles=len(p_atoms)
        # Number of molecules present in the simulation
        n_molecules=len(molecules)
        # Number of timesteps in the simulation
        n_timesteps=int(configuration.shape[0])
        # Number of spatial dimensions
        n_dimensions=int(configuration.shape[2])

        logger.information(str(time.time()-start_time) + " s")


        logger.information("Transforming coordinates...")
        start_time=time.time()

        # Box size for each timestep. Shape: timesteps x (3 consecutive 3-vectors)
        box_size=trajectory.variables["box_size"]

        # Reshape the paralellepipeds into 3x3 tensors for coordinate transformations. Shape: (# of timesteps) x (3-vectors) x (# of spatial dimensions)
        box_size_tensors=10.0*np.array([box_size[j].reshape((3,3)) for j in range(n_timesteps)])

        # Extract box dimensions (assuming orthorhombic simulation cell, diagonal matrix)
        box_size_x=np.array([box_size_tensors[i,0,0] for i in range(n_timesteps)])
        box_size_y=np.array([box_size_tensors[i,1,1] for i in range(n_timesteps)])
        box_size_z=np.array([box_size_tensors[i,2,2] for i in range(n_timesteps)])

        # Copy the configuration object into a numpy array
        configuration_copy=np.array([configuration[i] for i in range(n_timesteps)])

        # Swap the time and particle axes
        configuration_copy=np.swapaxes(configuration_copy,0,1)

        # Transform particle trajectories (configuration array) to Cartesian coordinates at each time step.
        cartesian_configuration=np.array([[np.dot(box_size_tensors[j],np.transpose(configuration_copy[i,j])) for j in range(n_timesteps)] for i in range(n_particles)])

        logger.information(str(time.time()-start_time) + " s")


        logger.information("Calculating orientation vectors...")
        start_time=time.time()

        # Initialise orientation vector array. Shape: (# of molecules) x (# of timesteps) x (# of dimensions)
        orientation_vectors=np.zeros((n_molecules,n_timesteps,n_dimensions))

        for i in range(n_molecules):
            # Retrieve constituents of the ith molecule
            temp=molecules_to_atoms[i]
            # Find which constituents belong to species one and which belong to species two
            species_one=[]
            species_two=[]
            for j in temp:
                if atoms_to_species[j]==type1.lower():
                    species_one.append(j)
                if atoms_to_species[j]==type2.lower():
                    species_two.append(j)
            # Find the average positions and the orientation vector
            sum_position_species_one=np.zeros((n_timesteps,n_dimensions))
            sum_position_species_two=np.zeros((n_timesteps,n_dimensions))

            for k in species_one:
                sum_position_species_one+=cartesian_configuration[k]
            for l in species_two:
                sum_position_species_two+=cartesian_configuration[l]
            avg_position_species_one=1.0*sum_position_species_one/len(species_one)
            avg_position_species_two=1.0*sum_position_species_two/len(species_two)

            # Find the vectors connecting the two atoms
            vectors=avg_position_species_two-avg_position_species_one

            # Scaled coordinates
            dx=np.divide(vectors[:,0],box_size_x)
            dy=np.divide(vectors[:,1],box_size_y)
            dz=np.divide(vectors[:,2],box_size_z)

            # Wrapping the vectors
            x=np.array([(dx[k]-round(dx[k]))*box_size_x[k] for k in range(n_timesteps)])
            y=np.array([(dy[k]-round(dy[k]))*box_size_y[k] for k in range(n_timesteps)])
            z=np.array([(dz[k]-round(dz[k]))*box_size_z[k] for k in range(n_timesteps)])

            # Normalisation
            norm=np.sqrt(x*x+y*y+z*z)
            x=np.divide(x,norm)
            y=np.divide(y,norm)
            z=np.divide(z,norm)

            # Store calculations in the orientation_vectors array
            orientation_vectors[i]=np.swapaxes(np.array([x,y,z]),0,1)

        logger.information(str(time.time()-start_time) + " s")


        logger.information("Calculating angular auto-correlations...")
        start_time=time.time()

        R_avg=np.zeros(n_timesteps)
        for i in range(n_molecules):
            R_avg+=self.auto_correlation(orientation_vectors[i])

        R_avg=1.0*R_avg/n_molecules

        logger.information(str(time.time()-start_time)+" s")


        # Initialise & populate the output_ws workspace
        nrows=1
        nbins=n_timesteps
        step=float(self.getPropertyValue("Timestep"))
        xvals=np.arange(0,np.ceil((n_timesteps)/2.0))*step/1000.0
        yvals=np.empty(0)
        # Store folded angular auto-correlation function
        yvals=np.append(yvals,self.fold_correlation(R_avg))
        evals=np.zeros(np.shape(yvals))

        output_name=self.getPropertyValue("OutputWorkspace")
        output_ws=CreateWorkspace(OutputWorkspace=output_name,DataX=xvals,UnitX="ps",DataY=yvals,DataE=evals,NSpec=nrows,VerticalAxisUnit="Text",VerticalAxisValues=["Axis 1"])
        # Set output workspace to output_ws
        self.setProperty('OutputWorkspace',output_ws)

        #Fourier transform output to workspace
        nrows=1
        yvals=np.empty(0)
        yvals=np.append(yvals,np.fft.rfft(R_avg))
        evals=np.zeros(np.shape(yvals))
        xvals=np.arange(0,np.shape(yvals)[0])

        FT_output_name=self.getPropertyValue("OutputWorkspaceFT")
        FT_output_ws=CreateWorkspace(OutputWorkspace=FT_output_name,DataX=xvals,UnitX="fs",DataY=yvals,DataE=evals,NSpec=nrows,VerticalAxisUnit="Text",VerticalAxisValues=["FT Axis 1"])
        self.setProperty("OutputWorkspaceFT",FT_output_ws)


    def auto_correlation(self,v):
        # Returns angular auto-correlation of a normalised time-dependent 3-vector
        n=np.shape(v)[0]
        norm=np.arange(np.ceil(n/2.0),n+1)
        norm=np.append(norm,(np.arange(n/2+1,n)[::-1]))

        # x dimension
        R=np.divide(np.correlate(v[:,0],v[:,0],"same"),norm)
        # y dimension
        R+=np.divide(np.correlate(v[:,1],v[:,1],"same"),norm)
        # z dimension
        R+=np.divide(np.correlate(v[:,2],v[:,2],"same"),norm)

        return R


    def fold_correlation(self,w):
        # Folds an array with symmetrical values into half by averaging values around the centre
        right_half=w[len(w)/2:]
        left_half=w[:int(np.ceil(len(w)/2.0))][::-1]

        return (left_half+right_half)/2.0


# Subscribe algorithm to Mantid software
AlgorithmFactory.subscribe(AngularAutoCorrelationsSingleAxis)