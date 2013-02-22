import mantidqtpython
import os
import tempfile
from mantidplotpy.proxies import threadsafe_call

# Where to save the screenshots.
screenshotdir = get_screenshot_dir()
print "Writing screenshots to " + screenshotdir

"""
Take a screenshot of the algorithm dialog associated with the algorithmDeprecationMessage
@param alg_name : Name of the algorithm to create the dialog for
"""
def screenShotAlgorithm(alg_name):
	interface_manager = mantidqtpython.MantidQt.API.InterfaceManager()
	dlg = threadsafe_call( interface_manager.createDialogFromName, alg_name, True)
	file = alg_name + "_dlg"
	screenshot(widget=dlg, filename=file, screenshot_dir=screenshotdir)
	threadsafe_call(dlg.close)
	file_abs = os.path.join(screenshotdir, file + ".png")

"""
Screenshot all registered algorithms.
"""
def screenShotAll():
	include_hidden_algorithms = True
	algs = AlgorithmFactory.getRegisteredAlgorithms(include_hidden_algorithms)
	for alg_name, versions in algs.iteritems():
		if specialization_exists(name):
			continue
		try:
			screenShotAlgorithm(alg_name)
		except Exception:
			print "Failed to generate dialog for " + alg_name
			continue
			
#Execute the 	
screenShotAll();








