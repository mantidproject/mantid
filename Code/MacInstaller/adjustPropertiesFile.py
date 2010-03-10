#!/bin/python
#
# Copies the default Mantid.properties file, adjusting directories where
# necessary.
#
prop_file = open('../Mantid/Properties/Mantid.properties','r')
prop_file_ins = open('MantidPlot.app/Contents/MacOS/Mantid.properties','w')
for line in prop_file:
    if line.find('plugins.directory') >= 0:
        prop_file_ins.write('plugins.directory = ../../plugins\n')
    elif line.find('pythonscripts.directory') >= 0:
        prop_file_ins.write('pythonscripts.directory = ../../scripts\n')
    elif line.find('instrumentDefinition.directory') >= 0:
        prop_file_ins.write('instrumentDefinition.directory = ../../instrument\n')
    elif line.find('pythonalgorithms.directories') >= 0:
        prop_file_ins.write('pythonalgorithms.directories = ../../plugins/PythonAlgs\n')
    else:
        prop_file_ins.write(line)
prop_file_ins.close()
prop_file.close()