
#global definitions for constants and variables

#Tab indices - based upon the order of the tabs as built via Qt and these must be the same as the Qt GUI.
SYST_TAB=0
HIST_TAB=1
PROC_TAB=2
USER_TAB=3
OPTS_TAB=4

#Global Variables:
psutilVer=0      #flag for the version of psutil being used
nompl=False      #Flag to indicate if using matplotlib - True, then do not use plots, False, then try to use matplotlib plots
mplLoaded=False  #flag for matplotlib loading
custom=False     #flag to indicate if the custom interface is to be used (usually not)
matlabchk='lmstat -S -c 27010@licenses1.sns.gov'   #text command to status the Matlab license server (only for custom mode with SysMon.pyw)
basefontsize=9   #basic font size when GUI is at minimum size
fscl=0.5         #scale factor for font size change with GUI resizing - less than one, font size slower than GUI size change, and greater than one font size faster than GUI size change
pltFont=9        #initial font size for matplotlib plots
linewidth=1      #initial line widths for matplotlib plots