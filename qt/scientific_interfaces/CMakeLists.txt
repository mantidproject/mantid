# Ugly hack to get around issue in Boost version 1.48.0 in conjunction with Qt 4.7.4 or greater
set(qt_version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH})
if(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)
  set(extra_options "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
else(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)
  set(extra_options "")
endif(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)

# Now add the packages one-by-one, building up the dependencies as we go
add_subdirectory(General)
add_subdirectory(Direct)
add_subdirectory(Muon)
add_subdirectory(ISISReflectometry)
add_subdirectory(Inelastic) # Inelastic is a dependency of Indirect, so must come first
add_subdirectory(Indirect)
