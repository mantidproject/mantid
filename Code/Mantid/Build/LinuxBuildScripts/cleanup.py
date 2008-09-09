import os

libs = []

f=open('Mantid/libs.txt', 'r')
for line in f:
	libs.insert(0, line.replace("\n",""))

f.close()

for lib in libs:
	try:
		os.remove('Mantid/' + lib)
	except:
		print "No file to delete"


