################################################################################
#
# Controlling class
#
# == Data download and storage ==
# - Local data storage (local-mode)
# - Download from internet to cache (download-mode)
#
################################################################################
import os
import sys
import urllib2

import numpy

try:
    import mantid
except ImportError:
    # In case Mantid is not in the python path
    homedir = os.path.expanduser('~')
    mantiddir = os.path.join(os.path.join(homedir, 'Mantid/Code/debug/bin'))
    print 'Mantid Dir = %s' % mantiddir
    if os.path.exists(mantiddir) is False:
        raise RuntimeError('Mantid bin directory %s cannot be found.' % (mantiddir))

    # import again
    sys.path.append(mantiddir)
    import mantid
import mantid.simpleapi as api
from mantid.api import AnalysisDataService

DebugMode = True

DET_X_SIZE = 256
DET_Y_SIZE = 256


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
        self._currentExpNumber = 0
        # Flag to delete cache dir
        self._rmdirFlag = False

        return

    def get_peaks(self):
        """
        Get peaks from
        :return:
        """
        return None


    def set_peak(self, peakws, rowindex):
        """ Set peak information from a peak workspace
        """
        raise NotImplementedError("ASAP")


class CWSCDReductionControl(object):
    """ Controlling class for reactor-based single crystal diffraction reduction
    """
    def __init__(self, instrument_name=None):
        """ init
        """
        if isinstance(instrument_name, str):
            self._instrumentName = instrument_name
        elif instrument_name is None:
            self._instrumentName = ''
        else:
            raise RuntimeError('Instrument name %s of type %s is not allowed.' % (str(instrument_name),
                                                                                  str(type(instrument_name))))

        # Experiment number, data storage
        # No Use/Confusing: self._expNumber = 0

        self._dataDir = None
        self._workDir = '/tmp'

        self._myServerURL = ''

        # Some set up
        self._expNumber = None

        # Container for loaded workspaces
        self._spiceTableDict = {}
        # Container for loaded raw pt workspace
        self._rawDataDict = {}
        # A dictionary to manage all loaded and processed MDEventWorkspaces
        self._expDataDict = {}

        return

    def addPeakToCalUB(self, peakws, ipeak, matrixws):
        """ Add a peak to calculate ub matrix
        :param peakws:
        :param ipeak:
        :param matrixws:
        :return:
        """
        # Get HKL
        try:
            m_h = float(int(matrixws.run().getProperty('_h').value))
            m_k = float(int(matrixws.run().getProperty('_k').value))
            m_l = float(int(matrixws.run().getProperty('_l').value))
        except ValueError:
            return (False, "Unable to locate _h, _k or _l in input matrix workspace.")

        new_peak = peakws.getPeak(ipeak)
        new_peak.setHKL(m_h, m_k, m_l)

        return

    def calculateUBMatrix(self, peakws, a, b, c, alpha, beta, gamma):
        """
        Calculate UB matrix
        :param peakws:
        :param a:
        :param b:
        :param c:
        :param alpha:
        :param beta:
        :param gamma:
        :return:
        """
        api.CalculateUMatrix(PeaksWorkspace=peakws,
                a=a,b=b,c=c,alpha=alpha,beta=beta,gamma=gamma)

        ubmatrix = peakws.sample().getOrientedLattice().getUB()
        print ubmatrix

        return

    def does_raw_loaded(self, exp_no, scan_no, pt_no):
        """
        Check whether the raw Workspace2D for a Pt. exists
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :return:
        """
        return (exp_no, scan_no, pt_no) in self._rawDataDict

    def does_spice_loaded(self, exp_no, scan_no):
        """ Check whether a SPICE file has been loaded
        :param exp_no:
        :param scan_no:
        :return:
        """
        return (exp_no, scan_no) in self._spiceTableDict

    def download_spice_file(self, exp_number, scan_number, over_write):
        """
        Download a scan/pt data from internet
        :param exp_number: experiment number
        :param scan_number:
        :return:
        """
        # TODO - Implement parameter over_write
        # Generate the URL for SPICE data file
        file_url = '%sexp%d/Datafiles/HB3A_exp%04d_scan%04d.dat' % (self._myServerURL, exp_number,
                                                                    exp_number, scan_number)
        file_name = '%s_exp%04d_scan%04d.dat' % (self._instrumentName, exp_number, scan_number)
        file_name = os.path.join(self._dataDir, file_name)

        # Download
        try:
            api.DownloadFile(Address=file_url, Filename=file_name)
        except RuntimeError as e:
            return False, str(e)

        # Check file exist?
        if os.path.exists(file_name) is False:
            return False, "Unable to locate downloaded file %s." % file_name

        return True, file_name

    def download_spice_xml_file(self, scan_no, pt_no, exp_no=None, overwrite=False):
        """ Download a SPICE XML file for one measurement in a scan
        :param scan_no:
        :param pt_no:
        :param exp_no:
        :param overwrite:
        :return: tuple (boolean, local file name/error message)
        """
        # Experiment number
        if exp_no is None:
            exp_no = self._expNumber

        # Form the target file name and path
        xml_file_name = '%s_exp%d_scan%04d_%04d.xml' % (self._instrumentName, exp_no, scan_no, pt_no)
        local_xml_file_name = os.path.join(self._dataDir, xml_file_name)
        if os.path.exists(local_xml_file_name) is True and overwrite is True:
            return True, local_xml_file_name

        # Generate the URL for XML file
        xml_file_url = '%sexp%d/Datafiles/%s' % (self._myServerURL, exp_no, xml_file_name)

        # Download
        try:
            api.DownloadFile(Address=xml_file_url,
                             Filename=local_xml_file_name)
        except RuntimeError as e:
            return False, 'Unable to download Detector XML file %s dur to %s.' % (xml_file_name, str(e))

        # Check file exist?
        if os.path.exists(local_xml_file_name) is False:
            return False, "Unable to locate downloaded file %s."%(local_xml_file_name)

        return True, local_xml_file_name

    def download_data_set(self, scan_list):
        """
        Download data set including (1) spice file for a scan and (2) XML files for measurements
        :param scan_list:
        :return:
        """
        # Check
        if self._expNumber is None:
            raise RuntimeError('Experiment number is not set up for controller.')

        error_message = ''

        for scan_no in scan_list:
            # Download single spice file for a run
            status, ret_obj = self.download_spice_file(exp_number=self._expNumber,
                                                       scan_number=scan_no,
                                                       over_write=False)

            # Reject if SPICE file cannot download
            if status is False:
                error_message += '%s\n' % ret_obj
                continue

            # Load SPICE file to Mantid
            spice_file_name = ret_obj
            status, ret_obj = self.load_spice_scan_file(self._expNumber, scan_no, spice_file_name)
            if status is False:
                error_message = ret_obj
                return False, error_message
            else:
                spice_table = self._spiceTableDict[(self._expNumber, scan_no)]
                assert spice_table
            pt_no_list = self._getPtListFromTable(spice_table)

            # Download all single-measurement file
            for pt_no in pt_no_list:
                status, ret_obj = self.download_spice_xml_file(scan_no, pt_no)
                if status is False:
                    error_message += '%s\n' % ret_obj
            # END-FOR
        # END-FOR (scan_no)

        return True, error_message

    def existDataFile(self, scanno, ptno):
        """
        Check whether data file for a scan or pt number exists
        :param scanno:
        :param ptno:
        :return:
        """
        # Check spice file
        spice_file_name = '%s_exp%04d_scan%04d.dat'%(self._instrumentName,
                                                   self._expNumber, scanno)
        spice_file_name = os.path.join(self._dataDir, spice_file_name)
        if os.path.exists(spice_file_name) is False:
            return False, 'Spice data file %s cannot be found.'% spice_file_name

        # Check xml file
        xmlfilename = '%s_exp%d_scan%04d_%04d.xml'%(self._instrumentName, self._expNumber,
                scanno, ptno)
        xmlflename = os.path.join(self._dataDir, xmlfilename)
        if os.path.exists(xmlfilename) is False:
            return (False, "Pt. XML file %s cannot be found."%(xmlfilename))

        return True, ""

    def find_peak(self, exp_number, scan_number, pt_number):
        """ Find peak in sample Q space
        :param scan_number:
        :param pt_number:
        :return:tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)

        This part will be redo as 11847_Load_HB3A_Experiment
        """
        # Download data
        status_sp, spice_file_name = self.download_spice_file(exp_number, scan_number, over_write=False)
        status_det, det_file_name = self.download_spice_xml_file(scan_number, pt_number, exp_number, overwrite=False)
        if status_sp is False or status_det is False:
            err_msg = 'Unable to download SPICE file or detector file. Reason: %s; %s' % (
                spice_file_name, det_file_name)
            return False, err_msg

        # Load data
        status, error_message = self.load_raw_data_to_md(exp_number, scan_number, pt_number,
                                                         spice_file_name, det_file_name)
        if status is False:
            return status, error_message

        # TODO - From this step, refer to script 'load_peaks_md.py'
        mdwksp = self._getMDWorkspace(runnumber)

        peakws = api.FindPeaksMD(InputWorkspace=mdwksp)

        # Get number of peaks
        numpeaks = peakws.rowCount()
        if numpeaks == 0:
            return (False, "Unable to find peak in scan %d/pt %d" % (scan_number, pt_number))
        elif numpeaks >= 2:
            raise NotImplementedError("It is not implemented for finding more than 1 peak.")

        # Ony case: number of peaks is equal to 1
        self._storePeakWorkspace(peakws)
        peakinfo = PeakInfo(peakws, 0)

        # Result shall be popped to table tableWidget_peaksCalUB in GUI

        return True, peakinfo

    def get_pt_numbers(self, exp_no, scan_no, load_spice_scan=False):
        """ Get Pt numbers (as a list) for a scan in an experiment
        :param exp_no:
        :param scan_no:
        :param load_spice_scan:
        :return:
        """
        print '[DB] Get Pt. for Exp %d Scan %d' % (exp_no, scan_no)
        table_ws = self._get_spice_workspace(exp_no, scan_no)
        if table_ws is None:
            if load_spice_scan is False:
                return False, 'Spice file for Exp %d Scan %d is not loaded.' % (exp_no, scan_no)
            else:
                status, error_message = self.load_spice_scan_file(exp_no, scan_no)
                if status is True:
                    table_ws = self._get_spice_workspace(exp_no, scan_no)
                    if table_ws is None:
                        raise NotImplementedError('Logic error! Cannot happen!')
                else:
                    return False, 'Unable to load Spice file for Exp %d Scan %d due to %s.' % (
                        exp_no, scan_no, error_message)

        col_name_list = table_ws.getColumnNames()
        i_pt = col_name_list.index('Pt.')
        if i_pt < 0 or i_pt >= len(col_name_list):
            return False, 'No column with name Pt. can be found in SPICE table.'

        pt_number_list = []
        num_rows = table_ws.rowCount()
        for i in xrange(num_rows):
            pt_number = table_ws.cell(i, i_pt)
            pt_number_list.append(pt_number)

        return True, pt_number_list

    def get_raw_detector_counts(self, exp_no, scan_no, pt_no):
        """
        Get counts on raw detector
        :param scan_no:
        :param pt_no:
        :return: boolean, 2D numpy data
        """
        # Get workspace (in memory or loading)
        raw_ws = self._get_raw_workspace(exp_no, scan_no, pt_no)
        if raw_ws is None:
            return False, 'Raw data for Exp %d Scan %d Pt %d is not loaded.' % (exp_no, scan_no, pt_no)

        # Convert to numpy array
        array2d = numpy.ndarray(shape=(DET_X_SIZE, DET_Y_SIZE), dtype='float')
        for i in xrange(DET_X_SIZE):
            for j in xrange(DET_Y_SIZE):
                array2d[i][j] = raw_ws.readY(i * DET_X_SIZE + j)[0]

        return array2d

    def indexPeaks(self, srcpeakws, targetpeakws):
        """ Index peaks in a peak workspace by UB matrix
        :param srcpeakws: peak workspace containing UB matrix
        :param targetpeakws: peak workspace to get peaks indexed
        :return: (Boolean, Object) - Boolean = True,  Object = ??? - Boolean = False, Object = error message (str)
        """
        # Check input
        if isinstance(peakws, PeakWorkspace) is False:
            return False, "Input workspace %s is not a PeakWorkspace" % (str(peakws))

        # Get UB matrix
        if DebugMode is True:
            ubmatrix = peakws.sample().getOrientedLattice().getUB()
            print "[DB] UB matrix is %s\n" % (str(ubmatrix))

        # Copy sample/ub matrix to target
        api.CopySample(InputWorkspace=srcpeakws,
                       OutputWorkspace=str(targetpeakws),
                       CopyName=0, CopyMaterial=0, CopyEnvironment=0,
                       CopyShape=0,CopyLattice=1,CopyOrientationOnly=0)

        numindexed = CalculatePeaksHKL(PeaksWorkspace=targetpeakws, Overwrite=True)

        print "[INFO] There are %d peaks that are indexed." % (numindexed)

        return True, ""

    def load_spice_scan_file(self, exp_no, scan_no=None, spice_file_name=None):
        """
        Load a SPICE scan file to table workspace and run information matrix workspace.
        :param scan_no:
        :param spice_file_name:
        :return: status (boolean), error message (string)
        """
        # Check whether the workspace has been loaded
        assert(isinstance(exp_no, int))
        assert(isinstance(scan_no, int))
        out_ws_name = get_spice_table_name(exp_no, scan_no)
        if (exp_no, scan_no) in self._spiceTableDict:
            return True, out_ws_name

        # Form standard name for a SPICE file if name is not given
        if spice_file_name is None:
            spice_file_name = os.path.join(self._dataDir, get_spice_file_name(exp_no, scan_no))

        # Load SPICE file
        try:
            spice_table_ws, info_matrix_ws = api.LoadSpiceAscii(Filename=spice_file_name,
                                                                OutputWorkspace=out_ws_name,
                                                                RunInfoWorkspace='TempInfo')
        except RuntimeError as e:
            return False, 'Unable to load SPICE data %s due to %s' % (spice_file_name, str(e))

        # Store
        self._add_spice_workspace(exp_no, scan_no, spice_table_ws)

        return True, out_ws_name

    def load_spice_xml_file(self, exp_no, scan_no, pt_no, xml_file_name=None):
        """
        Load SPICE's XML file to
        :param scan_no:
        :param pt_no:
        :return:
        """
        # Form XMIL file as ~/../HB3A_exp355_scan%04d_%04d.xml'%(scan_no, pt)
        if xml_file_name is None:
            xml_file_name = os.path.join(self._dataDir,
                                         'HB3A_exp%d_scan%04d_%04d.xml' % (exp_no, scan_no, pt_no))

        # Get spice table
        spice_table_ws = self._get_spice_workspace(exp_no, scan_no)
        assert isinstance(spice_table_ws, mantid.dataobjects._dataobjects.TableWorkspace)
        spice_table_name = spice_table_ws.name()

        # Load SPICE Pt. file
        # spice_table_name = 'Table_Exp%d_Scan%04d' % (exp_no, scan_no)
        pt_ws_name = get_raw_data_workspace_name(exp_no, scan_no, pt_no)
        try:
            ret_obj = api.LoadSpiceXML2DDet(Filename=xml_file_name,
                                            OutputWorkspace=pt_ws_name,
                                            DetectorGeometry='256,256',
                                            SpiceTableWorkspace=spice_table_name,
                                            PtNumber=pt_no)
        except RuntimeError as e:
            return False, str(e)

        pt_ws = ret_obj

        # Add data storage
        self._add_raw_workspace(exp_no, scan_no, pt_no, pt_ws)

        return True, pt_ws_name

    def load_raw_data_to_md(self, exp_number, scan_number, pt_number,
                            spice_file_name, det_file_name):
        """ To rebin and set goniometer
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        # FIXME - Need a method to standard this
        # spice_table_name = 'HB3A_%03d_%04d_Raw' % (exp_number, scan_number)

        # Load raw
        if (exp_number, scan_number) not in self._spiceTableDict:
            status, err_msg = self.load_spice_scan_file(exp_number, scan_number, spice_file_name)
            if status is False:
                return False, err_msg

        if (exp_number, scan_number, pt_number) not in self._expDataDict:
            status, ret_obj = self.load_spice_xml_file(exp_number, scan_number, pt_number, det_file_name)
            if status is False:
                err_msg = ret_obj
                return False, err_msg

        pt_ws = self._get_raw_workspace(exp_number, scan_number, pt_number)
        pt_ws_name = pt_ws.name()

        # Rebin and set goniometer
        api.Rebin(InputWorkspace=pt_ws_name, OutputWorkspace=pt_ws_name,
                  Params='6,0.01,6.5')
        api.SetGoniometer(Workspace=pt_ws_name,
                          Axis0='_omega,0,1,0,-1', Axis1='_chi,0,0,1,-1', Axis2='_phi,0,1,0,-1')
        self._add_raw_workspace(exp_number, scan_number, pt_number, pt_ws_name)

        return True, ''

    def process_pt_wksp(self, scan_no, pt_no):
        """

        :param scan_no:
        :param pt_no:
        :return:
        """
        # Rebin in momentum space: UB matrix only
        try:
            ret_obj = api.Rebin(InputWorkspace='s%04d_%04d'%(scan_no, pt_no),
                                OutputWorkspace='s%04d_%04d'%(scan_no, pt_no),
                                Params='6,0.01,6.5')
        except RuntimeError as err:
            return False, str(err)

        # Set Goniometer
        try:
            ret_obj = api.SetGoniometer(Workspace='s%04d_%04d'%(scan_no, pt_no),
                              Axis0='_omega,0,1,0,-1',
                              Axis1='_chi,0,0,1,-1',
                              Axis2='_phi,0,1,0,-1')
        except RuntimeError as err:
            return False, str(err)

        processed_ws = ret_obj

        return True, processed_ws

    def import_data_to_mantid(self, exp_no, scan_no, pt_no):
        """

        :param exp_no:
        :param scan_no:
        :param pt_no:
        :return:
        """
        # Check existence of SPICE workspace
        if self._spiceTableDict.has_key(scan_no) is False:
            status, ret_obj = self.load_spice_scan_file(exp_no, scan_no)
            if status is False:
                return False, ret_obj

        # Check existence of matrix workspace
        if self._expDataDict[self._expNumber].has_key((scan_no, pt_no)) is False:
            self.load_spice_xml_file(exp_no, scan_no, pt_no)

        return True, ''

    def groupWS(self):
        """

        :return:
        """
        inputws = ''
        for pt in [11]:
            
            inputws += 's%04d_%04d'%(scanno, pt)
            if pt != numrows:
                inputws += ','
        # ENDFOR
        
        GroupWorkspaces(InputWorkspaces=inputws, OutputWorkspace='Group_exp0355_scan%04d'%(scanno))

    def set_server_url(self, server_url):
        """
        Set URL for server to download the data
        :param server_url:
        :return:
        """
        # Server URL must end with '/'
        self._myServerURL = str(server_url)
        if self._myServerURL.endswith('/') is False:
            self._myServerURL += '/'

        # Test URL valid or not
        is_url_good = False
        error_message = None
        try:
            result = urllib2.urlopen(self._myServerURL)
        except urllib2.HTTPError, err:
            error_message = str(err.code)
        except urllib2.URLError, err:
            error_message = str(err.args)
        else:
            is_url_good = True
            result.close()

        if error_message is None:
            error_message = ''
        else:
            error_message = 'Unable to open data server URL: %s due to %s.' % (server_url, error_message)

        return is_url_good, error_message

    def setWebAccessMode(self, mode):
        """
        Set data access mode form server
        :param mode:
        :return:
        """
        if isinstance(mode, str) is False:
            raise RuntimeError('Input mode is not string')

        if mode == 'cache':
            self._cacheDataOnly = True
        elif mode == 'download':
            self._cacheDataOnly = False

        return

    def set_local_data_dir(self, local_dir):
        """
        Set local data storage
        :param local_dir:
        :return:
        """
        # Get absolute path 
        if os.path.isabs(local_dir) is False:
            # Input is relative path to current working directory
            cwd = os.getcwd()
            local_dir = os.path.join(cwd, local_dir)

        # Create cache directory if necessary
        if os.path.exists(local_dir) is False:
            try:
                os.mkdir(local_dir)
            except OSError as e:
                return False, str(e)

        # Check whether the target is writable
        if os.access(local_dir, os.W_OK) is False:
            return False, 'Specified local data directory %s is not writable.' % local_dir

        # Successful
        self._dataDir = local_dir

        return True, ''

    def set_working_directory(self, work_dir):
        """
        Set up the directory for working result
        :return: (boolean, string)
        """
        if os.path.exists(work_dir) is False:
            try:
                os.mkdir(work_dir)
            except OSError as e:
                return False, 'Unable to create working directory %s due to %s.' % (work_dir, str(e))
        elif os.access(work_dir, os.W_OK) is False:
            return False, 'User specified working directory %s is not writable.' % work_dir

        return True, ''

    def set_instrument_name(self, instrument_name):
        """
        Set instrument name
        :param instrument_name:
        :return:
        """
        # Check
        if isinstance(instrument_name, str) is False:
            return False, 'Input instrument name is not a string but of type %s.' % str(type(instrument_name))
        if len(instrument_name) == 0:
            return False, 'Input instrument name is an empty string.'

        self._instrumentName = instrument_name

        return True, ''

    def set_exp_number(self, exp_number):
        """ Add experiment number
        :param exp_number:
        :return:
        """
        assert(isinstance(exp_number, int))
        self._expNumber = exp_number

        return True

    def _add_raw_workspace(self, exp_no, scan_no, pt_no, raw_ws):
        """ Add raw Pt.'s workspace
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param raw_ws: workspace or name of the workspace
        :return: None
        """
        # Check
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(pt_no, int)

        if isinstance(raw_ws, str):
            # Given by name
            matrix_ws = AnalysisDataService.retrieve(raw_ws)
        else:
            matrix_ws = raw_ws
        assert isinstance(matrix_ws, mantid.dataobjects._dataobjects.Workspace2D)

        self._rawDataDict[(exp_no, scan_no, pt_no)] = matrix_ws

        return

    def _add_md_workspace(self, exp_no, scan_no, pt_no, md_ws):
        """
        """

    def _add_spice_workspace(self, exp_no, scan_no, spice_table_ws):
        """
        """
        assert(isinstance(exp_no, int))
        assert(isinstance(scan_no, int))
        assert(isinstance(spice_table_ws, mantid.dataobjects._dataobjects.TableWorkspace))
        self._spiceTableDict[(exp_no, scan_no)] = spice_table_ws

        return

    def _get_spice_workspace(self, exp_no, scan_no):
        """ Get SPICE's scan table workspace
        :param exp_no:
        :param scan_no:
        :return: Table workspace or None
        """
        try:
            ws = self._spiceTableDict[(exp_no, scan_no)]
        except KeyError:
            print '[DB] Keys to SPICE TABLE: %s' % str(self._spiceTableDict.keys())
            return None

        return ws

    def _get_raw_workspace(self, exp_no, scan_no, pt_no):
        """
        """
        try:
            ws = self._rawDataDict[(exp_no, scan_no, pt_no)]
        except KeyError:
            return None

        return ws

    def _get_md_workspace(self, exp_no, scan_no, pt_no):
        """
        """

        return


    def _getPtListFromTable(self, spicetable):
        # TODO Doc
        """
        """
        numrows = spicetable.rowCount()
        ptlist = []
        for irow in xrange(numrows):
            ptno = int(spicetable.cell(irow, 0))
            ptlist.append(ptno)

        return ptlist


def get_spice_file_name(exp_number, scan_number):
    """

    :param exp_number:
    :param scan_num:
    :return:
    """
    file_name = 'HB3A_exp%04d_scan%04d.dat' % (exp_number, scan_number)

    return file_name


def get_spice_table_name(exp_number, scan_number):
    """ Form the name of the table workspace for SPICE
    :param exp_number:
    :param scan_number:
    :return:
    """
    table_name = 'HB3A_%03d_%04d_SpiceTable' % (exp_number, scan_number)

    return table_name


def get_raw_data_workspace_name(exp_number, scan_number, pt_number):
    """ Form the name of the matrix workspace to which raw pt. XML file is loaded
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'HB3A_%03d_%04d_%04d_Raw' % (exp_number, scan_number, pt_number)

    return ws_name