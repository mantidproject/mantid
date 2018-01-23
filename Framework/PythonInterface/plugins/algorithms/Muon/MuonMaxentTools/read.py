import numpy as np
import math
import mantid
from multimaxalpha import MULTIMAX
from mantid.api import * # PythonAlgorithm, registerAlgorithm, WorkspaceProperty
from mantid.kernel import *
from mantid.simpleapi import *

# translated from read.for
# changed to read from Mantid workspace(s)
"""
c
C***********************************************************************
C           SUBROUTINE TO READ RUN DETAILS FROM OPENGENIE
C************************************************************************
C

      subroutine opengenie_maxent(pars_in,pars_out)
c     implicit none
        
      external pars_in,pars_out

	character*255 datafilename,readdata
      logical onepulse, varydt, fixphasein, retlog
      integer nptsin, zeroch, istgoodch, ierr, debug
      real deflevel, Sigmaloss

      INTEGER P,FIELD,ig
      CHARACTER*1 ANS,fitdead,fixphase,fitamp,firstgo
	common/MaxPage/n,f
      COMMON/ANS/ANS,firstgo
      COMMON/FASE/PHASE,phshift
      common/fac/factor,facfake,ratio
      common/channels/itzero,ipulse1,ipulse2,i1stgood,itotal
      common/points/npts,ngroups,nhists
      common/pulses/npulse,DEF
      COMMON/RUNDATA/RES,FRAMES,FNORM,IRUNNO,HISTS
      common/datall/rdata(64,4096)
      common/framin/iframnew
      common/datin/runin(64,4096),histco(64),hblock(15)
      integer rdata,runin
      character*80 hblock,histco
      COMMON/FLAGS/FITDEAD,FIXPHASE,FITAMP
      COMMON/FILE/NAME
      COMMON/GROUPING/GROUP(64)
      COMMON/MACH/MACHINE
	common/sense/phi(64),TAUD(64)
      common/savetime/ngo,i2pwr
      common/moments/bmindef
      CHARACTER*40 TITLE,NAME 
      character*3 machine
      character*1 mulrun
      CHARACTER*40 GRPTITLE
      CHARACTER*80 COMMENTLINE
	character*2046 str
      REAL HISTS(64)
      REAL NFRAM, F(68000),fchan(68000),fperchan
      INTEGER ITOTAL,NHISTOS,HISTO,DEST
      INTEGER GROUP, tempdata(256000)
      REAL RES


	integer histlen, nhisto

      call module_get_int(pars_in,'RunNo',Irunno)
      call module_get_int(pars_in,'frames',Ifram)
      call module_get_int(pars_in,'res',Ires)
	res=real(Ires)/1e6
c	write(99,*) res
      call module_print("Hello")

      call module_get_int(pars_in,'Tzeroch',zeroch)
	call module_get_int(pars_in,'firstgoodch',istgood)
	call module_get_int(pars_in,'fitphase',fph)
	if (fph.eq.1) then
	  fixphasein=.true.
	else
	  fixphasein=.false.
	endif
	call module_get_int(pars_in,'fitdt',fdt)
	if (fph.eq.1) then
	  varydt=.true.
	else
	  varydt=.false.
	endif

	call module_get_real(pars_in,'deflevel',deflevel)
	onepulse=.true.

	call module_get_real(pars_in,'sigloose',Sigmaloss)

	call module_get_int(pars_in,'ptstofit',nptsin)

	call module_get_int(pars_in,'histolen',histlen)
	itotal=histlen
	call module_get_int(pars_in,'nhisto',nhisto)
	nhists=nhisto
	write(10,*) varydt, fixphasein, onepulse, deflevel,sigmaloss
     +,nptsin,histlen,nhisto

      call module_get_int_array
     +              (pars_in,'counts',tempdata,histlen*nhisto)
c	 write(69,*)tempdata
	do i=1,nhisto
	 do j=1,histlen
	   rdata(i,j)=tempdata((i-1)*histlen+j)
	 enddo
	enddo










      
c  default values of various consts collected for easy changing
      if (firstgo.ne.'T') goto 998
c for second time round, default value is prev. chosen
      irundef=-1
      npulsdef=1
      fitdead='Y'
      fixphase='Y'
      fitamp='Y'
      mulrun='N'
      defdef=0.1
      nptsdef=4096
      bmindef=25.
c itzero and i1stgood for 1/2 pulses, res= 8/16 ns
      itzdef18=24
      itzdef28=44
      i1stdef8=80
      itzdef116=39
      itzdef216=17
      i1stdef16=50
      facdef=1.04
998   mulrun='N'
997   write(99,*) ' Please enter -1 for currant bun ...'
!      pause
      write(99,'('' Run number ['',i5,'']:'',$)')irundef
!	pause
!      read(5,5,end=1) irunno
!      read(5,5) irunno
!5     format(i5)
      datafilename=""
      write(99,*) datafilename
!1     rewind(5)
      if(irunno.eq.0)irunno=irundef
c re-read the same data
      if(irunno.lt.0)irunno=0
c -ve means read the currant bun, which goes to multibunrd as 0
      ihaveaproblem=0
c      call multibunrd(datafilename,ihaveaproblem)
      if (ihaveaproblem.gt.0) then
        write(99,*) ' error in reading file....'
        goto 997
      endif
      irundef=irunno  
      firstgo='F'
      write(99,'( '' Resolution: '',f8.4)')res
      FRAMES=IFRAM
      write(69,*) frames
994   write(99,993) mulrun
993   FORMAT(1X,'Do you want to add in more runs? [',a1,']:',$)
      ans=' '
!      read(5,992) ans
      ans='N'
992   FORMAT(A)  
      if (ans.eq.'y'.or.ans.eq.'Y') then
        mulrun='Y'
        ans=' '
      endif
      if (ans.eq.' '.and.mulrun.eq.'Y') then
        mulrun='Y'
c        irundef=irundef+1
        goto 997 
      else
        mulrun='N'
      endif
      mulrun='N'
      write(99,'('' Single (1) or double (2) pulse? ['',
     + i1,''] :'',$)')npulsdef
C      read(5,82,end=80) npulse
!       read(5,82) npulse
      IF (onepulse) npulse=1
	IF (.not.onepulse) npulse=2

C80    rewind(5)
82    format(i)
      if(npulse.lt.1.or.npulse.gt.2) npulse=npulsdef
      npulsdef=npulse
	write(99,*) npulse
      write(99,4)fitdead 
4     FORMAT(1X,'Fit dead time correction ? [',a1,']:',$)
      IF (varydt) ans='Y'
	write(99,*) ans
!      read(5,3)ans
      IF (ans.eq.'Y'.or.ans.eq.'y') then
        fitdead='Y'
      ELSEIF (ans.ne.' ') then 
        fitdead='N'    
      ENDIF
101   write(99,2)fixphase      
2     FORMAT(1X,'Do you want to keep the phases fixed ['
     + ,a1,']:',$)
      IF (fixphasein) then 
	 ans='Y'
	else
	 ans='N'
	endif
	write(99,*) ans
!      read(5,3) ans
3     FORMAT(A)  
      if (ans.eq.'y'.or.ans.eq.'Y') then
          fixphase='Y' 
      elseif (ans.ne.' ') then
          fixphase='N'
      endif
      write(99,'('' What default level? ['',f5.2,''] :'',$)')defdef
      def=deflevel
	write(99,*) deflevel
!      read(5,122) def
C120   rewind(5)
122    format(g6.3)
      if(def.le.0.0) def=defdef
      defdef=def
125    write(99,'('' Points to fit? ['',i4,''] :'',$)')nptsdef
      npts=nptsin
	write(99,*) npts
!      read(5,123) npts
C124    rewind(5)
      if(npts.gt.8192) then
        write(99,*) ' Max points at present is 8192'
        goto 125
      endif
      if(npts.eq.0) npts=nptsdef
123    format(I4)
      tn=log(float(npts))/log(2.)
	write(99,*) tn
      if(abs(tn-float(int(tn+.5))).gt.1.e-5) goto 125
      i2pwr=int(tn+.5)
      nptsdef=npts
	write(99,*) res
      n=1024*npts/(2048*int(.016/res+.5))
      write(99,*)' No of frequency spectrum points chosen as:',n
c this'll give approx 1000 gauss max, for any res or npts
      if(n.gt.npts) n=npts
      if(n.lt.256) n=256
c will do for now till we make it like the psi program
      IF(NPULSE.EQ.2) THEN
c assuming res= .008 < .010 or .016 > .010
        if (res.lt..010) then
          write(99,'('' Enter zero-time channel ['',
     + i2,'']:'',$)')itzdef28
          itzero=zeroch
	    write(99,*) itzero
!          read(5,20) itzero
c231       rewind(5)
          IF(ITZERO.EQ.0) ITZERO=itzdef28
          itzdef28=itzero
        else
          write(99,'('' Enter zero-time channel ['',
     + i2,'']:'',$)')itzdef216
          itzero=zeroch
	    write(99,*) itzero
!          read(5,20) itzero
c232       rewind(5)
          IF(ITZERO.EQ.0) ITZERO=itzdef216
          itzdef216=itzero
        endif
      ELSEIF(NPULSE.EQ.1) THEN
        if (res.lt..010) then
          write(99,'('' Enter zero-time channel ['',
     + i2,'']:'',$)')itzdef18
          itzero=zeroch
	    write(99,*) itzero
!          read(5,20) itzero
C233       rewind(5)
          IF(ITZERO.EQ.0) ITZERO=itzdef18
          itzdef18=itzero
        else
          write(99,'('' Enter zero-time channel ['',
     + i2,'']:'',$)')itzdef116
          itzero=zeroch
	    write(99,*) itzero
!          read(5,20) itzero
C234       rewind(5)
          IF(ITZERO.EQ.0) ITZERO=itzdef116
          itzdef116=itzero
        endif
      ENDIF 
      if (res.lt..010) then
         write(99,'('' Enter first good channel ['',
     + i3,'']:'',$)')i1stdef8
      i1stgood=istgood
	write(99,*) "hello"
	write(99,*) i1stgood
!         read(5,20) i1stgood
C241      rewind(5)
         IF(i1stgood.EQ.0) i1stgood=i1stdef8
         i1stdef8=i1stgood
      else
         write(99,'('' Enter first good channel ['',
     + i3,'']:'',$)')i1stdef16
      i1stgood=istgood
	write(99,*) "goodbye"
	write(99,*) i1stgood
!         read(5,20) i1stgood
C242      rewind(5)
         IF(i1stgood.EQ.0) i1stgood=i1stdef16
         i1stdef16=i1stgood
      endif
      write(99,'('' Enter sigma looseness factor  ['',
     + f4.2,'']'',$)')facdef
      factor=sigmaloss
	write(99,*) factor
!      read(5,21) factor
C26    rewind(5)
      IF(FACTOR.LE.0.0) FACTOR=facdef
      facdef=factor
20    FORMAT(I2)
21    FORMAT(F4.2)
22    FORMAT(F5.1)
!      CLOSE(5)
C       LOADS IN GROUPING 
C       NGROUPS MAY CHANGE AT THIS POINT
      OPEN(20,FILE='STDGRP.PAR',STATUS='OLD')
      READ(20,300) GRPTITLE
      write(99,*) ' Heading of grouping table read in: ',GRPTITLE
300   FORMAT(1X,A40)
      READ(20,301) COMMENTLINE
301   FORMAT(1X,A80)
      READ(20,302) NHISTOS,NGROUPS
302   FORMAT(1X,I2,1X,I2)
      READ(20,301) COMMENTLINE
      READ(20,301) COMMENTLINE
      DO 900 J=1,nhistos
      READ(20,302) HISTO,GROUP(J)
900   CONTINUE
      close(20)
      DO 945 J=1,NGROUPS
      HISTS(J)=0
945   CONTINUE
C      This allows for the removal of a whole group, but...
c      scaling will be buggered if all groups are not the same size.
      DO 55 I=1,64
        TAUD(I)=0
55    CONTINUE
C       LOADS IN DEADTIMES
      OPEN(16,FILE='TAUD.DAT',STATUS='OLD')  
      DO 50 I=1,NGROUPS
      READ(16,*) TAUD(I)
50    CONTINUE
      CLOSE(16)
      write(str,*) ' Initial values of dead times'
	call module_print(TRIM(str))
      write(str,*) (taud(i),i=1,ngroups)
	call module_print(TRIM(str))
 	tcounts=0
	write(69,*) nhists, histlen
      do ig=1,NHISTS
	  sum=0 !integer sum of all data in this group
	  do ic=1,histlen
	    sum=sum+int(rdata(ig,ic))
		if(tcounts.lt.2000000000)tcounts=tcounts+rdata(ig,ic)

	  enddo
	  write(histco(ig),1978) sum

1978  FORMAT('End: ',i10)
      enddo
	write(69,*) histco
	write(69,*) "tcounts",tcounts,sum
      call multimax
	fperchan=1./(RES*float(npts)*2.)
      do  i=1,n
       fchan(i)=float(i-1)*fperchan/135.5e-4
	enddo


      call module_put_real_array(
     +    pars_out,"f",f,n)
	 call module_put_real_array(
     +    pars_out,"fchan",fchan,n)
	  call module_put_real_array(
     +    pars_out,"taud",taud,ngroups)
	 call module_put_real_array(
     +    pars_out,"phi",phi,ngroups)




      RETURN
      END
"""
# input params from GEnie
# RunNo (int)
# Frames (int)
# Ires (int) = resolution in ps
# Tzeroch (int)
# firstgoodch (int)
# fitphase (int==bool)
# fitdt (int==bool)
# deflevel (real)
# sigloose(real)
# ptstofit(int)
# histolen(int)
# nhisto(int)
# counts (int array)
#
# outputs to Genie
# fchan (real array)
# taud (real array)
# phi (real array)

class OpengenieMaxent(PythonAlgorithm):
	def category(self):
		return "Muon;Arithmetic\\FFT"
	
	def Pyinit(self):
		self.declareProperty(WorkspaceProperty("InputWorkspace","",direction=Direction.Input,validator=RawCountsValidator(True)),doc="Raw muon workspace to process")
		self.declareProperty(ITableWorkspaceProperty("InputPhaseTable","",direction=Direction.Input,optional=PropertyMode.Optional),doc="Phase table (initial guess)") # from CalMuonDetectorPhases
		self.declareProperty(ITableWorkspaceProperty("InputDeadTimeTable","",direction=Direction.Input,optional=PropertyMode.Optional),doc="Dead time table (initial)") # from LoadMuonNexus or blanl=k
		self.declareProperty(ITableWorkspaceProperty("GroupTable","",direction=Direction.Input,optional=PropertyMode.Optional),doc="Group Table") # from LoadMuonNexus, none=do all spectra individually
		self.declareProperty("FirstGoodTime",0.1,doc="First good data time")
		self.declareProperty("LastGoodTime",33.0,doc="Last good data time")
		self.declareProperty("Npts",2,doc="Number of frequency points to fit (should be power of 2)",validator=IntListValidator([2**i for i in range(8,21)]))
		self.declareProperty("MaxField",1000.0,doc="Maximum field for spectrum")
		self.declareProperty("FixPhases",False,doc="Fix phases to initial values")
		self.declareProperty("FitDeadTime",True,doc="Fit deadtimes")
		self.declareProperty("DoublePulse",False,doc="Double pulse data")
		self.declareProperty("DefaultLevel",0.1,validator=FloatBoundedValidator(lower=0).setLowerExclusive(True),doc="Default Level, magic number chitarget?")
		self.declareProperty("Factor",1.04,doc="Sigma looseness factor, magic number A?")
		self.declareProperty(WorkspaceProperty("OutputWorkspace","",direction=Direction.Output),doc="Output Spectrum (combined) versus field")
		self.declareProperty(ITableWorkspaceProperty("OutputPhaseTable","",direction=Direction.Output,optional=PropertyMode.Optional),doc="Output phase table (optional)")
		self.declareProperty(ITableWorkspaceProperty("OutputDeadTimeTable","",direction=Direction.Output,optional=PropertyMode.Optional),doc="Output dead time table (optional)")
		self.declareProperty(WorkspaceProperty("ReconstructedSpectra","",direction=Direction.Output,optional=PropertyMode.Optional),doc="Reconstructed time spectra (optional)")
		
	def PyExec(self):
		# logging
		mylog=self.log()
		# progress
		prog=Progress(self,start=0.0,end=1.0,nreports=100)
		#
		ws=self.getProperty("InputWorkspace").value
		RUNDATA_irunno=ws.getRunNumber() # probably not needed, but...
		RUNDATA_frames=ws.getRun().getProperty("goodfrm").value
		RUNDATA_res=(ws.readX(0)[-1]-ws.readX(0)[0])/(len(ws.readX(0)-1.0) # assume linear!
		CHANNELS_itzero=int(-ws.readX(0)/RUNDATA_res+0.5) # note, may be negative for pre-cropped data
		t1stgood=self.getProperty("FirstGoodTime").value
		CHANNELS_i1stgood=max(int(math.floor(t1stgood-ws.readX(0)/RUNDATA_res+1.0)),0)
		FLAGS_fixphase=self.getProperty("FixPhases").value
		FLAGS_fitdead=self.getProperty("FitDeadTime").value
		tlast=self.getProperty("LastGoodTime").value
		ilast=min(int(math.floor(tlast-ws.readX(0)/RUNDATA_res+1.0)),len(ws.readY(0))
		nhisto=ws.getNumberHistograms()
		POINTS_nhists=nhisto
		histlen=ilast-CHANNELS_i1stgood # actual data points to process, including before i1stgood
		# fill rdata with raw counts
		CHANNELS_itotal=histlen
		DATALL_rdata=numpy.zeros([nhisto,ilast])
		for i in range(nhisto):
			rdata[i,:]=ws.readY(i)[:ilast]
		if(self.getProperty("DoublePulse").value):
			PULSES_npulse=2
		else:
			PULSES_npulse=1
		PULSES_def=self.getProperty("DefaultLevel").value
		#
		# note on lengths of transforms, etc:
		# input data has CHANNELS_itotal data points with time spacing RUNDATA_res
		# Frequency spectrum has MAXPAGE_n data points with frequency spacing fperchan
		# maximum frequency fperchan*MAXPAGE_n should be greater than anything expected in the data (or resolved due to pulse width, etc)
		# Frequency spectrum is zero padded to POINTS_npts points and another POINTS_npts negative frequency components added, all of those are zeros
		# Fourier transform performed on POINTS_npts*2 points (signed frequency axis)
		# after transform, only the first CHANNELS_itotal values are compared to the raw data, the others can take any value. (Data set actually padded to POINTS_npts with errors set to 1.E15 beyond good range)
		# length constraints:
		# POINTS_npts >=CHANNELS_itotal and POINTS_npts >= MAXPAGE_n
		# POINTS_npts should be a power of 2 for speed (though numpy.fft.fft() will cope if it isn't)
		# no requirement that CHANNELS_itotal or MAXPAGE_n are sub-multiples of POINTS_npts, or powers of 2 themselves
		# relationship between bin sizes, etc:
		# fperchan=1./(RUNDATA_res*float(POINTS_npts)*2.)
		#
		POINTS_npts=self.getProperty("Npts").value
		# e.g. npts=8192
		# i2pwr=log2(8192)=13
		# in zft and opus: call FFT with i2pwr+1 (=14)
		#in FFT: uses 2**14 points
		# so set I2 to be 2*npts (allows for all the negative ones!)
		if(CHANNELS_itotal > POINTS_npts):
			mylog.notice( "truncating useful data set from {0} to {1} data points".format(CHANNELS_itotal,POINTS_npts))
			CHANNELS_itotal = POINTS_npts # short transform, omit some data points
		SAVETIME_i2=POINTS_npts*2
		maxfield=self.getProperty("MaxField").value
		MAXPAGE_n=int(maxfield*0.01355*2*POINTS_npts*RUNDATA_res) # number of active frequency points, need not now be a power of 2?
		if(MAXPAGE_n<256) MAXPAGE_n=256
		if(MAXPAGE_n>POINTS_npts) MAXPAGE_n=POINTS_npts
		# load grouping. Mantid group table is different: one row per group, 1 column "detectors" with list of values
		RUNDATA_hists=numpy.zeros(nhisto) # not necessary?
		if(self.getProperty("GroupTable").isDefault):
			# no table provided, map 1:1 and use all spectra
			POINTS_ngroups=RUNDATA_hists
			GROUPING_group=np.array(range(POINTS_ngroups))
		else:
			GROUPING_group=np.zeros(nhisto)
			GROUPING_group[:]=-1 # for unused histograms in no group
			POINTS_ngroups=len(self.getProperty("GroupTable").value)
			for g,row in self.getProperty("GroupTable").value:
				for hh in map(int,row["Detectors"].split(",")):
					GROUPING_group[h]=g
		# load dead times (note Maxent needs values per GROUP!)
		# standard dead time table is per detector. Take averages
		SENSE_taud=np.zeros([POINTS_ngroups]) # default zero if not provided
		tmpTaud=[[] for i in range(POINTS_ngroups)]
		if(not self.getProperty("DeadTimeTable").isDefault):
			# load data from standard Mantid dead time table
			for r in self.getProperty("DeadTimeTable").value:
				if GROUPING_group[r[0]] >=0:
					tmpTaud[GROUPING_group[r[0]]].append(r[1])
			for g in range(POINTS_ngroups):
				SENSE_taud[g]=np.mean(tmpTaud[g])
		# sum histograms for total counts (not necessary?)
		# load Phase Table (previously done in BACK.for)
		# default being to distribute phases uniformly over 2pi, will work for 2 groups F,B
		if(self.getProperty("InputPhaseTable").isDefault:
			if(FLAGS_fixphase and POINTS_ngroups>2):
				raise ValueError("Supply phases to fix to")
			filePHASE=np.arange(POINTS_ngroups)*math.pi*2.0/POINTS_ngroups
		else:
			filePHASE=np.zeros([POINTS_ngroups])
			pt=self.getProperty("InputPhaseTable").value
			if(len(pt)==POINTS_ngroups): # phase table for grouped data, or when not grouping
				for r in pt:
					filePHASE[r["Spectrum number"]]=r["Phase"]
			elif (len(pt)==POINTS_nhist): # phase table for ungrouped data. Pick a representative detector for each group (the last one)
				for r in pt:
					filePHASE[GROUPING_group[r["Spectrum number"]]]=r["Phase"]
		#
		# do the work! Lots to pass in and out
		(MISSCHANNELS_mm,RUNDATA_fnorm,RUNDATA_hists,MAXPAGE_f,FAC_factor,FAC_facdef,FAC_facfake,FAC_ratio,
				DETECT_a,DETECT_b,DETECT_c,DETECT_d,DETECT_e,PULSESHAPE_convol,SENSE_taud,FASE_phase,SAVETIME_ngo,
				AMPS_amp,SENSE_phi,OUTSPEC_test,OUTSPEC_guess) =
		MULTIMAX(POINTS_nhists,POINTS_ngroups,POINTS_npts,CHANNELS_itzero,CHANNELS_i1stgood,CHANNELS_itotal,RUNDATA_res,RUNDATA_frames,
						GROUPING_group,DATALL_rdata,FAC_factor,SENSE_taud,SENSE_phi,MAXPAGE_n,filePHASE,PULSES_def,FLAGS_fitdead,FLAGS_fixphase,
						mylog,prog)
		#
		fperchan=1./(RUNDATA_res*float(POINTS_npts)*2.)
		fchan=np.linspace(0.0,MAXPAGE_n*fperchan/135.5e-4 ,MAXPAGE_n,endpoint=False)
		# write results! Frequency spectra
		outSpec=WorkspaceFactory.create("Workspace2D",NVectors=1,XLength=MAXPAGE_n,YLength=MAXPAGE_n)
		outSpec.dataX(0)[:]=fchan
		outSpec.dataY(0)[:]=MAXPAGE_f
		self.setProperty("OutputWorkspace",outSpec)
		# revised dead times
		if(not self.getProperty("OutputDeadTimeTable").isDefault):
			outTaud=WorkspaceFactory.createTable()
			outTaud.addColumn("int","spectrum",1)
			outTaud.addColumn("double","dead-time",2)
			for i in range(POINTS_ngroups):
				outTaud.addRow([i,SENSE_taud[i]])
			self.setProperty("OutputDeadTimeTable",outTaud)
		# revised phases (and amplitudes since they're in the table too)
		if(not self.getProperty("OutputPhaseTable").isDefault):
			outPhase=WorkspaceFactory.createTable()
			outPhase.addColumn("int","Spectrum number",1)
			outPhase.addColumn("double","Asymmetry",1)
			outPhase.addColumn("double","Phase",1)
			for i in range(POINTS_ngroups):
				outPhase.addRow([i,AMPS_amp[i],SENSE_phi[i]])
			self.setProperty("OutputPhaseTable",outPhase)
		# reconstructed spectra passed back from OUTSPEC
		if(not self.property("ReconstructedSpectra").isDefault):
			i2=CHANNELS_itotal
			i1=CHANNELS_i1stgood # channel range in guess, etc (t0 at start)
			k1=i1+CHANNELS_itzero
			k2=i2+CHANNELS_itzero # channel range in source workspace accounting for instrumental t0
			recSpec=WorkspaceFactory.create(ws,NVectors=POINTS_ngroups,XLength=i2-i1+1,YLength=i2-i1)
			for j in range(POINTS_ngroups):
				recSpec.dataX(j)[:]=ws.dataX(j)[k1:k2+1]
				recSpec.dataY(j)[:]=OUTSPEC_guess[i1:i2,j]
			self.setProperty("ReconstructedSpectra"),recSpec

AlgorithmFactory.subscribe(OpengenieMaxent)
