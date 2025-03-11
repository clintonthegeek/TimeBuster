cmake_minimum_required(VERSION 3.19)
project(TimeBuster LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(TimeBuster
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    calendaritem.h calendaritem.cpp
    cal.h cal.cpp
    calendarview.h calendarview.cpp
    collection.h collection.cpp
    collectionmanager.h collectionmanager.cpp
    syncbackend.h syncbackend.cpp
    localbackend.h localbackend.cpp
    caldavbackend.h caldavbackend.cpp
)

target_link_libraries(TimeBuster
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS TimeBuster
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TimeBuster
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
