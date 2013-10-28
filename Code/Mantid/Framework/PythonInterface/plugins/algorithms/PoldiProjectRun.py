"""*WIKI* 



PoldiProjectRun algorithm is used to analyze a bunch of POLDI raw data
files, following a standard POLDI analysis process. This algorithm 
take as parameter a tableMatrix with a list of the sample to analyze, 
and for each sample a bunch of setup information for the different 
algorithms (such as the data file path, etc...).

This tableWorkspace can be built easily using the 
two algorithms [[PoldiProjectAddFile]] and [[PoldiProjectAddDir]], 
which will create and/or fill properly a targeted tableWorkspace. 
The needed columns and there content are describe in the 
following [[PoldiProjectRun#Data Manager|Data Manager]] paragraph. 


*WIKI_USAGE*
The algorithm is used the classical way. Only one parameter is compulsory.
 OutputWorkspace = PoldiProjectRun(InputWorkspace=sample_manager_ws)

*WIKI_USAGE*




Data are processed alone, or grouped together. For each 
acquisition file, setup information have to be loaded. During 
the data treatment process, transitional workspace are created. 

In a close future, it will possible to share different workspace 
between data-file: for example when one knows that some 
acquisitions should be strictly the same, auto-correlation 
and peak detection could be done only one for all the data.

=== Data manager ===


A MatrixWorkspace is created to store all the information 
about data-files and the future workspace needed during the analysis.
The stored information are:
<ul>
 <li> spl Name       - name of the sample, extract from the sample 
 file name, without the extension </li>
 <li> year           - year of the acquisition </li>
 <li> number         - id number of the acquisition </li>
 <li> data file      - full path of the data file </li>
 <li> spl log        - name of the MatrixWorkspace where the data log are loaded </li>
 <li> spl corr       - name of the MatrixWorkspace where the 
 correlated spectra is loaded</li>
 <li> spl dead wires - name of the MatrixWorkspace where the 
 dead wires are loaded </li>
 <li> spl peak       - name of the MatrixWorkspace where the 
 detected peak information are stored </li>
</ul>




=== POLDI setup manager ===

For each acquisition file, the IDF are loaded:
<UL>
 <LI> Instrument Definition files - The POLDI instrument geometry. </LI>
 <LI> Instrument Parameters files - The setup parameters 
 for the data, at t he time of the acquisition. </LI>
</UL>

The POLDI setup informations can be shared between 
acquisition obtained during the same beam-time. 
While loading each instrument files, the different POLDI 
configurations used are stored in a MatrixWorkspace (most 
often, there is only one per year), with an example of 
data. The needed POLDI setup informations will then be 
extracted from the IDF of each of these example sample.

Therefore each POLDI setup are loaded only once and shared 
between the different data files.



=== Analysis steps ===

==== Loading the data ====
Each data-file is loaded on a 2DWorkspace. The associated 
log and setup information are loaded in dedicated workspace 
as specified in the sample-manager TableWorkspace.


:[[LoadSINQFile]]

The raw data are loaded in a 2DWorkspace, using the generic 
file-loader for SINQ data, given the instrument name ''POLDI'' as parameter.
 LoadSINQFile(Instrument      = "POLDI", 
              Filename        = sample_file_path, 
              OutputWorkspace = sample_name)

:[[PoldiLoadLog]]

The associated ''logs'' informations are extracted from 
the ''hdf'' raw data file, an store in a dedicated MatrixWorkspace. 
A dictionary file contains the set of key/path to extract 
and store all the needed information.
More specifically, the acquisition starting time is extracted 
and store in the sample WS to initialize the ''run_start'' variable.
 PoldiLoadLog(InputWorkspace = sample_output_ws, 
              Filename       = sample_file_path, 
              Dictionary     = poldi_dictionnary_file_path, 
              PoldiLog       = sample_log_ws)

:[[LoadInstrument]]

For each raw data WS, the corresponding IDF is loaded, based 
on the acquisition starting time.
 LoadInstrument(Workspace         = sample_output_ws, 
                InstrumentName    = "Poldi", 
                RewriteSpectraMap = True)

:[[PoldiRemoveDeadWires]]

Some wires are permanently dead and should not be taken into 
account. They are listed in the IDF of a given setup (IPP).
Some others wires should not be used, because they seem 
untrustable (dead wires, hot wires, random behavior,...). These 
wires are detected by successive comparison with there neighbors: 
intensity from two successive wires should not differ more 
than ''BadWiresThreshold''(*100)%. One by one, the most deviant 
wires are checks and removed until they all fit the condition.
 PoldiRemoveDeadWires(InputWorkspace      = sample_output_ws, 
                      RemoveExcludedWires = True, 
                      AutoRemoveBadWires  = True, 
                      BadWiresThreshold   = BadWiresThreshold, 
                      PoldiDeadWires      = sample_dead_wires_ws)

==== Loading POLDI parameters ====
While loading the data, the different needed setup have been 
store in a dedicated workspace.

they are now all extracted, using an example sample for each of them.


:[[PoldiLoadChopperSlits]]

The chopper configuration is loaded in a dedicated Workspace, 
one per ''Poldi IPP'' setup detected.
 PoldiLoadChopperSlits(InputWorkspace    = ex_of_sample_ws, 
                       PoldiChopperSlits = ipp_chopper_slits)

:[[PoldiLoadSpectra]]

The characteristic Poldi spectra (''Intensity=f(wavelength)'') is 
extracted from each IDF.
 PoldiLoadSpectra(InputWorkspace = ex_of_sample_ws, 
                  PoldiSpectra   = ipp_Poldi_spectra)

:[[PoldiLoadIPP]]
 
Local setup information (such as the detector position, chopper 
offset, etc...) are extracted and stores in a dedicated workspace.
 PoldiLoadIPP(InputWorkspace = ex_of_sample_ws, 
              PoldiIPP       = ipp_ipp_data)

==== Pre-analyzing data ====
In order to setup the 2D fit to analyze the data, some 
information need to be extracted from the file, such as an 
idea of the peaks position. This is done using an autocorrelation 
function, following by a peak detection algorithm.

The process has been cut in different algorithm in order to give 
the possibility to change/improve/modify each steps. For example, 
the peak detection process can be based on some previous results 
to not start from scratch, or given the sample crystal 
structure/symetries/space group...


:[[PoldiAutoCorrelation]]

Almost all the previous loaded workspace are used by this algorithm. 
From the sample manager workspace, and the Poldi setup workspace, all 
the targeted workspace can be found and given as parameters to the 
algorithm. The auto-correlated graph is store in a dedicated 
workspace, on row (0).
 PoldiAutoCorrelation(InputWorkspace    = sample_output_ws, 
                      PoldiSampleLogs   = sample_log_ws, 
                      PoldiDeadWires    = sample_dead_wires_ws, 
                      PoldiChopperSlits = ipp_chopper_slits, 
                      PoldiSpectra      = ipp_Poldi_spectra, 
                      PoldiIPP          = ipp_ipp_data, 
                      wlenmin           = wlen_min,
                      wlenmax           = wlen_max, 
                      OutputWorkspace   = sample_correlated_ws)
                  
:[[PoldiPeakDetection]]

The previous autocorrelation function is analyzed to detected 
possible peaks. The found peak are stored in a dedicated workspace, 
and added to the previously created ''sample_correlated_ws'': 
on row (1) the detected peak, on row (2) the fitted peak.
 PoldiPeakDetection(InputWorkspace         = sample_correlated_ws,
                    PeakDetectionThreshold = PeakDetectionThreshold,
                    OutputWorkspace        = sample_peak_ws)



== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to 
proceed POLDI data. The introductions can be found in the 
wiki page of [[PoldiProjectRun]].


*WIKI*"""

from mantid.api import PythonAlgorithm
from mantid.api import ITableWorkspaceProperty
from mantid.api import AlgorithmFactory, WorkspaceFactory

from mantid.kernel import Direction

from mantid.simpleapi import config, mtd
from mantid.simpleapi import (LoadSINQFile, 
                              PoldiLoadLog, 
                              LoadInstrument, 
                              PoldiRemoveDeadWires, 
                              PoldiLoadChopperSlits, 
                              PoldiLoadSpectra,
                              PoldiLoadIPP,
                              PoldiAutoCorrelation,
                              PoldiPeakDetection)
import os.path




class PoldiProjectRun(PythonAlgorithm):
    
    def category(self):
        """ Mantid required
        """
        return "Poldi"

    def name(self):
        """ Mantid required
        """
        return "PoldiProjectRun"

    def PyInit(self):
        """ Mantid required
        """
        
        self.setWikiSummary("""Run the POLDI analysis process for a bunch of data files stored in a tableWorkspace.""")

        
        
        self.declareProperty(ITableWorkspaceProperty("InputWorkspace", "PoldiAnalysis", direction=Direction.Input), 
                             "Poldi analysis main worksheet")

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "PoldiIPPmanager", direction=Direction.Output),
                              "Poldi IPP table manager")
        
        self.declareProperty("wlenmin", 1.1, 
                             doc = 'minimal wavelength considered' , 
                             direction = Direction.Input)
        self.declareProperty("wlenmax", 5.0, 
                             doc = 'maximal wavelength considered' , 
                             direction = Direction.Input)
        
        
        self.declareProperty("BadWiresThreshold", 0.8, 
                             doc = 'Bad wires threshold parameter', 
                             direction = Direction.Input)
        
        self.declareProperty("PeakDetectionThreshold", 0.2, 
                             doc = 'Peak detection threshold parameter', 
                             direction = Direction.Input)
        
    
    
    

    
    def PyExec(self):
        """ Mantid required
        """
        self.log().debug('Poldi Data Analysis ---- start')
        
        sample_info_ws = self.getProperty("InputWorkspace").value
        
        load_data_at_the_end = False
        try:
            sample_ipp_ws_name = self.getProperty("OutputWorkspace").value
            if(sample_ipp_ws_name == ""):
                sample_ipp_ws_name = "PoldiIPPmanager"
            self.log().debug('Poldi IPP manager ---- %s'%(sample_info_ws_name))
            sample_ipp_ws = mtd["PoldiIPPmanager"]
            self.log().debug('                    ---- workspace ipp loaded')
        except:
            self.log().debug('                    ---- workspace ipp created')
            sample_ipp_ws = WorkspaceFactory.createTable()
            sample_ipp_ws.addColumn("str","spl Name")
            sample_ipp_ws.addColumn("str","ipp version")
            load_data_at_the_end = True
        
        
        wlen_min  = self.getProperty("wlenmin").value
        wlen_max  = self.getProperty("wlenmax").value
        
        bad_wires_threshold  = self.getProperty("BadWiresThreshold").value
        peak_detect_threshold  = self.getProperty("PeakDetectionThreshold").value
        
        self.log().error('Poldi run with parameters')
        self.log().error('      -  wlen_min : %s' %(wlen_min))
        self.log().error('      -  wlen_max : %s' %(wlen_max))
        self.log().error('      -  bad_wires_threshold   : %s' %(bad_wires_threshold))
        self.log().error('      -  peak_detect_threshold : %s' %(peak_detect_threshold))
        
        dictsearch=os.path.join(config['instrumentDefinition.directory'],"nexusdictionaries","poldi.dic")
        self.log().error('Poldi instr folder -  %s' %(dictsearch))
        
        firstOne=""               
                
        self.log().debug('Poldi - load data')
        nb_of_sample = sample_info_ws.rowCount()
        self.log().error('Poldi -  %d samples listed' %(nb_of_sample))
                
        for sample in range(nb_of_sample):
            sampleName      = sample_info_ws.column("spl Name")[sample]
            filePath        = sample_info_ws.column("data file")[sample]
            sampleNameLog   = sample_info_ws.column("spl log")[sample]
            sampleDeadWires = sample_info_ws.column("spl dead wires")[sample]
                
            self.log().error('Poldi - sample %s' %(sampleName))
            LoadSINQFile(Instrument="POLDI", 
                         Filename=filePath, 
                         OutputWorkspace=sampleName)
            
            sample_output_ws = mtd[sampleName]
            
            PoldiLoadLog(InputWorkspace=sample_output_ws, 
                         Filename=filePath, 
                         Dictionary=dictsearch, 
                         PoldiLog=sampleNameLog)
            
            LoadInstrument(Workspace=sample_output_ws, 
                           InstrumentName="Poldi", 
                           RewriteSpectraMap=True)
            
            
            self.log().debug('Poldi - set ipp')
            sample_instrument = sample_output_ws.getInstrument()
            ipp_version = sample_instrument.getStringParameter("ipp")[0]
            
            add_this_ipp = True
            for ipp in range(sample_ipp_ws.rowCount()):
                if(sample_ipp_ws.column("ipp version")[ipp] == ipp_version):
                    add_this_ipp = False
            if(add_this_ipp):
                sample_ipp_ws.addRow([sampleName, ipp_version])
            
            
            self.log().debug('Poldi - dead wires')
            PoldiRemoveDeadWires(InputWorkspace=sample_output_ws, 
                                 RemoveExcludedWires=True, 
                                 AutoRemoveBadWires=True, 
                                 BadWiresThreshold=bad_wires_threshold, 
                                 PoldiDeadWires=sampleDeadWires)
        
        
        
        nb_of_ipp = sample_ipp_ws.rowCount()
        self.log().error('Poldi -  %d ipp listed' %(nb_of_ipp))
        
        for ipp in range(nb_of_ipp):
            ex_of_sample    = sample_ipp_ws.column("spl Name")[ipp]
            PoldiIPP        = sample_ipp_ws.column("ipp version")[ipp]
            ipp_chopper_slits = "%s_Chopper" %PoldiIPP
            ipp_Poldi_spectra = "%s_Spectra" %PoldiIPP
            ipp_ipp_data      = "%s_Data"    %PoldiIPP
            
            ex_of_sample_ws = mtd[ex_of_sample]
        
            self.log().debug('Poldi - chopper slits')
            PoldiLoadChopperSlits(InputWorkspace=ex_of_sample_ws, 
                                  PoldiChopperSlits=ipp_chopper_slits)
        
        
            self.log().debug('Poldi - spectra')
            PoldiLoadSpectra(InputWorkspace=ex_of_sample_ws, 
                             PoldiSpectra=ipp_Poldi_spectra)
        
        
            self.log().debug('Poldi - IPP')
            PoldiLoadIPP(InputWorkspace=ex_of_sample_ws, 
                         PoldiIPP=ipp_ipp_data)
        
        
        
        
        
        for sample in range(nb_of_sample):
            sampleName     = sample_info_ws.column("spl Name" )[sample]
            filePath       = sample_info_ws.column("data file")[sample]
            sampleNameLog  = sample_info_ws.column("spl log"  )[sample]
            sampleDeadWires= sample_info_ws.column("spl dead wires"  )[sample]
            sampleNameCorr = sample_info_ws.column("spl corr" )[sample]
            
            sample_ipp = sample_output_ws.getInstrument().getStringParameter("ipp")[0]
            PoldiIPP        = sample_ipp_ws.column("ipp version")[ipp]
            ipp_chopper_slits = "%s_Chopper" %PoldiIPP
            ipp_Poldi_spectra = "%s_Spectra" %PoldiIPP
            Ipp_ipp_data      = "%s_Data"    %PoldiIPP
        
            PoldiAutoCorrelation(InputWorkspace=mtd[sampleName], 
                                 PoldiSampleLogs=sampleNameLog, 
                                 PoldiDeadWires=mtd[sampleDeadWires], 
                                 PoldiChopperSlits=mtd[ipp_chopper_slits], 
                                 PoldiSpectra=mtd[ipp_Poldi_spectra], 
                                 PoldiIPP=mtd[Ipp_ipp_data],
                                 wlenmin=wlen_min,
                                 wlenmax=wlen_max, 
                                 OutputWorkspace=sampleNameCorr)
            
            
            sampleNamePeak = sample_info_ws.column("spl peak" )[sample]
        
            PoldiPeakDetection(InputWorkspace=sampleNameCorr,
                               PeakDetectionThreshold=peak_detect_threshold,
                               OutputWorkspace=sampleNamePeak)
            
            

        if(load_data_at_the_end):
            self.setProperty("OutputWorkspace", sample_ipp_ws)

AlgorithmFactory.subscribe(PoldiProjectRun)
