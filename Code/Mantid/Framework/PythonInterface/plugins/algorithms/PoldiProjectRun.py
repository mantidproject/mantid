#pylint: disable=no-init
from mantid.api import PythonAlgorithm
from mantid.api import ITableWorkspaceProperty
from mantid.api import AlgorithmFactory, WorkspaceFactory

from mantid.kernel import Direction, ConfigServiceImpl

from mantid.simpleapi import config, mtd
from mantid.simpleapi import (LoadSINQFile,
                              PoldiLoadLog,
                              LoadInstrument,
                              PoldiRemoveDeadWires,
                              PoldiLoadChopperSlits,
                              PoldiLoadSpectra,
                              PoldiLoadIPP,
                              #PoldiAutoCorrelation,
                              #PoldiPeakDetection,
                              GroupWorkspaces,
                              RenameWorkspace)
import os.path




class PoldiProjectRun(PythonAlgorithm):

    def category(self):
        """ Mantid required
        """
        return "SINQ\\Poldi\\Obsolete"

    def name(self):
        """ Mantid required
        """
        return "PoldiProjectRun"

    def summary(self):
        return "Run the POLDI analysis process for a bunch of data files stored in a tableWorkspace."

    def PyInit(self):
        """ Mantid required
        """
        self.declareProperty(ITableWorkspaceProperty("InputWorkspace", "PoldiAnalysis", direction=Direction.Input),
                             "Poldi analysis main worksheet")

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "PoldiIPPmanager", direction=Direction.Output),\
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
            if sample_ipp_ws_name == "":
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

        self.log().information('Poldi run with parameters')
        self.log().information('      -  wlen_min : %s' %(wlen_min))
        self.log().information('      -  wlen_max : %s' %(wlen_max))
        self.log().information('      -  bad_wires_threshold   : %s' %(bad_wires_threshold))
        self.log().information('      -  peak_detect_threshold : %s' %(peak_detect_threshold))

        dictsearch=os.path.join(config['instrumentDefinition.directory'],"nexusdictionaries","poldi.dic")
        self.log().information('Poldi instr folder -  %s' %(dictsearch))

        firstOne=""

        self.log().debug('Poldi - load data')
        nb_of_sample = sample_info_ws.rowCount()
        self.log().information('Poldi -  %d samples listed' %(nb_of_sample))

        for sample in range(nb_of_sample):
            sampleName      = sample_info_ws.column("spl Name")[sample]
            filePath        = sample_info_ws.column("data file")[sample]
            sampleNameLog   = sample_info_ws.column("spl log")[sample]
            sampleDeadWires = sample_info_ws.column("spl dead wires")[sample]

            self.log().information('Poldi - sample %s' %(sampleName))
            LoadSINQFile(Instrument="POLDI",
                         Filename=filePath,
                         OutputWorkspace=sampleName)

            sample_output_ws = mtd[sampleName]

            PoldiLoadLog(InputWorkspace=sample_output_ws,
                         Filename=filePath,
                         Dictionary=dictsearch,
                         PoldiLog=sampleNameLog)

            cfgService = ConfigServiceImpl.Instance()

            LoadInstrument(Workspace=sample_output_ws,
                           Filename=cfgService.getInstrumentDirectory() + "POLDI_Definition_ipp13.xml",
                           RewriteSpectraMap=True)


            self.log().debug('Poldi - set ipp')
            sample_instrument = sample_output_ws.getInstrument()
            ipp_version = sample_instrument.getStringParameter("ipp")[0]

            add_this_ipp = True
            for ipp in range(sample_ipp_ws.rowCount()):
                if sample_ipp_ws.column("ipp version")[ipp] == ipp_version:
                    add_this_ipp = False
            if add_this_ipp:
                sample_ipp_ws.addRow([sampleName, ipp_version])


            self.log().debug('Poldi - dead wires')
            PoldiRemoveDeadWires(InputWorkspace=sample_output_ws,
                                 RemoveExcludedWires=True,
                                 AutoRemoveBadWires=False,
                                 BadWiresThreshold=bad_wires_threshold,
                                 PoldiDeadWires=sampleDeadWires)



        nb_of_ipp = sample_ipp_ws.rowCount()
        self.log().information('Poldi -  %d ipp listed' %(nb_of_ipp))

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

            groupedResults = GroupWorkspaces([mtd[sampleName].name(), sampleNameLog, sampleDeadWires])
            RenameWorkspace(InputWorkspace=groupedResults,
                            OutputWorkspace="%s_Metadata" % sampleName)

        if load_data_at_the_end:
            self.setProperty("OutputWorkspace", sample_ipp_ws)

AlgorithmFactory.subscribe(PoldiProjectRun)
