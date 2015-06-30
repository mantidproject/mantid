################################################################################
#
# Controlling class 
#
################################################################################
import os
import sys
import urllib2

scriptdir = os.getcwd()
sourcedir = scriptdir.split('scripts')[0]
mantiddir = os.path.join( os.path.join(sourcedir, os.pardir), 'debug/bin')
if os.path.exists(mantiddir) is False: 
    raise RuntimeError('Mantid bin directory %s cannot be found.' % (mantiddir))
else:
    sys.path.append(mantiddir)

import mantid
import mantid.simpleapi as api

class PeakInfo(object):
    """ Class containing a peak's information for GUI
    """
    def __init__(self):
        """ Init
        """
        # Flag for data storage mode
        self._cacheDataOnly = False
        # Data server's URL
        self._myServerURL = '' 
        # Local data cache directory
        self._localCacheDir = ''
        # Experiment number to process
        self._myExpNumber = -1
        # Flag to delete cache dir 
        self._rmdirFlag = False

        return


    def setPeak(self, peakws, rowindex):
        """ Set peak information from a peak workspace
        """


        raise NotImplementedError("ASAP")



class CWSCDReductionControl(object):
    """ Controlling class for reactor-based single crystal diffraction reduction
    """
    def __init__(self, instrumentname=None):
        """ init
        """
        if isinstance(instrumentname, str):
            self._instrumentName = instrumentname
        elif instrumentname is None:
            self._instrumentName = ''
        else:
            raise RuntimeError('Instrument name %s of type %s is not allowed.' % (str(instrumentname),
                str(type(instrumentname))))

        # Experiment number
        self._expNumber = 0
        self._dataDir = "/tmp"

        return


    def addPeakToCalUB(self, peakinfo):
        """ Add a peak to calculate ub matrix
        """


        return


    def downloadData(self, scanno, ptno):
        """ Download a scan/pt data from internet
        """
        # Generate the URL for SPICE data file
        if self._myServerURL.endswith('/') is False:
            self._myServerURL + '/'
        spicerunfileurl = self._myServerURL + 'exp%d/Datafiles/'%(self._expNumber) + \
                "HB3A_exp%04d_scan%04d.dat" % (self._expNumber, scanno)

        spicefilename = '%s_exp%04d_scan%04d.dat'%(self._instrumentName, self._expNumber, 
                scanno)
        spicefilename = os.path.join(self._dataDir, spicefilename)

        # Download
        try:
            api.DownloadFile(Address=spicerunfileurl, Filename=spicefilename)
        except Exception as e:
            return (False, str(e))

        # Check file exist?
        if os.path.exists(spicefilename) is False:
            return (False, "Unable to locate downloaded file %s."%(spicefilename))

        # Generate the URL for XML file

        # TODO - Implement downloading XML file 
        # Example: HB3A_exp399_scan0134_0082.xml

        return (True, '')
        


    def existDataFile(self, scanno, ptno):
        """ Check whether data file for a scan or pt number exists
        """
        # Check spice file 
        spicefilename = '%s_exp%04d_scan%04d.dat'%(self._instrumentName, self._expNumber, 
                scanno)
        spicefilename = os.path.join(self._dataDir, spicefilename)
        if os.path.exists(spicefilename) is False:
            return (False, "Spice data file %s cannot be found."%(spicefilename))

        # Check xml file
        xmlfilename = '%s_exp%d_scan%04d_%04d.xml'%(self._instrumentName, self._expNumber,
                scanno, ptno)
        xmlflename = os.path.join(self._dataDir, xmlfilename)
        if os.path.exists(xmlfilename) is False:
            return (False, "Pt. XML file %s cannot be found."%(xmlfilename))

        return (True, "")


    def findPeak(self, scanno, ptno):
        """ Find peak in sample Q space

        Return :: tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)
        """
        # Check existence
        runnumber = self._checkExist(scanno, ptno)
        if runnumber is None:
            return (False, "Unable to locate scan %s/pt %s" % (str(scanno), str(ptno)))

        # 
        mdwksp = slef._getMDWorkspace(runnumber)
        
        peakws = api.FindPeaksMD(InputWorkspace=mdwksp)

        # Get number of peaks
        numpeaks = peakws.rowCount()
        if numpeaks == 0:
            return (False, "Unable to find peak in scan %d/pt %d" % (scanno, ptno))
        elif numpeaks >= 2:
            raise NotImplementedError("It is not implemented for finding more than 1 peak.")

        # Ony case: number of peaks is equal to 1 
        self._storePeakWorkspace(peakws) 
        peakinfo = PeakInfo(peakws, 0) 

        return (True, peakinfo)


    def setServerURL(self, serverurl):
        """ Set server's URL
        """
        self._myServerURL = str(serverurl)

        urlgood = False
        try:
            urllib2.urlopen(self._myServerURL)
        except urllib2.HTTPError, e:
            print(e.code) 
        except urllib2.URLError, e: 
            print(e.args)
        else:
            urlgood = True

        return urlgood


    def setWebAccessMode(self, mode):
        """ Set data access mode form server
        """
        if isinstance(mode, str) is False:
            raise RuntimeError('Input mode is not string')

        if mode == 'cache':
            self._cacheDataOnly = True
        elif mode == 'download':
            self._cacheDataOnly = False

        return


    def setLocalCache(self, localdir):
        """ Set local cache diretory
        """
        # TODO Doc : ..., localdir is ...

        # Get absolute path 
        if os.path.isabs(localdir) is True: 
            self._localCacheDir = localdir
        else:
           cwd = os.getcwd() 
           self._localCacheDir = os.path.join(cwd, localdir)

        # Create cache directory
        if os.path.exists(self._localCacheDir) is False:
            os.mkdir(self._localCacheDir)
            self._rmdirFlag = True

        # Synchronize with data directory
        self._dataDir = self._localCacheDir

        return os.path.exists(self._localCacheDir)


    def setExpNumber(self, expno):
        """ Set experiment number
        """
        self._expNumber = expno

        return
