from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config
import os.path, math

class IndirectTransmission(PythonAlgorithm):

    def category(self):
    	return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
    	return "Calculates the scattering & transmission for Indirect Geometry spectrometers."

    def PyInit(self):
    	self.declareProperty(name='Instrument',defaultValue='IRIS',validator=StringListValidator(['IRIS','OSIRIS']), doc='Instrument')
    	self.declareProperty(name='Analyser',defaultValue='graphite',validator=StringListValidator(['graphite','fmica']), doc='Analyser')
    	self.declareProperty(name='Reflection',defaultValue='002',validator=StringListValidator(['002','004']), doc='Reflection')
    	self.declareProperty(name='ChemicalFormula',defaultValue='',validator=StringMandatoryValidator(), doc='Sample chemical formula')
    	self.declareProperty(name='NumberDensity', defaultValue=0.1, doc='Number denisty. Default=0.1')
    	self.declareProperty(name='Thickness', defaultValue=0.1, doc='Sample thickness. Default=0.1')
    	self.declareProperty(WorkspaceProperty('OutputWorkspace', "", Direction.Output), doc="The name of the output workspace.")

    def PyExec(self):
    	from IndirectCommon import StartTime, EndTime

    	StartTime('IndirectTransmission')

    	instrumentName = self.getPropertyValue('Instrument')
    	analyser = self.getPropertyValue('Analyser')
    	reflection = self.getPropertyValue('Reflection')
    	formula = self.getPropertyValue('ChemicalFormula')
    	density = self.getPropertyValue('NumberDensity')
    	thickness = self.getPropertyValue('Thickness')

    	#Load instrument defintion file
    	idfDir = config['instrumentDefinition.directory']
    	idf = idfDir + instrumentName + '_Definition.xml'
    	workspace = '__empty_'+instrumentName
    	LoadEmptyInstrument(OutputWorkspace=workspace, Filename=idf)

    	#Load instrument parameter file
    	nameStem = instrumentName + '_' + analyser + '_' + reflection
    	ipf = idfDir + nameStem + '_Parameters.xml'
    	LoadParameterFile(Workspace=workspace, Filename=ipf)

    	#Get efixed value
    	instrument = mtd[workspace].getInstrument()
    	efixed = instrument.getNumberParameter('efixed-val')[0]

    	logger.notice('Analyser : ' +analyser+reflection +' with energy = ' + str(efixed))

    	result = SetSampleMaterial(InputWorkspace=workspace,ChemicalFormula=formula)

    	#elastic wavelength
    	wave=1.8*math.sqrt(25.2429/efixed)

    	absorptionXSection = result[5]*wave/1.7982
    	coherentXSection = result[4]
    	incoherentXSection = result[3]
    	scatteringXSection = incoherentXSection+coherentXSection

    	thickness = float(thickness)
    	density = float(density)

    	totalXSection = absorptionXSection + scatteringXSection

    	transmission = math.exp(-density*totalXSection*thickness)
    	scattering = 1.0 - math.exp(-density*scatteringXSection*thickness)

    	#Create table workspace to store calculations
    	tableWs = self.getPropertyValue('OutputWorkspace')
    	tableWs = CreateEmptyTableWorkspace(OutputWorkspace=tableWs)
    	tableWs.addColumn("str", "Name")
    	tableWs.addColumn("double", "Value")

    	# Names for each of the output values
    	outputNames = ['Wavelength', 'Absorption Xsection', 'Coherent Xsection', 'Incoherent Xsection',
    							'Total scattering Xsection', 'Number density', 'Thickness', 'Transmission (abs+scatt)', 'Total scattering']

    	# List of the calculated values
    	outputValues = [wave, absorptionXSection, coherentXSection, incoherentXSection,
    							scatteringXSection, density, thickness, transmission, scattering]

    	#build table of values
    	for data in zip (outputNames, outputValues):
    		tableWs.addRow(list(data))
    		logger.information(': '.join(map(str,list(data))))

    	#remove idf/ipf workspace
    	DeleteWorkspace(workspace)
    	self.setProperty("OutputWorkspace", tableWs)
    	EndTime('IndirectTransmission')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTransmission)
