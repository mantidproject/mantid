from isis_reflgui.config import *
import xml.etree.ElementTree as xml

'''
This is an ISIS specific search tool for the ISIS reflectometry GUI. In the future, ICAT may prove to be a better interface to do this.

Requires an instrument name and a mount point (defined in a config.xml file)
The mount point is the top level directory for the NDXInstruments.

When getLatestJournalRuns is called it will return a list of all the names of the runs from the most recent cycle for that instrument. It will also add this data directory
onto the managed user directories list.
'''
class LatestISISRuns(object):
    
    __instrument = None
    __mountpoint = None
    
    def __init__(self, instrument):
        
        self.__instrument = instrument.upper().strip()    
        
        settings = isis_reflgui.config.Config() # This will throw a missing config exception if no config file is available.
        try:
            self.__mountpoint = settings.get_named_setting("DataMountPoint")
        except KeyError:
            print "DataMountPoint is missing from the config.xml file."
            raise
    
    def __getLatestJournalPath(self, base_path):
        path = os.path.join(base_path, 'journal_main.xml') 
        tree = xml.parse(path)
        dom = tree.getroot()
        latest_journal = dom[-1].attrib.get('name')
        journal_path = os.path.join(base_path, latest_journal)
        return journal_path

    def __findCycleId(self, path):
        tree = xml.parse(path)    
        root = tree.getroot()
        for  run in root:
            for elem in run:
                if elem.tag.split('}')[-1] == 'isis_cycle':
                    return elem.text
                    
    def __checkPath(self, path):
        if not os.path.exists(path):
            raise RuntimeError("The path %s does not exist" % path)
           

    def __getLocations(self):
                
            instr_path = os.path.join(self.__mountpoint, 'NDX'+  self.__instrument, 'Instrument')
            self.__checkPath(instr_path)
            
            base_path =  os.path.join(instr_path, 'logs', 'journal')
            self.__checkPath(base_path)
            
            journal_path = self.__getLatestJournalPath(base_path)
            self.__checkPath(journal_path)
            
            cycle_dir =  'cycle_'+ self.__findCycleId(journal_path)
            cycle_path = os.path.join(instr_path, 'data', cycle_dir)
            self.__checkPath(cycle_path)
            
            return cycle_path, journal_path

    def __addSettingDirToManagedUserDirs(self, cycle_dir):
        current_search_dir= config.getDataSearchDirs()
        if cycle_dir:
            if not cycle_dir in current_search_dir:
                config.appendDataSearchDir(cycle_dir)
                
    def getLatestJournalRuns(self):
        cycle_dir, journal_path  = self.__getLocations()
        # side effect.
        self.__addSettingDirToManagedUserDirs(cycle_dir)

        runnames = []
        tree = xml.parse(journal_path)    
        root = tree.getroot()
        for  run in root:
            runnames.append(run.get('name'))
        return runnames