unit FpImgReaderBase;

{$mode objfpc}{$H+}
{$IFDEF INLINE_OFF}{$INLINE OFF}{$ENDIF}
{$modeswitch advancedrecords}

interface
{$ifdef CD_Cocoa}{$DEFINE MacOS}{$ENDIF}
{$IFDEF Darwin}{$DEFINE MacOS}{$ENDIF}

uses
  {$ifdef windows}
  Windows, // After LCLType
  {$endif}
  fgl, LazFglHash, LazFileUtils, LazLoggerBase,
  fpDbgSymTable,
  Classes, SysUtils, DbgIntfBaseTypes, contnrs,
  FpDbgCommon, crc;

type
  TDbgImageSection = record
    RawData: Pointer;
    Size: QWord;
    VirtualAddress: QWord;
    // Use this flag to identify sections that should be uploaded via RSP
    // This is probably only relevant for uploads to low level targets (embedded, FreeRTOS...)
    IsLoadable: Boolean;
    Name: String;
  end;
  PDbgImageSection = ^TDbgImageSection;

  TDbgImageSectionEx = record
    Sect: TDbgImageSection;
    Offs: QWord;
    Loaded: Boolean;
  end;
  PDbgImageSectionEx = ^TDbgImageSectionEx;

type
  TDbgAddressMap = record
    OrgAddr: QWord;
    Length: QWord;
    NewAddr: QWord;
    class operator =(r1,r2: TDbgAddressMap) : boolean;
  end;
  PDbgAddressMap = ^TDbgAddressMap;
  TDbgAddressMapList = specialize TFPGList<TDbgAddressMap>;

  { TDbgAddressMapHashList }

  TDbgAddressMapHashList = class(specialize TLazFPGHashTable<TDbgAddressMap>)
  public
    function ItemFromNode(ANode: THTCustomNode): TDbgAddressMap;
    function ItemPointerFromNode(ANode: THTCustomNode): PDbgAddressMap;
  end;

  { TDbgAddressMapPointerHashList }

  TDbgAddressMapPointerHashList = class(specialize TLazFPGHashTable<PDbgAddressMap>)
  public
    function ItemPointerFromNode(ANode: THTCustomNode): PDbgAddressMap;
  end;

  { TDbgFileLoader }
  {$ifdef windows}
    {$define USE_WIN_FILE_MAPPING}
  {$endif}

  TDbgFileLoader = class(TObject)
  private
    {$ifdef USE_WIN_FILE_MAPPING}
    FFileHandle: THandle;
    FMapHandle: THandle;
    FModulePtr: Pointer;
    {$endif}
    FStream: TStream;
    FList: TList;
    FFileName: String;

    function GetFileName: String;
    procedure OpenStream;
  public
    constructor Create(AFileName: String);
    {$ifdef USE_WIN_FILE_MAPPING}
    constructor Create(AFileHandle: THandle);
    {$endif}
    destructor Destroy; override;
    procedure Close;
    function  Read(AOffset, ASize: QWord; AMem: Pointer): QWord;
    function  LoadMemory(AOffset, ASize: QWord; out AMem: Pointer): QWord;
    procedure UnloadMemory({%H-}AMem: Pointer);
    property FileName: String read GetFileName;
  end;

  { TDbgImageReader }

  TDbgImageReader = class(TObject) // executable parser
  private
    FImageBase: QWord;
    FImageSize: QWord;
    FRelocationOffset: QWord;
    FLoadedTargetImageAddr: TDBGPtr;
    FReaderErrors: String;
    FUUID: TGuid;
  protected
    FTargetInfo: TTargetDescriptor;
    function GetSubFiles: TStrings; virtual;
    function GetAddressMapList: TDbgAddressMapList; virtual;
    function GetSection(const AName: String): PDbgImageSection; virtual; abstract;
    function GetSection(const ID: integer): PDbgImageSection; virtual; abstract;
    procedure SetUUID(AGuid: TGuid);
    procedure SetImageBase(ABase: QWord);
    procedure SetImageSize(ASize: QWord);
    procedure SetRelocationOffset(AnOffset: QWord);
    procedure AddReaderError(AnError: String);
    function  ReadGnuDebugLinkSection(out AFileName: String; out ACrc: Cardinal): Boolean;
    function  LoadGnuDebugLink(ASearchPath, AFileName: String; ACrc: Cardinal): TDbgFileLoader;
    property LoadedTargetImageAddr: TDBGPtr read FLoadedTargetImageAddr;
  public
    class function isValid(ASource: TDbgFileLoader): Boolean; virtual; abstract;
    class function UserName: AnsiString; virtual; abstract;
    procedure ParseSymbolTable(AFpSymbolInfo: TfpSymbolList); virtual;
    procedure ParseLibrarySymbolTable(AFpSymbolInfo: TfpSymbolList); virtual;
    // The LoadedTargetImageAddr is the start-address of the data of the binary
    // once loaded into memory.
    // On Linux it is 0 for executables, and had another value for libraries
    // which are relocated into another position.
    // On Windows it is the same as the ImageBase, except when the binary has
    // been relocated. This only happens if two libraries with the same ImageBase
    // are loaded. In that case, one of them is being relocated to another
    // LoadedTargetImageAddr.
    constructor Create({%H-}ASource: TDbgFileLoader; {%H-}ADebugMap: TObject; ALoadedTargetImageAddr: TDbgPtr; OwnSource: Boolean); virtual;
    procedure AddSubFilesToLoaderList(ALoaderList: TObject; PrimaryLoader: TObject); virtual;
    function EnclosesAddressRange(AStartAddress, AnEndAddress: TDBGPtr): Boolean; virtual;
    // The ImageBase is the address at which the linker assumed the binary will be
    // loaded at. So it is stored inside the binary itself and all addresses inside
    // the binary assume that once loaded into memory, it is loaded at this
    // address. (Which is not the case for relocated libraries, see the comments
    // for the LoadedTargetImageAddr)
    // On Linux it is always 0, on Windows it is set by the linker/compiler.
    property ImageBase: QWord read FImageBase;
    property ImageSize: QWord read FImageSize;
    // The RelocationOffset is the offset between the addresses as they are encoded
    // into the binary, and their real addresses once loaded in memory.
    // On linux it is equal to the LoadedTargetImageAddr.
    // On Windows it is 0, except for libraries which are re-located. In that
    // case the offset is LoadedTargetImageAddr-ImageBase.
    property RelocationOffset: QWord read FRelocationOffset;

    property TargetInfo: TTargetDescriptor read FTargetInfo;

    property UUID: TGuid read FUUID;
    property Section[const AName: String]: PDbgImageSection read GetSection;
    property SectionByID[const ID: integer]: PDbgImageSection read GetSection;
    property SubFiles: TStrings read GetSubFiles;
    property AddressMapList: TDbgAddressMapList read GetAddressMapList;
    property ReaderErrors: String read FReaderErrors;
  end;
  TDbgImageReaderClass = class of TDbgImageReader;

function GetImageReader(ASource: TDbgFileLoader; ADebugMap: TObject; ALoadedTargetImageAddr: TDbgPtr; OwnSource: Boolean): TDbgImageReader; overload;
procedure RegisterImageReaderClass(DataSource: TDbgImageReaderClass);

implementation

var
  DBG_WARNINGS: PLazLoggerLogGroup;

const
  // Symbol-map section name
  _gnu_dbg_link        = '.gnu_debuglink';

var
  RegisteredImageReaderClasses  : TFPList;

{ TDbgAddressMapPointerHashList }

function TDbgAddressMapPointerHashList.ItemPointerFromNode(ANode: THTCustomNode
  ): PDbgAddressMap;
begin
  Result := THTGNode(ANode).Data;
end;

{ TDbgAddressMapHashList }

function TDbgAddressMapHashList.ItemFromNode(ANode: THTCustomNode
 ): TDbgAddressMap;
begin
  Result := THTGNode(ANode).Data;
end;

function TDbgAddressMapHashList.ItemPointerFromNode(ANode: THTCustomNode
  ): PDbgAddressMap;
begin
  Result := @THTGNode(ANode).Data;
end;

 class operator TDbgAddressMap.=(r1,r2: TDbgAddressMap) : boolean;
 begin
   result := (r1.OrgAddr=r2.OrgAddr) and (r1.Length=r2.Length) and (r1.NewAddr=r2.NewAddr);
 end;

function GetImageReader(ASource: TDbgFileLoader; ADebugMap: TObject; ALoadedTargetImageAddr: TDbgPtr; OwnSource: Boolean): TDbgImageReader;
var
  i   : Integer;
  cls : TDbgImageReaderClass;
begin
  Result := nil;
  if not Assigned(ASource) then Exit;

  for i := 0 to RegisteredImageReaderClasses.Count - 1 do begin
    cls :=  TDbgImageReaderClass(RegisteredImageReaderClasses[i]);
    try
      if cls.isValid(ASource) then begin
        Result := cls.Create(ASource, ADebugMap, ALoadedTargetImageAddr, OwnSource);
        ASource.Close;
        Exit;
      end
      else
        ;
    except
      on e: exception do begin
        //writeln('exception! WHY? ', e.Message);
      end;
    end;
  end;
  ASource.Close;
  Result := nil;
end;

procedure RegisterImageReaderClass( DataSource: TDbgImageReaderClass);
begin
  if Assigned(DataSource) and (RegisteredImageReaderClasses.IndexOf(DataSource) < 0) then
    RegisteredImageReaderClasses.Add(DataSource)
end;


{$ifdef USE_WIN_FILE_MAPPING}
type
  TGetFinalPathNameByHandle = function(hFile: THandle; lpszFilePath: PWideChar; cchFilePath, dwFlags: DWORD): DWORD; stdcall;
var
  GetFinalPathNameByHandle: TGetFinalPathNameByHandle = nil;
function GetFinalPathNameByHandleDummy(hFile: THandle; lpszFilePath: PWideChar; cchFilePath, dwFlags: DWORD): DWORD; stdcall;
begin
  Result := 0;
end;
function FileHandleToFileName(Handle : THandle): string;
var
  U: WideString;
  hmod: HMODULE;
begin
  if not Assigned(GetFinalPathNameByHandle) then
  begin
    hmod := GetModuleHandle('kernel32');
    if hmod <> 0 then
      Pointer(GetFinalPathNameByHandle) := GetProcAddress(hmod,'GetFinalPathNameByHandleW');
    if not Assigned(GetFinalPathNameByHandle) then
      GetFinalPathNameByHandle := @GetFinalPathNameByHandleDummy;
  end;

  SetLength(U, MAX_PATH+1);
  SetLength(U, GetFinalPathNameByHandle(Handle, @U[1], Length(U), 0));
  if Copy(U, 1, 4)='\\?\' then
    Delete(U, 1, 4);
  Result := U;
end;
{$endif}

{ TDbgFileLoader }

{$ifdef USE_WIN_FILE_MAPPING}
function CreateFileW(lpFileName:LPCWSTR; dwDesiredAccess:DWORD; dwShareMode:DWORD; lpSecurityAttributes:LPSECURITY_ATTRIBUTES; dwCreationDisposition:DWORD;dwFlagsAndAttributes:DWORD; hTemplateFile:HANDLE):HANDLE; stdcall; external 'kernel32' name 'CreateFileW';
{$ENDIF}

function TDbgFileLoader.GetFileName: String;
begin
  {$ifdef USE_WIN_FILE_MAPPING}
  if (FFileName = '') and (FFileHandle <> 0) then begin
    FFileName := FileHandleToFileName(FFileHandle);
  end;
  {$endif}
  Result := FFileName;
end;

constructor TDbgFileLoader.Create(AFileName: String);
{$IFDEF MacOS}
var
  s: String;
{$ENDIF}
{$ifdef USE_WIN_FILE_MAPPING}
var
  s: UnicodeString;
{$ENDIF}
begin
  {$ifdef USE_WIN_FILE_MAPPING}
  FFileName := AFileName;
  s := UTF8Decode(AFileName);
  FFileHandle := CreateFileW(PWideChar(s), GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  Create(FFileHandle);
  {$else}
  FList := TList.Create;
  {$IFDEF MacOS}
  if (RightStr(AFileName,4) = '.app') then begin
    s := ExtractFileName(AFileName);
    s := AFileName + PathDelim + 'Contents' + PathDelim + 'MacOS' + PathDelim + copy(s, 1, Length(s) - 4);
    if (FileExists(s)) then AFileName := s
  end;
  {$ENDIF}
  FFileName := AFileName;
  FStream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyNone);
  inherited Create;
  {$endif}
end;

{$ifdef USE_WIN_FILE_MAPPING}
constructor TDbgFileLoader.Create(AFileHandle: THandle);
begin
  FFileHandle := AFileHandle;
  if FFileHandle = INVALID_HANDLE_VALUE
  then begin
    raise Exception.Create('Invalid file handle');
  end;

  FMapHandle := CreateFileMapping(FFileHandle, nil, PAGE_READONLY{ or SEC_IMAGE}, 0, 0, nil);
  if FMapHandle = 0
  then begin
    if GetLastError = 8 then begin // not enough memory, use a stream instead
      FList := TList.Create;
      FStream := THandleStream.Create(AFileHandle);
      inherited Create;
      exit;
    end;

    raise Exception.CreateFmt('Could not create module mapping, error %d', [GetLastError]);
    Exit;
  end;

  FModulePtr := MapViewOfFile(FMapHandle, FILE_MAP_READ, 0, 0, 0);
  if FModulePtr = nil
  then begin
    if GetLastError = 8 then begin // not enough memory, use a stream instead
      CloseHandle(FMapHandle);
      FMapHandle := 0;
      FList := TList.Create;
      FStream := THandleStream.Create(AFileHandle);
      inherited Create;
      exit;
    end;
    raise Exception.Create('Could not map view: ' + IntToStr(GetLastOSError));
    Exit;
  end;

  inherited Create;
end;
{$endif}

destructor TDbgFileLoader.Destroy;
begin
  {$ifdef USE_WIN_FILE_MAPPING}
  if FModulePtr <> nil
  then UnmapViewOfFile(FModulePtr);
  if FMapHandle <> 0
  then CloseHandle(FMapHandle);
  if FFileHandle <> INVALID_HANDLE_VALUE
  then CloseHandle(FFileHandle);
  {$endif}
  if FList <> nil then
    while FList.Count > 0 do
      UnloadMemory(FList[0]);
  FreeAndNil(FList);
  FreeAndNil(FStream);
  inherited Destroy;
end;

procedure TDbgFileLoader.OpenStream;
begin
  if FStream <> nil then
    exit;
  if FFileName <> '' then
    FStream := TFileStream.Create(FFileName, fmOpenRead or fmShareDenyNone)
  {$ifdef USE_WIN_FILE_MAPPING}
  else
    FStream := THandleStream.Create(FFileHandle)
  {$endif}
  ;
end;

procedure TDbgFileLoader.Close;
begin
  FreeAndNil(FStream);
end;

function TDbgFileLoader.Read(AOffset, ASize: QWord; AMem: Pointer): QWord;
begin
  {$ifdef USE_WIN_FILE_MAPPING}
  if FMapHandle <> 0 then begin
    move((FModulePtr + AOffset)^, AMem^, ASize);
    Result := ASize;
    exit;
  end;
  {$endif}
  Result := 0;
  if AMem = nil then
    exit;
  if FStream = nil then
    OpenStream;
  FStream.Position := AOffset;
  Result := FStream.Read(AMem^, ASize);
end;

function TDbgFileLoader.LoadMemory(AOffset, ASize: QWord; out AMem: Pointer): QWord;
begin
  {$ifdef USE_WIN_FILE_MAPPING}
  if FMapHandle <> 0 then begin
    AMem := FModulePtr + AOffset;
    Result := ASize;
    exit;
  end;
  {$endif}
  Result := 0;
  AMem := AllocMem(ASize);
  if AMem = nil then
    exit;
  if FStream = nil then
    OpenStream;
  FList.Add(AMem);
  FStream.Position := AOffset;
  Result := FStream.Read(AMem^, ASize);
end;

procedure TDbgFileLoader.UnloadMemory(AMem: Pointer);
begin
  if FList = nil then
    exit;
  FList.Remove(AMem);
  Freemem(AMem);
end;

{ TDbgImageReader }

function TDbgImageReader.GetAddressMapList: TDbgAddressMapList;
begin
  result := nil;
end;

function TDbgImageReader.GetSubFiles: TStrings;
begin
  result := nil;
end;

procedure TDbgImageReader.SetUUID(AGuid: TGuid);
begin
  FUUID := AGuid;
end;

procedure TDbgImageReader.SetImageBase(ABase: QWord);
begin
  FImageBase := ABase;
  if FLoadedTargetImageAddr = 0 then
    FLoadedTargetImageAddr := ABase;
end;

procedure TDbgImageReader.SetImageSize(ASize: QWord);
begin
  FImageSize := ASize;
end;

procedure TDbgImageReader.SetRelocationOffset(AnOffset: QWord);
begin
  FRelocationOffset := AnOffset;
end;

procedure TDbgImageReader.AddReaderError(AnError: String);
begin
  if FReaderErrors <> '' then
    FReaderErrors := FReaderErrors + LineEnding;
  FReaderErrors := FReaderErrors + AnError;
end;

function TDbgImageReader.ReadGnuDebugLinkSection(out AFileName: String; out
  ACrc: Cardinal): Boolean;
var
  p: PDbgImageSectionEx;
  i: Integer;
begin
  p := PDbgImageSectionEx(Section[_gnu_dbg_link]);
  Result := p <> nil;
  if Result then
  begin
    i := IndexByte(p^.Sect.RawData^, p^.Sect.Size-4, 0);
    Result := i > 0;
    if Result then
    begin
      SetLength(AFileName, i);
      move(PDbgImageSectionEx(p)^.Sect.RawData^, AFileName[1], i);

      i := align(i+1, 4);
      Result := (i+4) <= p^.Sect.Size;
      if Result then
        move((p^.Sect.RawData+i)^, ACrc, 4);
    end;
  end;
end;

function TDbgImageReader.LoadGnuDebugLink(ASearchPath, AFileName: String;
  ACrc: Cardinal): TDbgFileLoader;

  function LoadFile(AFullName: String): TDbgFileLoader;
  var
    i, j: Int64;
    c: Cardinal;
    mem: Pointer;
  begin
    Result := TDbgFileLoader.Create(AFullName);

    i := FileSizeUtf8(AFullName) - 4096;
    j := 0;
    c:=0;
    while j < i do begin
      Result.LoadMemory(j, 4096, mem);
      c:=Crc32(c, mem, 4096);
      Result.UnloadMemory(mem);
      inc(j, 4096)
    end;
    i := i - j + 4096;
    Result.LoadMemory(j, i, mem);
    c:=Crc32(c, mem, i);
    Result.UnloadMemory(mem);

    DebugLn(DBG_WARNINGS and (c <> ACrc), ['Invalid CRC for ext debug info: ', AFullName]);
    if c <> ACrc then
      FreeAndNil(Result);
  end;

begin
  Result := nil;

  if FileExists(AppendPathDelim(ASearchPath) + AFileName) then
    Result := LoadFile(AppendPathDelim(ASearchPath) + AFileName);

  if (Result = nil) and
     FileExists(AppendPathDelim(ASearchPath) + AppendPathDelim('.debug') + AFileName)
  then
    Result := LoadFile(AppendPathDelim(ASearchPath) + AppendPathDelim('.debug') + AFileName);
end;

procedure TDbgImageReader.ParseSymbolTable(AFpSymbolInfo: TfpSymbolList);
begin
  // The format of the symbol-table-section(s) can be different on each
  // platform. That's why parsing the data is done in TDbgImageReader.
end;

procedure TDbgImageReader.ParseLibrarySymbolTable(AFpSymbolInfo: TfpSymbolList);
begin
  //
end;

constructor TDbgImageReader.Create(ASource: TDbgFileLoader; ADebugMap: TObject; ALoadedTargetImageAddr: TDbgPtr; OwnSource: Boolean);
begin
  inherited Create;
  FLoadedTargetImageAddr := ALoadedTargetImageAddr;
end;

procedure TDbgImageReader.AddSubFilesToLoaderList(ALoaderList: TObject;
  PrimaryLoader: TObject);
begin
  //
end;

function TDbgImageReader.EnclosesAddressRange(AStartAddress, AnEndAddress: TDBGPtr): Boolean;
begin
  Result := False;
end;

procedure InitDebugInfoLists;
begin
  RegisteredImageReaderClasses := TFPList.Create;
end;

procedure ReleaseDebugInfoLists;
begin
  FreeAndNil(RegisteredImageReaderClasses);
end;

initialization
  DBG_WARNINGS := DebugLogger.FindOrRegisterLogGroup('DBG_WARNINGS' {$IFDEF DBG_WARNINGS} , True {$ENDIF} );

  InitDebugInfoLists;

finalization
  ReleaseDebugInfoLists;

end.

