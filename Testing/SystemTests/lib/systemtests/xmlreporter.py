import os
import sys
from xml.dom.minidom import getDOMImplementation
import stresstesting

class XmlResultReporter(stresstesting.ResultReporter):

	_time_taken = 0.0
	_failures = []
	_skipped = []
	
	def __init__(self, showSkipped=True):
		self._doc = getDOMImplementation().createDocument(None,'testsuite',None)
		self._show_skipped = showSkipped

	def reportStatus(self):
		return len(self._failures) == 0

	def getResults(self):
		# print the command line summary version of the results
		self._failures.sort()
		self._skipped.sort()
		print
		if self._show_skipped and len(self._skipped) > 0:
			print "SKIPPED:"
			for test in self._skipped:
				print test.name
		if len(self._failures) > 0:
			print "FAILED:"
			for test in self._failures:
				print test.name

		# return the xml document version
		docEl = self._doc.documentElement
		docEl.setAttribute('name','SystemTests')
		docEl.setAttribute('tests',str(len(docEl.childNodes)))
		docEl.setAttribute('failures',str(len(self._failures)))
		docEl.setAttribute('skipped', str(len(self._skipped)))
		docEl.setAttribute('time',str(self._time_taken))
		return self._doc.toxml()

	def dispatchResults(self, result):
		''' This relies on the order and names of the items to give the correct output '''
		test_name = result.name.split('.')
		if len(test_name) > 1:
			class_name = '.'.join(test_name[:-1])
			name = test_name[-1]
		else:
			class_name = result.name
			name = result.name
		elem = self._doc.createElement('testcase')
		elem.setAttribute('classname',"SystemTests." + class_name)
		elem.setAttribute('name',name)
		if result.status == 'skipped':
			self._skipped.append(result)
			skipEl = self._doc.createElement('skipped')
			if len(result.output) > 0:
				if "Missing required file" in result.output:
					skipEl.setAttribute('message', "MissingRequiredFile")
				else:
					skipEl.setAttribute('message', result.output)
				skipEl.appendChild(self._doc.createTextNode(result.output))
			elem.appendChild(skipEl)
		elif result.status != 'success':
			self._failures.append(result)
			failEl = self._doc.createElement('failure')
			failEl.setAttribute('file',result.filename)
			output = ''
			if len(result.output) > 0:
				output += result.output
			if len(output) > 0:
				failEl.appendChild(self._doc.createTextNode(output))
			elem.appendChild(failEl)
		else:
			time_taken = 0.0
			for t in result.resultLogs():
				if t[0] == 'iteration time_taken':
					time_taken = float(t[1].split(' ')[1])
					self._time_taken += time_taken
				if t[0] == 'memory footprint increase':
					memEl = self._doc.createElement('memory')
					memEl.appendChild(self._doc.createTextNode(t[1]))
					elem.appendChild(memEl)
			elem.setAttribute('time',str(time_taken))
			elem.setAttribute('totalTime',str(time_taken))
		self._doc.documentElement.appendChild(elem)
