import sys
import re
import subprocess as sp


process = sp.Popen("doxygen "+sys.argv[1],stderr=sp.PIPE,shell=True)

regExStrings = ['return type of member.*(c_|m_|g_).*is not documented', #remove warnings about return type of member and global variables
'return type of member.*Mantid::PhysicalConstants::.*is not documented',#not return type moans about our constants file please
'Problems running dvips', #moans about dvips not being installed
'Problems running latex'] #moans about latex not being installed

inGarbageBlock=0
currentLine = ''
for line in process.stderr:
	
	if inGarbageBlock == 1:
		#i'm in the garbage block - discard the next line
		inGarbageBlock = 0 #I'll be out of it now
		break
	
	#first check for the interprocess interuption error string 'program' is not recognized as an internal or external command,
	regExString = '\'.*\' is not recognized as an internal or external command,'
	(newLine, numberOfChanges) = re.subn(regExString,'',line) 
	if (numberOfChanges == 0):
		currentLine += line.rstrip() 
	else:
		#next line needs to be stripped as well
		inGarbageBlock = 1
		break
		
	if inGarbageBlock == 0:
		matchCount = 0
		for regExString in regExStrings:
			if (re.search(regExString,currentLine) != None):
				matchCount+=1
				break
		if (matchCount == 0):
			print >> sys.stderr, currentLine.rstrip()
		currentLine = ''

