import os

libs = []

f=open('Mantid/src/libs.txt', 'r')
for line in f:
	libs.insert(0, line.replace("\n",""))

f.close()

for lib in libs:
	try:
		os.remove('Mantid/src/' + lib)
	except:
		print os.cwd()
		print lib
		print "No file to delete"


