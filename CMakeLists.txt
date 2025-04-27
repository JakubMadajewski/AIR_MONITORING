cmake_minimum_required(VERSION 3.19)
project(AIR_MONITORING LANGUAGES CXX)

#some magic
set(CMAKE_AUTOMOC ON)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
INCLUDE(C:/GITHUB/vcpkg/scripts/buildsystems/vcpkg.cmake)  # so that vcpkg installed packages are seen

find_package(jsoncpp CONFIG REQUIRED)
find_package(CURL REQUIRED)

find_package(Qt6 6.5 REQUIRED COMPONENTS Charts Core Gui Widgets Concurrent )

qt_standard_project_setup()

qt_add_executable(AIR_MONITORING
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    API_CALL.h
    contentwidget.h
    datetimeaxiswidget.h
    contentwidget.cpp
    datetimeaxiswidget.cpp
)

target_link_libraries(AIR_MONITORING
    PRIVATE
        Qt::Charts
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Concurrent
)

target_link_libraries(AIR_MONITORING PRIVATE JsonCpp::JsonCpp)
target_link_libraries(AIR_MONITORING PRIVATE CURL::libcurl)

include(GNUInstallDirs)

install(TARGETS AIR_MONITORING
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AIR_MONITORING
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
