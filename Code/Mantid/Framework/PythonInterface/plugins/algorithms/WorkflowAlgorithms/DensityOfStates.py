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

			with open(fname, 'r') as fhandle:
				content = fhandle.read()
				
			ext = os.path.splitext(fname)[1]
			self.fileType = ext

			if ext == '.phonon':
				freqs, irIntens, ramanIntens, weights = self.readPhononFile(content)
			elif ext == '.castep':
				freqs, irIntens, ramanIntens, weights = self.readCastepFile(content)

			if ( len(freqs) == 0 ):
				raise ValueError("Failed to load any frequencies from file.")

			if ( len(ramanIntens) == 0 and len(irIntens) == 0 ):
				raise ValueError("Failed to load any intensities from file.")

			return freqs, irIntens, ramanIntens, weights

		def readCastepFile(self, content):
			'''
			Read frequencies from <>.castep file

			@param content - string of the data read from file
			@return the frequencies, infra red and raman intensities and weights of frequency blocks
			'''
			#regex patterns to extract data the header and data information from each block

			#Header regex. Looks for lines in the following format:
			# +  q-pt=    1 (  0.000000  0.000000  0.000000)     1.0000000000              +
			headerRegexStr = r" +\+ +q-pt= +\d+ \( *(?: *(%(s)s)) *(%(s)s) *(%(s)s)\) +(%(s)s) +\+" % {'s' : self.fnumber}

			#Data regex. Looks for lines in the following format:
 			# +     1      -0.051481   a          0.0000000  N            0.0000000  N     + 
			dataRegexStr = r" +\+ +\d+ +(%(s)s)(?: +\w)? *((?: *%(s)s?(?: +[YN])?){0,2}) *\+"% {'s': self.fnumber}

			# Find all of the frequency data blocks, also splits text into header and data groups 
			freqBlockRegexStr = r" \+ \-+ \+\s+( +\+ +q-pt= +\d+ \( *(?: *%(s)s) *%(s)s *%(s)s\) +%(s)s +\+)((?:[\+\w\-\d\s\<\>\=\(\)\*\/\\]|\-?%(s)s|Frequency irrep\.)*) +\+" % {'s': self.fnumber}

			return self.parseFrequencies(content, freqBlockRegexStr, headerRegexStr, dataRegexStr)

		def readPhononFile(self, content):
			'''
			Read frequencies from <>.phonon file

			@param content - string of the data read from file
			@return the frequencies, infra red and raman intensities and weights of frequency blocks
			'''
			#regex patterns to extract data the header and data information from each block

			#Header regex. Looks for lines in the following format:
			#     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
			headerRegexStr = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': self.fnumber}

			#Data regex. Looks for lines in the following format:
			#       1      -0.051481              0.0000000               0.0000000  
			dataRegexStr = r"\s+\d+ +(%(s)s)((?: +%(s)s){0,2})"% {'s': self.fnumber}

			# Find all of the frequency data blocks, also splits text into header and data groups 
			freqBlockRegexStr = r"( +q-pt=\s+\d+ +%(s)s +%(s)s +%(s)s (?: *%(s)s){0,4})((?:\s|%(s)s)*)Phonon Eigenvectors" % {'s': self.fnumber}

			return self.parseFrequencies(content, freqBlockRegexStr, headerRegexStr, dataRegexStr)

		def parseFrequencies(self, content, blockRegex, headerRegexStr, dataRegexStr):
			'''
			Extract frequencies, infra-red and raman intensities from the contents of a file.

			@param content - string of the data read from file
			@param freqBlockRegexStr - regex to split the frequncy blocks
			@param headerRegexStr - regex to extract the header from the block
			@param dataRegexStr - regex to extract a line of data from the block
			@return the frequencies, infra red and raman intensities and weights of frequency blocks
			'''

			headerRegex = re.compile(headerRegexStr)
			dataRegex = re.compile(dataRegexStr)

			freqBlocks = re.findall(blockRegex, content)
			nFreqBlocks = len(freqBlocks)

			if(nFreqBlocks <= 0): 
				raise Exception('Error parsing file. Failed to find any frequency blocks.')

			freqs, ir_intensities, raman_intensities, weights = [], [], [], []

			#iterate over each block of frequencies and intensities
			for i, freqBlock in enumerate(freqBlocks):
				header, data = freqBlock

				#extract data from header
				freqHeaderData = headerRegex.match(header)
				freqHeaderData = map (float, freqHeaderData.groups())
				q1, q2, q3, weight = freqHeaderData

				if i > 0 and sum([q1,q2,q3]) == 0 and self.specType != 'DOS':
					weight = 0.0
				
				#extract the frequency and intensity data from this block
				block_freqs, block_ir, block_raman = [], [], []
				for freq, intensities in dataRegex.findall(data):
					block_freqs.append(freq)

					#reformat intensities into list
					intensities = intensities.strip()
					
					if self.fileType == '.castep':
						#if using IR active we need to check which intensities were active
						if self.specType == 'IR Active':

							intensities = intensities.split()
							ir, active = intensities[:2]

							if active == 'N':
								intensities[0] = '0.0'

							intensities = ' '.join(intensities)
					
					intensities = re.sub('[YN]', '', intensities)
					intensities = intensities.split()

					#append any intensities found on this line to our arrays
					for value, arr in zip(intensities, (block_ir, block_raman)):
						arr.append(value.strip())

				#convert to numpy arrays and convert to floats
				block_freqs = np.array(block_freqs).astype(np.float)
				block_ir = np.array(block_ir).astype(np.float)
				block_raman = np.array(block_raman).astype(np.float)
				
				#append any columns found to the existing arrays
				for column, arr in zip((block_freqs, block_ir, block_raman), (freqs, ir_intensities, raman_intensities)):
					if len(column) > 0:
						arr.append(column)

				weights.append(weight)
			
			#convert weights into list of weights
			warray = np.repeat(weights, len(freqs[0]))
			
			return np.array(freqs), np.array(ir_intensities), np.array(raman_intensities), np.array(warray)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(DensityOfStates)