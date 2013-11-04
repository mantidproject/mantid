from isis_reflgui.settings import *
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
    __most_recent_cycle = None
    __cycleMap = None
    
    def __init__(self, instrument):
        
        self.__instrument = instrument.upper().strip()    
        
        usersettings = Settings() # This will throw a missing config exception if no config file is available.
        try:
            self.__mountpoint = usersettings.get_named_setting("DataMountPoint")
        except KeyError:
            print "DataMountPoint is missing from the config.xml file."
            raise
        
        self.__cycleMap = self.makeMappings(firstonly = True)
        
    def __make_map_entry(self, map, element, base_path, instr_path):
        journal_file = element.attrib.get('name')
        journal_path = os.path.join(base_path, journal_file)
        cycle_id = self.__findCycleId(journal_path)
        cycle = 'cycle_'+ cycle_id
        cycle_dir_path = os.path.join(instr_path, 'data', cycle)
        map[cycle] = journal_path, cycle_dir_path
        self.__most_recent_cycle = cycle
        
    def makeMappings(self, firstonly = False):
        instr_path = os.path.join(self.__mountpoint, 'NDX'+  self.__instrument, 'Instrument')
        self.__checkPath(instr_path)
            
        base_path =  os.path.join(instr_path, 'logs', 'journal')
        self.__checkPath(base_path)
        
        path = os.path.join(base_path, 'journal_main.xml') 
        tree = xml.parse(path)
        map = dict()

        dom = tree.getroot()
        if firstonly:
            self.__make_map_entry(map, dom[-1], base_path, instr_path)
        else:
            for elem in dom:
                self.__make_map_entry(map, elem, base_path, instr_path)
        return map
    
    def getLatestCycle(self):
        return self.__most_recent_cycle
    
    def getInstrument(self):
        return self.__instrument
    
    def getCycles(self):
        cached_cycle_keys = list(self.__cycleMap.keys())
        if len(cached_cycle_keys) == 1:
            journal_path, cycle_dir_path = self.__cycleMap[self.__most_recent_cycle]
            parent_dir = os.path.abspath(os.path.join(cycle_dir_path,".."))
            print parent_dir
            all_cycles_dirs = [d for d in os.listdir(parent_dir) if os.path.isdir(os.path.join(parent_dir, d))] 
            return all_cycles_dirs
        return cached_cycle_keys
    
    def __findCycleId(self, path):
        tree = xml.parse(path)    
        root = tree.getroot()
        for  run in root:
            for elem in run:
                if elem.tag.split('}')[-1] == 'isis_cycle':
                    return  elem.text
                    
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
                   
    def getJournalRuns(self, cycle=None):
        if not cycle:
            journal_path, cycle_dir_path = self.__cycleMap[self.__most_recent_cycle]
        else:
            if len(self.__cycleMap) == 1:
                self.__cycleMap = self.makeMappings() # This will take some time
            journal_path, cycle_dir_path = self.__cycleMap[str(cycle).strip()]
            
        # side effect.
        self.__addSettingDirToManagedUserDirs(cycle_dir_path)

        runnames = []
        tree = xml.parse(journal_path)    
        root = tree.getroot()
        for  run in root:
            runnames.append(run.get('name'))
        return runnames