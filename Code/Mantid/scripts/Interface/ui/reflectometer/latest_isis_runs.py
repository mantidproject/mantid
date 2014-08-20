from isis_reflectometry.settings import *
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
    __number_of_cycles = None
    __cycleMap = None

    def __init__(self, instrument):
        self.__instrument = instrument.upper().strip()
        usersettings = Settings() # This will throw a missing config exception if no config file is available.
        try:
            self.__mountpoint = usersettings.get_named_setting("DataMountPoint")
        except KeyError:
            print "DataMountPoint is missing from the config.xml file."
            raise
        self.getPaths()
        self.setLatestCycle()
    def getPaths(self):
        self.instr_path = os.path.join(self.__mountpoint, 'NDX'+  self.__instrument, 'Instrument')
        self.__checkPath(self.instr_path)
        self.base_path =  os.path.join(self.instr_path, 'logs', 'journal')
        self.__checkPath(self.base_path)
        self.main_path = os.path.join(self.base_path, 'journal_main.xml')
        self.text_path = os.path.join(self.base_path, 'summary.txt')
    def setLatestCycle(self):
        tree = xml.parse(self.main_path)
        dom = tree.getroot()
        self.__number_of_cycles = len(dom)
        element = dom[-1]
        journal_file = element.attrib.get('name')
        journal_path = os.path.join(self.base_path, journal_file)

        cycle_id = self.__findCycleId(journal_path)
        cycle = 'cycle_'+ cycle_id
        cycle_dir_path = os.path.join(self.instr_path, 'data', cycle)

        self.recentrunPaths = journal_path, cycle_dir_path
        self.__most_recent_cycle = cycle
    def getLatestCycle(self):
        return self.__most_recent_cycle
    def getInstrument(self):
        return self.__instrument
    def getNumCycles(self):
        return self.__number_of_cycles
    def __findCycleId(self, path):
        tree = xml.parse(path)
        root = tree.getroot()
        for  run in root:
            for elem in run:
                if elem.tag.split('}')[-1] == 'isis_cycle':
                    return elem.text
        return None
    def __checkPath(self, path):
        if not os.path.exists(path):
            raise RuntimeError("The path %s does not exist" % path)
    def __addSettingDirToManagedUserDirs(self, cycle_dir):
        current_search_dir= config.getDataSearchDirs()
        if cycle_dir:
            if not cycle_dir in current_search_dir:
                config.appendDataSearchDir(cycle_dir)
    def gettextway(self, eID):
        runnames = []
        linecache = []
        if eID:
            f = open(self.text_path, 'r')
            expected_eID_length = -8
            eLength = -1*(len(eID)+1)
            if eLength > expected_eID_length:
                eLength = expected_eID_length
            for line in f:
                ###start line correcting block
                #this is here for the purpose of correcting a badly formatted summary file
                #this concatenates lines broken by EOL characters where they shouldn't be
                #as there were some in the INTER summary file when I wrote this
                expected_line_length = 89
                if len(line) < expected_line_length:
                    linecache.append(line)
                    cachecount = 0
                    for entry in linecache:
                        cachecount = cachecount + len(entry)
                    if cachecount == expected_line_length:
                        line = ''.join(linecache)
                        linecache = []
                    elif cachecount > expected_line_length:
                        linecache = []
                    else:
                        continue
                else:
                    linecache = []
                ###end line correcting block
                lineID = line[eLength:].strip()
                if str(eID) == str(lineID):
                    #grab and concatenate the run number and investigation name.
                    item = line[3:8] + ": " + line[28:52].strip()
                    runnames.append(item)
        return runnames
    def getxmlway(self, eID, maxDepth):
        runnames = []
        tree = xml.parse(self.main_path)
        dom = tree.getroot()
        depth = 0
        revDom = reversed(dom)
        print "Searching Journals for RB number: " + str(eID)
        for cycle in revDom:
            journal_file = cycle.attrib.get('name')
            journal_path = os.path.join(self.base_path, journal_file)
            cycle_id = self.__findCycleId(journal_path)
            if cycle_id:
                cycle = 'cycle_'+ cycle_id
                cycle_dir_path = os.path.join(self.instr_path, 'data', cycle)
                # side effect.
                self.__addSettingDirToManagedUserDirs(cycle_dir_path)
                cycletree = xml.parse(journal_path)
                root = cycletree.getroot()
                for run in root:
                    runno = None
                    title = None
                    if (run[4].text == eID):
                        runno = run[6].text.strip()
                        title = run[0].text.strip()
                        if title and runno:
                            journalentry = runno + ": " + title
                            runnames.append(journalentry)
                depth = depth + 1
                if depth >= maxDepth:
                    break
            else:
                print "Error reading journal: " + journal_path + " - could not identify associated cycle. Skipping journal."
        print "Search for RB number " + str(eID) + " complete. Search yielded " + str(len(runnames)) + " results"
        return runnames
    def getJournalRuns(self, eID, maxDepth = 1):
        if maxDepth < 1:
            maxDepth = 1
        return self.getxmlway(eID.strip(), maxDepth)
