"""*WIKI*
== Description ==

LoadSINQ loads SINQ NeXus files. The algorithm calculates the file name from the instrument,  year and numor and tries to locate the file. Both at SINQ standard paths as well as the data directories configured for Mantid. Then it calls LoadSINQFile for the located data file. 

The Mantid standard Load algorithm selects based on file extensions. The file extensions used at SINQ, mainly .hdf and .h5, were already taken. Thus the need for a separate loader.


*WIKI*"""
#--------------------------------------------------------------
# Algorithm which loads a SINQ file. 
# This algorithm calculates the filename from instrument
# year and filenumber and then goes away to call
# LoadSINQFile
#
# Mark Koennecke, November 2012
#--------------------------------------------------------------
from mantid.api import PythonAlgorithm, registerAlgorithm, WorkspaceFactory, FileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, StringListValidator, ConfigServiceImpl
import mantid.simpleapi
import datetime
import math
import os.path

#------------------------- where files live at SINQ
datapath='/afs/psi.ch/project/sinqdata'

class LoadSINQ(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def PyInit(self):
        instruments=["AMOR","BOA","DMC","FOCUS","HRPT","MARSI","MARSE","POLDI",
                     "RITA-2","SANS","SANS2","TRICS"]
        self.declareProperty("Instrument","AMOR",
                             StringListValidator(instruments),
                             "Choose Instrument",direction=Direction.Input)
        now = datetime.datetime.now()
        self.declareProperty("Year",now.year,"Choose year",direction=Direction.Input)
        self.declareProperty('Numor',0,'Choose file number',direction=Direction.Input)
        self.declareProperty("OutputWorkspace","nexus")
        self.setWikiSummary("SINQ data file loader")

    def PyExec(self):
        inst=self.getProperty('Instrument').value
        year=self.getProperty('Year').value
        num=self.getProperty('Numor').value
        self.log().error('Running LoadSINQ for ' + inst + " Y= " + str(year) + " N= " +str(num))
        out=self.getProperty('OutputWorkspace').value

        instmap = {}
        instmap['AMOR'] = 'amor'
        instmap['BOA'] = 'boa'
        instmap['DMC'] = 'dmc'
        instmap['FOCUS'] = 'focus'
        instmap['HRPT'] = 'hrpt'
        instmap['MARSI'] = 'mars'
        instmap['MARSE'] = 'mars'
        instmap['POLDI'] = 'poldi'
        instmap['RITA-2'] = 'rita2'
        instmap['SANS'] = 'sans'
        instmap['SANS2'] = 'sans2'
        instmap['TRICS'] = 'trics'

        hun=math.floor(num/1000.)
        filename= '%03d/%s%04dn%06d.hdf' % (hun,instmap[inst],year,num)
        fullpath= '%s/%04d/%s/%s' % (datapath,year,instmap[inst],filename)
        if os.path.exists(fullpath):
            mantid.simpleapi.LoadSINQFile(fullpath,inst,out)
        else:
            searchDirs = ConfigServiceImpl.Instance().getDataSearchDirs()
            filename= '%s%04dn%06d.hdf' % (instmap[inst],year,num)
            for entry in searchDirs:
                fullpath = '%s/%s' % (entry, filename)
                if os.path.exists(fullpath):
                    mantid.simpleapi.LoadSINQFile(fullpath,inst,out)
                    return
            raise Exception('File %s NOT found!' % filename)


#---------- register with Mantid
registerAlgorithm(LoadSINQ())
