# Algorithm to start Bayes programs
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator, logger

class MuscatData(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
        return "Calculates multiple scattering using a sample S(Q,w)."

    def PyInit(self):
        self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
        self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']))
        self.declareProperty(name='Geom',defaultValue='Flat',validator=StringListValidator(['Flat','Cyl']), doc='Sample geometry')
        self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample data run number')
        self.declareProperty(name='SqwInput',defaultValue='',validator=StringMandatoryValidator(), doc='Sqw file run number')
        self.declareProperty(name='NR1', defaultValue=1000, doc='MonteCarlo neutrons NR1. Default=1000')
        self.declareProperty(name='NR2', defaultValue=1000, doc='MonteCarlo neutrons NR2. Default=1000')
        self.declareProperty(name='Nms', defaultValue=1, doc='Number of scatterings. Default=1')
        self.declareProperty(name='DetAngle', defaultValue=90.0, doc='Detector angle. Default=90.0')
        self.declareProperty(name='Thick', defaultValue='',validator=StringMandatoryValidator(), doc='Sample thickness')
        self.declareProperty(name='Width', defaultValue='',validator=StringMandatoryValidator(), doc='Sample width')
        self.declareProperty(name='Height', defaultValue=3.0, doc='Sample height. Default=3.0')
        self.declareProperty(name='Density', defaultValue=0.1, doc='Sample number density. Default=0.1')
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
        sam = self.getPropertyValue('SamNumber')
        sqwi = self.getPropertyValue('SqwInput')
        sname = prefix+sam+'_'+ana
        sqw = prefix+sqwi+'_'+ana

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
        temp = self.getPropertyValue('Temperature')
        sam = [float(temp), float(dens), float(siga), float(sigb)]

        kr1 = 1
        verbOp = self.getProperty('Verbose').value
        plotOp = self.getPropertyValue('Plot')
        saveOp = self.getProperty('Save').value
        Main.MuscatDataStart(sname,geom,neut,beam,sam,sqw,kr1,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(MuscatData)                    # Register algorithm with Mantid
