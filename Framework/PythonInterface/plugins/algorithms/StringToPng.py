#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from six import u
import mantid


class StringToPng(mantid.api.PythonAlgorithm):

    def category(self):
        """ Category
        """
        return "DataHandling\\Plots"

    def seeAlso(self):
        return [ "SavePlot1D" ]

    def name(self):
        """ Algorithm name
        """
        return "StringToPng"

    def summary(self):
        return "Creates an image file containing a string."

    def checkGroups(self):
        return False

    def PyInit(self):
        #declare properties
        self.declareProperty("String","", mantid.kernel.StringMandatoryValidator(),"String to plot")
        self.declareProperty(mantid.api.FileProperty('OutputFilename', '', action=mantid.api.FileAction.Save, extensions = ["png"]),
                             doc='Name of the image file to savefile.')

    def PyExec(self):
        ok2run=''
        try:
            import matplotlib
        except ImportError:
            ok2run='Problem importing matplotlib'
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__)<LooseVersion("1.2.0"):
                ok2run='Wrong version of matplotlib. Required >= 1.2.0'
        if ok2run!='':
            raise RuntimeError(ok2run)
        if 'backend_mtdqt4agg' not in matplotlib.get_backend():
            matplotlib.use("agg")
        import matplotlib.pyplot as plt
        fig=plt.figure(figsize=(.1,.1))
        ax1=plt.axes(frameon=False)
        ax1.text(0.,1,bytearray(u(self.getProperty("String").valueAsStr), 'utf-8').decode('unicode_escape'),va="center",fontsize=16)
        ax1.axes.get_xaxis().set_visible(False)
        ax1.axes.get_yaxis().set_visible(False)
        filename = self.getProperty("OutputFilename").value
        fig.savefig(filename,bbox_inches='tight')
        plt.close(fig)


mantid.api.AlgorithmFactory.subscribe(StringToPng)
