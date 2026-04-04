cmake_minimum_required(VERSION 3.16)
project(PriceBell VERSION 1.1.0 LANGUAGES CXX)

include(cmake/VersionInfo.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

# ── Build options ──────────────────────────────────────────────────────────────
option(BUILD_EXAMPLES "Build example plugins" OFF)

# AddressSanitizer support — enable with -DCMAKE_BUILD_TYPE=Asan
if(CMAKE_BUILD_TYPE STREQUAL "Asan")
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg LinguistTools Multimedia)

set(TS_FILES
    i18n/pricebell_en.ts
    i18n/pricebell_ar.ts
    i18n/pricebell_fr.ts
)
qt5_add_translation(QM_FILES ${TS_FILES})

# Collect all sources
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.hpp")

set(PLATFORM_FLAGS "")
set(WIN_RESOURCES "")
if(WIN32)
    set(PLATFORM_FLAGS WIN32)
    set(WIN_RESOURCES assets/app.rc)
    if(WIN_VERSION_RC)
        list(APPEND WIN_RESOURCES "${WIN_VERSION_RC}")
    endif()
endif()

add_executable(PriceBell ${PLATFORM_FLAGS} ${SOURCES} ${HEADERS} assets/resources.qrc ${QM_FILES} ${WIN_RESOURCES})

pricebell_apply_version_info(PriceBell)

target_include_directories(PriceBell PRIVATE include)

target_link_libraries(PriceBell
    Qt5::Widgets
    Qt5::Sql
    Qt5::Network
    Qt5::Svg
    Qt5::Multimedia
)

# Create plugins directory next to binary
add_custom_command(TARGET PriceBell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:PriceBell>/plugins"
    COMMENT "Creating plugins directory"
)

# Copy assets next to binary
add_custom_command(TARGET PriceBell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:PriceBell>/assets"
    COMMENT "Copying assets"
)

# Copy translation files next to binary
add_custom_target(copy_translations ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/i18n"
    COMMAND ${CMAKE_COMMAND} -E copy ${QM_FILES} "${CMAKE_BINARY_DIR}/i18n/"
    DEPENDS ${QM_FILES}
    COMMENT "Copying translation files to i18n/"
)

# ── Tests ──────────────────────────────────────────────────────────────────────
enable_testing()

set(TEST_COMMON_SOURCES
    src/core/PriceChecker.cpp
    src/storage/Database.cpp
    src/storage/ProductRepository.cpp
    src/storage/AlertRepository.cpp
    src/utils/Logger.cpp
)

file(GLOB TEST_SOURCES "tests/test_price_checker.cpp" "tests/test_repository.cpp" "tests/test_currency_utils.cpp")
foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC} ${TEST_COMMON_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE include)
    target_link_libraries(${TEST_NAME} Qt5::Sql Qt5::Network)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Plugin validation test
set(TEST_VALIDATION_SOURCES
    src/core/PluginManager.cpp
    src/handlers/SteamHandler.cpp
    src/handlers/UdemyHandler.cpp
    src/handlers/AmazonHandler.cpp
    src/handlers/GenericWebHandler.cpp
    src/storage/Database.cpp
    src/storage/ProductRepository.cpp
    src/storage/AlertRepository.cpp
    src/utils/Logger.cpp
    src/utils/HttpClient.cpp
    src/utils/SettingsProvider.cpp
    include/utils/HttpClient.hpp
)
add_executable(test_validation tests/test_validation.cpp ${TEST_VALIDATION_SOURCES})
target_include_directories(test_validation PRIVATE include)
target_link_libraries(test_validation Qt5::Widgets Qt5::Sql Qt5::Network)
add_test(NAME test_validation COMMAND test_validation)

# URL validation test — needs handler + HttpClient sources
set(URL_VALIDATION_SOURCES
    src/handlers/SteamHandler.cpp
    src/handlers/UdemyHandler.cpp
    src/handlers/AmazonHandler.cpp
    src/handlers/GenericWebHandler.cpp
    src/utils/HttpClient.cpp
    src/utils/Logger.cpp
    src/utils/SettingsProvider.cpp
    include/utils/HttpClient.hpp
)
add_executable(test_url_validation tests/test_url_validation.cpp ${URL_VALIDATION_SOURCES})
target_include_directories(test_url_validation PRIVATE include)
target_link_libraries(test_url_validation Qt5::Widgets Qt5::Network)
add_test(NAME test_url_validation COMMAND test_url_validation)

# ── Examples (opt-in) ──────────────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples/plugin-starter)
endif()

# ── Install rules ──────────────────────────────────────────────────────────────
install(TARGETS PriceBell DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/PriceBell/assets)
install(FILES packaging/linux/pricebell.desktop
        DESTINATION share/applications)
install(FILES assets/logo.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME pricebell.svg)
# ── CPack packaging ────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME            "PriceBell")
set(CPACK_PACKAGE_VENDOR          "Abdulkhalek Muhammad")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Desktop price tracker for Steam, Udemy, Amazon")
set(CPACK_PACKAGE_VERSION         "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT         "abdulkhalek@example.com")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PriceBell")

# DEB
set(CPACK_DEBIAN_PACKAGE_MAINTAINER  "Abdulkhalek Muhammad")
set(CPACK_DEBIAN_PACKAGE_SECTION     "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt5widgets5 (>= 5.12), libqt5sql5 (>= 5.12), libqt5sql5-sqlite, libqt5network5 (>= 5.12), libqt5multimedia5 (>= 5.12), libqt5svg5 (>= 5.12)")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/linux/postinst")

# RPM
set(CPACK_RPM_PACKAGE_SUMMARY     "Desktop price tracker")
set(CPACK_RPM_PACKAGE_REQUIRES    "qt5-qtbase >= 5.12, qt5-qtmultimedia >= 5.12, qt5-qtsvg >= 5.12")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_SOURCE_DIR}/packaging/linux/postinst")

# Generator is platform-specific
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "DEB;RPM")
endif()
include(CPack)
