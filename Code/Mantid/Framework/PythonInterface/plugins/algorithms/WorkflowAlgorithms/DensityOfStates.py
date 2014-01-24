"""*WIKI* 

Calculates the Density of States from either a .phonon or .castep file.

*WIKI*"""
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import scipy.constants
import numpy as np
import re
import os.path
import math

class DensityOfStates(PythonAlgorithm):
 
		def PyInit(self):
			#declare properties
			self.declareProperty(FileProperty('File', '', action=FileAction.Load, 
				extensions = ["phonon", "castep"]),
				doc='Filename of the file.')
			
			self.declareProperty(name='Function',defaultValue='Gaussian', 
				validator=StringListValidator(['Gaussian', 'Lorentzian']), 
				doc="Type of function to fit to peaks.")

			self.declareProperty(name='Width', defaultValue=10.0, 
				doc='Set Gaussian/Lorentzian FWHM for broadening. Default is 10')
			
			self.declareProperty(name='SpectrumType',defaultValue='DOS', 
				validator=StringListValidator(['DOS', 'IR_Active', 'Raman_Active']), 
				doc="Type of intensities to extract and model (fundamentals-only) from .phonon.")
			
			self.declareProperty(name='Scale', defaultValue=1.0, 
				doc='Scale the intesity by the given factor. Default is no scaling.')

			self.declareProperty(name='BinWidth', defaultValue=1.0, 
				doc='Set histogram resolution for binning (eV or cm**-1). Default is 1')

			self.declareProperty(name='Temperature', defaultValue=300, 
				doc='Temperature to use (in raman spectrum modelling). Default is 300')
			
			self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output), 
			  doc="Name to give the output workspace.")
			
			#regex pattern for a floating point number
			self.fnumber = '\-?(?:\d+\.?\d*|\d*\.?\d+)'
			self.fileType = '.phonon'
			self.fzerotol = 3.0
			self.base = 1.0e21

			#number of modes and ions
			self.nions = 0
			self.nmodes = 0
   
		def PyExec(self):
			# Run the algorithm
			self.getProperties()

			fname = self.getPropertyValue('File')
			freqs, irIntens, ramanIntens, weights = self.readDataFromFile(fname)

			if self.specType == 'DOS':
				self.computeDos(freqs, np.ones(freqs.shape), weights)

				#set y units on output workspace
				mtd[self.wsName].setYUnit('(D/A)^2/amu')
				mtd[self.wsName].setYUnitLabel('Intensity')

			elif self.specType == 'IR_Active':
				if irIntens.size == 0:
					raise ValueError("Could not load any IR intensities from file.")
				
				self.computeDos(freqs, irIntens, weights)
				#set y units on output workspace
				mtd[self.wsName].setYUnit('(D/A)^2/amu')
				mtd[self.wsName].setYUnitLabel('Intensity')

			elif self.specType == 'Raman_Active':
				if ramanIntens.size == 0:
					raise ValueError("Could not load any Raman intensities from file.")

				self.computeRaman(freqs, ramanIntens, weights)
				#set y units on output workspace
				mtd[self.wsName].setYUnit('A^4')
				mtd[self.wsName].setYUnitLabel('Intensity')

			self.setProperty("OutputWorkspace", self.wsName)

		def getProperties(self):
			"""
			Set the properties passed to the algorithm
			"""
			self.temperature = self.getProperty('Temperature').value
			self.binWidth = self.getProperty('BinWidth').value
			self.specType = self.getPropertyValue('SpectrumType')
			self.peakFunc = self.getPropertyValue('Function')
			self.wsName = self.getPropertyValue('OutputWorkspace')
			self.width = self.getProperty('Width').value
			self.scale = self.getProperty('Scale').value

		def fitPeaks(self, hist, peaks):
			"""
			Fit Gaussian or Lorentzian peaks to each peak in the data

			@param hist - array of counts for each bin
			@param peaks - the indicies of each non-zero point in the data 
			@return the fitted y data
			"""
			
			if self.peakFunc == "Gaussian":
				nGauss = int( 3.0* self.width / self.binWidth )
				sigma = self.width / 2.354

				dos = np.zeros(len(hist)-1 + nGauss)

				for index in peaks:
					for g in range(-nGauss, nGauss):
						if index + g > 0:
							dos[index+g] += hist[index] * math.exp( - (g * self.binWidth)**2 / (2 * sigma **2)) / (math.sqrt(2*math.pi) * sigma)
				
			elif self.peakFunc == "Lorentzian":
				
				nLorentz = int( 25.0 * self.width / self.binWidth )
				gammaby2 = self.width / 2

				dos = np.zeros(len(hist)-1 + nLorentz)
				
				for index in peaks:
					for l in range(-nLorentz, nLorentz):
						if index + l > 0:
							dos[index+l] += hist[index] * gammaby2 / ( l ** 2 + gammaby2 **2 ) / math.pi
				
			return dos

		def computeDos(self, freqs, intensities, weights):
			"""
			Compute Density Of States

			@param freqs - frequencies read from file
			@param intensities - intensities read from file
			@param weights - weights for each frequency block
			"""

			print freqs.size, intensities.size
			if ( freqs.size > intensities.size ):
				#if we have less intensities than frequencies fill the difference with ones.
				diff = freqs.size-intensities.size
				intensities = np.concatenate((intensities, np.ones(diff)))
			
			if ( freqs.size != weights.size or freqs.size != intensities.size ):
				raise ValueError("Number of data points must match!")

			#ignore values below fzerotol
			fzeroMask = np.where(np.absolute(freqs) < self.fzerotol)
			intensities[fzeroMask] = 0.0
				
			#sort data to follow natural ordering
			permutation = freqs.argsort()
			freqs = freqs[permutation]
			intensities = intensities[permutation]
			weights = weights[permutation]
				
			#weight intensities
			intensities = intensities * weights
			
			#create histogram x data
			xmin, xmax = freqs[0], freqs[-1]+self.binWidth
			print xmin, xmax, self.binWidth
			bins = np.arange(xmin, xmax, self.binWidth)

			#sum values in each bin
			hist = np.zeros(bins.size)
			for index, (lower, upper) in enumerate(zip(bins, bins[1:])):
				binMask = np.where((freqs >= lower) & (freqs < upper))
				hist[index] = intensities[binMask].sum()

			#find and fit peaks
			peaks = hist.nonzero()[0]
			dos = self.fitPeaks(hist, peaks)

			dataX = np.arange(xmin, xmin+(dos.size/self.binWidth), self.binWidth)
			CreateWorkspace(DataX=dataX, DataY=dos, OutputWorkspace=self.wsName)
			unitx = mtd[self.wsName].getAxis(0).setUnit("Label")
			unitx.setLabel("Energy Shift", 'cm^-1')

			if self.scale != 1:
				Scale(InputWorkspace=self.wsName, OutputWorkspace=self.wsName, Factor=self.scale)
	
		def computeRaman(self, freqs, intensities, weights):
			'''
			Compute Raman intensities

			@param freqs - frequencies read from file
			@param intensities - raman intensities read from file
			@param weights - weights for each frequency block
			'''
			#we only want to use the first set
			freqs = freqs[:self.num_branches]
			intensities = intensities[:self.num_branches]
			weights = weights[:self.num_branches]

			#speed of light in vaccum in m/s
			c = scipy.constants.c
			#wavelength of the laser
			laser_wavelength = 514.5e-9
			#planck's constant
			planck = scipy.constants.h
			# cm(-1) => K conversion 
			cm1_to_K = scipy.constants.codata.value('inverse meter-kelvin relationship')*100
			factor = (math.pow((2*math.pi / laser_wavelength), 4) * planck) / (8 * math.pi**2 * 45) * 1e12
			crossSections = np.zeros(len(freqs))

			#use only the first set of frequencies and ignore small values
			xSecMask = np.where( freqs > self.fzerotol )
			frequencyXSections = freqs[xSecMask]
			intensityXSections = intensities[xSecMask]
			
			boseOcc = 1.0 / ( np.exp(cm1_to_K * frequencyXSections / self.temperature) -1)
			crossSections[xSecMask] = factor / frequencyXSections * (1 + boseOcc) * intensityXSections
			
			self.computeDos(freqs, crossSections, weights)

		def readDataFromFile(self, fname):
			'''
			Select the appropriate file parser and check data was successfully 
			loaded from file.

			@param fname - path to the file.
			@return tuple of the frequencies, ir and raman intensities and weights
			'''
			ext = os.path.splitext(fname)[1]
			self.fileType = ext

			if ext == '.phonon':
				file_data = self.parsePhononFile(fname)

			elif ext == '.castep':
				file_data = self.parseCastepFile(fname)

			freqs, irIntens, ramanIntens, weights = file_data

			if ( freqs.size == 0 ):
				raise ValueError("Failed to load any frequencies from file.")

			if ( ramanIntens.size == 0 and ramanIntens.size == 0 ):
				raise ValueError("Failed to load any intensities from file.")

			return freqs, irIntens, ramanIntens, weights

		def parseBlockHeader(self, header_match, block_count):
			"""
			Parse the header of a block of frequencies and intensities

			@param header_match - the regex match to the header
			@param block_count - the count of blocks found so far 
			@return weight for this block of values
			"""
			#found header block at start of frequencies
			q1, q2, q3, weight = map(float, header_match.groups())
			if block_count > 1 and sum([q1,q2,q3]) == 0:
				weight = 0.0
			return weight
						
		def parsePhononFileHeader(self, f_handle):
			'''
			Read information from the header of a <>.phonon file

			@param f_handle - handle to the file.
			@return tuple of the number of ions and branches in the file
			'''

			while True:
				line = f_handle.readline()

				if not line:
					raise IOError("Could not find any header information.")

				if 'Number of ions' in line:
					num_ions = int(line.strip().split()[-1])
				elif 'Number of branches' in line:
					num_branches = int(line.strip().split()[-1])

				if 'END header' in line: 
					return num_ions, num_branches

		def parsePhononFreqBlock(self, f_handle):
				for _ in xrange( self.num_branches):
					line = f_handle.readline()
					line_data = line.strip().split()[1:]
					line_data = map(float, line_data)
					yield line_data

		def parsePhononFile(self, fname):
			'''
			Read frequencies from a <>.phonon file

			@param fname - file path of the file to read
			@return the frequencies, infra red and raman intensities and weights of frequency blocks
			'''
			
			#Header regex. Looks for lines in the following format:
			#     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
			headerRegexStr = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': self.fnumber}
			headerRegex = re.compile(headerRegexStr)

			eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
			block_count = 0

			freqs, ir_intensities, raman_intensities, weights = [], [], [], []
			data_lists = (freqs, ir_intensities, raman_intensities)
			with open(fname, 'r') as f_handle:
				self.num_ions, self.num_branches = self.parsePhononFileHeader(f_handle)

				while True:
					line = f_handle.readline()
					#check we've reached the end of file
					if not line: break

					#check if we've found a block of frequencies
					header_match = headerRegex.match(line)
					if header_match:
						block_count += 1

						weight = self.parseBlockHeader(header_match, block_count)
						weights.append(weight)
						
						#parse block of frequencies

						for line_data in self.parsePhononFreqBlock(f_handle):
							for data_list, item in zip(data_lists, line_data):
								data_list.append(item)

					#skip over eigenvectors
					vector_match = eigenvectors_regex.match(line)
					if vector_match:
						for _ in xrange(self.num_ions*self.num_branches):
							line = f_handle.readline()
							if not line:
								raise IOError("Could not parse file. Invalid file format.")
			
			freqs = np.asarray(freqs) 
			ir_intensities = np.asarray(ir_intensities)
			raman_intensities = np.asarray(raman_intensities)
			warray = np.repeat(weights, self.num_branches)

			return freqs, ir_intensities, raman_intensities, warray

		def parseCastepFileHeader(self, f_handle):
			'''
			Read information from the header of a <>.castep file

			@param f_handle - handle to the file.
			@return tuple of the number of ions and branches in the file
			'''

			num_species, num_ions = 0,0
			while True:
				line = f_handle.readline()

				if not line:
					raise IOError("Could not find any header information.")

				if 'Total number of ions in cell =' in line:
					num_ions = int(line.strip().split()[-1])
				elif 'Total number of species in cell = ' in line:
					num_species = int(line.strip().split()[-1])

				if num_species > 0 and num_ions > 0:
					num_branches = num_species*num_ions 
					return num_ions, num_branches

		def parseCastepFreqBlock(self, f_handle, line):
			for _ in xrange(self.num_branches):
				line_data = line.strip().split()[1:-1]
				freq = line_data[1]
				intensity_data = line_data[3:]

				#remove non-active intensities from data
				intensities = []
				for value, active in zip(intensity_data[::2], intensity_data[1::2]):
					if self.specType == 'IR_Active' or self.specType == 'Raman_Active':
						if active == 'N' and value != 0: 
							value = 0.0
					intensities.append(value)

				line_data = [freq] + intensities
				line_data = map(float, line_data)
				yield line_data
				line = f_handle.readline()

		def parseCastepFile(self, fname):
			'''
			Read frequencies from a <>.castep file

			@param fname - file path of the file to read
			@return the frequencies, infra red and raman intensities and weights of frequency blocks
			'''
			#Header regex. Looks for lines in the following format:
			# +  q-pt=    1 (  0.000000  0.000000  0.000000)     1.0000000000              +
			headerRegexStr = r" +\+ +q-pt= +\d+ \( *(?: *(%(s)s)) *(%(s)s) *(%(s)s)\) +(%(s)s) +\+" % {'s' : self.fnumber}
			headerRegex = re.compile(headerRegexStr)

			#Data regex. Looks for lines in the following format:
 			# +     1      -0.051481   a          0.0000000  N            0.0000000  N     + 
			dataRegexStr = r" +\+ +\d+ +(%(s)s)(?: +\w)? *(%(s)s)? *([YN])? *(%(s)s)? *([YN])? *\+"% {'s': self.fnumber}
			dataRegex = re.compile(dataRegexStr)

			block_count = 0
			freqs, ir_intensities, raman_intensities, weights = [], [], [], []
			data_lists = (freqs, ir_intensities, raman_intensities)
			with open(fname, 'r') as f_handle:
				self.num_ions, self.num_branches = self.parseCastepFileHeader(f_handle)

				while True:
					line = f_handle.readline()
					#check we've reached the end of file
					if not line: break

					#check if we've found a block of frequencies
					header_match = headerRegex.match(line)
					if header_match:
						block_count += 1
						weight = self.parseBlockHeader(header_match, block_count)
						weights.append(weight)
						
						#move file pointer forward to start of intensity data
						while True:
							line = f_handle.readline()
							if not line: 
								raise IOError("Could not parse frequency block. Invalid file format.")
							if dataRegex.match(line): break

						#parse block of frequencies
						for line_data in self.parseCastepFreqBlock(f_handle, line):
							for data_list, item in zip(data_lists, line_data):
								data_list.append(item)

			freqs = np.asarray(freqs) 
			ir_intensities = np.asarray(ir_intensities)
			raman_intensities = np.asarray(raman_intensities)
			warray = np.repeat(weights, self.num_branches)

			return freqs, ir_intensities, raman_intensities, warray

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DensityOfStates)