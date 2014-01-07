"""*WIKI* 

TODO

*WIKI*"""
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

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
			
			self.declareProperty(name='FitFunction',defaultValue='Gaussian', 
				validator=StringListValidator(['Gaussian', 'Lorentzian']), 
				doc="Type of function to fit to peaks.")

			self.declareProperty(name='Width', defaultValue=10.0, 
				doc='Set Gaussian/Lorentzian FWHM for broadening. Default is 10')
			
			self.declareProperty(name='SpectrumType',defaultValue='IR', 
				validator=StringListValidator(['DOS', 'IR Active', 'Raman Active']), 
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

			if self.specType == 'DOS' or self.specType == 'IR Active':

				if len(irIntens) == 0:
					raise ValueError("Could not load any IR intensities from file.")

				self.computeDos(freqs, irIntens, weights)

				#set y units on output workspace
				unity = mtd[self.wsName].getAxis(1).setUnit("Label")
				unity.setLabel("IR Intensity", '(D/A)^2/amu')

			elif self.specType == 'Raman Active':

				if len(ramanIntens) == 0:
					raise ValueError("Could not load any Raman intensities from file.")

				self.computeRaman(freqs, ramanIntens, weights)

				#set y units on output workspace
				unity = mtd[self.wsName].getAxis(1).setUnit("Label")
				unity.setLabel("Raman Intensity", 'A^4')

			self.setProperty("OutputWorkspace", self.wsName)

		def getProperties(self):
			"""
			Set the properties passed to the algorithm
			"""
			self.temperature = self.getProperty('Temperature').value
			self.binWidth = self.getProperty('BinWidth').value
			self.specType = self.getPropertyValue('SpectrumType')
			self.peakFunc = self.getPropertyValue('FitFunction')
			self.wsName = self.getPropertyValue('OutputWorkspace')
			self.width = self.getProperty('Width').value
			self.scale = self.getProperty('Scale').value


		def findContent(self, regex, content):
			"""
			Search a string to file the groups in the regex

			@param regex - the regex pattern to search for
			@param content - the string to search in
			@retrun the matched groups in the string
			"""
			match = re.search(regex, content, re.DOTALL)

			if match:
				return match.groups()
			else:
				raise Exception('Error parsing file. Failed to find content.')

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
			#flatten arrays
			freqs = np.hstack(freqs)
			intensities = np.hstack(intensities)
			
			if ( self.base > 1.0e20 ):
				self.base = freqs[0]

			if ( len(freqs) > len(intensities) ):
				#if we have less intensities than frequencies fill the difference with ones.
				diff = len(freqs)-len(intensities)
				intensities = np.concatenate((intensities, np.ones(diff)))

				#ignore values below fzerotol
				fzeroMask = np.where(freqs < self.fzerotol)
				intensities[fzeroMask] = 0
				
			#sort data to follow natural ordering
			permutation = freqs.argsort()
			freqs = freqs[permutation]
			intensities = intensities[permutation]
			weights = weights[permutation]
				
			if ( len(freqs) != len(weights) ):
				raise ValueError("Number of data points must match!")

			#weight intensities
			intensities = intensities * weights
			
			#create histogram x data
			xmin, xmax = self.base, freqs[-1]+self.binWidth
			bins = np.arange(xmin, xmax, self.binWidth)

			#sum values in each bin
			hist = np.zeros(len(bins))
			for index, (lower, upper) in enumerate(zip(bins, bins[1:])):
				binMask = np.where((freqs >= lower) & (freqs < upper))
				hist[index] = intensities[binMask].sum()

			#find and fit peaks
			peaks = hist.nonzero()[0]
			dos = self.fitPeaks(hist, peaks)

			dataX = np.arange(xmin, xmin+(len(dos)/self.binWidth), self.binWidth)
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
			#speed of light in m/s
			c = 299792458
			#wavelength of the laser
			laserWavelength = 514.5e-9
			#planck's constant
			planck = 6.6260755e-34
			# cm(-1) => K conversion 
			cm1k = 1.438769

			factor = (math.pow((2*math.pi / laserWavelength), 4) * planck) / (8 * math.pi**2 * 45) * 1e12

			crossSections = np.zeros(len(freqs[0]))

			#use only the first set of frequencies and ignore small values
			xSecMask = np.where( freqs[0] > self.fzerotol )
			frequencyXSections = freqs[0][xSecMask]
			intensityXSections = intensities[0][xSecMask]
			
			boseOcc = 1.0 / ( np.exp(cm1k * frequencyXSections / self.temperature) -1)
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
				freqs, irIntens, ramanIntens, weights = self.parsePhononFile(fname)

			elif ext == '.castep':
				freqs, irIntens, ramanIntens, weights = self.parseCastepFile(fname)

			if ( len(freqs) == 0 ):
				raise ValueError("Failed to load any frequencies from file.")

			if ( len(ramanIntens) == 0 and len(irIntens) == 0 ):
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
			if block_count > 1 and sum([q1,q2,q3]) == 0 and self.specType != 'DOS':
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

			freqs, ir_intensities, raman_intensities, weights = [], [], [], []
			with open(fname, 'r') as f_handle:
				num_ions, num_branches = self.parsePhononFileHeader(f_handle)
				block_count = 0
				prog_reporter = Progress(self,start=0.0,end=1.0, nreports=1)

				while True:
					line = f_handle.readline()
					#check we've reached the end of file
					if not line: break

					header_match = headerRegex.match(line)
					if header_match:
						#found header block at start of frequencies
						block_count+=1
						weight = self.parseBlockHeader(header_match, block_count)
						weights.append(weight)
						prog_reporter.setNumSteps(block_count+1)
						
						#parse block of frequencies
						block_freqs, block_ir, block_raman = [], [], []
						for _ in xrange(num_branches):
							line = f_handle.readline()
							line_data = line.strip().split()[1:]

							data_lists = (block_freqs, block_ir, block_raman)
							for data_list, data_item in zip(data_lists, line_data):
								if data_item:
									data_list.append(float(data_item))

						freqs.append(block_freqs)
						ir_intensities.append(block_ir)
						raman_intensities.append(block_raman)

						prog_reporter.report("Reading intensities.")

					#skip over eigenvectors
					vector_match = eigenvectors_regex.match(line)
					if vector_match:
						for _ in xrange(num_ions*num_branches):
							line = f_handle.readline()
							if not line:
								raise IOError("Could not parse file. Invalid file format.")
			
			freqs = np.asarray(freqs) 
			ir_intensities = np.asarray(ir_intensities)
			raman_intensities = np.asarray(raman_intensities)
			warray = np.repeat(weights, len(freqs[0]))

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

			freqs, ir_intensities, raman_intensities, weights = [], [], [], []
			with open(fname, 'r') as f_handle:
				num_ions, num_branches = self.parseCastepFileHeader(f_handle)
				block_count = 0
				prog_reporter = Progress(self,start=0.0,end=1.0, nreports=1)

				while True:
					line = f_handle.readline()
					#check we've reached the end of file
					if not line: break

					header_match = headerRegex.match(line)
					if header_match:
						#found header block at start of frequencies
						block_count+=1
						weight = self.parseBlockHeader(header_match, block_count)
						weights.append(weight)
						prog_reporter.setNumSteps(block_count+1)
						
						#move file pointer forward to start of intensity data
						while True:
							line = f_handle.readline()
							if not line: 
								raise IOError("Could not parse frequency block. Invalid file format.")
							if dataRegex.match(line): break

						#parse block of frequencies
						block_freqs, block_ir, block_raman = [], [], []
						for _ in xrange(num_branches):
							line_data = line.strip().split()[1:-1]
							freq = line_data[1]
							intensity_data = line_data[3:]

							#remove non-active intensities from data
							intensities = []
							for value, active in zip(intensity_data[::2], intensity_data[1::2]):
								if active == 'N':	value = 0.0
								intensities.append(value)

							#append data to block lists
							data_lists = (block_freqs, block_ir, block_raman)
							for data_list, data_item in zip(data_lists, [freq] + intensities):
								if data_item:
									data_list.append(float(data_item))

							line = f_handle.readline()

						freqs.append(block_freqs)
						ir_intensities.append(block_ir)
						raman_intensities.append(block_raman)

						prog_reporter.report("Reading intensities.")

			freqs = np.asarray(freqs) 
			ir_intensities = np.asarray(ir_intensities)
			raman_intensities = np.asarray(raman_intensities)
			warray = np.repeat(weights, len(freqs[0]))

			return freqs, ir_intensities, raman_intensities, warray

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DensityOfStates)