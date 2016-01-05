#pylint: disable=no-init, invalid-name, line-too-long
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np

class QECoverage(PythonAlgorithm):
    """
    QECoverage - Calculates the Q(E) limits for a direct geometry spectrometer given Ei and the instrument detector angles

    QECoverage(ei)
    QECoverage(Ei=ei, Instrument=instr)
    QECoverage(Ei=ei, Instrument=instr, PlotOver=True, PlotOverWindow='QECoverage')

    Input:
            ei      -   A float or comma-separated list of floats indicating the incident energy
            instr   -   The instrument name as a string. Must be one of ('LET', 'MAPS', 'MARI', 'MERLIN') [default: 'MERLIN']

    If a QECoverage plot window exists, the options PlotOver and PlotOverWindow are available to force the function to
    plot over an existing QECoverage plot.
    """

    # By Helen Walker (2015)
    # Additional coding by Duc Le (2015)

    def category(self):
        return 'Inelastic;PythonAlgorithms'

    def name(self):
        return 'QECoverage'

    def summary(self):
        return 'Calculates the Q(E) limits for a direct geometry spectrometer given Ei and the instrument detector angles'

    def PyInit(self):
        self.declareProperty(name='Ei', defaultValue='', doc='Incident Energy in meV')
        whitelist = StringListValidator(['LET', 'MAPS', 'MARI', 'MERLIN'])
        self.declareProperty(name='Instrument', defaultValue='MERLIN', validator=whitelist, doc="Instrument name")
        # Get list of all active graph windows with titles starting with 'QECoverage'
        # On first initialisation, pymantidplot is declared after all user algorithms are initialised so the next line will fail - catch it and make it an empty list.
        try:
            import pymantidplot
            nameqegraph = [val.name() for _, val in enumerate(pymantidplot.activeFolder().windows()) if val.name().startswith('QECoverage')]
        except (AttributeError, ImportError):
            nameqegraph = []
        # If there are any windows named 'QECoverage' then allow option to overplot (otherwise these options are not declared, so are not visible/enabled)
        if nameqegraph == []:
            nameqegraph = ['']
        self.declareProperty(name='PlotOver', defaultValue=False, doc="Plot new boundary on same graph as previous")
        self.declareProperty(name='PlotOverWindow', defaultValue='', doc='Name of window to plot over', validator=StringListValidator(nameqegraph))
        # Output variables - Q-E trajectory as an array
        self.declareProperty(name='Q', defaultValue=[0.], direction=Direction.Output)
        self.declareProperty(name='E', defaultValue=[0.], direction=Direction.Output)
        # PS. The list can only be repopulated by rerunning the algorithm (PyInit is only run once at the start)

    def PyExec(self):
        Inst = self.getPropertyValue('Instrument')
        if Inst == 'LET':
            tthlims = [2.65, 140]
        elif Inst == 'MAPS':
            tthlims = [3.0, 19.8, 21.1, 29.8, 31.1, 39.8, 41.1, 49.8, 51.1, 59.8]
        elif Inst == 'MARI':
            tthlims = [3.43, 29.14, 30.86, 44.14, 45.86, 59.15, 60.86, 74.14, 75.86, 89.14, 90.86, 104.14, 105.86, 119.14, 120.86, 134.14]
        elif Inst == 'MERLIN':
            tthlims = [2.838, 135.69]

        myE = self.getProperty('Ei').value
        eierr = '\n-----------------------------------------------------------------------------------\n'
        eierr += 'Error: Invalid input Ei. This must be a number or a comma-separated list of numbers\n'
        eierr += '-----------------------------------------------------------------------------------\n'
        if ',' not in myE:
            try:
                myEs = [float(myE)]
            except ValueError:
                raise ValueError(eierr)
        else:
            try:
                myEs = [float(val) for val in myE.split(',')]
            except ValueError:
                raise ValueError(eierr)

        # Mantid test framework does not include mantidplot - no plotting is possible. Handle this case
        plotit = True
        try:
            import pymantidplot
            # Checks if other 'QECoverage' windows exists (before first iteration) so we know to increase the increment in the number at end of name
            nameqegraph = [val.name() for ind, val in enumerate(pymantidplot.activeFolder().windows()) if val.name().startswith('QECoverage')]
        except ImportError:
            plotit = False

        # Get list of all active graph windows with titles starting with 'QECoverage'
        for myE in myEs:
            Et0 = np.linspace(-myE/5, myE, 200)
            q1 = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*myE*1.6e-22-Et0*1.6e-22-2*1.6e-22*np.sqrt(myE*(myE-Et0))*np.cos(np.deg2rad(tthlims[0]))))/1e10
            q1 = np.concatenate((np.flipud(q1), q1))
            Et = np.concatenate((np.flipud(Et0), Et0))
            for lines in range(1, len(tthlims)):
                q2 = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*myE*1.6e-22-Et0*1.6e-22-2*1.6e-22*np.sqrt(myE*(myE-Et0))*np.cos(np.deg2rad(tthlims[lines]))))/1e10
                q2 = np.concatenate((np.flipud(q2), q2))
                q1 = np.append(q1, q2)
                Et = np.append(Et, np.concatenate((np.flipud(Et0), Et0)))
            wsname = 'QECoverage_'+Inst+'_Ei='+str(myE)
            CreateWorkspace(DataX=q1, DataY=Et, NSpec=1, OutputWorkspace=wsname)

            if plotit:
                if nameqegraph != []:
                    # Checks if we want to overplot or not
                    if self.getPropertyValue('PlotOver') == '1':
                        # Gets the name of the window to overplot in
                        windowname = self.getPropertyValue('PlotOverWindow')
                        windowhand = [val for ind, val in enumerate(pymantidplot.activeFolder().windows()) if val.name() == windowname]
                        fig = pymantidplot.plotSpectrum(mtd[wsname], 0, type=0, window=windowhand[0])
                        figName = windowname
                    # First Ei and we don't want to overplot. So make a new window
                    elif myE == myEs[0]:
                        fig = pymantidplot.plotSpectrum(mtd[wsname], 0, type=0)
                        lastwindow = nameqegraph[-1]
                        ind = lastwindow.find('-')+1
                        ind = lastwindow[ind:]
                        figName = 'QECoverage-'+str(int(ind)+1)
                    # Second and subsequent Ei, use new window created in previous block.
                    else:
                        pymantidplot.plotSpectrum(mtd[wsname], 0, type=0, window=fig)
                else:
                    if myE == myEs[0]:
                        fig = pymantidplot.plotSpectrum(mtd[wsname], 0, type=0)
                        # Default name of windows is 'QECoverage-1'
                        figName = 'QECoverage-1'
                    else:
                        pymantidplot.plotSpectrum(mtd[wsname], 0, type=0, window=fig)
                l = fig.activeLayer()
                # This will override scale on overplots. No way to check what current limits of a graph are in Mantid
                #     Need to edit MantidPlot/src/Graph.cpp to make another method to get limits from QtiPlot.
                l.setAxisScale(pymantidplot.Layer.Bottom, min(q1), max(q1))
                l.setAxisTitle(pymantidplot.Layer.Bottom, 'Q (A^{-1})')
                l.setAxisTitle(pymantidplot.Layer.Left, 'Energy Transfer (meV)')
                l.setTitle('Q-E coverage')
                fig.setName(figName)

        # Output only the final energy listed.
        self.setProperty('Q', q1)
        self.setProperty('E', Et)

AlgorithmFactory.subscribe(QECoverage)
