#!/usr/bin/env python
VERSION = "1.4.2"

from suds.client import Client

import nxs, os, numpy, sys, posixpath
import xml.utils.iso8601, ConfigParser
from datetime import datetime

class IngestNexus():
    def __init__(self, infilename):
        self._infilename = infilename
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
      
        #find facility, investigation_type 
        config = ConfigParser.RawConfigParser()
        config.read('/etc/autoreduce/icat4.cfg')
    
        #open nexus file
        file = nxs.open(self._infilename, 'r')
        file.opengroup('entry')
    
        listing = file.getentries()
         
        #investigation name 
        if listing.has_key('experiment_identifier'):
            file.opendata('experiment_identifier')
            name = file.getdata()
            file.closedata()
        else:
            name = "IPTS-0000"
    
        #investigation title
        if listing.has_key('title'):
            file.opendata('title')
            title = file.getdata()
            file.closedata()
        else:
            title = "NONE"
    
        #dataset run number
        file.opendata('run_number')
        runNumber = file.getdata() 
        file.closedata()
    
        #dataset notes 
        if listing.has_key('notes'):
            file.opendata('notes')
            notes = file.getdata()
            file.closedata()
    
        #dataset proton_charge 
        file.opendata('proton_charge')
        protonCharge = file.getdata()
        file.closedata()
    
        #dataset total_counts 
        file.opendata('total_counts')
        totalCounts = file.getdata()
        file.closedata()
    
        #dataset duration 
        file.opendata('duration')
        duration = file.getdata()
        file.closedata()
    
        #investigation instrument 
        file.opengroup('instrument')
        file.opendata('name')
        for attr,value in file.attrs():
            if attr == 'short_name':
                instrumentName = value
                instrument = self._factory.create("instrument")
                instrument.id = config.get('Instrument', value.lower())
        file.closedata()
        file.closegroup()
    
        print "Search investigation: ", str(datetime.now()) 
        investigations = self._service.search(self._sessionId, "Investigation INCLUDE Sample [name = '" + name + "'] <-> Instrument [name = '" + instrumentName + "']")
    
        if len(investigations) == 0:
            investigation = self._factory.create("investigation")
            createInv = True
    
            #find facility, investigation_type 
            facility = self._factory.create("facility")
            facility.id = config.get('Facility', 'sns')
            investigation.facility = facility
            
            invType = self._factory.create("investigationType")
            invType.id = config.get('InvestigationType', 'experiment')
            investigation.type = invType 
    
            investigation.name = name 
            investigation.instrument = instrument 
            investigation.title = title 
        else:
            createInv = False 
            print "found investigation"
            investigation = investigations[0]
    
        #set dataset name 
        dataset = self._factory.create("dataset")
    
        dsType = self._factory.create("datasetType")
        dsType.id = config.get('DatasetType', 'experiment_raw')
        dataset.type = dsType
        dataset.name = runNumber 
        dataset.description = title 
      
        #set dataset start time 
        file.opendata('start_time')
        dataset.startDate = file.getdata()
        file.closedata()
    
        #set dataset end time 
        file.opendata('end_time')
        dataset.endDate = file.getdata()
        file.closedata()
    
        #set dataset parameters
        parameters = []
    
        #1) parameter proton_charge 
        if protonCharge:
            parameterType = self._factory.create("parameterType")
            parameterType.id = config.get('ParameterType', 'proton_charge')
            parameterType.applicableToDataset = config.getboolean('ParameterType', 'proton_charge_applicable_to_dataset')
            datasetParameter = self._factory.create("datasetParameter")
            datasetParameter.type = parameterType
            datasetParameter.numericValue = protonCharge 
            parameters.append(datasetParameter)
    
        #2) parameter total_counts 
        if totalCounts:
            parameterType = self._factory.create("parameterType")
            parameterType.id = config.get('ParameterType', 'total_counts')
            parameterType.applicableToDataset = config.getboolean('ParameterType', 'total_counts_applicable_to_dataset')
            datasetParameter = self._factory.create("datasetParameter")
            datasetParameter.type = parameterType 
            datasetParameter.numericValue = totalCounts
            parameters.append(datasetParameter)
    
        #3) parameter duration 
        if duration:
            parameterType = self._factory.create("parameterType")
            parameterType.id = config.get('ParameterType', 'duration')
            parameterType.applicableToDataset = config.getboolean('ParameterType', 'duration_applicable_to_dataset')
            datasetParameter = self._factory.create("datasetParameter")
            datasetParameter.type = parameterType 
            datasetParameter.numericValue = duration 
            parameters.append(datasetParameter)
    
        dataset.parameters = parameters
        dataset.location = self._infilename 
    
        datafiles = []
    
        filepath = os.path.dirname(self._infilename)
        filename = os.path.basename(self._infilename)
        datafile = self._factory.create("datafile")
        extension = os.path.splitext(filename)[1][1:]
        datafile.name = filename
        datafile.location = self._infilename 
        dfFormat = self._factory.create("datafileFormat")
        dfFormat.id = config.get('DatafileFormat', extension)
        datafile.datafileFormat = dfFormat
        modTime = os.path.getmtime(filepath)
        datafile.datafileCreateTime = xml.utils.iso8601.tostring(modTime)
        datafile.fileSize = os.path.getsize(filepath)
        datafiles.append(datafile)
    
        runPath = posixpath.abspath(posixpath.join(self._infilename, '../../adara'))
        print runPath
        for dirpath, dirnames, filenames in os.walk(runPath):
            for filename in [f for f in filenames]:
                if runNumber in filename:
                    datafile = self._factory.create("datafile")
                    filepath = os.path.join(dirpath,filename)
                    extension = os.path.splitext(filename)[1][1:]
                    datafile.name = filename
                    datafile.location = filepath
                    dfFormat = self._factory.create("datafileFormat")
                    dfFormat.id = config.get('DatafileFormat', extension)
                    datafile.datafileFormat = dfFormat 
                    modTime = os.path.getmtime(filepath)
                    datafile.datafileCreateTime = xml.utils.iso8601.tostring(modTime)
                    datafile.fileSize = os.path.getsize(filepath)
    
                    datafiles.append(datafile)
    
        dataset.datafiles = datafiles
    
        samples = []
    
        sample = self._factory.create("sample")
        sample.name = 'NONE'
    
        if listing.has_key('sample'):
            file.opengroup('sample')
            listSample = file.getentries()
            if listSample.has_key('name'):
                file.opendata('name')
                sample.name = file.getdata()
                file.closedata()
    
            sampleParameters = []
    
            #set sample nature
            if listSample.has_key('nature'):
                file.opendata('nature')
                nature = file.getdata()
                file.closedata()
                if nature:       
                    parameterType = self._factory.create("parameterType")
                    parameterType.id = config.get('ParameterType', 'nature')
                    parameterType.applicableToSample = config.getboolean('ParameterType', 'nature_applicable_to_sample')
                    sampleParameter = self._factory.create("sampleParameter")
                    sampleParameter.type = parameterType
                    sampleParameter.stringValue = nature 
                    sampleParameters.append(sampleParameter)
            
            if listSample.has_key('identifier'):
                file.opendata('identifier')
                identifier = file.getdata()
                file.closedata()
      
                if identifier:
                    parameterType = self._factory.create("parameterType")
                    parameterType.id = config.get('ParameterType', 'identifier')
                    parameterType.applicableToSample = config.getboolean('ParameterType', 'identifier_applicable_to_sample')
                    sampleParameter = self._factory.create("sampleParameter")
                    sampleParameter.type = parameterType
                    sampleParameter.stringValue = identifier
                    sampleParameters.append(sampleParameter)
           
            if len(sampleParameters): 
                sample.parameters = sampleParameters
    
        samples.append(sample)
    
        file.closegroup()
        file.close()
        if createInv == True:
            #Create new investigation
            datasets = []
            datasets.append(dataset)
            investigation.datasets = datasets
            investigation.samples = samples
            print "Creating new investigation: ", str(datetime.now()) 
            invId = self._service.create(self._sessionId, investigation)
    
            print "Getting dataset: ", str(datetime.now()) 
            newInvestigations = self._service.search(self._sessionId, "Investigation INCLUDE Dataset [id = '" + str(invId) + "']")
            
            if len(newInvestigations) == 1:
                newInvestigation = newInvestigations[0]
            
            if len(newInvestigation.datasets) == 1:
                newDatasets = newInvestigation.datasets
                newDataset = newDatasets[0]
            
            print "Getting sample ", str(datetime.now()) 
            newInvestigations = self._service.search(self._sessionId, "Investigation INCLUDE Sample [id = '" + str(invId) + "']")
            
            if len(newInvestigations) == 1:
                newInvestigation = newInvestigations[0]
                newSamples = newInvestigation.samples
            
            newDataset.sample = newSamples
            newDataset.investigation = newInvestigation
            newDataset.type = dsType
            print "Updating dataset: ", str(datetime.now()) 
            self._service.update(self._sessionId, newDataset)
    
        else:
            #Found investigation, may create new sample or add new dataset
            print "Searching investigation and dataset ", str(datetime.now()) 
            investigations = self._service.search(self._sessionId, "Investigation [name = '" + investigation.name + "']  <-> Instrument [name = '" + instrumentName + "']<-> Dataset [name = '" + dataset.name + "']")
            if len(investigations) == 0:
                createSample = True 
                for invSample in investigation.samples:
                    if invSample.name == sample.name: 
                        createSample = False 
                        sample = invSample
                        break
    
                if createSample == True:
                    sample.investigation = investigation
                    print "Adding new sample to existing investigation: ", str(datetime.now()) 
                    sample.id = self._service.create(self._sessionId, sample)
     
                dataset.investigation = investigation
                dataset.sample = sample 
                dataset.type = dsType
                print "Adding new dataset to investigation: ", str(datetime.now()) 
                self._service.create(self._sessionId, dataset)
            else:
                print "Run " + runNumber + " is already cataloged."
             
    
        '''print "INVESTIGATION:"
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
    
        print "SAMPLE: "
        print "  NAME: %s"%(str(sample.name))'''
    
        print investigation
