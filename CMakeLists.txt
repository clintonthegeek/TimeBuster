cmake_minimum_required(VERSION 3.19)
project(TimeBuster LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Test)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

qt_standard_project_setup()
find_package(KF6CalendarCore REQUIRED)
find_package(KF6DAV REQUIRED)
find_package(KF6CoreAddons REQUIRED)

add_library(TimeBusterCore SHARED
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    calendaritem.h calendaritem.cpp
    cal.h cal.cpp

    collection.h collection.cpp
    collectioncontroller.h collectioncontroller.cpp
    syncbackend.h
    localbackend.h localbackend.cpp
    caldavbackend.h caldavbackend.cpp
    configmanager.h configmanager.cpp
    credentialsdialog.h credentialsdialog.cpp
    collectioninfowidget.h collectioninfowidget.cpp
    databasemanager.h databasemanager.cpp
    sessionmanager.h sessionmanager.cpp
    calendaritemfactory.h calendaritemfactory.cpp

    backendinfo.h
    viewinterface.h viewinterface.cpp
    calendartableview.h calendartableview.cpp
    editpane.h editpane.cpp
    deltaentry.h
)

target_link_libraries(TimeBusterCore
    PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Sql
    KF6::CalendarCore
    KF6::DAV
    KF6::CoreAddons)

qt_add_executable(TimeBuster
    WIN32 MACOSX_BUNDLE
    main.cpp

)

target_link_libraries(TimeBuster
    PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Sql
    KF6::CalendarCore
    KF6::DAV
    KF6::CoreAddons
    TimeBusterCore
)

target_include_directories(TimeBuster PRIVATE ${KF6CalendarCore_INCLUDE_DIRS})

target_include_directories(TimeBuster PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include(GNUInstallDirs)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

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
