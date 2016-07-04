#pylint: disable=invalid-name, no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import Direction, logger

def export_masks(ws,fileName='',returnMasks=False):
    """Exports masks applied to Mantid workspace
       (e.g. drawn using the instrument view) and write these masks
       into the old fashioned ascii msk file containing masked spectra numbers.
    
       The file is Libisis/Mantid old ISIS format compartible and can be read by libisis
       or Manid LoadMasks algorithm
 
       If optional parameter fileName is present, the masks are saved 
       in the file with this name
       Otherwise, the file with the name equal to the workspace 
       name and the extension .msk is used.
    
       If returnMasks is set to True, the function does not write to file but returns
       list of masks instead.
    """
   # get pointer to the workspace    
    if (type(ws) == str):
        pws = mtd[ws]
    else:
        pws = ws
 
 
    ws_name=pws.getName()       
    nhist = pws.getNumberHistograms()
 
    masks = []
    for i in range(nhist):
        # set provisional spectra ID
        ms = i+1
        try: 
            sp = pws.getSpectrum(i)
            # got real spectra ID, which would correspond real spectra num to spectra ID map
            ms = sp.getSpectrumNo();
        except Exception: 
            print " Can not get spectra No: ",i
            masks.append(ms) 
            continue        
        
        try:
            det = pws.getDetector(i)
        except Exception:
            masks.append(ms)        
            continue
        if det.isMasked():
            masks.append(ms)
 
      

    nMasks = len(masks);
    if nMasks == 0:
        print 'workspace ',ws_name,' have no masked spectra'
        return masks
    print 'workspace ',ws_name,' have ',nMasks,' masked spectra'
    
    filename=''
    if len(fileName)==0 :
        filename=ws_name+'.msk'
    else:
        filename = fileName
        
    if returnMasks :
        return masks
    else:
        writeISISmasks(filename,masks,8)
        
        
def flushOutString(f,OutString,BlockSize,BlockLimit):
    """Internal function for writeISISmasks procedure, 
       which writes down specified number of mask blocks.
    """
    BlockSize+=1;
    if BlockSize >= BlockLimit: 
       if len(OutString)>0:
           f.write(OutString+'\n');
       OutString = ''
       BlockSize = 0
    return (f,BlockSize,OutString)

    
def  writeISISmasks(filename,masks,nSpectraInRow=8):
    """Function writes input array in the form of ISSI mask file array
       This is the helper function for export_mask procedure,
       which can be used separately
    
       namely, if one have array 1,2,3,4, 20 30,31,32
       file will have the following ascii stgings:
       1-4 20 30-32
        
       nSpectaInRow indicates the number of the separate spectra ID (numbers) which the program 
       needs to fit into one row. For the example above the number has to be 5 or more 
       to fit all spectra into a single row. Setting it to one will produce 8 rows with single number in each.
    
    Usage: 
    >>writeISISmasks(fileName,masks)
    where:
    fileName  -- the name of the output file
    masks     -- the array with data
    """
    ext = os.path.splitext(filename)[1]
    if len(ext) == 0 :
        filename=filename+'.msk'

    f = open(filename,'w')   
    
    # prepare and write mask data in conventional msk format
    # where adjusted spectra are separated by "-" sign
    OutString   = ''
    LastSpectraN= ''
    BlockSize = 0;
    iDash = 0;
    im1=masks[0]
    for i in masks:       
        if len(OutString)== 0:
            OutString = str(i)        
            (f,BlockSize,OutString) = flushOutString(f,OutString,BlockSize,nSpectraInRow)
            im1 = i  
            continue
        # if the current spectra is different from the previous one by 1 only, we may want to skip it
        if im1+1 == i :
            LastSpectraN = str(i)
            iDash += 1;
        else :  # it is different and should be dealt separately
            if iDash > 0 :
                OutString = OutString+'-'+LastSpectraN
                iDash = 0
                LastSpectraN=''
                # write the string if it is finished
                (f,BlockSize,OutString) = flushOutString(f,OutString,BlockSize,nSpectraInRow)

      
            if len(OutString) == 0:
                OutString = str(i)
            else:
                OutString = OutString + ' ' + str(i)
            # write the string if it is finished
            (f,BlockSize,OutString) = flushOutString(f,OutString,BlockSize,nSpectraInRow)             
        #endif
      
        # current spectra is the previous now
        im1 = i  
    # end masks loop
    if iDash > 0 :
        OutString = OutString+'-'+LastSpectraN   
    (f,OutString,BlockSize)=flushOutString(f,OutString,BlockSize,0)                   
    f.close();


class ExportASCIIMask(PythonAlgorithm):
    """ Export workspace's masks 
    """
    def category(self):
        """ Return category
        """
        return "DataHandling\\Masking"

    def name(self):
        """ Return name
        """
        return "ExportASCIIMask"

    def summary(self):
        return "Exports workspace masks as legacy ASCII file with .msk extension, "\
            "containing list of masked spectra numbers."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(WorkspaceProperty("Workspace", "",Direction.Input), "The workspace to export masks from.")
        self.declareProperty("LogNames","","Names of the logs to look for")
        self.declareProperty("Result","A string that will be empty if all the logs are found, "\
            "otherwise will contain an error message",Direction.Output)
        return

    def PyExec(self):
        """ Main execution body
        """
        #get parameters
        w = self.getProperty("Workspace").value
        logNames = self.getProperty("LogNames").value
        resultString=''
        #check for parameters and build the result string
        for value in logNames.split(','):
            value=value.strip()
            if len(value)>0:
                if not w.run().hasProperty(value):
                    resultString+='Property '+value+' not found\n'

        #return the result
        logger.notice(resultString)
        self.setProperty("Result",resultString)
        return


AlgorithmFactory.subscribe(ExportASCIIMask)
