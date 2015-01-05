#suppress deprecation warnings that can occur when importing psutil version 2
#note - all deprecation warnings will probably be suppressed using this filterwarnings
#as specifying the psutil module specifically in filterwarnings did not suppress 
#these warnings
import warnings
warnings.filterwarnings('ignore',category=DeprecationWarning)
import psutil

from PyQt4 import Qt, QtCore, QtGui
import datetime
import numpy as np
import config
import math
import getpass
import re
import sys
import time

#check if command line flag --nompl set to disable matplotlib 
if not(config.nompl):
    try:
        import matplotlib
        if matplotlib.get_backend() != 'QT4Agg':
            matplotlib.use('QT4Agg')
        from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
        from matplotlib.backends.backend_qt4 import NavigationToolbar2QT as NavigationToolbar
        import matplotlib.pyplot as plt
        config.mplLoaded=True
    #    print "matplotlib try successful"
    except:
    #    print "matplotlib except case"
        #case where matplotlib not available - need to know this for handling plotting tabs
        config.mplLoaded=False
        pass
else:
    config.mplLoaded=False #use this flag to test case when matplotlib is not available    
    
def constantUpdateActor(self,config):

    #set duration number
    Ndur=self.duration

    #get current CPU percent busy
    percentcpubusy = psutil.cpu_percent()

    percentmembusy=psutil.virtual_memory().percent
    self.ui.progressBarStatusMemory.setValue(round(percentmembusy))
    Ncpus=len(psutil.cpu_percent(percpu=True))
    self.ui.Ncpus=Ncpus
    totalcpustr='CPU Count: '+str(Ncpus)
#        print "Total CPU str: ",totalcpustr
    self.ui.labelCPUCount.setText(totalcpustr+'  - CPU Utilization:')
    totalmem=int(round(float(psutil.virtual_memory().total)/(1024*1024*1024))) #psutil syntax OK for both versions
#        print "Total Mem: ",totalmem
    self.ui.totalmem=totalmem
    totalmemstr=str(totalmem)+' GB'
    self.ui.labelMemUsage.setText('Memory Usage: '+str(totalmem*percentmembusy/100)+' GB of '+totalmemstr)
#        print "Total Mem str: ",totalmemstr
    
    # update system tab
    if self.ui.tabWidget.currentIndex() == config.SYST_TAB:
        #determine the computer uptime
        if config.psutilVer == 1:
            uptime = str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.BOOT_TIME))
        else:
            uptime = str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.boot_time()))
        self.ui.labelUptime.setText("System Uptime: "+uptime)


        
        #update the number of users each time interval as well
        userInfo=psutil.get_users() if config.psutilVer == 1 else psutil.users()
        lst=[]
        for item in userInfo:
            lst.append(item.name)
        uusers=set(lst)
        Nuusers=len(uusers)
        self.ui.labelNUsers.setText("Number of Users Logged On: "+str(Nuusers))
        
    #determine "Me" user CPU and memory statistics
    Me=getpass.getuser()
    cpupctMe=0
    memValMe=0
    cpupctTot=0
    memValTot=0
    for proc in psutil.process_iter():
        try:
            #check if process still exists, if so, update dictionaries
            cpupct=proc.get_cpu_percent(interval=0) if config.psutilVer == 1 else proc.cpu_percent(interval=0)
            memVal=proc.get_memory_percent() if config.psutilVer == 1 else proc.memory_percent()

            try:
                #some processes give permission denied when getting the name, if so, fail out gracefully using this try.
                if config.psutilVer == 1:
                    uname=proc.username
                else:
                    uname=proc.username()
                #print "proc.username: ",uname," type: ",type(uname),"  Me: ",Me," type: ",type(Me)
            except:
                uname=''
        except:
            #skip process - case where process no longer exists
            cpupct=0
            memVal=0
            uname=''
        cpupctTot+=cpupct
        memValTot+=memVal
        #print "uname: ",uname,"  Me: ",Me
        #note that on Windows systems that getpass.getuser() does not return the same base username as proc.username, so check for the smaller 
        if Me in uname:
            cpupctMe+=cpupct
            memValMe+=memVal
    #print "cpupctMe: ",cpupctMe,"  memValMe: ",memValMe
    
    #update first position with most recent value overwriting oldest value which has been shifted to first position
    self.ui.cpu=np.roll(self.ui.cpu,1)
    #check if CPU smoothing to be applied
    sm=int(str(self.ui.comboBoxCPUHistSmooth.currentText()))
    if sm == 1:
        self.ui.cpu[0]=percentcpubusy
    elif sm >1:
        self.ui.cpu[0]=(percentcpubusy+np.sum(self.ui.cpu[1:sm]))/sm
    else:
        #unknown case - default to no smoothing
        self.ui.cpu[0]=percentcpubusy
    #update progress bar with (potentially) smoothed cpu percentage
    self.ui.progressBarStatusCPU.setValue(round(self.ui.cpu[0]))
    self.ui.mem=np.roll(self.ui.mem,1)
    self.ui.mem[0]=percentmembusy
    self.ui.cpuMe=np.roll(self.ui.cpuMe,1)
    #check if CPU smoothing to be applied
    if sm == 1:
        self.ui.cpuMe[0]=cpupctMe/(Ncpus)
    elif sm>1:
        self.ui.cpuMe[0]=(cpupctMe/(Ncpus)+np.sum(self.ui.cpuMe[1:sm]))/sm
    else:
        #use no filtering in case sm is unknown (should never happen...)
        self.ui.cpuMe[0]=cpupctMe/(Ncpus)
    self.ui.memMe=np.roll(self.ui.memMe,1)
    self.ui.memMe[0]=memValMe    


    #update the history plot
    if self.ui.tabWidget.currentIndex() == config.HIST_TAB:
        #only update history plot if tab is active    
        font = {'family' : 'sans-serif',
            'weight' : 'bold',
            'size'   : config.pltFont+1}
        matplotlib.rc('font', **font)
     
        xtime=range(0,self.ui.Nsamples+1,self.update)
        
        Npts=Ndur/self.update
        
        plt.figure(self.ui.figure.number)     #make plot figure active   
        plt.clf() #clear figure each time for rolling updates to show
        plt.plot(xtime[0:Npts+1],self.ui.cpu[0:Npts+1],color='Blue',label='CPU: All',linewidth=config.linewidth)
        plt.plot(xtime[0:Npts+1],self.ui.mem[0:Npts+1],color='Green',label='Mem: All',linewidth=config.linewidth)
        plt.plot(xtime[0:Npts+1],self.ui.cpuMe[0:Npts+1],color='red',label='CPU: '+Me,linewidth=config.linewidth)
        plt.plot(xtime[0:Npts+1],self.ui.memMe[0:Npts+1],color='cyan',label='Mem: '+Me,linewidth=config.linewidth)

        plt.title('Composite CPU and Memory Activity',fontsize=config.pltFont+1,fontweight='bold')
        plt.ylabel('% Used',fontsize=config.pltFont+0.5,fontweight='bold')
        
        if self.update == 1:
            xlab="Seconds with 1 Second Updates"
        elif self.update == 2:
            xlab="Seconds with 2 Second Updates"
        elif self.update == 5:
            xlab="Seconds with 5 Second Updates"    
        elif self.update == 10:
            xlab="Seconds with 10 Second Updates"
        plt.xlabel(xlab,fontsize=config.pltFont+0.5,fontweight='bold')
        plt.legend(loc="upper right",prop={'size':config.pltFont})
        
        plt.xlim([0,Ndur])
        self.ui.canvas.draw()
    
    #update the process table
    if self.ui.tabWidget.currentIndex() == config.PROC_TAB:
        #only update process table if tab is active
        updateProcTable(self,config)
    
    #update the Users bar chart
    if self.ui.tabWidget.currentIndex() == config.USER_TAB:
        #only update bar chart if the tab is active.
        updateUserChart(self,config)
    
    
    
def updateProcTable(self,config):
    if self.doUpdates==True:
        
        Ncpus=len(psutil.cpu_percent(percpu=True))
        
        table=self.ui.tableWidgetProcess
        #first remove all rows
        Nrows=table.rowCount()
        for row in range(Nrows):
            table.removeRow(0)
        
        #get the processes
        pidList=psutil.get_pid_list() if config.psutilVer == 1 else psutil.pids()
        Npids=len(pidList)
        
        #now add rows to the table according to the number of processes
    #    for row in range(Npids):
    #        table.insertRow(0)
        
        #now populate the table
        row=0  #table row counter incremented for each row added to the process table
        rout=0 #counter for number of rows to remove due to invalid processes 
        memtot=psutil.virtual_memory()  #psutil syntax OK for both versions
        
        #determine which column has been selected for sorting
        column_sorted=table.horizontalHeader().sortIndicatorSection()
        order = table.horizontalHeader().sortIndicatorOrder()
        #temporarily set sort column to outside column range so that table items can be filled properly
        table.sortItems(5,order=QtCore.Qt.AscendingOrder)
        #print table.horizontalHeader().sortIndicatorSection()
        
        #create empty dictionaries to be used by the process table
        d_user={}
        d_cpu={}
        d_mem={}
        d_name={}
        d_cpuTimes={}
        d_procTimes={}

        #fill the dictionaries - seems to need to be done faster than within loop which also fills the table...not sure why...

        try:
            #check if this dictionary exists or not
            self.ui.d_procTimes
            self.ui.d_cpuTimes
            #if so, move on
            pass
        except:
            #case to initialize the dictionary
            for proc in psutil.process_iter():
                try:
                    #check if acess denied 
                    pname=proc.name
                    proctime=proc.get_cpu_times() #get 
                    cputime=psutil.cpu_times()
                    d_procTimes.update({proc.pid:proctime})
                    d_cpuTimes.update({proc.pid:cputime})
                except:
                    #case we skip a process
                    pass
            self.ui.d_cpuTimes=d_cpuTimes
            self.ui.d_procTimes=d_procTimes
            d_cpuTimes={}
            d_procTimes={}

        updateInterval=float(self.update) #timer interval in seconds
        for proc in psutil.process_iter():
            #try:
            if psutil.Process(proc.pid).is_running():
            #if proc.pid == 37196:    
                #check if process still exists, if so, update dictionaries
                try:
                    #check if process previously existed - if so we can calculate a cpupct
                    proctimeHold=self.ui.d_procTimes[proc.pid]                    
                    proctime=proc.get_cpu_times() #get 
                    deltaProcTime=(proctime.user+proctime.system) - (proctimeHold.user+proctimeHold.system)                    
                    
                    cputimeHold=self.ui.d_cpuTimes[proc.pid]
                    cputime=psutil.cpu_times()
                    deltaCPUTime=(cputime.user+cputime.system+cputime.idle) - (cputimeHold.user+cputimeHold.system+cputimeHold.idle)
                    
                    if deltaProcTime > 0:
                        if deltaCPUTime < updateInterval:
                            deltaCPUTime=updateInterval
                        else:
                            pass
                        cpupct=float(deltaProcTime)/float(deltaCPUTime)*100.0
                        
                    else:
                        cpupct=0
                    
                    if cpupct < 0:
                        cpupct=0
                    
                    cpupct=float(int(float(cpupct)*100))/100  #only keep two decimal places
                    memVal=float(int(float(proc.get_memory_percent())*100.0))/100.0 if config.psutilVer == 1 else float(int(float(proc.memory_percent())*100.0))/100.0
                    
                    try:
                        #don't update dictionaries if name gives an access denied error when checking process name
                        #print "Updating"
                        pname=proc.name if config.psutilVer == 1 else proc.name()
                        d_user.update({proc.pid:proc.username}) if config.psutilVer == 1 else d_user.update({proc.pid:proc.username()})
                        d_cpu.update({proc.pid:cpupct})
                        d_mem.update({proc.pid:memVal})
                        d_name.update({proc.pid:pname})
                        d_cpuTimes.update({proc.pid:cputime})
                        d_procTimes.update({proc.pid:proctime})
    
                    except:
                        #print "psutil General Error: ",sys.exc_info()[0]
                        pass
                    
                except:
                    #else process did not previously exist and we cannot give an update this iteration
                    #print "except - pid: ",proc.pid

                    pass
                
           
        #once the dictionarys are updated, update cpu times for next loop
        self.ui.d_cpuTimes=d_cpuTimes
        self.ui.d_procTimes=d_procTimes
        
        #now fill the table for display
        for proc in d_user.keys():
            #print "proc: ",proc," type: ",type(proc)
            pid=int(proc)
            #print "pid: ",pid

            table.insertRow(0)
            #print "inserting row"
            #set process id
            item=QtGui.QTableWidgetItem()
            item.setData(QtCore.Qt.DisplayRole,pid) 
            table.setItem(0,0,item)
            #set username
            #print " d_user[proc]: ",d_user[proc],"  proc: ",proc
            table.setItem(0,1,QtGui.QTableWidgetItem(d_user[proc]))
            #set CPU %
            item=QtGui.QTableWidgetItem()
            item.setData(QtCore.Qt.DisplayRole,d_cpu[proc])
            table.setItem(0,2,item)
            #set memory %
            item=QtGui.QTableWidgetItem()
            item.setData(QtCore.Qt.DisplayRole,d_mem[proc])
            table.setItem(0,3,item)
            #set process name
            table.setItem(0,4,QtGui.QTableWidgetItem(d_name[proc]))
            row+=1

            
     #   for row in range(rout):
     #       table.removeRow(Npids-row)
        #restore sort to previously selected column
        #table.sortItems(column_sorted,order=QtCore.Qt.AscendingOrder)
        table.sortItems(column_sorted,order=order)
        self.ui.labelLastUpdate.setText("Last Update: "+str(datetime.datetime.now()))

def updateUserChart(self,config):
    
    font = {'family' : 'sans-serif',
        'weight' : 'bold',
        'size'   : config.pltFont}

    matplotlib.rc('font', **font)
    
    f=plt.figure(self.ui.figure2.number)

#    self.ui.figure2=plt.figure(2)
    plt.clf()
    plt.cla()
#    f.gca().cla()
    plt.subplot(121) #divide plot area into two: plot on left and legend on right
    #create empty dictionaries to be used by the process table
    d_user={}
    d_cpu={}
    d_mem={}
    d_name={}
    d_cpuTimes={}
    d_procTimes={}
    
    try:
        #check if this dictionary exists or not
        self.ui.d_procTimes
        self.ui.d_cpuTimes
        #if so, move on
        pass
    except:
        #case to initialize the dictionary
        for proc in psutil.process_iter():
            try:
                #check if acess denied 
                pname=proc.name
                proctime=proc.get_cpu_times() #get 
                cputime=psutil.cpu_times()
                d_procTimes.update({proc.pid:proctime})
                d_cpuTimes.update({proc.pid:cputime})
            except:
                #case we skip a process
                pass
        self.ui.d_cpuTimes=d_cpuTimes
        self.ui.d_procTimes=d_procTimes
        d_cpuTimes={}
        d_procTimes={}

    updateInterval=float(self.update) #timer interval in seconds
    totcpupct=0
    for proc in psutil.process_iter():
        try:
            psutil.Process(proc.pid).is_running()
        #if proc.pid == 37196:    
            #check if process still exists, if so, update dictionaries
            try:
                #check if process previously existed - if so we can calculate a cpupct
                proctimeHold=self.ui.d_procTimes[proc.pid]                    
                proctime=proc.get_cpu_times() #get 
                deltaProcTime=(proctime.user+proctime.system) - (proctimeHold.user+proctimeHold.system)                    
                
                cputimeHold=self.ui.d_cpuTimes[proc.pid]
                cputime=psutil.cpu_times()
                deltaCPUTime=(cputime.user+cputime.system+cputime.idle) - (cputimeHold.user+cputimeHold.system+cputimeHold.idle)
                
                if deltaProcTime > 0:
                    if deltaCPUTime < updateInterval:
                        deltaCPUTime=updateInterval
                    else:
                        pass
                    cpupct=float(deltaProcTime)/float(deltaCPUTime)*100.0
                    
                else:
                    cpupct=0
                
                if cpupct < 0:
                    cpupct=0
                
                cpupct=float(int(float(cpupct)*100))/100  #only keep two decimal places
                totcpupct+=cpupct
                memVal=float(int(float(proc.get_memory_percent())*100.0))/100.0 if config.psutilVer == 1 else float(int(float(proc.memory_percent())*100.0))/100.0
                
                try:
                    #don't update dictionaries if name gives an access denied error when checking process name
                    #print "Updating"
                    pname=proc.name if config.psutilVer == 1 else proc.name()
                    d_user.update({proc.pid:proc.username}) if config.psutilVer == 1 else d_user.update({proc.pid:proc.username()})
                    #System Idle process should not be listed in users cpu totals so set it to zero
                    if pname =="System Idle Process":
                        cpupct=0
                    d_cpu.update({proc.pid:cpupct})
                    d_mem.update({proc.pid:memVal})
                    d_name.update({proc.pid:pname})
                    d_cpuTimes.update({proc.pid:cputime})
                    d_procTimes.update({proc.pid:proctime})

                except:
                    #print "psutil General Error: ",sys.exc_info()[0]
                    pass
                
            except:
                #else process did not previously exist and we cannot give an update this iteration
                #print "except - pid: ",proc.pid

                pass
        except:
            #process no longer exists - do nothing
            pass
            
                
    self.ui.d_cpuTimes=d_cpuTimes
    self.ui.d_procTimes=d_procTimes
      
    #print "** Total Mem Used: ",sum(d_mem.values())
    users=d_user.values()
    users_unique=list(set(users)) #use set() to find unique users then convert the resulting set to a list via list()
    Nusers=len(users_unique)

    #create cpu and memory by users dictionaries
    cpu_by_users={}
    mem_by_users={}
    for u in range(Nusers):
        cpu_by_users.update({users_unique[u]:0})
        mem_by_users.update({users_unique[u]:0})
    #apparently update does not order keys in sequence to users_unique
    #thus need to re-create users_unique according to the order of the users
    #in the cpu and mem dictionary keys
    users_unique=list(cpu_by_users.keys())

    #fill cpu and memory dictionaries sequencing thru each PID
    for pid in d_user.keys():
        user=d_user[pid]
        cpu_by_users[user]=cpu_by_users[user] + d_cpu[pid]
        mem_by_users[user]=mem_by_users[user] + d_mem[pid]
    #print d_cpu[35296],d_cpu[37196],d_cpu[35296]+d_cpu[37196]
    #now convert to a list which we can index
    cpu_by_users_lst=cpu_by_users.values()
    mem_by_users_lst=mem_by_users.values()
        
    width=0.85

    colors=['b','g','r','c','m','y','gray','hotpink','brown','k']
    Nmax=len(colors) #don't want to have more users than colors...

    if self.ui.radioButtonCPU.isChecked():
        sortBy='cpu'
    elif self.ui.radioButtonMem.isChecked():
        sortBy='mem'
    elif self.ui.radioButtonMax.isChecked():
        sortBy='max'
    else:
        print "invalid radio button selection - using CPU sort as default"
        sortBy='cpu'
    #sortBy='cpu' # 'cpu' or 'mem' - use for debugging
    #create sort index
    if sortBy=='cpu':
        indx=sorted(range(len(cpu_by_users_lst)), key=cpu_by_users_lst.__getitem__,reverse=True)
    elif sortBy=='mem':
        indx=sorted(range(len(mem_by_users_lst)), key=mem_by_users_lst.__getitem__,reverse=True)
    elif sortBy=='max':
        #determine if cpu or mem is larger
        if sum(cpu_by_users_lst) > sum(mem_by_users_lst):
            #case where cpu usage is larger value
            indx=sorted(range(len(cpu_by_users_lst)), key=cpu_by_users_lst.__getitem__,reverse=True)
        else:
            #case where mem usage is larger
            indx=sorted(range(len(mem_by_users_lst)), key=mem_by_users_lst.__getitem__,reverse=True)
    else:
        print 'Incorrect sort parameter'
        
    #sort lists
    cpu_by_users_sorted=[cpu_by_users_lst[x] for x in indx]
    mem_by_users_sorted=[mem_by_users_lst[x] for x in indx]
    users_unique_sorted=[users_unique[x] for x in indx]

    #restrict the number of users we'll show to Nmax
    if Nusers > Nmax:
        #replace the Nmaxth - 1 element with the total of the values from index Nmax - 1 to the end of the list
        cpu_by_users_sorted[Nmax-1]=sum(cpu_by_users_sorted[Nmax-1:])
        mem_by_users_sorted[Nmax-1]=sum(mem_by_users_sorted[Nmax-1:])
        users_unique_sorted[Nmax-1]='Remaining'
        Nshow=Nmax
    else:
        Nshow=Nusers

    if min(cpu_by_users_sorted) < 0:
        print " *** cpu_by_users_sorted has values less than zero - should not occur, please check"
        print cpu_by_users_sorted
        print " ***"
    if min(mem_by_users_sorted) < 0:
        print " *** mem_by_users_sorted has values less than zero - should not occur, please check"
        print mem_by_users_sorted
        print " ***"        

    #range check the values of the sorted lists - may not be necessary, just being cautious...
    tst=np.array(cpu_by_users_sorted)<0  #need an array for summing
    indx=list(np.array(cpu_by_users_sorted)<0)           #need bool list for indexing
    #check if any users have less than zero CPU usage and set usage to 0 for these users
    if sum(tst) > 0:
        print "cpu_by_users < 0: ",sum(indx)
        cpu_by_users_sorted=[0 if x<0 else x for x in cpu_by_users_sorted]
    tst=np.array(cpu_by_users_sorted)>(self.ui.Ncpus*100)
    indx=list(np.array(cpu_by_users_sorted)>self.ui.Ncpus*100)
    #check if any users have CPU usage greater than possible number of CPUs and set usage to max CPU usage for those users
    if sum(tst) > 0:
        print "cpu_by_users > Ncpus*100: ",sum(indx)
        cpu_by_users_sorted=[self.ui.Ncpus*100 if x>self.ui.Ncpus*100 else x for x in cpu_by_users_sorted]
    tst=np.array(mem_by_users_sorted)<0
    indx=list(np.array(mem_by_users_sorted)<0)
    #check if any users have less than zero memory usage and set these users to zero usage
    if sum(tst) > 0:
        print "mem_by_users < 0: ",sum(indx)
        mem_by_users_sorted=[0 if x<0 else x for x in mem_by_users_sorted]
    tst=np.array(mem_by_users_sorted)>self.ui.totalmem
    indx=np.array(mem_by_users_sorted)>self.ui.totalmem
    #check if any users have memory usage greater than the total system memory - should never happen...
    if sum(tst) > 0:
        #if true, then need to adjust maximum usage for these users to the total memory possible
        #print "mem_by_users > totalmem: ",sum(indx),"  indx: ",indx,"  mem_by_users: ",mem_by_users_sorted
        mem_by_users_sorted=[self.ui.totalmem if x>self.ui.totalmem else x for x in mem_by_users_sorted]
    
    
    p=[] #list to contain plot objects for use by the legend   
    ind=np.arange(2)
    #print "**************"
    #print mem_by_users_sorted[0:Nshow]
    for u in range(Nshow):
        if u == 0:
            p.append(plt.bar(ind,(cpu_by_users_sorted[u],mem_by_users_sorted[u]),width,bottom=(0,0),color=colors[u]))
        else:
            p.append(plt.bar(ind,(cpu_by_users_sorted[u],mem_by_users_sorted[u]),width,bottom=(sum(cpu_by_users_sorted[0:u]),sum(mem_by_users_sorted[0:u])),color=colors[u]))
    
    plt.title('Usage by User',fontsize=config.pltFont+1,fontweight='bold')
    
    #remove default yaxis ticks then redraw them via ax1 and ax2 below
    frame=plt.gca()
    frame.axes.get_yaxis().set_ticks([])
    plt.xticks(np.arange(2)+width/2.,('CPU','Mem'),fontsize=config.pltFont,fontweight='bold')
    ymaxCPU=int(round(sum(cpu_by_users_sorted)+10))/10*10    #range ymaxCPU to nearest 10%
    ymaxMEM=int(round(sum(mem_by_users_sorted)+10))/10*10  #range ymaxMEM to nearest 10%
    
    ymaxMAX=max([ymaxCPU,ymaxMEM])

    if sortBy == 'cpu':
        ymax=max([ymaxCPU,10])
        auto=False
    elif sortBy == 'mem':
        ymax=max([ymaxMEM,10])
        auto=False
    elif sortBy == 'max':
        ymax=max([ymaxMAX,10])
        auto=True
    #print 'ymaxCPU: ',ymaxCPU,'  ymaxMEM: ',ymaxMEM,'  ymaxMAX: ',ymaxMAX,'  ymax: ',ymax
    #print 'sum(cpu_by_users_sorted): ',sum(cpu_by_users_sorted),'sum(mem_by_users_sorted): ',sum(mem_by_users_sorted)
    #print cpu_by_users
    plt.ylim(0,ymax,auto=True)
    

    
    #compute composite %
    sumCPU=sum(cpu_by_users_sorted)
    sumCPU=float(int(sumCPU*100))/100 #use two digits
    ylab=np.arange(5)/4.0*float(sumCPU)#/float(self.ui.Ncpus)
    scl=float(ymax)/float(sumCPU)
    ylab=ylab*100*scl
    tmp=ylab.astype('int')
    tmp1=tmp.astype('float')
    tmp1=tmp1/100
    ylab1=np.round(tmp1)
    
    ax1=plt.twinx()
    ax1.set_ylabel('Composite CPU Percent',fontsize=config.pltFont,fontweight='bold')
    ax1.yaxis.set_ticks_position('left')
    ax1.yaxis.set_label_position('left')
    ax1.set_ybound(lower=0,upper=max(ylab1))
    ax1.set_yticks(ylab1)
    
#    print 'ylab1: ',ylab1
    
    #place warnings if MEM or CPU go out of range
    if ymax < ymaxCPU:
        plt.text(2.7,0.0,'CPU Above Axis',color='red')
    if ymax < ymaxMEM:
        plt.text(2.7,0.0,'MEM Above Axis',color='green')    
        
    usersLegend=users_unique_sorted[0:Nshow]
    #reverse legend print order to have legend correspond with the order data are placed in the bar chart
    usersLegend.reverse()
    p.reverse()
    #place legend outside of plot to the right
    if not re.match('1.[0-1]',matplotlib.__version__):
        #if not an old version of matplotlib, then use the following command 
        plt.legend(p,usersLegend,bbox_to_anchor=(1.45, 1.1), loc="upper left", borderaxespad=0.1,fontsize=config.pltFont-0.5,title='Users')
    else:
        plt.legend(p,usersLegend,bbox_to_anchor=(1.45, 1.1), loc="upper left", borderaxespad=0.1,title='Users')
        
    #place second y axis label on plot
    ylab2=np.arange(5)/4.0*float(ymax)
    ax2=plt.twinx()
    ax2.set_ylabel('Memory Percent',fontsize=config.pltFont,fontweight='bold',position=(0.9,0.5))
    ax2.set_yticks(ylab2)
    #ax2.set_yticks(ylab2)
    ax2.yaxis.set_ticks_position('right')
    ax2.yaxis.set_label_position('right')

    self.ui.canvas2.draw()
    
def reLayout(self):
    tmpWidget=QtGui.QWidget()
    tmpWidget.setLayout(self.ui.layout2)
    tmpLayout = QtGui.QVBoxLayout(self.ui.frameBar)