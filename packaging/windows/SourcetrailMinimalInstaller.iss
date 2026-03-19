#ifndef MyAppName
  #define MyAppName "Sourcetrail"
#endif

#ifndef MyAppVersion
  #define MyAppVersion "2026.3.16"
#endif

#ifndef MyAppPublisher
  #define MyAppPublisher "PERA Software Solutions GmbH"
#endif

#ifndef MyAppURL
  #define MyAppURL "http://sourcetrail.de"
#endif

#ifndef MyAppSourceDir
  #define MyAppSourceDir "A:\\dist\\sourcetrail-stage\\Sourcetrail"
#endif

#ifndef MyOutputDir
  #define MyOutputDir "A:\\dist\\installer"
#endif

[Setup]
AppId={{C2D2196B-2A43-4F23-A5FB-0BEECD20F744}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf64}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMABlockSize=65536
LZMADictionarySize=65536
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-setup-x64
SetupIconFile={#MyAppSourceDir}\app\Sourcetrail.ico
UninstallDisplayIcon={app}\app\Sourcetrail.exe
ChangesAssociations=no
UsePreviousAppDir=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#MyAppSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\app\Sourcetrail.exe"; WorkingDir: "{app}\app"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\app\Sourcetrail.exe"; WorkingDir: "{app}\app"; Tasks: desktopicon

[Run]
Filename: "{app}\app\Sourcetrail.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
