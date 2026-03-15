cmake_minimum_required(VERSION 3.16)
project(PriceBell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg LinguistTools)

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
endif()

add_executable(PriceBell ${PLATFORM_FLAGS} ${SOURCES} ${HEADERS} assets/resources.qrc ${QM_FILES} ${WIN_RESOURCES})

target_include_directories(PriceBell PRIVATE include)

target_link_libraries(PriceBell
    Qt5::Widgets
    Qt5::Sql
    Qt5::Network
    Qt5::Svg
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

file(GLOB TEST_SOURCES "tests/test_price_checker.cpp" "tests/test_repository.cpp")
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
    include/utils/HttpClient.hpp
)
add_executable(test_validation tests/test_validation.cpp ${TEST_VALIDATION_SOURCES})
target_include_directories(test_validation PRIVATE include)
target_link_libraries(test_validation Qt5::Widgets Qt5::Sql Qt5::Network)
add_test(NAME test_validation COMMAND test_validation)
