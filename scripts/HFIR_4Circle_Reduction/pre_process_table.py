import NTableWidget


class ScanPreProcessStatusTable(NTableWidget.NTableWidget):
    """
    Extended table widget for scans to process
    """
    TableSetup = [('Scan', 'int'),
                  ('Status', 'str'),
                  ('File', 'str'),
                  ('Note', 'str')]

    def __init__(self, parent):
        """
        Initialization
        :param parent::
        :return:
        """
        super(ScanPreProcessStatusTable, self).__init__(parent)

        # column index of k-index
        self._iColScanNumber = None
        self._iColStatus = None
        self._iColFile = None
        self._iColNote = None

        # a quick-reference list
        self._scanRowDict = dict()

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.TableSetup)

        # set columns' width
        self.setColumnWidth(0, 35)
        self.setColumnWidth(1, 60)
        self.setColumnWidth(2, 90)
        self.setColumnWidth(3, 120)

        # set the column index
        self._iColScanNumber = self.TableSetup.index(('Scan', 'int'))
        self._iColStatus = self.TableSetup.index(('Status', 'str'))
        self._iColFile = self.TableSetup.index(('File', 'str'))
        self._iColNote = self.TableSetup.index(('Note', 'str'))

        return

    def add_new_scans(self, scan_numbers, append):
        """
        add scans to the
        :param scan_numbers:
        :return:
        """
        # check input
        assert isinstance(scan_numbers, list), 'blabla'

        # sort
        scan_numbers.sort()
        part_dict = dict()

        # append to table
        for scan_number in scan_numbers:
            # skip the scan number that has been added to table
            if scan_number in self._scanRowDict:
                continue

            # append scan
            print '[DB...BAT] Append row for scan {0}'.format(scan_number)
            status, msg = self.append_row([scan_number, '', '', ''])
            if not status:
                raise RuntimeError('Failed to append a new row due to {0}'.format(msg))
            num_rows = self.rowCount()
            self._scanRowDict[scan_number] = num_rows - 1
            part_dict[scan_number] = num_rows - 1
        # END-FOR

        return part_dict

    def set_status(self, row_number, status):
        """

        :param row_number:
        :param status:
        :return:
        """
        # check ... blabla

        self.update_cell_value(row_number, self._iColStatus, status)

        return
