#!/usr/bin/env python
VERSION = "1.4.2"

from suds.client import Client

import nxs, os, numpy, sys, posixpath, glob
import xml.utils.iso8601, ConfigParser
from datetime import datetime

class IngestReduced():
    def __init__(self, facilityName, instrumentName, investigationName, runNumber):
        self._facilityName = facilityName
        self._instrumentName = instrumentName
        self._investigationName = investigationName
        self._runNumber = runNumber
        config = ConfigParser.RawConfigParser()
        config.read('/etc/autoreduce/icatclient.properties')
        hostAndPort = config.get('icat41', 'hostAndPort')
        password = config.get('icat41', 'password')
        plugin = "db"
    
        client = Client("https://" + hostAndPort + "/ICATService/ICAT?wsdl")
        service = client.service
        factory = client.factory
        self._service = service
        self._factory = factory
 
        credentials = factory.create("credentials")
        entry = factory.create("credentials.entry")
        entry.key = "username"
        entry.value = "root"
        credentials.entry.append(entry)
        entry = factory.create("credentials.entry")
        entry.key = "password"
        entry.value = password
        credentials.entry.append(entry)
    
        print "Begin login at: ", str(datetime.now()) 
        sessionId = service.login(plugin, credentials)
        self._sessionId = sessionId
        print "End login at: ", str(datetime.now()) 
   
    def logout(self): 
        print "Begin logout at: ", str(datetime.now()) 
        self._service.logout(self._sessionId)
        print "End logout at: ", str(datetime.now()) 

    def execute(self):
    
        config = ConfigParser.RawConfigParser()
        config.read('/etc/autoreduce/icat4.cfg')
    
        directory = "/" + self._facilityName + "/" + self._instrumentName + "/" +  self._investigationName + "/shared/autoreduce"
        print "reduction output directory: " + directory
    
        print "Search investigation: ", str(datetime.now()) 
        investigations = self._service.search(self._sessionId, "Investigation INCLUDE Sample [name = '" + self._investigationName + "'] <-> Dataset [name = '" + self._runNumber + "'] <-> Instrument [name = '" + self._instrumentName + "']")
    
        if len(investigations) == 1:
            print "Investigation found", str(datetime.now()) 
            investigation = investigations[0]
        else:
            print "Investigation not found", str(datetime.now()) 
            return 1
    
        print "End getInvestigation at: ", str(datetime.now()) 
    
        #set dataset name 
        dataset = self._factory.create("dataset")
    
        dsType = self._factory.create("datasetType")
        dsType.id = config.get('DatasetType', 'reduced')
        dataset.type = dsType
        dataset.name = self._runNumber  
        dataset.location = directory
        datafiles = []
    
        pattern =  '*' + self._runNumber + '*'
        print "pattern: " + pattern
        listing = glob.glob(os.path.join(directory, pattern))
        for filepath in listing:
            filename =os.path.basename(filepath )
            datafile = self._factory.create("datafile")
            datafile.location = filepath 
            datafile.name = filename
            extension = os.path.splitext(filename)[1][1:]
            dfFormat = self._factory.create("datafileFormat")
            dfFormat.id = config.get('DatafileFormat', extension)
            datafile.datafileFormat = dfFormat 
            modTime = os.path.getmtime(filepath)
            modTime = os.path.getmtime(filepath)
            datafile.datafileCreateTime = xml.utils.iso8601.tostring(modTime)
            datafile.fileSize = os.path.getsize(filepath)
    
            datafiles.append(datafile)
    
        dataset.datafiles = datafiles
        dataset.type = dsType
        dataset.investigation = investigation
        dataset.sample = investigation.samples[0]
            
        print "Updating dataset: ", str(datetime.now()) 
        self._service.create(self._sessionId, dataset)
    
         
        print "INVESTIGATION:"
        print "  NAME: %s"%(str(investigation.name))
    
        print "DATASET:"
        print "  RUN NUMBER: %s"%(str(dataset.name))
        print "  TITLE: %s"%(str(dataset.description))
        print "  START TIME: %s"%(str(dataset.startDate))
        print "  END TIME: %s"%(str(dataset.endDate))
    
        for datafile in dataset.datafiles:
            print "DATAFILE:"
            print "  NAME: %s"%(str(datafile.name))
            print "  LOCATION: %s"%(str(datafile.location))
