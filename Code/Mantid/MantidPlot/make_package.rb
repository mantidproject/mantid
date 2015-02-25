#!/usr/bin/env ruby
#This loop changes the linking from /usr/local/lib to @rpath

require 'pathname'

lib_dir = Pathname.new("/usr/local/lib")
openssl_dir = Pathname.new("/usr/local/opt/openssl/lib")

#filenames with path for all shared libraries used by MantidPlot and its dependencies.
library_filenames = ["libboost_regex-mt.dylib",
                     "libboost_date_time-mt.dylib",
                     "libboost_python-mt.dylib",
                     "libgsl.dylib",
                     "libgslcblas.dylib",
                     "libjsoncpp.dylib",
                     "libmuparser.dylib",
                     "libNeXus.dylib",
                     "libNeXusCPP.dylib",
                     "libPocoFoundation.dylib",
                     "libPocoUtil.dylib",
                     "libPocoXML.dylib",
                     "libPocoNet.dylib",
                     "libPocoCrypto.dylib",
                     "libPocoNetSSL.dylib",
                     "libTKernel.dylib",
                     "libTKBO.dylib",
                     "libTKernel.dylib",
                     "libTKBO.dylib",
                     "libTKPrim.dylib",
                     "libTKMesh.dylib",
                     "libTKBRep.dylib",
                     "libTKGeomAlgo.dylib",
                     "libTKTopAlgo.dylib",
                     "libTKMath.dylib",
                     "libTKG2d.dylib",
                     "libTKG3d.dylib",
                     "libTKGeomBase.dylib",
                     "libqwt.dylib",
                     "libqwtplot3d.dylib",
                     "libqscintilla2.dylib",
                     "libmxml.dylib",
                     "libhdf5.dylib",
                     "libhdf5_hl.dylib",
                     "libmfhdf.dylib",
                     "libdf.dylib",
                     "libsz.dylib",
                     "libjpeg.dylib",
                     "libssl.dylib",
                     "libcrypto.dylib"]

#This copies the libraries over, then changes permissions and the id from /usr/local/lib to @rpath
library_filenames.each do |filename|
    if filename.include? "libssl.dylib"
        `cp #{openssl_dir+filename} Contents/MacOS/`
    elsif  filename.include? "libcrypto.dylib"
        `cp #{openssl_dir+filename} Contents/MacOS/`
    else
        `cp #{lib_dir+filename} Contents/MacOS/`
    end
    `chmod +w Contents/MacOS/#{filename}`
    `install_name_tool -id @rpath/#{filename} Contents/MacOS/#{filename}`
end

#use install_name_tool to change dependencies form /usr/local to libraries in the package.
search_patterns = ["**/*.dylib","**/*.so","**/MantidPlot"]
search_patterns.each do |pattern|
    Dir[pattern].each do |library|
        dependencies = `otool -L #{library}`
        dependencies.split("\n").each do |dependency|
            currentname = dependency.strip.split(" ")
            name_split_on_slash = currentname[0].strip.split("/")
            name_split_on_period = name_split_on_slash[-1].split(".")
            prefix = name_split_on_period[0]+"."
            library_filenames.each do |filename|
                basename = File.basename(filename,"dylib")
                if prefix == basename
                    `install_name_tool -change #{currentname[0]} @rpath/#{basename+"dylib"} #{library}`
                end
            end
        end
    end
end

#We'll use macdeployqt to fix qt dependencies.
`macdeployqt ../MantidPlot.app`

#fix remaining QT linking issues
`install_name_tool -change /usr/local/lib/Qt3Support.framework/Versions/4/Qt3Support @loader_path/../Frameworks/Qt3Support.framework/Versions/4/Qt3Support Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtSvg.framework/Versions/4/QtSvg @loader_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtXml.framework/Versions/4/QtXml @loader_path/../Frameworks/QtXml.framework/Versions/4/QtXml Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtSql.framework/Versions/4/QtSql @loader_path/../Frameworks/QtSql.framework/Versions/4/QtSql Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtNetwork.framework/Versions/4/QtNetwork @loader_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtHelp.framework/Versions/4/QtHelp @loader_path/../Frameworks/QtHelp.framework/Versions/4/QtHelp  Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtWebKit.framework/Versions/4/QtWebKit @loader_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit Contents/MacOS/mantidqtpython.so`

`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/libqwtplot3d.dylib`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqwtplot3d.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqwtplot3d.dylib`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqwt.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqwt.dylib`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqscintilla2.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqscintilla2.dylib`

`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqscintilla2.dylib`
`install_name_tool -id @rpath/libqsqlite.dylib Contents/Frameworks/plugins/sqldrivers/libqsqlite.dylib`

#change id of all Qt4 imageformats libraries
qt4_patterns = ["**/imageformats/*.dylib"]
qt4_patterns.each do |pattern|
  Dir[pattern].each do |library|
    basename =  File.basename(library)
    `chmod +w Contents/Frameworks/plugins/imageformats/#{basename}`
    `install_name_tool -id @rpath/#{basename} Contents/Frameworks/plugins/imageformats/#{basename}`
  end
end

#change id of all PyQt4 libraries
pyqt4_patterns = ["**/PyQt4/*.so"]
pyqt4_patterns.each do |pattern|
  Dir[pattern].each do |library|
    basename =  File.basename(library)
    `chmod +w Contents/MacOS/PyQt4/#{basename}`  
    `install_name_tool -id @rpath/#{basename} Contents/MacOS/PyQt4/#{basename}`
  end
end

#fix PyQt4 and Qt4 linking issues
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtCore.so`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtGui.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtGui.so`

`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/PyQt4/QtOpenGL.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtOpenGL.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtOpenGL.so`

`install_name_tool -change /usr/local/lib/QtSql.framework/Versions/4/QtSql @loader_path/../../Frameworks/QtSql.framework/Versions/4/QtSql Contents/MacOS/PyQt4/QtSql.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtSql.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtSql.so`

`install_name_tool -change /usr/local/lib/QtSvg.framework/Versions/4/QtSvg @loader_path/../../Frameworks/QtSvg.framework/Versions/4/QtSvg Contents/MacOS/PyQt4/QtSvg.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtSvg.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtSvg.so`

`install_name_tool -change /usr/local/lib/QtXml.framework/Versions/4/QtXml @loader_path/../../Frameworks/QtXml.framework/Versions/4/QtXml Contents/MacOS/PyQt4/QtXml.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtXml.so`

#Copy over python libraries not included with OSX. 
`cp -r /Library/Python/2.7/site-packages/sphinx Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/sphinx_bootstrap_theme Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/IPython Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/zmq Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/pygments Contents/MacOS/`

#add other dependencies found in current package
#currently missing epics
`cp /Library/Python/2.7/site-packages/gnureadline.so Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/readline.py Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/readline.pyc Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/pyparsing.py Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/pyparsing.pyc Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/_markerlib/ Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/backports Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/certifi Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/tornado Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/markupsafe Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/jinja2 Contents/MacOS/`
`cp -r /usr/local/lib/python2.7/site-packages/nxs Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/psutil Contents/MacOS/`
`mkdir Contents/MacOS/bin`
`cp /usr/local/bin/ipython Contents/MacOS/bin/`

# current .pyc files have permissions issues. These files are recreated by CPack.
`rm Contents/MacOS/nxs/*.pyc`

#Lastly check for any libraries in the package linking against homebrew libraries.
search_patterns.each do |pattern|
  Dir[pattern].each do |library|
    dependencies = `otool -L #{library}`
    dependencies.split("\n").each do |dependency|
      if dependency.include? "/usr/local/"
         p "issue with library: #{library} linked against: #{dependency}"
      end
    end
  end
end
