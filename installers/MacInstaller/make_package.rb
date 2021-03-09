#! /usr/bin/env ruby
# frozen_string_literal: true

# Fixes up the bundle produced by CPack.
# It assumes all dependencies are provided by homebrew and the main bundle
# layout has been created

# Modules
require 'fileutils'
require 'open3'
require 'pathname'

#---------------------------------------------------------
# Constants
#---------------------------------------------------------
AT_EXECUTABLE_TAG = '@executable_path'
AT_LOADER_TAG = '@loader_path'
AT_RPATH_TAG = '@rpath'
# Collection modules to copy from system installation
# Required to install other packages with pip
BUNDLED_PY_MODULES_COMMON = [
  'easy_install.py',
  'pip',
  'pip*.*-info',
  'pkg_resources',
  'setuptools',
  'setuptools*.*-info',
  'wheel',
  'wheel*.*-info'
].freeze
# Brew Python packages to be copied to bundle
BUNDLED_PY_MODULES_MANTIDPLOT = [
  'PyQt4/__init__.py',
  'PyQt4/Qt.so',
  'PyQt4/QtCore.so',
  'PyQt4/QtGui.so',
  'PyQt4/QtOpenGL.so',
  'PyQt4/QtSql.so',
  'PyQt4/QtSvg.so',
  'PyQt4/QtXml.so',
  'PyQt4/sip.so',
  'PyQt4/uic'
].freeze
BUNDLED_PY_MODULES_WORKBENCH = [
  'PyQt5/__init__.py',
  'PyQt5/Qt.so',
  'PyQt5/QtCore.so',
  'PyQt5/QtGui.so',
  'PyQt5/QtOpenGL.so',
  'PyQt5/QtPrintSupport.so',
  'PyQt5/QtSql.so',
  'PyQt5/QtSvg.so',
  'PyQt5/QtTest.so',
  'PyQt5/QtWidgets.so',
  'PyQt5/QtXml.so',
  'PyQt5/sip.so',
  'PyQt5/uic',
].freeze
REQUIREMENTS_FILE = Pathname.new(__dir__) + 'requirements.txt'
REQUIREMENTS_WORKBENCH_FILE = Pathname.new(__dir__) + 'requirements-workbench.txt'
SITECUSTOMIZE_FILE = Pathname.new(__dir__) + 'sitecustomize.py'
DEBUG = 1
FRAMEWORK_IDENTIFIER = '.framework'
HOMEBREW_PREFIX = '/usr/local'
MANTID_PY_SO = ['_api.so', '_geometry.so', '_kernel.so'].freeze
QT4_PLUGINS_DIR = Pathname.new('/usr/local/opt/qt@4/lib/qt4/plugins')
QT5_PLUGINS_DIR = Pathname.new('/usr/local/opt/qt/plugins')
QT_PLUGINS_COMMON = ['imageformats', 'sqldrivers', 'iconengines'].freeze
QT_PLUGINS_BLACKLIST = ['libqsqlpsql.dylib'].freeze
QT_CONF = '[Paths]
Plugins = PlugIns
Imports = Resources/qml
Qml2Imports = Resources/qml
'

#---------------------------------------------------------
# Utility functions
#---------------------------------------------------------
# indentation level for output
$INDENT = 0
# cache of library ids
$ID_CACHE = {}

# Display a message and exits with a status 1
# Params:
# +msg+:: String to display
def fatal(msg)
  puts msg
  exit 1
end

# Display a message if DEBUG=1
# Params:
# +msg+:: String to display
def debug(msg)
  puts ' ' * $INDENT + msg.to_s if DEBUG == 1
end

# Executes a command and fails if the command fails
# Params:
# +cmd+:: String giving command
# return output string
def execute(cmd, ignore_fail = false)
  debug("Executing command #{cmd}")
  stdout, stderr, status = Open3.capture3(cmd)
  fatal("Command #{cmd} failed!\n" + stderr) if !ignore_fail && status != 0
  stdout
end

# Determine the version of python
# Return 3-array of major,minor,patch
def python_version(py_exe)
  # expects Python X.Y.Z
  version_info = execute(py_exe + ' --version')
  version_number_str = version_info.split()[1]
  # map to 3 integers
  return version_number_str.split('.').map { |x| x.to_i }
end

# Deploy the embedded Python bundle to the Frameworks subdirectory
# Params:
# +destination+:: Destination directory for bundle
# +host_python_exe+:: Executable of Python bundle to copy over
# +bundled_packages+:: A list of packages that should be bundled
# +requirements_files+:: A list of requirements files to install additional packages
# returns the bundle site packages directory
def deploy_python_framework(destination, host_python_exe,
                            bundled_packages,
                            requirements_files)
  host_py_home = host_python_exe.realpath.parent.parent
  py_ver = host_py_home.basename
  bundle_python_framework = destination + 'Frameworks/Python.framework'
  bundle_py_home = bundle_python_framework + "Versions/#{py_ver}"
  deployable_assets = [
    'Python', "bin/python#{py_ver}", "bin/2to3-#{py_ver}",
    "include/python#{py_ver}", "lib/python#{py_ver}", 'Resources'
  ]

  deployable_assets.each do |asset|
    src_path = host_py_home + asset
    dest_path = bundle_py_home + asset
    FileUtils.makedirs(dest_path.parent)
    if src_path.file?
      FileUtils.cp src_path, dest_path,
                   preserve: true
    elsif src_path.directory?
      FileUtils.cp_r host_py_home + asset, dest_path,
                     preserve: true
    end
  end

  # fixup bundled python exe/libraries
  Dir["#{bundle_python_framework}/Versions/*/bin/python*",
      "#{bundle_python_framework}/Versions/*/Resources/Python.app/Contents/MacOS/Python"].each do |path|
    pathname = Pathname.new(path)
    next if pathname.directory?

    find_dependencies(pathname).each do |dependency|
      next unless (dependency.start_with?(HOMEBREW_PREFIX))

      newpath = if path.include?('Python.app')
                  '@loader_path/../../../../Python'
                else
                  '@loader_path/../Python'
                end
      change_dependent_path(path, dependency, newpath)
    end
  end

  # add relative symlink for unversioned paths if they don't exist
  Dir.chdir(bundle_py_home + 'bin') do
    FileUtils.ln_s "python#{py_ver}", "python"
    FileUtils.ln_s "2to3-#{py_ver}", "2to3"
  end

  # add python symlink to MacOS for easier command-line access
  contents_macos = destination + 'MacOS'
  Dir.chdir(contents_macos) do
    py_exe = Pathname.new("#{bundle_python_framework}/Versions/#{py_ver}/bin/python#{py_ver}")
    FileUtils.ln_s "#{py_exe.relative_path_from(contents_macos)}", "python"
  end

  # remove Info.plist files so outer application controls app display name
  FileUtils.rm "#{bundle_py_home}/Resources/Info.plist"
  FileUtils.rm "#{bundle_py_home}/Resources/Python.app/Contents/Info.plist"
  
  # remove site-packages symlink, copy brew python modules and pip install the rest
  src_site_packages = Pathname.new("#{host_py_home}/lib/python#{py_ver}/site-packages")
  bundle_site_packages = Pathname.new("#{bundle_py_home}/lib/python#{py_ver}/site-packages")
  FileUtils.rm bundle_site_packages
  FileUtils.mkdir bundle_site_packages
  copy_selection_recursive(bundled_packages, src_site_packages,
                           bundle_site_packages)
  make_writable(bundle_site_packages)
  # remove distutils.cfg so pip paths are computed relative to the sys.prefix
  FileUtils.rm Pathname.new("#{bundle_py_home}/lib/python#{py_ver}/distutils/distutils.cfg")
  requirements_files.each do |requirements|
    stdout = execute("#{bundle_py_home}/bin/python -m pip install -r #{requirements}")
    debug(stdout)
  end

  # fix mpl_toolkit if it is missing __init__
  mpltoolkit_init =
    FileUtils.touch "#{bundle_site_packages}/mpl_toolkits/__init__.py"

  # add sitecustomize module
  FileUtils.cp SITECUSTOMIZE_FILE, bundle_site_packages,
               preserve: true
  
  bundle_site_packages
end

# Copies, recursively, the selected list of packages from the
# src to the destination. The destination must already exist
# Params:
# +packages+:: A list of items in src_dir to copy
# +src_dir+:: Source directory containing above packages
# +dest_dir+:: Destination directory
def copy_selection_recursive(packages, src_dir, dest_dir)
  packages.each do |package|
    package_dir = Pathname.new(package).dirname
    if package_dir == Pathname.new('.')
      destination = dest_dir
    else
      destination = dest_dir + package_dir
      FileUtils.makedirs destination
    end

    # use cp rather than FileUtils as cp will automatically follow symlinks
    execute("cp -r #{src_dir + package} #{destination}")
  end
end

# Install requested Qt plugins to bundle
# Params:
# +bundle_path+:: Root of the bundle
# +bundled_qt_plugins+:: List of plugin directories to install
# +host_qt_plugins_dir+:: Source directory containing the plugin subdirectories
# +blacklist+:: An optional blacklist to exclude
def install_qt_plugins(bundle_path, bundled_qt_plugins, host_qt_plugins_dir,
                       blacklist = [])
  contents_plugins = bundle_path + 'Contents/PlugIns'
  FileUtils.makedirs(contents_plugins)
  bundled_qt_plugins.each do |plugin|
    src_path = host_qt_plugins_dir + plugin
    # We assume each plugin is a directory. Ensure the
    # destination directory exists so cp_r copies the contents
    # into the subdirectory and not the parent
    FileUtils.cp_r src_path, contents_plugins,
                   preserve: true
    make_writable(contents_plugins + plugin)
    blacklist.each do |blacklisted|
      blacklisted_path = contents_plugins + plugin + blacklisted
      FileUtils.rm blacklisted_path if blacklisted_path.file?
    end
  end
  contents_resources = bundle_path + 'Contents/Resources'
  FileUtils.makedirs(contents_resources)
  File.open(contents_resources + 'qt.conf', 'w') do |file|
    file.write(QT_CONF)
  end
end

# Make a file writable
# Params:
# +path+:: Path to make wrtiable
def make_writable(path)
  if path.file?
    FileUtils.chmod 'u+w', path
  elsif path.directory?
    FileUtils.chmod_R 'u+w', path
  end
end

# Get the id field
def get_id(path)
  $ID_CACHE.fetch(path) do |_key|
    out = execute("otool -l #{path} | grep -A 2 LC_ID_DYLIB",
                  ignore_fail: true)
    id = if out != ''
           out.split("\n")[-1].split[1]
         else
           ''
         end
    $ID_CACHE[:path] = id
  end
end

# Set the id field of a binary
# Params:
# +path+:: Path to binary
# +id+:: New ID
def set_id(path, id)
  debug("Setting id to #{id} on #{path}")
  execute("install_name_tool -id #{id} #{path}")
end

# Change dependent path in binary
# Params:
# +target+: Path to binary
# +old+: Current path
# +new+: New path
def change_dependent_path(path, old, new)
  debug("Changing #{old} to #{new} on #{path}")
  execute("install_name_tool -change #{old} #{new} #{path}")
end

# Fixup all binaries located from the given basepath
# Params:
# +basepath+:: Root of search
# +destination+:: Destination for dependent libraries
# +extra+:: Optional list of extra binaries
def fixup_binaries(basepath, destination, extra = [])
  search_patterns = ["#{basepath}/**/*.dylib",
                     "#{basepath}/**/*.so"]
  search_patterns += extra
  search_patterns.each do |pattern|
    Dir[pattern].each do |library|
      fixup_binary(Pathname.new(library), Pathname.new(library),
                   destination)
    end
  end
end

# Fixup a given library. Change dependent paths, copy in dependencies
# Params:
# +library+:: Library path
# +library_orig+:: Path to the original library before bundling
# +destination+:: Destination for dependent libraries
def fixup_binary(library, library_orig, destination)
  debug("Fixing #{library}")
  set_id(library, "@rpath/#{library.basename}")
  is_brew_library = library_orig.to_s.start_with?(HOMEBREW_PREFIX)

  $INDENT += 2
  find_dependencies(library).each do |dependency|
    debug("Processing dependency: #{dependency}")
    # assume frameworks/mantid are already deployed
    next if is_mantid_library(dependency)

    if dependency.include?(FRAMEWORK_IDENTIFIER)
      if dependency.include?('Python') # already deployed
        change_dependent_path(library, dependency,
                              bundle_framework_path(destination, library,
                                                    dependency))
      elsif dependency.start_with?(HOMEBREW_PREFIX)
        deploy_framework(Pathname.new(dependency), library, library_orig,
                         destination)
      end
    elsif dependency.start_with?(HOMEBREW_PREFIX)
      deploy_dependency(Pathname.new(dependency), library,
                        library_orig, destination)
    elsif is_brew_library && (dependency.start_with?(AT_LOADER_TAG) ||
                               dependency.start_with?(AT_RPATH_TAG))
      unless (destination + Pathname.new(dependency).basename).file?
        rel_path = dependency.split('/')[1..-1].join('/')
        dependency = library_orig.parent + Pathname.new(rel_path)
        if dependency.file?
          deploy_dependency(Pathname.new(dependency), library,
                            library_orig, destination)
        else
          fatal("Cannot find dependency #{dependency} from #{library_orig}")
        end
      end
    end
  end
  $INDENT -= 2

  debug("\n")
end

# Is the given library a mantid library
# Params:
# +path+:: Path to library
# returns true if this is a mantid library
def is_mantid_library(path)
  return true if path.include?('Mantid')

  basename = Pathname.new(path).basename
  return true if MANTID_PY_SO.include?(basename.to_s)

  false
end

# Compute the internal path to the framework given a dependency
# Params:
# +library+:: Library path with this dependency
# +dependency+:: Pathname object to framework dependency in bundle
def bundle_framework_path(destination, library, dependency)
  components = dependency.split('/')
  framework_name_start = -1
  components.each_with_index do |component, i|
    framework_name_start = i if component.include?(FRAMEWORK_IDENTIFIER)
  end
  rel_path = destination.relative_path_from(library.parent)
  '@loader_path/' + rel_path.to_s + '/' +
    components[framework_name_start..-1].join('/')
end

# Params:
# +dependency+:: Pathname object of a dependency of a library in the bundle
# +library+:: The library that has this dependency
# +library_orig+:: Path to the original library before bundling
# +destination+:: Destination for dependent library
def deploy_framework(dependency, library, _library_orig, destination)
  dep_framework_top = nil
  dependency.descend do |component|
    if component.to_s.include?(FRAMEWORK_IDENTIFIER)
      dep_framework_top = component
      break
    end
  end

  dependency_target = destination + dependency.relative_path_from(dep_framework_top.parent)
  dependency_target_dir = dependency_target.parent
  basename = dependency_target.basename
  if !dependency_target.exist?
    FileUtils.makedirs(dependency_target.parent)
    # library itself
    FileUtils.copy dependency, dependency_target
    make_writable(dependency_target)
    set_id(dependency_target, "@rpath/#{basename}")
    fixup_binary(dependency_target, dependency, destination)
    # symlinks
    bundle_framework_root = dependency_target_dir.parent.parent
    version = dependency_target.parent.basename
    Dir.chdir(bundle_framework_root + 'Versions') do
      FileUtils.ln_s "#{version}", 'Current'
    end
    Dir.chdir(bundle_framework_root) do
      FileUtils.ln_s "Versions/Current/#{basename}", "#{basename}"
    end

    # resources
    src_resources = dependency.parent + 'Resources'
    if src_resources.exist?
      execute("cp -r #{src_resources} #{dependency_target_dir}")
      Dir.chdir(bundle_framework_root) do
        FileUtils.ln_s 'Versions/Current/Resources', 'Resources'
      end
    end

    # helpers
    src_helpers = dependency.parent + 'Helpers'
    if src_helpers.exist?
      execute("cp -r #{src_helpers} #{dependency_target_dir}")
      Dir.chdir(bundle_framework_root) do
        FileUtils.ln_s 'Versions/Current/Helpers', 'Helpers'
      end
      Dir[dependency_target_dir + 'Helpers/**/MacOS/*'].each do |binary|
        binary = Pathname.new(binary)
        make_writable(binary)
        set_id(dependency_target, "@rpath/#{binary.basename}")
        fixup_binary(binary, binary, destination)
      end
    end
  end

  rel_path = dependency_target.parent.relative_path_from(library.parent)
  new_path = '@loader_path/' + rel_path.to_s + "/#{basename}"
  change_dependent_path(library, dependency, new_path)
end

# Deploy a dependency in to the bundle. Copies the dependency into the bundle,
# sets the id and changes any internal paths to point to the bundle. Also fixes
# up any dependencies of the dependency itself.
# Params:
# +dependency+:: Pathname object of a dependency of a library in the bundle
# +library+:: The library that has this dependency
# +library_orig+:: Path to the original library before bundling
# +destination+:: Destination for dependent library
def deploy_dependency(dependency, library, _library_orig,
                      destination)
  basename = dependency.basename
  dependency_target = destination + Pathname.new(basename)

  if !dependency_target.exist?
    FileUtils.copy dependency, dependency_target
    make_writable(dependency_target)
    set_id(dependency_target, "@rpath/#{basename}")
    fixup_binary(dependency_target, dependency, destination)
  end
  rel_path = destination.relative_path_from(library.parent)
  new_path = '@loader_path/' + rel_path.to_s + "/#{basename}"
  change_dependent_path(library, dependency, new_path)
end

# Create a list of dependencies for the given binary that only includes
# non-system dependencies
# Params:
# +path+:: Path to Mach-O binary
def find_dependencies(path)
  path = Pathname.new(path)
  return [] unless path.file?

  id = get_id(path)
  # the first line is the path given to otool
  otool_lines = execute("otool -L #{path}").split("\n")[1..-1]
  enumerator = Enumerator.new do |yielder|
    otool_lines.each do |dependency_line|
      dependency = dependency_line.strip.split(' ')[0]
      next if dependency == id

      yielder.yield dependency
    end
  end
  enumerator
end

# Strip any library version from the filename leaving just .dylib
# Params:
# +filename+:: The original name of the library
def strip_version(filename)
  parts = filename.to_s.split('.')
  parts[0] + '.' + parts[-1]
end

# Check that all binaries have their dependencies either in the
# bundle or are system dependenies
# Params:
# +basepath+:: Root of bundle
def stop_if_bundle_not_self_contained(basepath, extra = [])
  search_patterns = ["#{basepath}/**/*.dylib",
                     "#{basepath}/**/*.so"]
  search_patterns += extra
  problem_libraries = []
  search_patterns.each do |pattern|
    Dir[pattern].each do |library|
      find_dependencies(Pathname.new(library)).each do |dependency|
        if dependency.include?(HOMEBREW_PREFIX)
          problem_libraries << library
          break
        end
      end
    end
  end

  unless problem_libraries.empty?
    msg = 'The bundle is not self contained. The following '\
          "libraries link to others outside of the bundle:\n"\
          "#{problem_libraries.join('\n')}"
    fatal(msg)
  end
end

#---------------------------------------------------------
# Main script
#---------------------------------------------------------
if (ARGV.length != 2)
  puts 'Usage: make_package bundle-path python-exe'
  puts '  - bundle-path: Path of bundle to fix'
  puts '  - python-exe: Path to Python executable to bundle. The whole Python.framework is bundled.'
  exit 1
end

# Host paths
host_python_exe = Pathname.new(ARGV[1])
fatal("Python executable #{python_exe} not found") unless host_python_exe.exist?

# Bundle paths
bundle_path = Pathname.new(ARGV[0])
contents = bundle_path + 'Contents'
contents_macos = bundle_path + 'Contents/MacOS'
contents_frameworks = bundle_path + 'Contents/Frameworks'
# additional executables not detectable by dependency analysis
executables = ["#{contents_macos}/MantidNexusParallelLoader"]

# Create list of packages to bundle
python_version_full = python_version(host_python_exe.to_s)
python_version_major = python_version_full[0]
python_version_minor = python_version_full[1]
so_suffix = ''

bundled_packages = BUNDLED_PY_MODULES_COMMON.map { |s| s % "cpython-%d%d%s-darwin" % [python_version_major, python_version_minor, so_suffix] }
requirements_files = [REQUIREMENTS_FILE]
# check we have a known bundle
if bundle_path.to_s.include?('MantidWorkbench')
  bundled_packages += BUNDLED_PY_MODULES_WORKBENCH
  requirements_files << REQUIREMENTS_WORKBENCH_FILE
  bundled_qt_plugins = QT_PLUGINS_COMMON + ['platforms', 'printsupport', 'styles']
  host_qt_plugins_dir = QT5_PLUGINS_DIR
  executables << "#{contents_macos}/#{bundle_path.basename.to_s.split('.')[0]}"
elsif bundle_path.to_s.include?('MantidPlot')
  bundled_packages += BUNDLED_PY_MODULES_MANTIDPLOT
  bundled_qt_plugins = QT_PLUGINS_COMMON
  host_qt_plugins_dir = QT4_PLUGINS_DIR
  executables << "#{contents_macos}/MantidPlot"
else
  fatal("Unknown bundle type #{bundle_path}. Expected MantidPlot.app or MantidWorkbench.app.")
end

# We start with the assumption CMake has installed all required target libraries/executables
# into the bundle and the main layout exists.
bundle_py_site_packages = deploy_python_framework(contents, host_python_exe,
                                                  bundled_packages, requirements_files)

install_qt_plugins(bundle_path, bundled_qt_plugins, host_qt_plugins_dir,
                   QT_PLUGINS_BLACKLIST)
# We choose not to use macdeployqt as it uses @executable_path so we have to essentially
# run over everything twice to switch to @loader_path for workbench where we don't have
# an executable. It also fails to fixup the QWebEngine internal app so we might as well
# do everything here and save runtime.
fixup_binaries(bundle_path, contents_frameworks, executables)
stop_if_bundle_not_self_contained(bundle_path)
