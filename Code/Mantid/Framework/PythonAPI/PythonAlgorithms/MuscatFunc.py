# Algorithm to start Bayes programs
from MantidFramework import *
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectMuscat as Main

class MuscatFunc(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']))
		self.declareProperty(Name='Geom',DefaultValue='Flat',Validator=ListValidator(['Flat','Cyl']),Description = '')
		self.declareProperty(Name='Dispersion',DefaultValue='Poly',Validator=ListValidator(['Poly','CE','SS']),Description = '')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = '')
		self.declareProperty(Name='NR1', DefaultValue=1000)
		self.declareProperty(Name='NR2', DefaultValue=1000)
		self.declareProperty(Name='Nms', DefaultValue=1)
		self.declareProperty(Name='DetAngle', DefaultValue=0.0)
		self.declareProperty(Name='NQ', DefaultValue=10)
		self.declareProperty(Name='dQ', DefaultValue=0.2)
		self.declareProperty(Name='NW', DefaultValue=100)
		self.declareProperty(Name='dW', DefaultValue=2.0)
		self.declareProperty(Name='Coeff1', DefaultValue=0.)
		self.declareProperty(Name='Coeff2', DefaultValue=0.)
		self.declareProperty(Name='Coeff3', DefaultValue=50.0)
		self.declareProperty(Name='Coeff4', DefaultValue=0.)
		self.declareProperty(Name='Coeff5', DefaultValue=0.)
		self.declareProperty(Name='Thick', DefaultValue='',Validator=MandatoryValidator())
		self.declareProperty(Name='Width', DefaultValue='',Validator=MandatoryValidator())
		self.declareProperty(Name='Height', DefaultValue=3.0)
		self.declareProperty(Name='Density', DefaultValue=0.1)
		self.declareProperty(Name='SigScat', DefaultValue=5.0)
		self.declareProperty(Name='SigAbs', DefaultValue=0.1)
		self.declareProperty(Name='Temperature', DefaultValue=300.0)
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Totals','Scat1','All']))
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):
		run_f2py_compatibility_test()

		self.log().information('Muscat input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		geom = self.getPropertyValue('Geom')
		disp = self.getPropertyValue('Dispersion')
		sam = self.getPropertyValue('SamNumber')
		sname = prefix+sam+'_'+ana

		NRUN1 = self.getPropertyValue('NR1')
		NRUN2 = self.getPropertyValue('NR2')
		NMST = self.getPropertyValue('Nms')
		JRAND = 12345
		MRAND = 67890
		neut = [NRUN1, NRUN2, JRAND, MRAND, NMST]

		HEIGHT = 3.0
		alfa = self.getPropertyValue('DetAngle')
		THICK = self.getPropertyValue('Thick')
		WIDTH = self.getPropertyValue('Width')
		HEIGHT = self.getPropertyValue('Height')
		if geom == 'Flat':
			beam = [THICK, WIDTH, HEIGHT, float(alfa)]
		if geom == 'Cyl':
			beam = [THICK, WIDTH, HEIGHT, 0.0]        #beam = [WIDTH, WIDTH2, HEIGHT, 0.0]
		dens = self.getPropertyValue('Density')
		sigb = self.getPropertyValue('SigScat')
		siga = self.getPropertyValue('SigAbs')
		sigss=sigb
		temp = self.getPropertyValue('Temperature')
		sam = [temp, dens, siga, sigb]

		NQ = self.getPropertyValue('NQ')
		dQ = self.getPropertyValue('dQ')
		Nw = self.getPropertyValue('NW')
		dw = self.getPropertyValue('dW')           #in microeV
		grid = [NQ, dQ, Nw, dw]
		c1 = self.getPropertyValue('Coeff1')
		c2 = self.getPropertyValue('Coeff2')
		c3 = self.getPropertyValue('Coeff3')
		c4 = self.getPropertyValue('Coeff4')
		c5 = self.getPropertyValue('Coeff5')
		coeff = [float(c1), float(c2), float(c3), float(c4), float(c5)]
		kr1 = 1

		verbOp = self.getProperty('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')
		Main.MuscatFuncStart(sname,geom,neut,beam,sam,grid,disp,coeff,kr1,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(MuscatFunc())         # Register algorithm with Mantid
