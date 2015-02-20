from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory, FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator
import warnings

_OUTPUTLEVEL = "NOOUTPUT"

class SelectPowderDiffPeaks(PythonAlgorithm):
    """ Algorithm to select the powder diffraction peaks for Le Bail Fit
    """
    def category(self):
        """
        """
        return "Diffraction;Utility"

    def name(self):
        """
        """
        return "SelectPowderDiffPeaks"

    def summary(self):
        return "Select the powder diffraction peaks for Le Bail Fit"

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(ITableWorkspaceProperty("BraggPeakParameterWorkspace", "", Direction.Input),
                "Name of Table Workspace containing peak parameters.")

        self.declareProperty(ITableWorkspaceProperty("ZscoreWorkspace", "", Direction.Input),
                "Name of Table Workspace containing z-score for the peak parametrs.")

        self.declareProperty(ITableWorkspaceProperty("OutputBraggPeakParameterWorkspace", "", Direction.Output),
                "Name of Table Workspace containing the filtered peaks' parameters.")

        self.declareProperty("MinimumPeakHeight", 0.0, "Minimum peak height allowed for the peaks to fit. ")

        self.declareProperty("ZscoreFilter", "", "Filter on the Zscore of the peak parameters.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        warnings.warn("A message", ModuleDeprecationWarning)

        # 1. Get Input properties
        inppeakws = self.getProperty("BraggPeakParameterWorkspace").value
        inpzscows = self.getProperty("ZscoreWorkspace").value

        minpeakheight = float(self.getPropertyValue("MinimumPeakHeight"))
        zscorefilterstr = self.getPropertyValue("ZscoreFilter")

        print "Input: PeakParameterWorkspace = %s;  ZscoreWorkspace = %s" % (inppeakws.name, inpzscows.name)
        print "       Minimum peak height = %f" % (minpeakheight)
        print "       Zscore filter: %s" % (zscorefilterstr)

        # 3. Parse Zscore table and peak parameters
        self.mPeaks = {}

        zscoredict = self.parseBraggPeakParameterTable(inpzscows)
        self.mPeaks = self.parseBraggPeakParameterTable(inppeakws)

        # 4. Filter by peak height
        self.filterByPeakHeight(minpeakheight)

        # 5. Filter by zscore
        zscorefilterdict = self.parseZscoreFilter(zscorefilterstr)
        self.filterByZscore(zscoredict, zscorefilterdict)

        # 6. Generate the output
        paramWS = WorkspaceFactory.createTable()
        self.genBraggPeakParameterWorkspace(paramWS)
        self.setProperty("OutputBraggPeakParameterWorkspace", paramWS)

        return


    def genBraggPeakParameterWorkspace(self, tablews):
        """ Create TableWorkspace containing peak parameters
        """
        # 1. Create an empty workspace and set the column
        tablews.addColumn("int", "H")
        tablews.addColumn("int", "K")
        tablews.addColumn("int", "L")
        tablews.addColumn("double", "d_h")
        tablews.addColumn("double", "TOF_h")
        tablews.addColumn("double", "Height")
        tablews.addColumn("double", "Alpha")
        tablews.addColumn("double", "Beta")
        tablews.addColumn("double", "Sigma")

        # 2. Sort the dictionary by d-spacing
        dspdict = {}
        hkls = self.mPeaks.keys()
        for hkl in hkls:
            dsp = self.mPeaks[hkl]["d_h"]
            dspdict[dsp] = hkl
        dhs = dspdict.keys()
        dhs = sorted(dhs)

        # 3. Add peaks
        for dsp in dhs:
            hkl = dspdict[dsp]

            h = hkl[0]
            k = hkl[1]
            l = hkl[2]

            tof_h = self.mPeaks[hkl]["TOF_h"]
            height = self.mPeaks[hkl]["Height"]
            alpha = self.mPeaks[hkl]["Alpha"]
            beta = self.mPeaks[hkl]["Beta"]
            sigma = self.mPeaks[hkl]["Sigma"]

            tablews.addRow([h, k, l, dsp, tof_h, height, alpha, beta, sigma])
        # ENDFOR

        return tablews

    def parseBraggPeakParameterTable(self, tablews):
        """ Parse Zscore parameter table workspace
        Return: zscoredict
        """
        numrows = tablews.rowCount()
        numcols  = tablews.columnCount()
        colnames = tablews.getColumnNames()

        peakparameterdict = {}

        for irow in xrange(numrows):
            ppdict = {}
            for icol in xrange(numcols):
                colname = colnames[icol]
                value = tablews.cell(irow, icol)
                ppdict[colname] = value
            # ENDFOR Column
            h = ppdict["H"]
            k = ppdict["K"]
            l = ppdict["L"]

            peakparameterdict[(h, k, l)] = ppdict
        # ENDFOR

        return peakparameterdict

    def filterByPeakHeight(self, minpeakheight):
        """ Filter by peak height
        """
        for hkl in self.mPeaks.keys():
            height = self.mPeaks[hkl]["Height"]
            wbuf = "Peak %d %d %d:  Height = %f " % (hkl[0], hkl[1], hkl[2], height)
            if height < minpeakheight:
                self.mPeaks.pop(hkl)
                wbuf += "Removed due to height < %f" % (minpeakheight)
            else:
                # wbuf += "Kept due to height > %f" % (minpeakheight)
                pass

            print wbuf
        # ENDFOR

        return


    def filterByZscore(self, zscoredict, zscorefilter):
        """ Filter by zscore
        """
        # 1. Loop over peaks
        for hkl in self.mPeaks.keys():

            zscores = zscoredict[hkl]

            deletethispeak = False
            errmsgout = False

            # 2. Loop over zscore filters
            for parname in zscorefilter.keys():
                # Maximum allowed Z score
                maxzscore = zscorefilter[parname]
                # Convert to regular parameter name to parameter name in Zcore workspace
                zparname = "Z_"+parname
                if zscores.has_key(zparname):
                    # Zscore table has this parameter's zscore
                    zscore = zscores[zparname]
                    if zscore > maxzscore:
                        deletethispeak = True
                        break
                    # ENDIF
                else:
                    # Zscore table has no such parameter
                    print "Warning! Zscore table has no parameter %s (from %s)" % (zparname, parname)
                    errmsgout = True
                # ENDIF
            # ENDFOR Fitler

            # 3. Delete if
            if deletethispeak is True:
                self.mPeaks.pop(hkl)

            if errmsgout is True:
                msg = "Parameters (keys):\t\t"
                for key in zscores.keys():
                    msg += "%s,\t\t" % (key)
                print msg

        # ENDFOR Each Peak

        return

    def parseZscoreFilter(self, zscorefilterstr):
        """ Parse Zscore filter string
        Return: zscore filter dictionary Parameter Name: Maximum Allowed
        """
        zscorefilter = {}

        terms = zscorefilterstr.split(',')
        if len(terms) % 2 == 1:
            raise NotImplementedError("Zscore filter is not defined correct.  It must have string and float in pair.")

        for i in xrange(len(terms)/2):
            parname = terms[2*i].strip()
            maxzscore = float(terms[2*i+1])
            zscorefilter[parname] = maxzscore

        return zscorefilter


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SelectPowderDiffPeaks)
