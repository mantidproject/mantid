"""*WIKI* 
Generate grouping files for ARCS, CNCS, HYSPEC, and SEQUOIA, by grouping py pixels along a tube and px tubes. 
py is 1, 2, 4, 8, 16, 32, 64, or 128. 
px is 1, 2, 4, or 8.

Author:  A. Savici

*WIKI*"""

import mantid
import mantid.api
import mantid.kernel
import mantid.simpleapi  
from numpy import arange
 
class UpdatePeakParameterTableValue(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    def category(self):
        """ Mantid required
        """
        return "Inelastic;PythonAlgorithms;Transforms\\Grouping"

    def name(self):
        """ Mantid require
        """
        return "UpdatePeakParameterTableValue"

    def PyInit(self):
        """ Property definition
        """
        tableprop = mantid.api.ITableWorkspaceProperty("PeakParameterWorkspace", "", mantid.kernel.Direction.Input)
	self.declareProperty(tableprop, "Name of Calibration Table Workspace")

        colchoices = ["Value", "FitOrTie"]
        self.declareProperty("Column", "Value", mantid.kernel.StringListValidator(colchoices), "Column to have value changed")

        rowprop = mantid.kernel.IntArrayProperty("Rows", [])
        self.declareProperty(rowprop, "Row number(s) to update value")

        parnameprop = mantid.kernel.StringArrayProperty("ParameterNames", [])
        self.declareProperty(parnameprop, "Parameter name(s) to update value")

        self.declareProperty("NewFloatValue", 0.0, "New double value to set to the selected cell(s).")

        self.declareProperty("NewStringValue", "Fit", "New string value to set to the selected cell(s).")

    def PyExec(self):
        """ Main Execution Body
        """
        # 1. Process input parameter TableWorkspace
        tableWS  = self.getProperty("PeakParameterWorkspace").value
        tableparameterdict = self.getInfo(tableWS)

        # 2. Process input row/column information
        rownumberlist = list(self.getProperty("Rows").value)
        parameterlist = self.getProperty("ParameterNames").value
        for parametername in parameterlist:
            rows = self.convertParameterNameToTableRows(parametername, tableparameterdict)
            rownumberlist.extend(rows)
        # ENDFOR

        if len(rownumberlist) > 1: 
            rownumberlist = sorted(rownumberlist)
        elif len(rownumberlist) == 0:
            # if no input row number/parameter name, set the value to all rows
            numrows = tablews.rowCount()
            rownumberlist = range(0, numrows)
        # ENDIF
        # for irow in rownumberlist:
        #     print "Update value on row %d" % (irow)

        # 3. Column and value
        colname = self.getProperty("Column").value
        if colname == "Value":
            icolumn = 1
        else:
            icolumn = 2

        # 3. Set value
        if icolumn == 1:
            value = self.getProperty("NewFloatValue").value
        elif icolumn == 2:
            value = self.getProperty("NewStringValue").value.lower()
        else:
            raise NotImplementedError("Impossible to have icolumn != 1 or 2")

        # print "Input Value = %s.  Type = %s" % (str(value), type(value))
        
        for irow in rownumberlist:
            irow = int(irow)
            tableWS.setCell(irow, icolumn, value)

        return
        
    def convertParameterNameToTableRows(self, parametername, parameterdict):
        """ Convert parameter name (incomplete might be) to row number(s)
        """
        # 1. Identify incomplete parameter name
        withstar = parametername.count('*') > 0
        if withstar is True:
            terms = parametername.split('*')
            parametername = ""
            for term in terms:
                parametername += term
            # ENDFOR
        # ENDIF

        # put to lower case for case insensitive
        parametername = parametername.lower()

        # 2. Get the row
        rows = []
        for irow in sorted(parameterdict.keys()):
            candidate = parameterdict[irow]
            if withstar is True and candidate.count(parametername) > 0:
                rows.append(irow)
            elif withstar is False and candidate == parametername:
                rows.append(irow)
            # ENDIF
        # ENDFOR

        wbuf = "Input Parameter Name = %s  Converted To Row " % (parametername)
        for ir in rows:
            wbuf += "%d, " % (ir)
        # print wbuf
               
        return rows

    def getInfo(self, tablews):
        """ Get information from table workspace
        """
        parameterdict = {}
        colnames = tablews.getColumnNames()
        if len(colnames) < 2:
            raise NotImplementedError("Input table workspace is not supported due to column size.")
        if colnames[0] != "Name" or colnames[1] != "Value":
            raise NotImplementedError("Input table workspace is not supported due to column name.")
        
        numrows = tablews.rowCount()
    
        parametersdict = {}
        for irow in xrange(numrows):
    	    parname = tablews.cell(irow, 0)
            parname = parname.lower()
            parameterdict[irow] = parname
        # ENDFOR
        
        return parameterdict


# ENDCLASS
mantid.api.registerAlgorithm(UpdatePeakParameterTableValue)
