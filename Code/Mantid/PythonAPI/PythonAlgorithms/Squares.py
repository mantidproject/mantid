from MantidFramework import *

class Squares(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty("MaxRange", -1, Validator = BoundedValidator(Lower=0), Description = "A value for the end of the range(inclusive)")
        self.declareProperty("Preamble", "", Validator = MandatoryValidator(), Description = "Optional preamble")
        self.declareProperty("Sum", False, Description = "If True, sum the squared values")
        self.declareFileProperty("OutputFile","", FileAction.Save, ['txt'])
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction = Direction.Output, Description = '')
        
    def PyExec(self):
        msg = self.getProperty("Preamble")
        endrange = self.getProperty("MaxRange")

        if endrange <= 0:
            raise RuntimeError('No values to use!')

        wspace = WorkspaceFactory.createMatrixWorkspace(1,endrange,endrange)
        sum = 0 
        for i in range(1,endrange + 1):
            wspace.dataX(0)[i-1] = 1
            wspace.dataY(0)[i-1] = i*i
            wspace.dataE(0)[i-1] = i
            sum += i*i
        self.log().information('The sum of the squares of numbers up to ' + str(endrange) + ' is: ' + str(sum))
            
        self.setProperty("OutputWorkspace", wspace)
        if self.getProperty('Sum'):
            filename = self.getProperty("OutputFile")
            file = open(filename,'w')
            file.write('The sum of the squares of numbers up to ' + str(endrange) + ' is: ' + str(sum) + '\n')
        
#############################################################################################

mtd.registerPyAlgorithm(Squares())
