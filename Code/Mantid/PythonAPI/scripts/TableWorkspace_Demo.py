#-------------------------------------------------------------
#
# Example: Accessing TableWorkspaces from Python
#          and using them in conjunction with 
#          MantidPlot tables 
#------------------------------------------------------------

LoadRaw(Filename="../../../../Test/Data/GEM40979.raw",OutputWorkspace="GEM40979")
alg = AlignDetectors("GEM40979","aligned","../../../../Test/Data/offsets_2006_cycle064.cal")
mtd.deleteWorkspace("GEM40979")
calfile = alg.getPropertyValue("CalibrationFile")
DiffractionFocussing("aligned","focussed",calfile)
mtd.deleteWorkspace("aligned")
FindPeaks("focussed","stripped", "peaks")

# Mantid table workspace
tablews=mantid.getTableWorkspace("peaks")
# MantidPlot table (these start at cell (column=1,row=1))
mplot_table = importTableWorkspace("peaks")

# Export to a file. First agrument is the file name and the second is a column separator
# which can be left blank and a <TAB> will be used
#mplot_table.exportASCII("peakslist.txt", "      ")

table_string=""
for c in range(0, mplot_table.numCols()):
	table_string += mplot_table.colName(c + 1) + '        '

table_string += '\n'
for r in range(0, mplot_table.numRows()):
	for c in range(0, mplot_table.numCols()):
		 table_string += str(mplot_table.cell(c + 1,r + 1)) + '          '
	table_string += '\n'

print table_string
