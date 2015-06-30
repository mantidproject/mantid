################################################################################
#
# Controlling class 
#
################################################################################

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

        return


    def setPeak(self, peakws, rowindex):
        """ Set peak information from a peak workspace
        """


        raise NotImplementedError("ASAP")



class CWSCDReductionControl(object):
    """ Controlling class for reactor-based single crystal diffraction reduction
    """
    def __init__(self):
        """ init
        """

        return


    def addPeakToCalUB(self, peakinfo):
        """ Add a peak to calculate ub matrix
        """


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

        return urlValid(self._myServerURL)


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
        self._localCacheDir = localdir

        return os.path.exists(self._localCacheDir)


    def setExpNumber(self, expno):
        """ Set experiment number
        """
        self._myExpNumber = expno

        return
