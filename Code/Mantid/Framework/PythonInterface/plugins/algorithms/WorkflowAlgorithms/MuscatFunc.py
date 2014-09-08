# Algorithm to start Bayes programs
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.api import PythonAlgorithm, AlgorithmFactory

class MuscatFunc(PythonAlgorithm):

    def category(self):
    	return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
    	return "Calculates multiple scattering using S(Q,w) from specified functions."

    def PyInit(self):
    	self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
    	self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']))
    	self.declareProperty(name='Geom',defaultValue='Flat',validator=StringListValidator(['Flat','Cyl']), doc='')
    	self.declareProperty(name='Dispersion',defaultValue='Poly',validator=StringListValidator(['Poly','CE','SS']), doc='')
    	self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample data run number')
    	self.declareProperty(name='NR1', defaultValue=1000, doc='MonteCarlo neutrons NR1. Default=1000')
    	self.declareProperty(name='NR2', defaultValue=1000, doc='MonteCarlo neutrons NR2. Default=1000')
    	self.declareProperty(name='Nms', defaultValue=1, doc='Number of scatterings. Default=1')
    	self.declareProperty(name='DetAngle', defaultValue=90.0, doc='Detector angle. Default=90.0')
    	self.declareProperty(name='NQ', defaultValue=10, doc='Q-w grid: number of Q values. Default=10')
    	self.declareProperty(name='dQ', defaultValue=0.2, doc='Q-w grid: Q increment. Default=0.2')
    	self.declareProperty(name='NW', defaultValue=100, doc='Q-w grid: number of w values. Default=100')
    	self.declareProperty(name='dW', defaultValue=2.0, doc='Q-w grid: w increment (microeV). Default=2.0')
    	self.declareProperty(name='Coeff1', defaultValue=0., doc='Coefficient 1. Default=0.0')
    	self.declareProperty(name='Coeff2', defaultValue=0., doc='Coefficient 2. Default=0.0')
    	self.declareProperty(name='Coeff3', defaultValue=50.0, doc='Coefficient 3. Default=50.0')
    	self.declareProperty(name='Coeff4', defaultValue=0., doc='Coefficient 4. Default=0.0')
    	self.declareProperty(name='Coeff5', defaultValue=0., doc='Coefficient 5. Default=0.0')
    	self.declareProperty(name='Thick', defaultValue='',validator=StringMandatoryValidator(), doc='Sample thickness')
    	self.declareProperty(name='Width', defaultValue='',validator=StringMandatoryValidator(), doc='Sample width')
    	self.declareProperty(name='Height', defaultValue=3.0, doc='Sample height. Default=3.0')
    	self.declareProperty(name='Density', defaultValue=0.1, doc='Sample density. Default=0.1')
    	self.declareProperty(name='SigScat', defaultValue=5.0, doc='Scattering cross-section. Default=5.0')
    	self.declareProperty(name='SigAbs', defaultValue=0.1, doc='Absorption cross-section. Default=0.1')
    	self.declareProperty(name='Temperature', defaultValue=300.0, doc='Sample temperature (K). Default=300.0')
    	self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','Totals','Scat1','All']))
    	self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
    	self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')

    def PyExec(self):
    	from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

    	if is_supported_f2py_platform():
    		import IndirectMuscat as Main

    	run_f2py_compatibility_test()

    	self.log().information('Muscat input')
    	prefix = self.getPropertyValue('Instrument')
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
    	neut = [int(NRUN1), int(NRUN2), int(JRAND), int(MRAND), int(NMST)]

    	HEIGHT = 3.0
    	alfa = self.getPropertyValue('DetAngle')
    	THICK = self.getPropertyValue('Thick')
    	WIDTH = self.getPropertyValue('Width')
    	HEIGHT = self.getPropertyValue('Height')
    	if geom == 'Flat':
    		beam = [float(THICK), float(WIDTH), float(HEIGHT), float(alfa)]
    	if geom == 'Cyl':
    		beam = [float(THICK), float(WIDTH), float(HEIGHT), 0.0]        #beam = [WIDTH, WIDTH2, HEIGHT, 0.0]
    	dens = self.getPropertyValue('Density')
    	sigb = self.getPropertyValue('SigScat')
    	siga = self.getPropertyValue('SigAbs')
    	sigss=sigb
    	temp = self.getPropertyValue('Temperature')
    	sam = [float(temp), float(dens), float(siga), float(sigb)]

    	NQ = self.getPropertyValue('NQ')
    	dQ = self.getPropertyValue('dQ')
    	Nw = self.getPropertyValue('NW')
    	dw = self.getPropertyValue('dW')           #in microeV
    	grid = [int(NQ), float(dQ), int(Nw), float(dw)]
    	c1 = self.getPropertyValue('Coeff1')
    	c2 = self.getPropertyValue('Coeff2')
    	c3 = self.getPropertyValue('Coeff3')
    	c4 = self.getPropertyValue('Coeff4')
    	c5 = self.getPropertyValue('Coeff5')
    	coeff = [float(c1), float(c2), float(c3), float(c4), float(c5)]
    	kr1 = 1

    	verbOp = self.getProperty('Verbose').value
    	plotOp = self.getPropertyValue('Plot')
    	saveOp = self.getProperty('Save').value
    	Main.MuscatFuncStart(sname,geom,neut,beam,sam,grid,disp,coeff,kr1,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(MuscatFunc)         # Register algorithm with Mantid
