"""
Some of the sphinx warnings come from the C++ code, from the properties of the algorithms or from the summary string
This test tries to detect the most common such errors. 
It also detects if a new category is created (i.e. someone uses Utilities instead of Utility)
"""
import stresstesting
import mantid
import re

class SphinxWarnings(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.allowedCategories=['Arithmetic',
                        'CorrectionFunctions',
                        'Crystal',
                        'DataHandling',
                        'Diagnostics',
                        'Diffraction',
                        'Events',
                        'Examples',
                        'ISIS',
                        'Inelastic',
                        'MDAlgorithms',
                        'MPI',
                        'Muon',
                        'Optimization',
                        'PythonAlgorithms',
                        'Quantification',
                        'Reflectometry',
                        'Remote',
                        'SANS',
                        'SINQ',
                        'Sample',
                        'Transforms',
                        'Utility',
                        'Workflow']
        self.errorMessage=""

    def checkString(self,s):
        tocheck=s
        outputString=''
        #replace strong emphasis: Space**NotSpaceText**
        sub=re.compile(r' \*\*[^ ].+?\*\*') 
        for i in sub.findall(tocheck):
            tocheck=tocheck.replace(i," ")
        #replace emphasis: Space*NotSpaceText*
        sub=re.compile(r' \*[^ ].+?\*')     
        for i in sub.findall(tocheck):
            tocheck=tocheck.replace(i," ")
        #replace correctly named hyperlinks: Space`Name link>`__
        sub=re.compile(r' \`.+? <.+?.\`__')
        for i in sub.findall(tocheck):
            tocheck=tocheck.replace(i," ")    
    
        #find strong emphasis errors
        sub=re.compile(r' \*\*[^ ]+') 
        result=sub.findall(tocheck)
        if len(result)>0:
            outputString+="Strong emphasis error: "+str(result)+"\n"
        #find emphasis errors
        sub=re.compile(r' \*[^ ]+') 
        result=sub.findall(tocheck)
        if len(result)>0:
            outputString+="Emphasis error: "+str(result)+"\n"
        #find potentially duplicate named hyperlinks 
        sub=re.compile(r' \`.+? <.+?.\`_')  
        result=sub.findall(tocheck) 
        if len(result)>0:
            outputString+="Potentially unsafe named hyperlink: "+str(result)+"\n"    
        #find potentially wrong substitutions
        sub=re.compile(r'\|.+?\|')
        result=sub.findall(tocheck)
        if len(result)>0:
            outputString+="Potentially unsafe substitution: "+str(result)+"\n"   
        return outputString
    
    def runTest(self):
        algs = mantid.AlgorithmFactory.getRegisteredAlgorithms(True)
        for (name, versions) in algs.iteritems():
            for version in versions:
                if mantid.api.DeprecatedAlgorithmChecker(name,version).isDeprecated()=='':
                    # get an instance
                    alg = mantid.AlgorithmManager.create(name, version)
                    #check categories
                    for cat in alg.categories():
                        if cat.split("\\")[0] not in self.allowedCategories:
                            self.errorMessage+=name+" "+str(version)+" Category: "+cat.split("\\")[0]+" is not in the allowed list. If you need this category, please add it to the systemtest.\n"
                    #check summary
                    summary=alg.summary()
                    result=self.checkString(summary)
                    if len(result)>0:
                        self.errorMessage+=name+" "+str(version)+" Summary: "+result+"\n"
                    #check properties
                    properties=alg.getProperties()
                    for prop in properties:
                        propName=prop.name           
                        propDoc=prop.documentation
                        result=self.checkString(propDoc)
                        if len(result)>0:
                            self.errorMessage+=name+" "+str(version)+" Property: "+propName+" Documentation: "+result +"\n"  
    
    def validate(self):
        if self.errorMessage!="":
            print "Found the following errors:\n",self.errorMessage
            return False

        return True   
