#this file changes the mantidqt.pri file to use the win64 libs
import os
import re

path = os.getcwd() + '\mantidqt.pri'
print path

lines = []

f = open(path, 'r')
for line in f:
	line = re.sub('/lib/win32/', '/lib/win64/', line)
	print line
	lines.append(line)
	
f.close()

f = open(path, 'w')
for line in lines:
	f.write(line)

f.close()
