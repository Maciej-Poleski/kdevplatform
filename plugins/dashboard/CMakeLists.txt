project(kdev-project-dashboard)
add_subdirectory(plasmoids)
add_subdirectory(declarative)

set(kdevprojectdasboard_SRCS
    appletselector.cpp
    dashboard.cpp
    dashboardcorona.cpp
    
    dashboardview.cpp
    dashboarddocument.cpp
    dashboardpluginloader.cpp
    dashboarddataengine.cpp
    kdevprojectdashboard.cpp)

kde4_add_ui_files(kdevprojectdasboard_SRCS appletselector.ui)
kde4_add_plugin(kdevprojectdashboard ${kdevprojectdasboard_SRCS})
target_link_libraries(kdevprojectdashboard ${KDE4_KDECORE_LIBS} ${KDE4_PLASMA_LIBS}
                        ${KDEVPLATFORM_INTERFACES_LIBRARIES} ${KDEVPLATFORM_PROJECT_LIBRARIES} ${KDEVPLATFORM_UTIL_LIBRARIES} ${KDEVPLATFORM_SUBLIME_LIBRARIES})

install(TARGETS kdevprojectdashboard DESTINATION ${PLUGIN_INSTALL_DIR} )
configure_file(kdevprojectdashboard.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdevprojectdashboard.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdevprojectdashboard.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

