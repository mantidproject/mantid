"""*WIKI* 

In algorithms related to [[Le Bail Fit]] and powder diffractomer instrument profile calibration, 
TableWorkspace containing the peak profile parameters' information are used as input and output. 
''UpdatePeakParameterTableValue'' gives user the method to change the value of parameters' information,
including its status to fit, value, minimum/maximum value (for boundary contrains) and step size (for Monte Carlo optimizer). 

== Format of TableWorkspace ==
TableWorkspace containing peak profile parameters must have 2 columns, "Name" and "Value".  It can have but not be limited to the following columns, "Min", "Max", "Stepsize" and "FitOrTie".

== Specify A Cell or Cells ==
The cell to have value updated can be specified by its row and column index.  
* Column index is determined by property "Column".  
* Row index can be specified by property "Row", which requires a list of row indexes, or property "ParameterNames", which requires a list of strings.  If "ParameterNames" is used as the input of row indexes, the algorithm will go through each row to match the string value of cell "Name" (i.e., parameter name) to each input parameter name.  
* If neither "Row" nor "ParameterNames" is given by user, then all cells in the column will have the value updated to a same value from either "NewFloatValue" or "NewStringValue" according to the type of the cell. 
* If multiple row indexes are specified, then all the cells of the specified column and rows are updated to same value from either "NewFloatValue" or "NewStringValue". 

== How to use algorithm with other algorithms ==
This algorithm is designed to work with [[Le Bail Fit|other algorithms]] to do Le Bail fit.


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
        self.setWikiSummary("Update cell value(s) in a TableWorkspace containing instrument peak profile parameters.")
        """ Property definition
        """
        tableprop = mantid.api.ITableWorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.InOut)
        self.declareProperty(tableprop, "TableWorkspace containing peak profile parameters")

        colchoices = ["Value", "FitOrTie", "Min", "Max", "StepSize"]
        self.declareProperty("Column", "Value", mantid.kernel.StringListValidator(colchoices), "Column name of the cell to have value updated.  Choices include 'FitOrTie', 'Max', 'Min', 'StepSize' and 'Value'")

        rowprop = mantid.kernel.IntArrayProperty("Rows", [])
        self.declareProperty(rowprop, "List of row numbers of the cell to have value updated")

        parnameprop = mantid.kernel.StringArrayProperty("ParameterNames", [])
        self.declareProperty(parnameprop, "List of names of parameters that will have their values updated")

        self.declareProperty("NewFloatValue", 0.0, "New value for the specified cell of type 'float'.")

        self.declareProperty("NewStringValue", "", "New value for the specified cell of type 'string'.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # 1. Process input parameter TableWorkspace
        tableWS = self.getProperty("InputWorkspace").value
        result = self.parseTableWorkspace(tableWS)
        paramnamedict = result[0]
        colnamedict = result[1]

        # 2. Process input row (to change) information
        rownumberlist = list(self.getProperty("Rows").value)
        parameterlist = self.getProperty("ParameterNames").value
        for parametername in parameterlist:
            rows = self.convertParameterNameToTableRows(parametername, paramnamedict)
            rownumberlist.extend(rows)
        # ENDFOR

        if len(rownumberlist) > 1: 
            rownumberlist = sorted(rownumberlist)
        elif len(rownumberlist) == 0:
            # if no input row number/parameter name, set the value to all rows
            numrows = tableWS.rowCount()
            rownumberlist = range(0, numrows)
        # ENDIF
        # for irow in rownumberlist:
        #     print "Update value on row %d" % (irow)

        # 3. Process column (to change) information
        colname = self.getProperty("Column").value
        if colnamedict.has_key(colname): 
            icolumn = colnamedict[colname]
        else:
            raise NotImplementedError("Column name %s does not exist in TableWorkspace %s" 
                    % (colname, tablews.name()))

        # 3. Set value
        if colname in ["FitOrTie", "Name"]:
            # string value
            value = self.getProperty("NewStringValue").value.lower()
        else:
            # float value
            value = self.getProperty("NewFloatValue").value

        # print "Input Value = %s.  Type = %s" % (str(value), type(value))
        
        for irow in rownumberlist:
            irow = int(irow)
            if irow >= 0: 
                tableWS.setCell(irow, icolumn, value)

        # 4. 
        self.setProperty("InputWorkspace", tableWS)

        return
        
    def convertParameterNameToTableRows(self, parametername, paramnamedict):
        """ Convert parameter name (incomplete might be) to row number(s)
        An algorithm to recognize parameter with blurred definition will be use such as
        (1) exactly same and case insensitive;
        (2) *partialname
        (3) partialname*
        (4) *partialname*
        (5) partialname?

        Argument:
         - parametername:  parameter name to change value
         - paramnamedict:  dictionary key = parameter name, value = row number in table workspace

        Return: List of row numbers (integer), a negative value might exit to represent a non-existing parameter name 
        """
        rownumbers= []

        # 1. make case insensitive
        parnametofit = parametername.lower()

        for parname in self.parameternames:
            parname_low = parname.lower()

            ismatch = False

            if parname_low == parnametofit:
                # exactly match
                ismatch = True

            elif parnametofit.startswith("*") and parnametofit.endswith("*"):
                # *XXXX*
                corestr = parnametofit.split("*")[1]
                if parname_low.count(corestr) >= 1:
                    ismatch = True

            elif parnametofit.startswith("*"):
                # *NNNNN
                corestr = parnametofit.split("*")[1]
                if parname_low.endswith(parnametofit):
                    ismatch = True
                    self.parametersToFit.append(corestr)

            elif parnametofit.endswith("*"):
                # NNNNN*
                corestr = parnametofit.split("*")[0]
                if parname_low.startswith(corestr):
                    ismatch = True

            elif parnametofit.endswith("?"):
                # NNNNN?
                corestr = parnametofit.split("?")[0]
                if parname_low.startswith(corestr) and len(parname_low) == len(parnametofit):
                    ismatch = True
                # ENDIFELSE

            if ismatch is True:
                rownumber = paramnamedict[parname]
                rownumbers.append(rownumber)
        # ENDFOR: 

        # 2. Take in the case that there is no match
        if len(rownumbers) == 0:
            # No Match
            print "Warning! There is no match for parameter %s" % (parnametofit)
            rownumbers.append(-1000)
        # ENDIFELSE
        
        return rownumbers

    def parseTableWorkspace(self, tablews):
        """ Get information from table workspace

        Return:  dictionary, key = row number, value = name
        """
        parameterdict = {}

        # 1. Column name information
        colnames = tablews.getColumnNames()
        self.tableColNames = colnames
        colnamedict = {}
        for ic in xrange( len(colnames) ):
            colnamedict[colnames[ic]] = ic

        # 2. Check validity of workspace
        if len(colnames) < 2:
            raise NotImplementedError("Input table workspace is not supported due to column size.")
        if colnames[0] != "Name" or colnames[1] != "Value":
            raise NotImplementedError("Input table workspace is not supported due to column name.")
       
        # 3. Parse!
        parametersdict = {}
        parnamedict = {}
        self.parameternames = []

        numrows = tablews.rowCount()
        for irow in xrange(numrows):
            parname = tablews.cell(irow, 0)
            parname = parname.lower().strip()
            parnamedict[parname] = irow
            self.parameternames.append(parname)

            # NEW! Collect each variable in 
            valuelist = []
            for icol in xrange(len(colnames)):
                value = tablews.cell(irow, icol)
                valuelist.append(value)
                parameterdict[parname] = valuelist
            # ENDFOR

        # ENDFOR
        
        return (parnamedict, colnamedict, parameterdict)


# ENDCLASS
mantid.api.AlgorithmFactory.subscribe(UpdatePeakParameterTableValue)
