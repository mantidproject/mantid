""" This is a class that has not been used at all. """


class MultiScanTabState(object):
    """ Description of the state of the multi-scan-tab is in
    """
    NO_OPERATION = 0
    RELOAD_DATA = 1
    REDUCE_DATA = 2

    def __init__(self):
        """ Initialization
        :return:
        """
        self._expNo = -1
        self._scanList = []
        self._xMin = None
        self._xMax = None
        self._binSize = 0
        self._unit = ''
        self._plotRaw = False
        self._useDetEfficiencyCorrection = False
        self._excludeDetectors = []

    def compare_state(self, tab_state):
        """ Compare this tab state and another tab state
        :param tab_state:
        :return:
        """
        if isinstance(tab_state, MultiScanTabState) is False:
            raise NotImplementedError('compare_state must have MultiScanTabStatus as input.')

        if self._expNo != tab_state.getExpNumber() or self._scanList != tab_state.getScanList:
            return self.RELOAD_DATA

        for attname in self.__dict__.keys():
            if self.__getattribute__(attname) != tab_state.__getattribute__(attname):
                return self.REDUCE_DATA

        return self.NO_OPERATION

    def getExpNumber(self):
        """ Get experiment number
        :return:
        """
        return self._expNo

    def getScanList(self):
        """ Get the list of scans
        :return:
        """
        return self._scanList[:]

    #pyline: disable=too-many-arguments
    def setup(self, exp_no, scan_list, min_x, max_x, bin_size, unit, raw, correct_det_eff, exclude_dets):
        """
        Set up the object
        :param exp_no:
        :param scan_list:
        :param min_x:
        :param max_x:
        :param bin_size:
        :param unit:
        :param raw:
        :param correct_det_eff:
        :param exclude_dets:
        :return:
        """
        self._expNo = int(exp_no)
        if isinstance(scan_list, list) is False:
            raise NotImplementedError('Scan_List must be list!')
        self._scanList = scan_list
        self._xMin = min_x
        self._xMax = max_x
        self._binSize = float(bin_size)
        self._unit = str(unit)
        self._plotRaw = raw
        self._useDetEfficiencyCorrection = correct_det_eff
        self._excludeDetectors = exclude_dets

        return

