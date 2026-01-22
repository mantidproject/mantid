# Doxygen Setup

## Unix Console Doxygen Setup

<table>
<colgroup>
<col style="width: 16%" />
<col style="width: 83%" />
</colgroup>
<tbody>
<tr class="odd">
<td><div class="line-block">Check for<br />
doxygen</div></td>
<td>You may well already have doxygen installed but is it most likely in
your systems repositories. If not, build from source <a
href="http://www.stack.nl/~dimitri/doxygen/download.html#srcbin">here</a>.</td>
</tr>
<tr class="even">
<td>Run cmake</td>
<td>CMake will genereate the doyxgen config file in
${CMAKE_DIR}/Framework/Doxygen/Mantid.doxyfile</td>
</tr>
<tr class="odd">
<td><div class="line-block">You're done!<br />
Try!</div></td>
<td><blockquote>
<ul>
<li>Type 'make doxygen'</li>
<li>This will run doxygen, showing the output in the console. You may
want to pipe warnings to a file to make them easy to read later: 'make
doxygen 2> doxygen_errors.log'</li>
<li>The documentation will go into a subdir doxygen/html of the
directory where cmake was run from.</li>
</ul>
</blockquote></td>
</tr>
</tbody>
</table>

## Visual Studio Doxygen Setup

<table>
<colgroup>
<col style="width: 16%" />
<col style="width: 83%" />
</colgroup>
<tbody>
<tr class="odd">
<td>Install doxygen binaries</td>
<td>Download the <a
href="http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc">Windows
binaries</a> and install them. I'll assume in the following you
installed doxygen in c:program filesdoxygen</td>
</tr>
<tr class="even">
<td>Rerun CMake</td>
<td>Run cmake for the build to ensure that the Mantid.doxyfile is
created</td>
</tr>
<tr class="odd">
<td>Add VC++ Tool: "DoxyGen"</td>
<td><ul>
<li>ToolsExternal Tool then click Add</li>
<li>Title: &DoxyGen</li>
<li>Command: C:Program FilesDoxygenbindoxygen.exe</li>
<li>Arguments: "$(SolutionDir)FrameworkDoxygenMantid.doxyfile" (include
the quotes!)</li>
<li>Initial Directory: $(SolutionDir)Build</li>
<li>Check the "Use output window" box</li>
</ul></td>
</tr>
<tr class="even">
<td>Add VC++ Tool: "view DoxyGen"</td>
<td><ul>
<li>ToolsExternal Tool then click Add</li>
<li>Title: &View DoxyGen</li>
<li>Command your favorite browser, e.g. C:program Filesinternet
Exploreriexplore.exe or C:Program Files
(x86)GoogleChromeApplicationchrome.exe</li>
<li>Arguments: "$(SolutionDir)doxygenhtmlindex.html"</li>
<li>Initial Directory: leave empty</li>
</ul></td>
</tr>
<tr class="odd">
<td>You're done! Try! "DoxyGen"</td>
<td><ul>
<li>Choose Tools/DoxyGen from the menu, and watch the magic happen
(DoxyGen will log it's progress and complaints to the output window).
Clicking on a warning message will take you to the location in the code
of the warning.</li>
<li>Choose Tools/View DoxyGen to explore the documentation.</li>
<li>The "Main Page" is probably rather boring. Click on "Namespaces" in
the menu line to browse your classes etc.</li>
</ul></td>
</tr>
</tbody>
</table>

## Eclipse Doxygen Setup

<table>
<colgroup>
<col style="width: 16%" />
<col style="width: 83%" />
</colgroup>
<tbody>
<tr class="odd">
<td>Check for doxygen</td>
<td>You may well already have doxygen installed, but if not you can
install it at the same time as the plugin below via the update site</td>
</tr>
<tr class="even">
<td>Run cmake</td>
<td>This will generate the doxygen config file in
${CMake_DIR}/Framework/Doxygen/Mantid.doxygen</td>
</tr>
<tr class="odd">
<td>Install Eclipse plugin: "Eclox"</td>
<td><ul>
<li><a href="http://eclox.eu/">Eclox</a> is a frontend plugin for
Eclipse.</li>
<li>Install it using the Eclipse Update Manager</li>
<li>To do this go to Help -> Software Updates...</li>
<li>Select the 'Available Software' tab then the 'Add Site...'
button</li>
<li>Enter <span
class="title-ref">http://download.gna.org/eclox/update</span> as the
location</li>
<li>Eclipse will add the site to the list and you can open the tree to
select and install Eclox</li>
</ul></td>
</tr>
<tr class="even">
<td>You're done! Try!</td>
<td><ul>
<li>You'll now have a 'build doxygen' button in your toolbar (a blue
'@')</li>
<li>The first time you click it you'll be prompted for the configuration
file. Point it at ${CMake_DIR}/Framework/Doxygen/Mantid.doxygen</li>
<li>This will run doxygen, showing the output in the console and adding
warnings symbols on the source files (as for compilation warnings).
Hovering over these will show the warning.</li>
<li>The documentation will go into a subdir doxygen/html of the
directory where cmake was run from.</li>
</ul></td>
</tr>
</tbody>
</table>
