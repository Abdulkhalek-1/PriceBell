# cmake/VersionInfo.cmake
# Generates Windows VERSIONINFO resource and sets macOS bundle properties.
# Include this BEFORE add_executable (sets WIN_VERSION_RC), then call
# pricebell_apply_version_info(target) AFTER add_executable for macOS.

if(WIN32)
    set(VERSION_RC "${CMAKE_BINARY_DIR}/version_info.rc")
    string(REPLACE "." "," VERSION_COMMA "${PROJECT_VERSION}")
    file(WRITE "${VERSION_RC}"
        "VS_VERSION_INFO VERSIONINFO\n"
        " FILEVERSION    ${VERSION_COMMA},0\n"
        " PRODUCTVERSION ${VERSION_COMMA},0\n"
        " FILETYPE       0x1L\n"
        "BEGIN\n"
        " BLOCK \"StringFileInfo\"\n"
        " BEGIN\n"
        "  BLOCK \"040904b0\"\n"
        "  BEGIN\n"
        "   VALUE \"FileDescription\",  \"PriceBell Price Tracker\"\n"
        "   VALUE \"FileVersion\",      \"${PROJECT_VERSION}\"\n"
        "   VALUE \"ProductName\",      \"PriceBell\"\n"
        "   VALUE \"ProductVersion\",   \"${PROJECT_VERSION}\"\n"
        "   VALUE \"CompanyName\",      \"Abdulkhalek Muhammad\"\n"
        "   VALUE \"LegalCopyright\",   \"Copyright (C) 2025-2026 Abdulkhalek Muhammad\"\n"
        "   VALUE \"InternalName\",     \"PriceBell\"\n"
        "   VALUE \"OriginalFilename\", \"PriceBell.exe\"\n"
        "  END\n"
        " END\n"
        " BLOCK \"VarFileInfo\"\n"
        " BEGIN\n"
        "  VALUE \"Translation\", 0x409, 1200\n"
        " END\n"
        "END\n"
    )
    set(WIN_VERSION_RC "${VERSION_RC}" PARENT_SCOPE)
endif()

macro(pricebell_apply_version_info target)
    if(APPLE)
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME            "PriceBell"
            MACOSX_BUNDLE_BUNDLE_VERSION         "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING   "${PROJECT_VERSION}"
            MACOSX_BUNDLE_GUI_IDENTIFIER         "com.abdulkhalek.pricebell"
            MACOSX_BUNDLE_COPYRIGHT              "Copyright (C) 2025-2026 Abdulkhalek Muhammad"
        )
    endif()
endmacro()
