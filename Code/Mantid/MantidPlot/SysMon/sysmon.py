
 
import sys, os, time
import re
import config  #application constants and variables
#suppress deprecation warnings that can occur when importing psutil version 2
#note - all deprecation warnings will probably be suppressed using this filterwarnings
#as specifying the psutil module specifically in filterwarnings did not suppress 
#these warnings
import warnings
warnings.filterwarnings('ignore',category=DeprecationWarning)
import psutil

#check psutil version as command syntax changes between version 1 and version 2
ver=psutil.__version__
#using positional numeric wildcards for re.match() to check sub versions
#match returns a match object if a match is found else returns NoneType
if re.match('0.[0-9].[0-9]',ver) or re.match('1.[0-9].[0-9]',ver) != None:
    #set flag to version 1 if either version 0 or version 1 psutil imported
    config.psutilVer=1
else:
    #set flat to version 2 for any versions higher than version 1
    config.psutilVer=2

from ui_sysmon import *
from sysmon_tools import *


import platform
import commands

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

#import application version information
__version__="unknown"
try:
    from _version import __version__
except ImportError:
    #_version.py file not found - version not known in this case so use
    #the default previously given.
    pass

class SysMon(QtGui.QWidget):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.ui = Ui_Form() #defined from ui_sysmon.py
        self.ui.setupUi(self)
        self.ui.parent=parent
        self.ui.progressBarStatusMemory.setStyleSheet("QProgressBar {width: 25px;border: 1px solid black; border-radius: 3px; background: white;text-align: center;padding: 0px;}"
                               +"QProgressBar::chunk:horizontal {background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00CCEE, stop: 0.3 #00DDEE, stop: 0.6 #00EEEE, stop:1 #00FFEE);}")
        self.ui.progressBarStatusCPU.setStyleSheet("QProgressBar {width: 25px;border: 1px solid black; border-radius: 3px; background: white;text-align: center;padding: 0px;}"
                               +"QProgressBar::chunk:horizontal {background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00CCEE, stop: 0.3 #00DDEE, stop: 0.6 #00EEEE, stop:1 #00FFEE);}")

        #setup timer to enable periodic events such as status update checks
        self.ctimer = QtCore.QTimer()
        self.ctimer.start(2000)  #time in mSec - set default update timer cycle to 2 seconds
        QtCore.QObject.connect(self.ctimer, QtCore.SIGNAL("timeout()"), self.constantUpdate)

        #update rate actions
        QtCore.QObject.connect(self.ui.radioButton1Sec, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update1Sec)
        QtCore.QObject.connect(self.ui.radioButton2Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update2Sec)
        QtCore.QObject.connect(self.ui.radioButton5Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update5Sec)
        QtCore.QObject.connect(self.ui.radioButton10Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update10Sec)
        self.update=2 #set default to 2 seconds update rate

        #duration actions
        QtCore.QObject.connect(self.ui.radioButton60Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update60Duration)
        QtCore.QObject.connect(self.ui.radioButton300Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update300Duration)
        QtCore.QObject.connect(self.ui.radioButton600Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update600Duration)
        QtCore.QObject.connect(self.ui.radioButton3600Secs, QtCore.SIGNAL(_fromUtf8("clicked(bool)")), self.update3600Duration)
        self.duration=60 #set default plot duration to 60 seconds

        QtCore.QObject.connect(self.ui.pushButtonUpdate, QtCore.SIGNAL('clicked(bool)'), self.updateProcesses)

        #Initialize System Tab
        self.ui.tabWidget.setCurrentIndex(config.SYST_TAB)
        self.ui.labelComputerName.setText("Computer Name: "+platform.node())
        if platform.os.name == 'nt':
            info=platform.win32_ver()
            oslabel="Windows "+info[0]+"  Version: "+info[1]
        elif platform.os.name == 'posix':
            info=platform.linux_distribution()
            oslabel=info[0]+" Version: "+info[1]
        elif platform.os.name == 'mac':
            info=platform.mac_ver()
            oslabel=info[0]+"  Version: "+info[1]
        else:
            oslabel=" "

        self.ui.labelOS.setText("Operating System: "+oslabel)
        info=platform.uname()
        self.ui.labelProcFam.setText("Processor Family: "+info[5])

        #determine the number of users on the computer
        userInfo=psutil.get_users() if config.psutilVer==1 else psutil.users()
        lst=[]
        for item in userInfo:
            lst.append(item.name)
        uusers=set(lst)
        Nuusers=len(uusers)
        self.ui.labelNUsers.setText("Number of Users Logged On: "+str(Nuusers))

        #determine the computer uptime
        if config.psutilVer == 1:
            uptime = str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.BOOT_TIME))
        else:
            uptime = str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.boot_time()))
        self.ui.labelUptime.setText("System Uptime: "+uptime)

        if config.mplLoaded:
            #if matplotlib loaded OK, initialize plotting
            #Initialize History Tab
            self.ui.tabWidget.setCurrentIndex(config.HIST_TAB)
            #Place Matplotlib figure within the GUI frame
            #create drawing canvas
            # a figure instance to plot on

            if not re.match('1.[0-1]',matplotlib.__version__):
                #if not an old version of matplotlib, then use the following command 
                matplotlib.rc_context({'toolbar':False})
            #initialize figure 1 and its canvas for cpu and memory history plots
            self.ui.figure = plt.figure(1)
            # the Canvas Widget displays the `figure`
            # it takes the `figure` instance as a parameter to __init__
            self.ui.canvas = FigureCanvas(self.ui.figure)
            layout=QtGui.QVBoxLayout(self.ui.framePlot)
            layout.addWidget(self.ui.canvas)
            self.ui.layout=layout

            #initialize figure 2 and its canvas for user usage bar chart
            self.ui.figure2 = plt.figure(2)
            self.ui.canvas2 = FigureCanvas(self.ui.figure2)
            layout2=QtGui.QVBoxLayout(self.ui.frameBar)
            layout2.addWidget(self.ui.canvas2)
            self.ui.layout2=layout2


        else:
            #if matplotlib not loaded, remove the tabs that depend upon it
            self.removeMPLTabs()

        #initialize history plot arrays - easier just to initialize these even if not doing plotting in case when matplotlib not available.
        Nsamples=3600
        self.ui.Nsamples=Nsamples
        #need one extra sample to fill the plotting interval
        self.ui.cpu=np.zeros(Nsamples+1)
        self.ui.mem=np.zeros(Nsamples+1)
        self.ui.dt=[None]*(Nsamples+1)
        self.ui.cpuMe=np.zeros(Nsamples+1)
        self.ui.memMe=np.zeros(Nsamples+1)

        self.ui.tabWidget.setTabsClosable(False)  #disable the ability to close tabs once state of matplotlib is handled

        #initialize the process table
        self.doUpdates=True #flag for updating the process tab table
        updateProcTable(self,config)
        
        #upon initialization completion, set System tab (first tab on left) as the visible tab
        self.ui.tabWidget.setCurrentIndex(config.SYST_TAB)
        
        #initialize version label
        self.ui.labelVersion.setText("Version: "+__version__+"_"+psutil.__version__)

    def constantUpdate(self):
        #redirct to global function
        constantUpdateActor(self,config)

    def update1Sec(self):
        self.update=1
        self.ctimer.stop()
        self.ctimer.start(1000)
        #clear persistent arrays when update rate changed
        self.ui.cpu=self.ui.cpu*0
        self.ui.mem=self.ui.mem*0
        self.ui.cpuMe=self.ui.cpuMe*0
        self.ui.memMe=self.ui.memMe*0
        self.ui.dt=[None]*self.ui.Nsamples

    def update2Sec(self):
        self.update=2
        self.ctimer.stop()
        self.ctimer.start(2000)
        #clear persistent arrays when update rate changed
        self.ui.cpu=self.ui.cpu*0
        self.ui.mem=self.ui.mem*0
        self.ui.cpuMe=self.ui.cpuMe*0
        self.ui.memMe=self.ui.memMe*0
        self.ui.dt=[None]*self.ui.Nsamples

    def update5Sec(self):
        self.update=5
        self.ctimer.stop()
        self.ctimer.start(5000)
        #clear persistent arrays when update rate changed
        self.ui.cpu=self.ui.cpu*0
        self.ui.mem=self.ui.mem*0
        self.ui.cpuMe=self.ui.cpuMe*0
        self.ui.memMe=self.ui.memMe*0
        self.ui.dt=[None]*self.ui.Nsamples

    def update10Sec(self):
        self.update=10
        self.ctimer.stop()
        self.ctimer.start(10000)
        #clear persistent arrays when update rate changed
        self.ui.cpu=self.ui.cpu*0
        self.ui.mem=self.ui.mem*0
        self.ui.cpuMe=self.ui.cpuMe*0
        self.ui.memMe=self.ui.memMe*0
        self.ui.dt=[None]*self.ui.Nsamples

    def update60Duration(self):
        self.duration=60
    def update300Duration(self):
        self.duration=300
    def update600Duration(self):
        self.duration=600
    def update3600Duration(self):
        self.duration=3600

    def updateProcesses(self):
        if self.doUpdates == True:
            #case to toggle to False
            self.doUpdates=False
            self.ui.pushButtonUpdate.setText('Continue')
        else:
            #case where updates must be off
            self.doUpdates=True
            self.ui.pushButtonUpdate.setText('Hold Updates')

    def resizeEvent(self,resizeEvent):
        sz=self.ui.tableWidgetProcess.size()
        w=sz.width()
        wmin=self.ui.parent.minimumSize().width() #establish minimum table size based upon parent widget minimum size
        if w < wmin:
            w=wmin
        #now use widget width to determine process table column width
        self.ui.tableWidgetProcess.setColumnWidth(0,3.5*w/20) #PID
        self.ui.tableWidgetProcess.setColumnWidth(1,4*w/20) #User
        self.ui.tableWidgetProcess.setColumnWidth(2,3.5*w/20) #CPU%
        self.ui.tableWidgetProcess.setColumnWidth(3,3.5*w/20) #MEM%
        self.ui.tableWidgetProcess.setColumnWidth(4,5.5*w/20) #Name
        
        #check size of GUI to determine the size of font to use.
        minSz=self.ui.parent.minimumSize().width() #establish minimum table size based upon parent widget minimum size
        curSz=self.ui.parent.size().width()
        #print "current size: ",curSz," type: ",type(curSz),"  min size: ",minSz,"  type: ",type(minSz)
        fsize=max([int(config.basefontsize*float(curSz)/float(minSz)*config.fscl),config.basefontsize])
        #print "Font Size: ",fsize
        config.pltFont=fsize
        
        #adapt plot line width to GUI size change
        config.linewidth=min([max([int(float(curSz)/float(minSz)),1]),3])

    def removeMPLTabs(self):
        #In case matplotlib not available, remove tabs requiring this
        #Note to developers - removing tabs changes the tab index numbers!
        #Keep this in mind regarding setting and using tab indices
        self.ui.tabWidget.removeTab(config.HIST_TAB) #History tab
        config.HIST_TAB=-1
        config.PROC_TAB-=1
        config.USER_TAB-=1
        config.OPTS_TAB-=1
        self.ui.tabWidget.removeTab(config.USER_TAB) #Users tab - originally tab 3, but becomes tab 2 once history tab is removed
        config.USER_TAB=-1
        config.OPTS_TAB-=1
        self.ui.tabWidget.setTabsClosable(False)
