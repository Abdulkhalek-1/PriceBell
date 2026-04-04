; PriceBell Windows NSIS Installer
; Requires NSIS 3.x: https://nsis.sourceforge.io/
; Build: makensis /DVERSION=1.3.0 packaging/windows/installer.nsi

!include "MUI2.nsh"
!include "LogicLib.nsh"

; --- General -----------------------------------------------------------------
Name              "PriceBell"
OutFile           "PriceBell-${VERSION}-windows-setup.exe"
InstallDir        "$PROGRAMFILES64\PriceBell"
InstallDirRegKey  HKLM "Software\PriceBell" "InstallDir"
RequestExecutionLevel admin
Unicode True

; --- MUI Pages ---------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\PriceBell.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Install Section ---------------------------------------------------------
Section "PriceBell" SecMain
    SetOutPath "$INSTDIR"
    File /r "..\..\build-release\*"

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\PriceBell"
    CreateShortcut "$SMPROGRAMS\PriceBell\PriceBell.lnk" "$INSTDIR\PriceBell.exe"
    CreateShortcut "$SMPROGRAMS\PriceBell\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    ; Optional Desktop shortcut
    MessageBox MB_YESNO "Create Desktop shortcut?" IDYES create_desktop IDNO skip_desktop
    create_desktop:
        CreateShortcut "$DESKTOP\PriceBell.lnk" "$INSTDIR\PriceBell.exe"
    skip_desktop:

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Registry entry for Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell" \
        "DisplayName"     "PriceBell"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell" \
        "DisplayVersion"  "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell" \
        "Publisher"       "Abdulkhalek Muhammad"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell" \
        "InstallLocation" "$INSTDIR"
SectionEnd

; --- Uninstall Section -------------------------------------------------------
Section "Uninstall"
    RMDir /r "$INSTDIR"
    Delete "$SMPROGRAMS\PriceBell\PriceBell.lnk"
    Delete "$SMPROGRAMS\PriceBell\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\PriceBell"
    Delete "$DESKTOP\PriceBell.lnk"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PriceBell"
    DeleteRegKey HKLM "Software\PriceBell"
SectionEnd
