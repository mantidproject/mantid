import os,  re
import urllib2

concepts = ['Algorithm',
'Analysis_Data_Service',
'categories',
'Create_an_IDF',
'Data_Service',
'Dynamic_Factory',
'Error_Propagation',
'EventWorkspace',
'Facilities_File',
'FitConstraint',
'Framework_Manager',
'Geometry',
'Geometry_of_Position',
'Geometry_of_Shape',
'HowToDefineGeometricShape',
'index',
'Instrument',
'InstrumentDefinitionFile',
'InstrumentParameterFile',
'Instrument_Data_Service',
'Lattice',
'LET_Sample_IDF',
'MatrixWorkspace',
'MDHistoWorkspace',
'MDWorkspace',
'Nexus_file',
'Plugin',
'Project',
'Properties',
'Properties_File',
'RAW_File',
'Run',
'SANS2D_Sample_IDF',
'Shared_Pointer',
'Table_Workspaces',
'Unit_Factory',
'UserAlgorithms',
'Workflow_Algorithm',
'Workspace',
'Workspace2D',
'WorkspaceGroup']


def outputError(alg, algVersion, description, notes=""):
  print "%s, %i, %s, %s" % (alg, algVersion, description, notes)

rstdir = r"C:\Mantid\Code\Mantid\docs\source\algorithms"
conceptsPattern = {}
for concept in concepts:
    #conceptsPattern[concept] = re.compile(concept, re.IGNORECASE)
    conceptsPattern[concept] = re.compile(r'`([\w\s]+?)\s*<http:\/\/www\.mantidproject\.org\/(' + concept + r')\s*>`_', re.IGNORECASE)
    #print r'`([\w\s]+?)\s*<http:\/\/www\.mantidproject\.org\/' + concept + r'\s*>`_'


algs = AlgorithmFactory.getRegisteredAlgorithms(True)
for alg in algs:
    algVersion = algs[alg][0]
    fileFound = False
    filename = os.path.join(rstdir,alg + "-v" + str(algVersion) + ".rst")
    if os.path.exists(filename):
    algText = ""
    with open (filename, "r") as algRst:
        fileFound = True
        algText = algRst.read()
    for concept in concepts:
    	regex = conceptsPattern[concept]
    	while (regex.search(algText) != None):
    	    outputError(alg, algVersion, "found", concept)
    	    algText = regex.sub(r":ref:`\1 <\2>`",algText)
                with open (filename, "w") as algRst:
    		algRst.write(algText)

    if fileFound==False:
        outputError(alg, algVersion, "File not found")

