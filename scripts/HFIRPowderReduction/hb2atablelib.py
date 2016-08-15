import NTableWidget as BaseTable


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
