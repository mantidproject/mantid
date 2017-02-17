== Summary ==

Multiple file loading allows users to specify lists / ranges of files to load (and optionally sum together) into Mantid.

This functionality is offered via the "Filename" property of [[Load]], and so is available by calling the algorithm or by using the LoadDialog window in the usual way.

== Syntax ==

=== Basic ===

The syntax for multi file loading involves the use of several context-sensitive operators.  Here is a run-down of those operators with some simple examples:

{| class="wikitable"
!rowspan="2" |Name
!width=200 rowspan="2" |Usage
!rowspan="2" |Description
!colspan="2" |Example
|-
!Input
!Result
|-
!align="center" |List
|align="center" |<code><run>,<run></code>
|Used to list runs.
|<code>INST1,2,3.ext</code>
|Load runs 1, 2 and 3.
|-
!align="center" |Plus
|align="center" |<code><run>+<run></code>
|Used to specify which runs that are to be loaded and then summed together.
|<code>INST1+2+3.ext</code>
|Load and sum runs 1, 2 and 3.
|-
!align="center" |Range
|align="center" |<code><run>:<run></code>
|Used to specify a range of runs to load.
|<code>INST1:4.ext</code>
|Load runs 1, 2, 3 and 4.
|-
!align="center" |Stepped Range
|align="center" |<code><run>:<run>:<step_size></code>
|Used to specify a ''stepped'' range of runs to load.
|<code>INST1:5:2.ext</code>
|Load runs 1, 3 and 5.
|-
!align="center" |Added Range
|align="center" |<code><run>-<run></code>
|Used to specify a range of runs that are to be loaded and then summed together.
|<code>INST1-4.ext</code>
|Load and then sum runs 1, 2, 3 and 4.
|-
!align="center" |Stepped Added Range
|align="center" |<code><run>-<run>:<step_size></code>
|Used to specify a ''stepped'' range of runs that are to be loaded and then summed together.
|<code>INST1-5:2.ext</code>
|Load and then sum runs 1, 3 and 5.
|}

=== Optional Info ===

Some information relating to the files can be left out, and the algorithm will attempt to fill in the details:

* '''Directory''' - Note that in the examples table above, the directory of the files have not been provided.  In cases such as this, standard Mantid behaviour is observed and the files will be loaded as long as they exist in the folders specified in your [[ManageUserDirectories]] list.

* '''Instrument''' - If the instrument is not specified then runs from your chosen default instrument will be loaded.

* '''File Extension''' - If the file extension is not specified, then Mantid will look for runs amongst the files with the standard extensions (.raw, .nxs, etc).  '''Note:''' Since Load does not currently support multiple loaders at the same time (see [[#Limitations]]) if you specify multiple runs without an extension, then Mantid will use the first resolved extension for the remaining files.  If some files have a specified extension but others dont, then the first extension that has been specified will be used for all files without a given extension.

* '''Zero Padding''' - There is some leeway regarding the number of zeroes expected in the run numbers for each instrument.  If you are trying to load the file <code>INST000001.ext</code>, then <code>INST1.ext</code> is an acceptable input.

=== Advanced ===

The basic syntax outlined above can be combined in a variety of ways:

<div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">
<source lang="python">

# Loads the Raw files 1, 2, 3, and 4 for TOSCA, but sums together runs 3 and 4.
Load(Filename='TSC1,2,3+4.raw', OutputWorkspace='Files')

# Loads the Raw files 1, 2 and 3 for TOSCA and 4, 5 and 6 for IRIS.
# The IRIS runs are added together.
Load(Filename='TSC1:3.raw,IRS4-6.raw', OutputWorkspace='Files')

# For TOSCA, adds together run 1 (found in c:/files/) and run 2 (found in c:/store/).
Load(Filename='c:/files/TSC1.raw+c:/store/TSC2.raw', OutputWorkspace='Files')

</source></div>

== Load Dialog ==

When you load a range of files from the Load Dialog and then reopen the dialog, the files that were loaded previously will now appear in a comma and plus separated list of fully resolved filenames.

== Limitations ==

* Currently, Load can only handle multiple files using a single loader at a time.  For example, loading a NeXuS file and a Raw file at the same time is not possible.
* It is not possible to plus together Ranges, or Stepped Ranges.  (Added Ranges and Stepped Added Ranges may be plussed however.)
* Files that are loaded along with other files will end up with the same algorithm history.  Generating a Python script for any of the workspaces, and then rerunning it will result in ''all'' the workspaces being loaded.
* The syntax for the different kind of ranges can only be used between run numbers, and '''not''' between fully or partially resolved files.  For example, <code>TSC1.raw-TSC3.raw</code> is not allowed; use <code>TSC1-3.raw</code> instead.
* For the ILL data, since the file names (numors) do not contain instrument prefixes, multiple file loading will work only if the default facility and instrument are set correspondingly from the  [http://www.mantidproject.org/MBC_Getting_set_up#MantidPlot_First-Time_Setup First Time Setup] page.
