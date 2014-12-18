text = """
===Algorithms===
* PlotPeakByLogValue now optionally outputs calculated spectra like the Fit algorithm.
* StartLiveData  now checks whether the instrument listener supports the provision of historic values.
* LiveData processing now handles the transition between runs much better at facilities that support this functionality (at present only the SNS).
* LoadEventNexus reads the Pause log and discards any events found during Paused time spans.
* GenerateEventsFilter has been improved to make it easier to use and allowing split sections to be added together into workplaces on a cyclic basis.
* LoadPreNexus is now more forgiving if it encounters bad event indexes in pulse ID file .

"""

import re

algs = AlgorithmFactory.getRegisteredAlgorithms(True)
for alg in algs:
    text = re.sub(r'\b' + alg+ r'\b',r'[http://docs.mantidproject.org/algorithms/' + alg + '.html ' + alg + '] ',text)
print text