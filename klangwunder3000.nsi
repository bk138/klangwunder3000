!define VERSION "0.3"

Name "Klangwunder3000 ${VERSION}"

OutFile "Klangwunder3000_${VERSION}-setup.exe"

InstallDir $PROGRAMFILES\Klangwunder3000

Page license
LicenseData COPYING.TXT

Page directory

Page instfiles

Section ""

  SetOutPath $INSTDIR

  File src\Klangwunder3000.exe
  File src\mingwm10.dll
  File NEWS.TXT	
  File README.TXT

  writeUninstaller $INSTDIR\Klangwunder3000-uninstall.exe

  # now the shortcuts
  CreateDirectory "$SMPROGRAMS\Klangwunder3000"
  createShortCut  "$SMPROGRAMS\Klangwunder3000\klangwunder3000.lnk" "$INSTDIR\klangwunder3000.exe"
  createShortCut  "$SMPROGRAMS\Klangwunder3000\Readme.lnk" "$INSTDIR\README.TXT"
  createShortCut  "$SMPROGRAMS\Klangwunder3000\News.lnk" "$INSTDIR\NEWS.TXT"
  createShortCut  "$SMPROGRAMS\Klangwunder3000\Uninstall klangwunder3000.lnk" "$INSTDIR\Klangwunder3000-uninstall.exe"

SectionEnd 

section "Uninstall"
 
  # Always delete uninstaller first
  delete $INSTDIR\Klangwunder3000-uninstall.exe

  # now delete installed files
  delete $INSTDIR\Klangwunder3000.exe
  delete $INSTDIR\mingwm10.dll
  delete $INSTDIR\NEWS.TXT
  delete $INSTDIR\README.TXT
  RMDir  $INSTDIR
 
  # delete shortcuts
  delete "$SMPROGRAMS\Klangwunder3000\klangwunder3000.lnk"
  delete "$SMPROGRAMS\Klangwunder3000\Readme.lnk"
  delete "$SMPROGRAMS\Klangwunder3000\News.lnk"
  delete "$SMPROGRAMS\Klangwunder3000\Uninstall klangwunder3000.lnk"
  RMDir  "$SMPROGRAMS\Klangwunder3000"
  
sectionEnd

Function un.onInit
    MessageBox MB_YESNO "This will uninstall Klangwunder3000. Continue?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
    NoAbort:
  FunctionEnd
