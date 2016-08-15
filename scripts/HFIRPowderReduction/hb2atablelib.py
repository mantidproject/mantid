import NTableWidget as BaseTable

"""
This module includes several table widgets extended from NTalbeWidget
"""


class ScanInfoTable(BaseTable.NTableWidget):
    """ A customized table to present HB2A information
    """
    SetupTable = [('Scan', 'int')]


    def __init__(self, parent):
        """ Initialization
        """
        BaseTable.NTableWidget.__init__(self, parent)

        self._columnNameList.append('Scan')

        return

    def add_scan_info(self, log_info_dict):
        """
        Add a new scan entry to the table
        """
        # check
        assert isinstance(log_info_dict, dict)

        # set up the row
        row_item_list = list() 
        
        for col_name in self._columnNameList:
            try:
                log_value = log_info_dict[col_name]
            except KeyError as key_err:
                raise RuntimeError('Table column %s does not exist in the input information dictionary.' % col_name)

            if isinstance(log_value, int):
                table_value = '%d' % log_value
            elif instance(log_value,float):
                table_value = '%.5f' % log_value
            else:
                table_value = str(log_value)

            row_item_list.append(table_value)
        # END-FOR

        self.append_row(row_item_list)

    def setup(self, table_colum_list):
        """
        set up the table
        """
        # check inputs
        assert isinstance(table_column_list, list)

        # add the input column list
        for col_name in table_column_list:
            :w
            # scan is always there!
            if col_name == 'Scan':
                continue

            self.SetupTable.append((col_name, 'str'))
            self._columnNameList.append(col_name)
        # END-FOR (col


class ScanInfoSetupTable(BaseTable.NTableWidget):
    """ A customized table to present HB2A information table setup
    """
    SetupTable = [('Item Name', 'str'),
                  ('Use', 'checkbox')]

    def __init__(self, parent):
        """
        Initialization
        """
        base.__init__(self, parent)

        return


    def get_setup(self):
        """
        """
        num_rows = self.rowCount()
        use_log_list = list()
        for i_row in range(num_rows):
            use_it = self.get_cell_value(i_row, self._colIndexUse)
            if use_it:
                item_name = self.get_cell_value(i_row, self._colIndexName)
                use_log_list.append(item_name)
        # END-FOR

        return use_log_list


    def setup(self):
        """ Set up table
        """
        self.init_setup(self.TableSetup)

        self._colIndexName = self.TableSetup.index(('Item Name', 'str'))
        self._colIndexUse = self.TableSetup.index(('Use', 'checkbox'))

        return

