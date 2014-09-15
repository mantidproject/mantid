# Algorithm to start Symmetrise
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator

class Symmetrise(PythonAlgorithm):

    def category(self):
    	return "Workflow\\MIDAS;PythonAlgorithms"

    def summary (self):
    	return "Takes and asymmetric S(Q,w) and makes it symmetric"

    def PyInit(self):
    	self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (_red.nxs) or Workspace')
    	self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
    	self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
    	self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
    	self.declareProperty(name='Xcut',defaultValue='',validator=StringMandatoryValidator(), doc='X cutoff value')
    	self.declareProperty('Verbose',defaultValue=True, doc='Switch Verbose Off/On')
    	self.declareProperty('Plot',defaultValue=True, doc='Switch Plot Off/On')
    	self.declareProperty('Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')

    def PyExec(self):

    	self.log().information('Symmetrise')
    	inType = self.getPropertyValue('InputType')
    	prefix = self.getPropertyValue('Instrument')
    	ana = self.getPropertyValue('Analyser')
    	sn = self.getPropertyValue('SamNumber')
    	sam = prefix+sn+'_'+ana+'_red'
    	cut = self.getPropertyValue('Xcut')
    	cut = float(cut)

    	verbOp = self.getProperty('Verbose').value
    	plotOp = self.getProperty('Plot').value
    	saveOp = self.getProperty('Save').value
        import IndirectSymm as Main
    	Main.SymmStart(inType,sam,cut,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(Symmetrise)         # Register algorithm with Mantid
