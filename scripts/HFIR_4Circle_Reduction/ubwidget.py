__author__ = 'wzz'

class UBMatrixWindow(QtGui.QMainWindow):
    """

    """
    def __init__(self):
        """

        :return:
        """

        self.connect(self.ui.pushButton_setUBSliceView, QtCore.SIGNAL('clicked()'),
                     self.do_set_ub_sv)

        self.ui.tableWidget_ubMergeScan.setup()
        self.ui.radioButton_ubFromTab1.setChecked(True)

        return

    def do_set_ub_sv(self):
        """ Set UB matrix in Slice view
        :return:
        """
        if self.ui.radioButton_ubFromTab1.isChecked():
            # from tab 'calculate UB'
            self.ui.tableWidget_ubMergeScan.set_from_matrix(self.ui.tableWidget_ubMatrix.get_matrix())
        elif self.ui.radioButton_ubFromTab3.isChecked():
            # from tab 'refine UB'
            self.ui.tableWidget_ubMergeScan.set_from_matrix(self.ui.tableWidget_refinedUB.get_matrix())
        elif self.ui.radioButton_ubFromList.isChecked():
            # set ub matrix manually
            ub_str = str(self.ui.plainTextEdit_ubInput.toPlainText())
            status, ret_obj = gutil.parse_float_array(ub_str)
            if status is False:
                # unable to parse to float arrays
                self.pop_one_button_dialog(ret_obj)
            elif len(ret_obj) != 9:
                # number of floats is not 9
                self.pop_one_button_dialog('Requiring 9 floats for UB matrix.  Only %d are given.' % len(ret_obj))
            else:
                # in good UB matrix format
                ub_str = ret_obj
                option = str(self.ui.comboBox_ubOption.currentText())
                if option.lower().count('spice') > 0:
                    # convert from SPICE UB matrix to Mantid one
                    spice_ub = gutil.convert_str_to_matrix(ub_str, (3, 3))
                    mantid_ub = r4c.convert_spice_ub_to_mantid(spice_ub)
                    self.ui.tableWidget_ubMergeScan.set_from_matrix(mantid_ub)
                else:
                    self.ui.tableWidget_ubMergeScan.set_from_list(ub_str)
        else:
            self.pop_one_button_dialog('None is selected to set UB matrix.')

        return