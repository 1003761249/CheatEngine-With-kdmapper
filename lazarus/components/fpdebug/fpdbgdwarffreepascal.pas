unit FpDbgDwarfFreePascal;

{$mode objfpc}{$H+}
{$TYPEDADDRESS on}
{$IFDEF INLINE_OFF}{$INLINE OFF}{$ENDIF}

interface

uses
  Classes, SysUtils, Types, math,
  FpDbgDwarfDataClasses, FpDbgDwarf, FpDbgInfo,
  FpDbgUtil, FpDbgDwarfConst, FpErrorMessages, FpdMemoryTools, FpDbgClasses,
  DbgIntfBaseTypes,
  {$ifdef FORCE_LAZLOGGER_DUMMY} LazLoggerDummy {$else} LazLoggerBase {$endif}, LazStringUtils;

type

  {%Region * ***** SymbolClassMap ***** *}

  { TFpDwarfFreePascalSymbolClassMap }

  TFpDwarfFreePascalSymbolClassMap = class(TFpDwarfDefaultSymbolClassMap)
  strict private
    class var ExistingClassMap: TFpSymbolDwarfClassMap;
  private
    FCompilerVersion: Cardinal;
  protected
    function CanHandleCompUnit(ACU: TDwarfCompilationUnit; AHelperData: Pointer): Boolean; override;
    class function GetExistingClassMap: PFpDwarfSymbolClassMap; override;
  public
    class function GetInstanceForCompUnit(ACU: TDwarfCompilationUnit): TFpSymbolDwarfClassMap; override;
    class function ClassCanHandleCompUnit(ACU: TDwarfCompilationUnit): Boolean; override;

    class function GetInstanceForDbgInfo(ADbgInfo: TDbgInfo):TFpDwarfFreePascalSymbolClassMap;
  public
    constructor Create(ACU: TDwarfCompilationUnit; AHelperData: Pointer); override;
    function IgnoreCfiStackEnd: boolean; override;
    function GetDwarfSymbolClass(ATag: Cardinal): TDbgDwarfSymbolBaseClass; override;
    function CreateScopeForSymbol(ALocationContext: TFpDbgLocationContext; ASymbol: TFpSymbol;
      ADwarf: TFpDwarfInfo): TFpDbgSymbolScope; override;
    function CreateProcSymbol(ACompilationUnit: TDwarfCompilationUnit;
      AInfo: PDwarfAddressInfo; AAddress: TDbgPtr; ADbgInfo: TFpDwarfInfo
      ): TDbgDwarfSymbolBase; override;

    function GetInstanceClassNameFromPVmt(APVmt: TDbgPtr;
      AContext: TFpDbgLocationContext; ASizeOfAddr: Integer;
      AClassName, AUnitName: PString; out AnError: TFpError): boolean;
  end;

  { TFpDwarfFreePascalSymbolClassMapDwarf2 }

  TFpDwarfFreePascalSymbolClassMapDwarf2 = class(TFpDwarfFreePascalSymbolClassMap)
  strict private
    class var ExistingClassMap: TFpSymbolDwarfClassMap;
  protected
    class function GetExistingClassMap: PFpDwarfSymbolClassMap; override;
  public
    class function ClassCanHandleCompUnit(ACU: TDwarfCompilationUnit): Boolean; override;
  public
    function GetDwarfSymbolClass(ATag: Cardinal): TDbgDwarfSymbolBaseClass; override;
    //class function CreateSymbolScope(AThreadId, AStackFrame: Integer; AnAddress: TDBGPtr; ASymbol: TFpSymbol;
    //  ADwarf: TFpDwarfInfo): TFpDbgSymbolScope; override;
    //class function CreateProcSymbol(ACompilationUnit: TDwarfCompilationUnit;
    //  AInfo: PDwarfAddressInfo; AAddress: TDbgPtr): TDbgDwarfSymbolBase; override;
  end;

  { TFpDwarfFreePascalSymbolClassMapDwarf3 }

  TFpDwarfFreePascalSymbolClassMapDwarf3 = class(TFpDwarfFreePascalSymbolClassMap)
  strict private
    class var ExistingClassMap: TFpSymbolDwarfClassMap;
  protected
    class function GetExistingClassMap: PFpDwarfSymbolClassMap; override;
  public
    class function ClassCanHandleCompUnit(ACU: TDwarfCompilationUnit): Boolean; override;
  public
    function GetDwarfSymbolClass(ATag: Cardinal): TDbgDwarfSymbolBaseClass; override;
    function CreateScopeForSymbol(ALocationContext: TFpDbgLocationContext; ASymbol: TFpSymbol;
      ADwarf: TFpDwarfInfo): TFpDbgSymbolScope; override;
    //class function CreateSymbolScope(AThreadId, AStackFrame: Integer; AnAddress: TDBGPtr; ASymbol: TFpSymbol;
    //  ADwarf: TFpDwarfInfo): TFpDbgSymbolScope; override;
    //class function CreateProcSymbol(ACompilationUnit: TDwarfCompilationUnit;
    //  AInfo: PDwarfAddressInfo; AAddress: TDbgPtr): TDbgDwarfSymbolBase; override;
  end;

  {%EndRegion }

  {%Region * ***** Context ***** *}

  { TFpDwarfFreePascalSymbolScope }

  TFpDwarfFreePascalSymbolScope = class(TFpDwarfInfoSymbolScope)
  private
    FOuterNestContext: TFpDbgSymbolScope;
    FOuterNotFound: Boolean;
    FClassVarStaticPrefix: String;

    FSystemCU, FSysUtilsCU, FTypInfoCU: TDwarfCompilationUnit;
    FFoundSystemInfoEntry: TDwarfInformationEntry;
    FInAllUnitSearch, FSearchSpecialCuDone: boolean;
  protected
    function FindExportedSymbolInUnit(CU: TDwarfCompilationUnit;
      const ANameInfo: TNameSearchInfo; out
      AnInfoEntry: TDwarfInformationEntry; out AnIsExternal: Boolean;
      AFindFlags: TFindExportedSymbolsFlags = []): Boolean; override;
    function FindExportedSymbolInUnits(const AName: String;
      const ANameInfo: TNameSearchInfo; SkipCompUnit: TDwarfCompilationUnit;
      out ADbgValue: TFpValue; const OnlyUnitNameLower: String = ''): Boolean;
      override;
    function FindLocalSymbol(const AName: String; const ANameInfo: TNameSearchInfo;
      InfoEntry: TDwarfInformationEntry; out ADbgValue: TFpValue): Boolean; override;
    function FindSymbolInStructure(const AName: String;
      const ANameInfo: TNameSearchInfo; InfoEntry: TDwarfInformationEntry; out
      ADbgValue: TFpValue): Boolean; override;
    procedure Init; override;
  public
    destructor Destroy; override;
  end;

  { TFpDwarfFreePascalSymbolScopeDwarf3 }

  TFpDwarfFreePascalSymbolScopeDwarf3 = class(TFpDwarfFreePascalSymbolScope)
  protected
    procedure Init; override;
  end;

  {%EndRegion }

  {%Region * ***** Value & Types ***** *}

  (* *** Class vs ^Record vs ^Object *** *)

  { TFpSymbolDwarfFreePascalTypeDeclaration }

  TFpSymbolDwarfFreePascalTypeDeclaration = class(TFpSymbolDwarfTypeDeclaration)
  protected
   // fpc encodes classes as pointer, not ref (so Obj1 = obj2 compares the pointers)
   // typedef > pointer > srtuct
   // while a pointer to class/object: pointer > typedef > ....
    function DoGetNestedTypeInfo: TFpSymbolDwarfType; override;
  end;

  { TFpSymbolDwarfFreePascalTypePointer }

  TFpSymbolDwarfFreePascalTypePointer = class(TFpSymbolDwarfTypePointer)
  private
    FIsInternalPointer: Boolean;
    function GetIsInternalPointer: Boolean; inline;
    function IsInternalDynArrayPointer: Boolean; inline;
  protected
    function GetInternalTypeInfo: TFpSymbol; override;
    procedure TypeInfoNeeded; override;
    procedure KindNeeded; override;
    function DoReadStride(AValueObj: TFpValueDwarf; out AStride: TFpDbgValueSize): Boolean; override;
    procedure ForwardToSymbolNeeded; override;
    function GetNextTypeInfoForDataAddress(ATargetType: TFpSymbolDwarfType): TFpSymbolDwarfType; override;
    function GetDataAddressNext(AValueObj: TFpValueDwarf; var AnAddress: TFpDbgMemLocation;
      out ADoneWork: Boolean; ATargetType: TFpSymbolDwarfType): Boolean; override;
    function DoReadDataSize(const AValueObj: TFpValue; out ADataSize: TFpDbgValueSize): Boolean; override;
  public
    function GetTypedValueObject(ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType = nil): TFpValueDwarf; override;
    property IsInternalPointer: Boolean read GetIsInternalPointer write FIsInternalPointer; // Class (also DynArray, but DynArray is handled without this)
  end;

  { TFpSymbolDwarfFreePascalTypeStructure }

  TFpSymbolDwarfFreePascalTypeStructure = class(TFpSymbolDwarfTypeStructure)
  protected
    procedure KindNeeded; override;
    //function GetInstanceClass(AValueObj: TFpValueDwarf): TFpSymbolDwarf; override;
    class function GetInstanceClassNameFromPVmt(APVmt: TDbgPtr;
      AContext: TFpDbgLocationContext; ASizeOfAddr: Integer;
      AClassName, AUnitName: PString; out AnError: TFpError;
      AParentClassIndex: integer = 0;
      ACompilerVersion: Cardinal = 0): boolean;
  public
    function GetInstanceClassName(AValueObj: TFpValue;
      AClassName, AUnitName: PString;
      AParentClassIndex: integer = 0): boolean; override;
  end;

  (* *** Record vs ShortString *** *)

  { TFpSymbolDwarfV2FreePascalTypeStructure }

  TFpSymbolDwarfV2FreePascalTypeStructure = class(TFpSymbolDwarfFreePascalTypeStructure)
  private
    FIsShortString: (issUnknown, issShortString, issStructure);
    function IsShortString: Boolean;
  protected
    procedure KindNeeded; override;
    function GetNestedSymbolCount: Integer; override;
    //function GetNestedSymbolByName(AIndex: String): TFpSymbol; override;
  public
    function GetTypedValueObject(ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType = nil): TFpValueDwarf; override;
  end;

  { TFpValueDwarfV2FreePascalShortString }

  TFpValueDwarfV2FreePascalShortString = class(TFpValueDwarf)
  protected
    function IsValidTypeCast: Boolean; override;
    function GetInternMemberByName(const AIndex: String): TFpValue;
    function GetMemberCount: Integer; override;
  private
    FValue: String;
    FValueDone: Boolean;
  protected
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsString: AnsiString; override;
    function GetAsWideString: WideString; override;
  public
    procedure Reset; override;
  end;

  (* *** "Open Array" in params *** *)

  { TFpSymbolDwarfFreePascalSymbolTypeArray }

  TFpSymbolDwarfFreePascalSymbolTypeArray = class(TFpSymbolDwarfTypeArray)
  public
    function GetTypedValueObject(ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType = nil): TFpValueDwarf; override;
  end;

  { TFpValueDwarfFreePascalArray }

  TFpValueDwarfFreePascalArray = class(TFpValueDwarfArray)
  protected
    function GetKind: TDbgSymbolKind; override;
    function GetMemberCount: Integer; override;
    function DoGetStride(out AStride: TFpDbgValueSize): Boolean; override;
    function DoGetMainStride(out AStride: TFpDbgValueSize): Boolean; override;
    function DoGetDimStride(AnIndex: integer; out AStride: TFpDbgValueSize): Boolean; override;
  public
    function GetFpcRefCount(out ARefCount: Int64): Boolean; override;
  end;

  (* *** Array vs AnsiString *** *)

  { TFpSymbolDwarfV3FreePascalSymbolTypeArray }

  TFpSymbolDwarfV3FreePascalSymbolTypeArray = class(TFpSymbolDwarfFreePascalSymbolTypeArray)
  private type
    TArrayOrStringType = (iasUnknown, iasArray, iasShortString, iasAnsiString, iasUnicodeString);
  private
    FArrayOrStringType: TArrayOrStringType;
    function GetInternalStringType: TArrayOrStringType;
  protected
    procedure KindNeeded; override;
    function DoReadSize(const AValueObj: TFpValue; out ASize: TFpDbgValueSize): Boolean; override;
  public
    function GetTypedValueObject(ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType = nil): TFpValueDwarf; override;
  end;

  { TFpValueDwarfV3FreePascalString }

  TFpValueDwarfV3FreePascalString = class(TFpValueDwarf) // short & ansi...
  private
    FValue: String;
    FLowBound, FHighBound: Int64;
    FValueDone, FBoundsDone: Boolean;
    FDynamicCodePage: TSystemCodePage;
    function GetCodePage: TSystemCodePage;
    function ObtainDynamicCodePage(Addr: TFpDbgMemLocation; out Codepage: TSystemCodePage): Boolean;
    procedure CalcBounds;
    // check if this is a string, and return bounds
    function CheckTypeAndGetAddr(out AnAddr: TFpDbgMemLocation): boolean;
  protected
    function IsValidTypeCast: Boolean; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsString: AnsiString; override;
    function GetAsWideString: WideString; override;
    procedure SetAsCardinal(AValue: QWord); override;
    function GetAsCardinal: QWord; override;
    function GetMemberCount: Integer; override;
  public
    procedure Reset; override;
    function GetSubString(AStartIndex, ALen: Int64; out ASubStr: AnsiString;
      AIgnoreBounds: Boolean = False): Boolean; override;
    function GetSubWideString(AStartIndex, ALen: Int64; out
      ASubStr: WideString; AIgnoreBounds: Boolean = False): Boolean; override;
    function GetFpcRefCount(out ARefCount: Int64): Boolean; override;
    property DynamicCodePage: TSystemCodePage read GetCodePage;
  end;

  { TFpSymbolDwarfFreePascalDataProc }

  TFpSymbolDwarfFreePascalDataProc = class(TFpSymbolDwarfDataProc)
  private
    FOrigSymbol: TFpSymbolDwarfFreePascalDataProc;
  protected
    function GetLine: Cardinal; override;
    function GetColumn: Cardinal; override;
    // Todo: LineStartAddress, ...
  public
    destructor Destroy; override;
    function ResolveInternalFinallySymbol(Process: Pointer): TFpSymbol; override;
  end;
  {%EndRegion }

  { TFpSymbolDwarfFreePascalDataParameter }

  TFpSymbolDwarfFreePascalDataParameter = class(TFpSymbolDwarfDataParameter)
    procedure NameNeeded; override;
  end;

implementation

uses
  FpDbgCommon;

var
  FPDBG_DWARF_VERBOSE: PLazLoggerLogGroup;

{ TFpDwarfFreePascalSymbolClassMap }

function TFpDwarfFreePascalSymbolClassMap.CanHandleCompUnit(
  ACU: TDwarfCompilationUnit; AHelperData: Pointer): Boolean;
begin
  Result := (FCompilerVersion = PtrUInt(AHelperData)) and
            inherited CanHandleCompUnit(ACU, AHelperData);
end;

class function TFpDwarfFreePascalSymbolClassMap.GetExistingClassMap: PFpDwarfSymbolClassMap;
begin
  Result := @ExistingClassMap;
end;

class function TFpDwarfFreePascalSymbolClassMap.GetInstanceForCompUnit(
  ACU: TDwarfCompilationUnit): TFpSymbolDwarfClassMap;
var
  s: String;
  i, j, AVersion: Integer;
begin
  AVersion := 0;
  s := ACU.Producer+' ';
  i := PosI('free pascal', s) + 11;

  if i > 11 then begin
    while (i < Length(s)) and (s[i] in [' ', #9]) do
      inc(i);
    delete(s, 1, i - 1);
    i := pos('.', s);
    if (i > 1) then begin
      j := StrToIntDef(copy(s, 1, i - 1), 0);
      if (j >= 0) then
        AVersion := j * $10000;
      delete(s, 1, i);
    end;
    if (AVersion > 0) then begin
      i := pos('.', s);
      if (i > 1) then begin
        j := StrToIntDef(copy(s, 1, i - 1), 0);
        if (j >= 0) and (j < 99) then
          AVersion := AVersion + j * $100
        else
          AVersion := 0;
        delete(s, 1, i);
      end;
    end;
    if (AVersion > 0) then begin
      i := pos(' ', s);
      if (i > 1) then begin
        j := StrToIntDef(copy(s, 1, i - 1), 0);
        if (j >= 0) and (j < 99) then
          AVersion := AVersion + j
        else
          AVersion := 0;
      end;
    end;
  end;

  Result := DoGetInstanceForCompUnit(ACU, Pointer(PtrUInt(AVersion)));
end;

class function TFpDwarfFreePascalSymbolClassMap.ClassCanHandleCompUnit(ACU: TDwarfCompilationUnit): Boolean;
begin
  Result := PosI('free pascal', ACU.Producer) > 0;
end;

var
  LastInfo: TDbgInfo = nil;
  FoundMap: TFpDwarfFreePascalSymbolClassMap = nil;

class function TFpDwarfFreePascalSymbolClassMap.GetInstanceForDbgInfo(
  ADbgInfo: TDbgInfo): TFpDwarfFreePascalSymbolClassMap;
var
  i: Integer;
begin
  if ADbgInfo <> LastInfo then begin
    FoundMap := nil;
    LastInfo := nil;
  end;

  Result := FoundMap;
  if LastInfo <> nil then
    exit;

  if not (ADbgInfo is TFpDwarfInfo) then
    exit;

  for i := 0 to TFpDwarfInfo(ADbgInfo).CompilationUnitsCount - 1 do
    if TFpDwarfInfo(ADbgInfo).CompilationUnits[i].DwarfSymbolClassMap is TFpDwarfFreePascalSymbolClassMap
    then begin
      FoundMap := TFpDwarfFreePascalSymbolClassMap(TFpDwarfInfo(ADbgInfo).CompilationUnits[i].DwarfSymbolClassMap);
    end;

  Result := FoundMap;
  LastInfo := ADbgInfo;
end;

constructor TFpDwarfFreePascalSymbolClassMap.Create(ACU: TDwarfCompilationUnit;
  AHelperData: Pointer);
begin
  FCompilerVersion := PtrUInt(AHelperData);
  inherited Create(ACU, AHelperData);
end;

function TFpDwarfFreePascalSymbolClassMap.IgnoreCfiStackEnd: boolean;
begin
  Result := FCompilerVersion < $030301;
end;

function TFpDwarfFreePascalSymbolClassMap.GetDwarfSymbolClass(
  ATag: Cardinal): TDbgDwarfSymbolBaseClass;
begin
  case ATag of
    DW_TAG_typedef:          Result := TFpSymbolDwarfFreePascalTypeDeclaration;
    DW_TAG_pointer_type:     Result := TFpSymbolDwarfFreePascalTypePointer;
    DW_TAG_structure_type,
    DW_TAG_class_type:       Result := TFpSymbolDwarfFreePascalTypeStructure;
    DW_TAG_array_type:       Result := TFpSymbolDwarfFreePascalSymbolTypeArray;
    DW_TAG_subprogram:       Result := TFpSymbolDwarfFreePascalDataProc;
    DW_TAG_formal_parameter: Result := TFpSymbolDwarfFreePascalDataParameter;
    else                     Result := inherited GetDwarfSymbolClass(ATag);
  end;
end;

function TFpDwarfFreePascalSymbolClassMap.CreateScopeForSymbol(
  ALocationContext: TFpDbgLocationContext; ASymbol: TFpSymbol;
  ADwarf: TFpDwarfInfo): TFpDbgSymbolScope;
begin
  Result := TFpDwarfFreePascalSymbolScope.Create(ALocationContext, ASymbol, ADwarf);
end;

function TFpDwarfFreePascalSymbolClassMap.CreateProcSymbol(
  ACompilationUnit: TDwarfCompilationUnit; AInfo: PDwarfAddressInfo;
  AAddress: TDbgPtr; ADbgInfo: TFpDwarfInfo): TDbgDwarfSymbolBase;
begin
  Result := TFpSymbolDwarfFreePascalDataProc.Create(ACompilationUnit, AInfo, AAddress, ADbgInfo);
end;

function TFpDwarfFreePascalSymbolClassMap.GetInstanceClassNameFromPVmt(
  APVmt: TDbgPtr; AContext: TFpDbgLocationContext; ASizeOfAddr: Integer;
  AClassName, AUnitName: PString; out AnError: TFpError): boolean;
begin
  Result := TFpSymbolDwarfFreePascalTypeStructure.GetInstanceClassNameFromPVmt(APVmt,
    AContext, ASizeOfAddr, AClassName, AUnitName, AnError);
end;

{ TFpDwarfFreePascalSymbolClassMapDwarf2 }

class function TFpDwarfFreePascalSymbolClassMapDwarf2.GetExistingClassMap: PFpDwarfSymbolClassMap;
begin
  Result := @ExistingClassMap;
end;

class function TFpDwarfFreePascalSymbolClassMapDwarf2.ClassCanHandleCompUnit(
  ACU: TDwarfCompilationUnit): Boolean;
begin
  Result := inherited ClassCanHandleCompUnit(ACU);
  Result := Result and (ACU.Version < 3);
end;

function TFpDwarfFreePascalSymbolClassMapDwarf2.GetDwarfSymbolClass(
  ATag: Cardinal): TDbgDwarfSymbolBaseClass;
begin
  case ATag of
    DW_TAG_structure_type:
      Result := TFpSymbolDwarfV2FreePascalTypeStructure; // maybe record
  //  // TODO:
  //  //DW_TAG_reference_type:   Result := TFpSymbolDwarfTypeRef;
  //  //DW_TAG_typedef:          Result := TFpSymbolDwarfTypeDeclaration;
  //  //DW_TAG_pointer_type:     Result := TFpSymbolDwarfTypePointer;
  //  //
  //  //DW_TAG_base_type:        Result := TFpSymbolDwarfTypeBasic;
  //  //DW_TAG_subrange_type:    Result := TFpSymbolDwarfTypeSubRange;
  //  //DW_TAG_enumeration_type: Result := TFpSymbolDwarfTypeEnum;
  //  //DW_TAG_enumerator:       Result := TFpSymbolDwarfDataEnumMember;
  //  //DW_TAG_array_type:       Result := TFpSymbolDwarfTypeArray;
  //  ////
  //  //DW_TAG_compile_unit:     Result := TFpSymbolDwarfUnit;
  //
    else
      Result := inherited GetDwarfSymbolClass(ATag);
  end;
end;

{ TFpDwarfFreePascalSymbolClassMapDwarf3 }

class function TFpDwarfFreePascalSymbolClassMapDwarf3.GetExistingClassMap: PFpDwarfSymbolClassMap;
begin
  Result := @ExistingClassMap;
end;

class function TFpDwarfFreePascalSymbolClassMapDwarf3.ClassCanHandleCompUnit(
  ACU: TDwarfCompilationUnit): Boolean;
begin
  Result := inherited ClassCanHandleCompUnit(ACU);
  Result := Result and (ACU.Version >= 3);
end;

function TFpDwarfFreePascalSymbolClassMapDwarf3.GetDwarfSymbolClass(
  ATag: Cardinal): TDbgDwarfSymbolBaseClass;
begin
  case ATag of
    DW_TAG_array_type:
      Result := TFpSymbolDwarfV3FreePascalSymbolTypeArray;
  //  DW_TAG_structure_type:
  //    Result := TFpSymbolDwarfV2FreePascalTypeStructure; // maybe record
  //  // TODO:
  //  //DW_TAG_reference_type:   Result := TFpSymbolDwarfTypeRef;
  //  //DW_TAG_typedef:          Result := TFpSymbolDwarfTypeDeclaration;
  //  //DW_TAG_pointer_type:     Result := TFpSymbolDwarfTypePointer;
  //  //
  //  //DW_TAG_base_type:        Result := TFpSymbolDwarfTypeBasic;
  //  //DW_TAG_subrange_type:    Result := TFpSymbolDwarfTypeSubRange;
  //  //DW_TAG_enumeration_type: Result := TFpSymbolDwarfTypeEnum;
  //  //DW_TAG_enumerator:       Result := TFpSymbolDwarfDataEnumMember;
  //  //DW_TAG_array_type:       Result := TFpSymbolDwarfTypeArray;
  //  ////
  //  //DW_TAG_compile_unit:     Result := TFpSymbolDwarfUnit;
  //
    else
      Result := inherited GetDwarfSymbolClass(ATag);
  end;
end;

function TFpDwarfFreePascalSymbolClassMapDwarf3.CreateScopeForSymbol(
  ALocationContext: TFpDbgLocationContext; ASymbol: TFpSymbol;
  ADwarf: TFpDwarfInfo): TFpDbgSymbolScope;
begin
  Result := TFpDwarfFreePascalSymbolScopeDwarf3.Create(ALocationContext, ASymbol, ADwarf);
end;

type

  { TFpDbgDwarfSimpleLocationContext }

  TFpDbgDwarfSimpleLocationContext = class(TFpDbgSimpleLocationContext)
  protected
    FStackFrame: Integer;
    function GetStackFrame: Integer; override;
  public
    constructor Create(AMemManager: TFpDbgMemManager; AnAddress: TDbgPtr;
      AnSizeOfAddr, AThreadId: Integer; AStackFrame: Integer);
  end;

{ TFpDbgDwarfSimpleLocationContext }

constructor TFpDbgDwarfSimpleLocationContext.Create(
  AMemManager: TFpDbgMemManager; AnAddress: TDbgPtr; AnSizeOfAddr,
  AThreadId: Integer; AStackFrame: Integer);
begin
  inherited Create(AMemManager, AnAddress, AnSizeOfAddr, AThreadId, AStackFrame);
  FStackFrame := AStackFrame;
end;

function TFpDbgDwarfSimpleLocationContext.GetStackFrame: Integer;
begin
  Result := FStackFrame;
end;

{ TFpDwarfFreePascalSymbolScope }

var
  ParentFpLowerNameInfo, ParentFp2LowerNameInfo: TNameSearchInfo; // case sensitive


function TFpDwarfFreePascalSymbolScope.FindExportedSymbolInUnit(
  CU: TDwarfCompilationUnit; const ANameInfo: TNameSearchInfo; out
  AnInfoEntry: TDwarfInformationEntry; out AnIsExternal: Boolean;
  AFindFlags: TFindExportedSymbolsFlags): Boolean;
begin
  // those units have scoped enums, that conflict with common types
  if (CU = FSysUtilsCU) or (CU = FTypInfoCU) then
    Include(AFindFlags, fsfIgnoreEnumVals);

  Result := inherited FindExportedSymbolInUnit(CU, ANameInfo, AnInfoEntry,
    AnIsExternal, AFindFlags);

  if Result and FInAllUnitSearch and (CU = FSystemCU) then begin
    FFoundSystemInfoEntry := AnInfoEntry;
    AnInfoEntry := nil;
    Result := False;
  end;
end;

function TFpDwarfFreePascalSymbolScope.FindExportedSymbolInUnits(
  const AName: String; const ANameInfo: TNameSearchInfo;
  SkipCompUnit: TDwarfCompilationUnit; out ADbgValue: TFpValue;
  const OnlyUnitNameLower: String): Boolean;
var
  i: Integer;
  CU: TDwarfCompilationUnit;
  FoundInfoEntry: TDwarfInformationEntry;
  FndIsExternal: Boolean;
  s: String;
begin
  if not FSearchSpecialCuDone then begin
    for i := 0 to Dwarf.CompilationUnitsCount - 1 do begin
      CU := Dwarf.CompilationUnits[i];
      s := LowerCase(CU.UnitName);
      if (s = 'system') then
        FSystemCU := CU;
      if (s = 'sysutils') then
        FSysUtilsCU := CU;
      if (s = 'typinfo') and (pos('objpas', LowerCase(CU.FileName)) > 0) then
        FTypInfoCU := CU;
    end;
    FSearchSpecialCuDone := True;
  end;

  FInAllUnitSearch := True;
  FFoundSystemInfoEntry := nil;
  Result := inherited FindExportedSymbolInUnits(AName, ANameInfo, SkipCompUnit,
    ADbgValue, OnlyUnitNameLower);
  FInAllUnitSearch := False;

  if (not Result) and (FFoundSystemInfoEntry <> nil) then
    ADbgValue := SymbolToValue(TFpSymbolDwarf.CreateSubClass(AName, FFoundSystemInfoEntry));

  FFoundSystemInfoEntry.ReleaseReference;
end;

function TFpDwarfFreePascalSymbolScope.FindLocalSymbol(const AName: String;
  const ANameInfo: TNameSearchInfo; InfoEntry: TDwarfInformationEntry; out
  ADbgValue: TFpValue): Boolean;
const
  selfname = 'self';
  // TODO: get reg num via memreader name-to-num
  RegFp64 = 6;
  RegPc64 = 16;
  RegFp32 = 5;
  RegPc32 = 8;
var
  StartScopeIdx, RegFp, RegPc: Integer;
  ParentFpVal: TFpValue;
  SearchCtx: TFpDbgDwarfSimpleLocationContext;
  par_fp, cur_fp, prev_fp, pc: TDbgPtr;
  d, i: Integer;
  ParentFpSym: TFpSymbolDwarf;
  Ctx: TFpDbgSimpleLocationContext;
begin
  Result := False;
  if not(Symbol is TFpSymbolDwarfDataProc) then
    exit;

  if Dwarf.TargetInfo.bitness = b64 then begin
    RegFP := RegFp64;
    RegPc := RegPc64;
  end
  else begin
    RegFP := RegFp32;
    RegPc := RegPc32;
  end;
  if (Length(AName) = length(selfname)) and (CompareUtf8BothCase(PChar(ANameInfo.NameUpper), PChar(ANameInfo.NameLower), @selfname[1])) then begin
    ADbgValue := GetSelfParameter;
    if ADbgValue <> nil then begin
      ADbgValue.AddReference;
      Result := True;
      exit;
    end;
  end;

  StartScopeIdx := InfoEntry.ScopeIndex;
  Result := inherited FindLocalSymbol(AName, ANameInfo, InfoEntry, ADbgValue);
  if Result then
    exit;

  if FOuterNotFound then
    exit;

  if FOuterNestContext <> nil then begin
    ADbgValue := FOuterNestContext.FindSymbol(AName); // TODO: pass upper/lower
    Result := True; // self, global was done by outer
    exit;
  end;


  InfoEntry.ScopeIndex := StartScopeIdx;
  if not InfoEntry.GoNamedChildEx(ParentFpLowerNameInfo) then begin
    InfoEntry.ScopeIndex := StartScopeIdx;
    if not InfoEntry.GoNamedChildEx(ParentFp2LowerNameInfo) then begin
      FOuterNotFound := True;
      exit;
    end;
  end;

  ParentFpSym := TFpSymbolDwarf.CreateSubClass(AName, InfoEntry);
  ParentFpVal := ParentFpSym.Value;
  if ParentFpVal = nil then begin
    Result := False;
    exit;
  end;
  ApplyContext(ParentFpVal);
  if not (svfOrdinal in ParentFpVal.FieldFlags) then begin
    DebugLn(FPDBG_DWARF_VERBOSE, 'no ordinal for parentfp');
    ParentFpSym.ReleaseReference;
    ParentFpVal.ReleaseReference;
    FOuterNotFound := True;
    exit;
  end;

  par_fp := ParentFpVal.AsCardinal;
  ParentFpVal.ReleaseReference;
  ParentFpSym.ReleaseReference;
  if par_fp = 0 then begin
    DebugLn(FPDBG_DWARF_VERBOSE, 'no ordinal for parentfp');
    FOuterNotFound := True;
    exit;
  end;

  // TODO: FindCallStackEntryByBasePointer, once all evaluates run in thread.
  i := LocationContext.StackFrame + 1;
  SearchCtx := TFpDbgDwarfSimpleLocationContext.Create(MemManager, 0, SizeOfAddress, LocationContext.ThreadId, i);

  cur_fp := 0;
  if LocationContext.ReadRegister(RegFp, cur_fp) then begin
    if cur_fp > par_fp then
      d := -1  // cur_fp must go down
    else
      d := 1;  // cur_fp must go up
    while not (cur_fp = par_fp) do begin
      SearchCtx.FStackFrame := i;
      // TODO: get reg num via memreader name-to-num
      prev_fp := cur_fp;
      if not SearchCtx.ReadRegister(RegFp, cur_fp) then
        break;
      inc(i);
      if (cur_fp = prev_fp) or ((cur_fp < prev_fp) xor (d = -1)) then
        break;  // wrong direction
      if i > LocationContext.StackFrame + 200 then break; // something wrong? // TODO better check
    end;
    dec(i);
  end;

  if (par_fp <> cur_fp) or (cur_fp = 0) or
     (i <= 0) or
     not SearchCtx.ReadRegister(RegPc, pc)
  then begin
    FOuterNotFound := True;
    SearchCtx.ReleaseReference;
    exit;
  end;

  SearchCtx.ReleaseReference;

  Ctx := TFpDbgSimpleLocationContext.Create(MemManager, pc, SizeOfAddress, LocationContext.ThreadId, i);
  FOuterNestContext := Dwarf.FindSymbolScope(Ctx, pc);
  Ctx.ReleaseReference;

  ADbgValue := FOuterNestContext.FindSymbol(AName); // TODO: pass upper/lower
  Result := True; // self, global was done by outer
end;

function TFpDwarfFreePascalSymbolScope.FindSymbolInStructure(
  const AName: String; const ANameInfo: TNameSearchInfo;
  InfoEntry: TDwarfInformationEntry; out ADbgValue: TFpValue): Boolean;
var
  CU: TDwarfCompilationUnit;
  CurClassName, StaticName, FoundName: String;
  MangledNameInfo: TNameSearchInfo;
  FoundInfoEntry: TDwarfInformationEntry;
  IsExternal: Boolean;
begin
  Result := inherited FindSymbolInStructure(AName, ANameInfo, InfoEntry, ADbgValue);
  if Result then
    exit;

  CU := InfoEntry.CompUnit;
  if (CU <> nil) and InfoEntry.HasValidScope and
     InfoEntry.ReadName(CurClassName) and not InfoEntry.IsArtificial
  then begin
    StaticName := FClassVarStaticPrefix + LowerCase(CurClassName) + '_' + UpperCase(AName);
    MangledNameInfo := NameInfoForSearch(StaticName);

    if CU.KnownNameHashes^[MangledNameInfo.NameHash and KnownNameHashesBitMask] then begin
      if FindExportedSymbolInUnit(CU, MangledNameInfo, FoundInfoEntry, IsExternal) then begin
        if {(IsExternal) and} (FoundInfoEntry.ReadName(FoundName)) then begin
          if FoundName = StaticName then begin // must be case-sensitive
            ADbgValue := SymbolToValue(TFpSymbolDwarf.CreateSubClass(AName, FoundInfoEntry));
            Result := True;
          end;
        end;
        FoundInfoEntry.ReleaseReference;
        if Result then
          exit;
      end;
    end;
  end;
end;

procedure TFpDwarfFreePascalSymbolScope.Init;
begin
  inherited Init;
  FClassVarStaticPrefix := '_static_';
end;

destructor TFpDwarfFreePascalSymbolScope.Destroy;
begin
  FOuterNestContext.ReleaseReference;
  inherited Destroy;
end;

{ TFpDwarfFreePascalSymbolScopeDwarf3 }

procedure TFpDwarfFreePascalSymbolScopeDwarf3.Init;
begin
  inherited Init;
  FClassVarStaticPrefix := '$_static_';
end;

{ TFpSymbolDwarfV2FreePascalTypeStructure }

function TFpSymbolDwarfV2FreePascalTypeStructure.IsShortString: Boolean;
var
  LenSym, StSym, StSymType: TFpSymbol;
begin
  if FIsShortString <> issUnknown then
    exit(FIsShortString = issShortString);

  Result := False;
  FIsShortString := issStructure;
  if (inherited NestedSymbolCount <> 2) then
    exit;

  if (Name <> 'ShortString') and (Name <> 'LongString') then  // DWARF-2 => user types are all caps
    exit;

  LenSym := inherited NestedSymbolByName['length'];
  if (LenSym = nil) or (LenSym.Kind <> skCardinal) // or (LenSym.Size <> 1) // not implemented yet
  then
    exit;

  StSym := inherited NestedSymbolByName['st'];
  if (StSym = nil) then
    exit;
  StSymType := StSym.TypeInfo;
  if (StSymType = nil) or (StSymType.Kind <> skArray) or not (StSymType is TFpSymbolDwarfTypeArray) then
    exit;

  FIsShortString := issShortString;
  Result := True;
end;

function TFpSymbolDwarfV2FreePascalTypeStructure.GetTypedValueObject(
  ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType): TFpValueDwarf;
begin
  if AnOuterType = nil then
    AnOuterType := Self;
  if not IsShortString then
    Result := inherited GetTypedValueObject(ATypeCast, AnOuterType)
  else
    Result := TFpValueDwarfV2FreePascalShortString.Create(AnOuterType);
end;

procedure TFpSymbolDwarfV2FreePascalTypeStructure.KindNeeded;
begin
  if not IsShortString then
    inherited KindNeeded
  else
    SetKind(skString);
end;

function TFpSymbolDwarfV2FreePascalTypeStructure.GetNestedSymbolCount: Integer;
begin
  if IsShortString then
    Result := 0
  else
    Result := inherited GetNestedSymbolCount;
end;

{ TFpSymbolDwarfFreePascalTypeDeclaration }

function TFpSymbolDwarfFreePascalTypeDeclaration.DoGetNestedTypeInfo: TFpSymbolDwarfType;
var
  ti: TFpSymbolDwarfType;
begin
  Result := inherited DoGetNestedTypeInfo;

  // Is internal class pointer?
  // Do not trigged any cached property of the pointer
  if (Result = nil) or
     not (Result is TFpSymbolDwarfFreePascalTypePointer)
  then
    exit;

  ti := TFpSymbolDwarfFreePascalTypePointer(Result).NestedTypeInfo;
  // only if it is NOT a declaration
  if ti is TFpSymbolDwarfTypeStructure then
    TFpSymbolDwarfFreePascalTypePointer(Result).IsInternalPointer := True;
end;

{ TFpSymbolDwarfFreePascalTypePointer }

function TFpSymbolDwarfFreePascalTypePointer.GetIsInternalPointer: Boolean;
begin
  Result := FIsInternalPointer or IsInternalDynArrayPointer;
end;

function TFpSymbolDwarfFreePascalTypePointer.IsInternalDynArrayPointer: Boolean;
var
  ti: TFpSymbol;
begin
  Result := False;
  ti := NestedTypeInfo;  // Same as TypeInfo, but does not try to be forwarded
  Result := ti is TFpSymbolDwarfTypeArray;
  if Result then
    Result := (sfDynArray in ti.Flags);
end;

function TFpSymbolDwarfFreePascalTypePointer.GetInternalTypeInfo: TFpSymbol;
begin
  if IsInternalPointer then
    Result := NestedTypeInfo.InternalTypeInfo
  else
    Result := inherited GetInternalTypeInfo;
end;

procedure TFpSymbolDwarfFreePascalTypePointer.TypeInfoNeeded;
var
  p: TFpSymbol;
begin
  p := NestedTypeInfo;
  if IsInternalPointer and (p <> nil) then
    p := p.TypeInfo;
  SetTypeInfo(p);
end;

procedure TFpSymbolDwarfFreePascalTypePointer.KindNeeded;
var
  k: TDbgSymbolKind;
begin
  if IsInternalPointer then begin
      k := NestedTypeInfo.Kind;
      if k in [skObject, skRecord] then   // TODO
        SetKind(skInterface)
      else
        SetKind(k);
  end
  else
    inherited;
end;

function TFpSymbolDwarfFreePascalTypePointer.DoReadStride(
  AValueObj: TFpValueDwarf; out AStride: TFpDbgValueSize): Boolean;
begin
  if IsInternalPointer then
    Result := NestedTypeInfo.ReadStride(AValueObj, AStride)
  else
    Result := inherited DoReadStride(AValueObj, AStride);
end;

procedure TFpSymbolDwarfFreePascalTypePointer.ForwardToSymbolNeeded;
begin
  if IsInternalPointer then
    SetForwardToSymbol(NestedTypeInfo) // Same as TypeInfo, but does not try to be forwarded
  else
    SetForwardToSymbol(nil); // inherited ForwardToSymbolNeeded;
end;

function TFpSymbolDwarfFreePascalTypePointer.GetNextTypeInfoForDataAddress(
  ATargetType: TFpSymbolDwarfType): TFpSymbolDwarfType;
begin
  if IsInternalPointer then
    Result := NestedTypeInfo
  else
    Result := inherited;
end;

function TFpSymbolDwarfFreePascalTypePointer.GetDataAddressNext(
  AValueObj: TFpValueDwarf; var AnAddress: TFpDbgMemLocation; out
  ADoneWork: Boolean; ATargetType: TFpSymbolDwarfType): Boolean;
begin
  Result := inherited GetDataAddressNext(AValueObj, AnAddress, ADoneWork, ATargetType);
  if (not IsInternalPointer) and (ATargetType = nil) then exit;

  if (not Result) or ADoneWork then
    exit;

  Result := AValueObj.MemManager <> nil;
  if not Result then
    exit;
  AnAddress := AValueObj.Context.ReadAddress(AnAddress, SizeVal(CompilationUnit.AddressSize));
  Result := IsValidLoc(AnAddress);

  if (not Result) and
     IsError(AValueObj.Context.LastMemError)
  then
    SetLastError(AValueObj, AValueObj.Context.LastMemError);
end;

function TFpSymbolDwarfFreePascalTypePointer.GetTypedValueObject(
  ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType): TFpValueDwarf;
begin
  if AnOuterType = nil then
    AnOuterType := Self;
  if IsInternalPointer then
    Result := NestedTypeInfo.GetTypedValueObject(ATypeCast, AnOuterType)
  else
    Result := inherited GetTypedValueObject(ATypeCast, AnOuterType);
end;

function TFpSymbolDwarfFreePascalTypePointer.DoReadDataSize(
  const AValueObj: TFpValue; out ADataSize: TFpDbgValueSize): Boolean;
begin
  if Kind = skClass then begin
    // TODO: get/adjust a value object to have the deref address // see ConstRefOrExprFromAttrData
    Result := NestedTypeInfo.ReadSize(AValueObj, ADataSize);
    if not Result then
      ADataSize := ZeroSize;
  end
  else
    Result := inherited DoReadDataSize(AValueObj, ADataSize);
end;

{ TFpSymbolDwarfFreePascalTypeStructure }

procedure TFpSymbolDwarfFreePascalTypeStructure.KindNeeded;
var
  t: TDbgSymbolKind;
begin
  (* DW_TAG_structure_type
     - Is either objec or record.
     - Except: fpc < 3.0 => can be class or interface too
     DW_TAG_class_type
     - Is either class, interface, or object (object only with virtual methods)

     tested up to fpc 3.2 beta
  *)
  if (InformationEntry.AbbrevTag = DW_TAG_interface_type) then begin
    SetKind(skInterface);
  end
  else
  if TypeInfo <> nil then begin // inheritance
    t := TypeInfo.Kind;
    if t = skRecord then
      t := skObject; // could be skInterface
    SetKind(t); // skClass, skInterface or skObject
  end
  else
  begin
    if NestedSymbolByName['_vptr$TOBJECT'] <> nil then
      SetKind(skClass)
    else
    if NestedSymbolByName['_vptr$'+Name] <> nil then // vptr is only present for skObject with virtual methods/Constructor
      SetKind(skObject)
    else
    if (InformationEntry.AbbrevTag = DW_TAG_class_type) then
      SetKind(skObject)   // could be skInterface  // fix in TFpSymbolDwarfFreePascalTypePointer.KindNeeded
    else
      SetKind(skRecord);  // could be skObject(?) or skInterface   // fix in TFpSymbolDwarfFreePascalTypePointer.KindNeeded
  end;
end;

function TFpSymbolDwarfFreePascalTypeStructure.GetInstanceClassName(
  AValueObj: TFpValue; AClassName, AUnitName: PString;
  AParentClassIndex: integer): boolean;
var
  AnErr: TFpError;
begin
  Result := AValueObj is TFpValueDwarf;
  if not Result then
    exit;
  Result := GetInstanceClassNameFromPVmt(LocToAddrOrNil(AValueObj.DataAddress),
    TFpValueDwarf(AValueObj).Context, TFpValueDwarf(AValueObj).Context.SizeOfAddress,
    AClassName, AUnitName, AnErr, AParentClassIndex,
    TFpDwarfFreePascalSymbolClassMap(CompilationUnit.DwarfSymbolClassMap).FCompilerVersion
  );

  if not Result then
    SetLastError(AValueObj, AnErr);
end;

class function TFpSymbolDwarfFreePascalTypeStructure.GetInstanceClassNameFromPVmt
  (APVmt: TDbgPtr; AContext: TFpDbgLocationContext; ASizeOfAddr: Integer;
  AClassName, AUnitName: PString; out AnError: TFpError;
  AParentClassIndex: integer; ACompilerVersion: Cardinal): boolean;

  function CheckIsReadableMem(AMem: TFpDbgMemLocation): Boolean;
  begin
    Result := IsReadableMem(AMem);
    if not Result then
      AnError := CreateError(fpErrCanNotReadMemAtAddr, [AMem.Address]);
  end;

var
  VmtAddr, ClassNameAddr, A: TFpDbgMemLocation;
  NameLen: QWord;
begin
  Result := False;
  AnError := NoError;
  if AClassName <> nil then AClassName^ := '';
  if AUnitName  <> nil then AUnitName^ := '';

  if not AContext.ReadAddress(TargetLoc(APVmt), SizeVal(ASizeOfAddr), VmtAddr) then begin
    AnError := AContext.LastMemError;
    AContext.ClearLastMemError;
    exit;
  end;
  if not CheckIsReadableMem(VmtAddr) then
    exit;

  while AParentClassIndex > 0 do begin
    {$PUSH}{$Q-}{$R-}
    VmtAddr.Address := VmtAddr.Address + TDBGPtr(2 * ASizeOfAddr);
    {$POP}
    A := VmtAddr;
    if not AContext.ReadAddress(A, SizeVal(ASizeOfAddr), VmtAddr) then begin
      AnError := AContext.LastMemError;
    AContext.ClearLastMemError;
      exit;
    end;
    if IsTargetNil(VmtAddr) then
      exit; // no error / top parent reached
    if not CheckIsReadableMem(VmtAddr) then
      exit;

    if (ACompilerVersion >= $030200)
    then begin
      A := VmtAddr;
      if not AContext.ReadAddress(A, SizeVal(ASizeOfAddr), VmtAddr) then begin
        AnError := AContext.LastMemError;
        AContext.ClearLastMemError;
        exit;
      end;
      if not CheckIsReadableMem(VmtAddr) then
        exit;
    end;

    dec(AParentClassIndex);
  end;

  {$PUSH}{$Q-}{$R-}
  VmtAddr.Address := VmtAddr.Address + TDBGPtr(3 * ASizeOfAddr);
  {$POP}

  if AClassName <> nil then begin
    if not AContext.ReadAddress(VmtAddr, SizeVal(ASizeOfAddr), ClassNameAddr) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;
    if not CheckIsReadableMem(ClassNameAddr) then
      exit;

    if not AContext.ReadUnsignedInt(ClassNameAddr, SizeVal(1), NameLen) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;
    if NameLen = 0 then begin
      AnError := CreateError(fpErrAnyError, ['No name found']);
      exit;
    end;
    if not AContext.MemManager.SetLength(AClassName^, NameLen) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;

    ClassNameAddr.Address := ClassNameAddr.Address + 1;
    Result := AContext.ReadMemory(ClassNameAddr, SizeVal(NameLen), @AClassName^[1]);
    if not Result then
      AnError := AContext.LastMemError;
    AContext.ClearLastMemError;
  end;

  if AUnitName <> nil then begin
    // get vTypeInfo
    {$PUSH}{$Q-}{$R-}
    VmtAddr.Address := VmtAddr.Address + TDBGPtr(4 * ASizeOfAddr);
    {$POP}

    if not AContext.ReadAddress(VmtAddr, SizeVal(ASizeOfAddr), ClassNameAddr) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;
    if not CheckIsReadableMem(ClassNameAddr) then
      exit;

    //inc(Pointer(classtypeinfo), PByte(Pointer(classtypeinfo)+1)^ + 2);
    A := ClassNameAddr;
    {$PUSH}{$Q-}{$R-}
    A.Address := A.Address + 1;
    {$POP}
    if not AContext.ReadUnsignedInt(A, SizeVal(1), NameLen) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;
    {$PUSH}{$Q-}{$R-}
    ClassNameAddr.Address := ClassNameAddr.Address + TDBGPtr(NameLen + 2) + TDBGPtr(2 * ASizeOfAddr + 2);
    if (ACompilerVersion >= $030300) then
      ClassNameAddr.Address := ClassNameAddr.Address + TDBGPtr(ASizeOfAddr);
    {$POP}
    // Maybe align to next qword


    if not AContext.ReadUnsignedInt(ClassNameAddr, SizeVal(1), NameLen) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;
    if NameLen = 0 then begin
      AnError := CreateError(fpErrAnyError, ['No name found']);
      exit;
    end;
    if not AContext.MemManager.SetLength(AUnitName^, NameLen) then begin
      AnError := AContext.LastMemError;
      AContext.ClearLastMemError;
      exit;
    end;

    ClassNameAddr.Address := ClassNameAddr.Address + 1;
    Result := AContext.ReadMemory(ClassNameAddr, SizeVal(NameLen), @AUnitName^[1]);
    if not Result then
      AnError := AContext.LastMemError;
    AContext.ClearLastMemError;
  end;
end;

{ TFpValueDwarfV2FreePascalShortString }

function TFpValueDwarfV2FreePascalShortString.IsValidTypeCast: Boolean;
begin
  // currently only allow this / used by array access
  Result := TypeCastSourceValue is TFpValueConstAddress;
end;

function TFpValueDwarfV2FreePascalShortString.GetInternMemberByName(
  const AIndex: String): TFpValue;
begin
  if HasTypeCastInfo then begin
    Result := TypeInfo.GetNestedValueByName(AIndex);
    TFpValueDwarf(Result).StructureValue := Self;
    if (TFpValueDwarf(Result).Context = nil) then
      TFpValueDwarf(Result).Context := Context;
  end
  else
    Result := MemberByName[AIndex];
end;

procedure TFpValueDwarfV2FreePascalShortString.Reset;
begin
  inherited Reset;
  FValueDone := False;
end;

function TFpValueDwarfV2FreePascalShortString.GetMemberCount: Integer;
var
  LenSym: TFpValueDwarf;
begin
  LenSym := TFpValueDwarf(GetInternMemberByName('length'));
  assert(LenSym is TFpValueDwarf, 'LenSym is TFpValueDwarf');
  Result := LenSym.AsInteger;
  LenSym.ReleaseReference;
end;

function TFpValueDwarfV2FreePascalShortString.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := inherited GetFieldFlags;
  Result := Result + [svfString];
end;

function TFpValueDwarfV2FreePascalShortString.GetAsString: AnsiString;
var
  len: QWord;
  Size: TFpDbgValueSize;
  LenSym, StSym: TFpValueDwarf;
begin
  if FValueDone then
    exit(FValue);

  LenSym := TFpValueDwarf(GetInternMemberByName('length'));
  assert(LenSym is TFpValueDwarf, 'LenSym is TFpValueDwarf');
  len := LenSym.AsCardinal;
  LenSym.ReleaseReference;

  if not GetSize(Size) then begin;
    SetLastError(CreateError(fpErrAnyError));
    exit('');
  end;
  if (Size < len) then begin
    SetLastError(CreateError(fpErrAnyError));
    exit('');
  end;

  if not MemManager.SetLength(Result, len) then begin
    SetLastError(MemManager.LastError);
    exit;
  end;

  StSym := TFpValueDwarf(GetInternMemberByName('st'));
  assert(StSym is TFpValueDwarf, 'StSym is TFpValueDwarf');

  if len > 0 then
    if not Context.ReadMemory(StSym.DataAddress, SizeVal(len), @Result[1]) then begin
      Result := ''; // TODO: error
      SetLastError(Context.LastMemError);
      StSym.ReleaseReference;
      exit;
    end;
  StSym.ReleaseReference;

  FValue := Result;
  FValueDone := True;
end;

function TFpValueDwarfV2FreePascalShortString.GetAsWideString: WideString;
begin
  Result := GetAsString;
end;

{ TFpSymbolDwarfFreePascalSymbolTypeArray }

function TFpSymbolDwarfFreePascalSymbolTypeArray.GetTypedValueObject(
  ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType): TFpValueDwarf;
begin
  if AnOuterType = nil then
    AnOuterType := Self;
  Result := TFpValueDwarfFreePascalArray.Create(AnOuterType, Self);
end;

{ TFpValueDwarfFreePascalArray }

function TFpValueDwarfFreePascalArray.GetKind: TDbgSymbolKind;
begin
  if TypeInfo <> nil then
    Result := TypeInfo.Kind
  else
    Result := inherited GetKind;
end;

function TFpValueDwarfFreePascalArray.GetMemberCount: Integer;
var
  t, t2: TFpSymbol;
  Info: TDwarfInformationEntry;
  n: AnsiString;
  UpperBoundSym: TFpSymbolDwarf;
  val: TFpValue;
  l, h: Int64;
  Addr: TFpDbgMemLocation;
begin
  Result := 0;
  t := TypeInfo;
  if (t.Kind <> skArray) or (t.NestedSymbolCount < 1) then // IndexTypeCount;
    exit(inherited GetMemberCount);

  t2 := t.NestedSymbol[0]; // IndexType[0];
  if not (t2 is TFpSymbolDwarfTypeSubRange) then
    exit(inherited GetMemberCount);


  TFpSymbolDwarfTypeSubRange(t2).GetValueBounds(Self, l, h);
  if (l <> 0) or
     (TFpSymbolDwarfTypeSubRange(t2).LowBoundState <> rfConst) or
     (TFpSymbolDwarfTypeSubRange(t2).HighBoundState <> rfNotFound) or
     (TFpSymbolDwarfTypeSubRange(t2).CountState <> rfNotFound)
  then
    exit(inherited GetMemberCount);

  // Check for open array param
  if (t is TFpSymbolDwarfTypeArray) and
     (DbgSymbol is TFpSymbolDwarfDataParameter) // open array exists only as param
  then begin
    Info := TFpSymbolDwarfDataParameter(DbgSymbol).InformationEntry.Clone;
    Info.GoNext;
    if Info.HasValidScope and
       Info.HasAttrib(DW_AT_location) and  // the high param must have a location / cannot be a constant
       Info.ReadName(n)
    then begin
      if (n <> '') and (n[1] = '$') then // dwarf3 // TODO: make required in dwarf3
        delete(n, 1, 1);
      if (copy(n,1,4) = 'high')
      and (CompareText(copy(n, 5, length(n)), DbgSymbol.Name) = 0) then begin
        UpperBoundSym := TFpSymbolDwarf.CreateSubClass('', Info);
        if UpperBoundSym <> nil then begin
          val := UpperBoundSym.Value;
          if val <> nil then begin
            TFpValueDwarf(val).Context := Context;
            h := Val.AsInteger;
            val.ReleaseReference;
            if (h >= 0) and (h < maxLongint) then begin
              Result := h + 1;
            end
            else
              Result := 0;
  // TODO h < -1  => Error
            Info.ReleaseReference;
            UpperBoundSym.ReleaseReference;
            exit;
          end;
        end;
      end;
    end;
    Info.ReleaseReference;
  end;

  // dynamic array
  if (sfDynArray in t.Flags) and (AsCardinal <> 0) and GetDwarfDataAddress(Addr) then begin
    if not (IsReadableMem(Addr) and (LocToAddr(Addr) > AddressSize)) then
      exit(0); // dyn array, but bad data
    Addr.Address := Addr.Address - AddressSize;
    if Context.ReadSignedInt(Addr, SizeVal(AddressSize), h) then begin
// TODO h < -1  => Error
      if (h >= 0) and (h < maxLongint) then
        Result := h+1;
      exit;
    end
    else
      SetLastError(Context.LastMemError);
    Result := 0;
    exit;
  end;

  // Should not be here. There is no knowledeg how many members there are
  Result := inherited GetMemberCount;
end;

function TFpValueDwarfFreePascalArray.DoGetStride(out AStride: TFpDbgValueSize
  ): Boolean;
begin
  if (TFpDwarfFreePascalSymbolClassMap(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030300)
  then
    Result := inherited DoGetStride(AStride)
  else
    Result := TFpSymbolDwarfType(TypeInfo.NestedSymbol[0]).ReadStride(Self, AStride);
end;

function TFpValueDwarfFreePascalArray.DoGetMainStride(out
  AStride: TFpDbgValueSize): Boolean;
begin
  if (TFpDwarfFreePascalSymbolClassMap(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030300)
  then
    Result := inherited DoGetMainStride(AStride)
  else
    Result := GetMemberSize(AStride);
end;

function TFpValueDwarfFreePascalArray.DoGetDimStride(AnIndex: integer; out
  AStride: TFpDbgValueSize): Boolean;
begin
  if (TFpDwarfFreePascalSymbolClassMap(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030300)
  then
    Result := inherited DoGetDimStride(AnIndex, AStride)
  else
  begin
    Result := True;
    AStride := ZeroSize;
  end;
end;

function TFpValueDwarfFreePascalArray.GetFpcRefCount(out ARefCount: Int64
  ): Boolean;
var
  Addr: TFpDbgMemLocation;
begin
  ARefCount := 0;
  Result := (TypeInfo <> nil) and (sfDynArray in TypeInfo.Flags);
  if not Result then
    exit;

  Result := AsCardinal = 0;
  if Result then
    exit;

  if not( GetDwarfDataAddress(Addr) and IsReadableLoc(Addr) ) then
    exit;

  Addr:= Addr - (AddressSize * 2);
  Result := Context.ReadSignedInt(Addr, SizeVal(AddressSize), ARefCount);
end;

{ TFpSymbolDwarfV3FreePascalSymbolTypeArray }

function TFpSymbolDwarfV3FreePascalSymbolTypeArray.GetInternalStringType: TArrayOrStringType;
var
  Info: TDwarfInformationEntry;
  t: Cardinal;
  t2: TFpSymbol;
  CharSize: TFpDbgValueSize;
  LocData: array of byte;
begin
  Result := FArrayOrStringType;
  if Result <> iasUnknown then
    exit;

  FArrayOrStringType := iasArray;
  Result := FArrayOrStringType;

  t2 := TypeInfo;
  if (t2 = nil) or (t2.Kind <> skChar) then
    exit;

  // TODO: check lowbound = 1 (const)

  Info := InformationEntry.FirstChild;
  if Info = nil then
    exit;

  while Info.HasValidScope do begin
    t := Info.AbbrevTag;
    if (t = DW_TAG_enumeration_type) then
      break;
    if (t = DW_TAG_subrange_type) then begin
      if Info.HasAttrib(DW_AT_byte_stride) or Info.HasAttrib(DW_AT_type) then
        break;

      // TODO: check the location parser, if it is a reference

      if InformationEntry.ReadValue(DW_AT_data_location, LocData) then begin
        if (Length(LocData) = 3) and
           (LocData[0] = $97) and
           (LocData[1] = $31) and
           (LocData[2] = $22)
        then begin
          FArrayOrStringType := iasShortString;
          break;
        end;
      end;

      if not t2.ReadSize(nil, CharSize) then
        CharSize := ZeroSize; // TODO: error
      if (CharSize.Size = 2) then
        FArrayOrStringType := iasUnicodeString
      else
        FArrayOrStringType := iasAnsiString;
      break;
    end;
    Info.GoNext;
  end;

  Info.ReleaseReference;
  Result := FArrayOrStringType;
end;

function TFpSymbolDwarfV3FreePascalSymbolTypeArray.GetTypedValueObject(
  ATypeCast: Boolean; AnOuterType: TFpSymbolDwarfType): TFpValueDwarf;
begin
  if AnOuterType = nil then
    AnOuterType := Self;
  if GetInternalStringType in [iasShortString, iasAnsiString, iasUnicodeString] then
    Result := TFpValueDwarfV3FreePascalString.Create(AnOuterType)
  else
    Result := inherited GetTypedValueObject(ATypeCast, AnOuterType);
end;

procedure TFpSymbolDwarfV3FreePascalSymbolTypeArray.KindNeeded;
begin
  case GetInternalStringType of
    iasShortString:
      SetKind(skString);
    iasAnsiString:
      SetKind(skString); // TODO skAnsiString
    iasUnicodeString:
      SetKind(skWideString);
    else
      inherited KindNeeded;
  end;
end;

function TFpSymbolDwarfV3FreePascalSymbolTypeArray.DoReadSize(
  const AValueObj: TFpValue; out ASize: TFpDbgValueSize): Boolean;
begin
  if GetInternalStringType in [iasAnsiString, iasUnicodeString] then begin
    ASize := ZeroSize;
    ASize.Size := CompilationUnit.AddressSize;
    Result := True;
  end
  else begin
    Result := inherited DoReadSize(AValueObj, ASize);
    if (not Result) and (GetInternalStringType = iasArray) then begin
      ASize := ZeroSize;
      ASize.Size := CompilationUnit.AddressSize;
      Result := True;
    end;
  end;
end;

{ TFpValueDwarfV3FreePascalString }

function TFpValueDwarfV3FreePascalString.GetCodePage: TSystemCodePage;
begin
  GetAsString;
  Result := FDynamicCodePage;
end;

function TFpValueDwarfV3FreePascalString.IsValidTypeCast: Boolean;
var
  f: TFpValueFieldFlags;
begin
  Result := HasTypeCastInfo;
  If not Result then
    exit;

  assert(TypeInfo.Kind in [skString, skWideString], 'TFpValueDwarfArray.IsValidTypeCast: TypeInfo.Kind = skArray');

  f := TypeCastSourceValue.FieldFlags;
  if (f * [svfAddress, svfSize, svfSizeOfPointer] = [svfAddress]) or
     (svfOrdinal in f)
  then
    exit;

  //if sfDynArray in TypeInfo.Flags then begin
  //  // dyn array
  //  if (svfOrdinal in f)then
  //    exit;
  //  if (f * [svfAddress, svfSize] = [svfAddress, svfSize]) and
  //     (TypeCastSourceValue.Size = TypeInfo.CompilationUnit.AddressSize)
  //  then
  //    exit;
  //  if (f * [svfAddress, svfSizeOfPointer] = [svfAddress, svfSizeOfPointer]) then
  //    exit;
  //end
  //else begin
  //  // stat array
  //  if (f * [svfAddress, svfSize] = [svfAddress, svfSize]) and
  //     (TypeCastSourceValue.Size = TypeInfo.Size)
  //  then
  //    exit;
  //end;
  Result := False;
end;

procedure TFpValueDwarfV3FreePascalString.Reset;
begin
  inherited Reset;
  FValueDone := False;
  FBoundsDone := False;
end;

function TFpValueDwarfV3FreePascalString.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := inherited GetFieldFlags;
  case TypeInfo.Kind of
    skWideString: Result := Result + [svfWideString, svfDataAddress];
    else          Result := Result + [svfString, svfDataAddress];
  end;
end;

function TFpValueDwarfV3FreePascalString.GetSubString(AStartIndex, ALen: Int64;
  out ASubStr: AnsiString; AIgnoreBounds: Boolean): Boolean;
var
  Addr: TFpDbgMemLocation;
  FullLen: Int64;
  t: TFpSymbol;
  WResult: WideString;
  RResult: RawByteString;
  Codepage: TSystemCodePage;
begin
  Result := True;
  ASubStr := '';

  if AStartIndex < 1 then begin // not supported, return partial
    Result := AIgnoreBounds;
    ALen := ALen + AStartIndex - 1;
    AStartIndex := 1;
  end;

  // get length
  CalcBounds;
  if FHighBound < FLowBound then
    exit; // empty string
  {$PUSH}{$Q-}{$R-}
  FullLen := FHighBound-FLowBound+1;
  {$POP}

  if AStartIndex - 1 + ALen > FullLen then begin
    Result := AIgnoreBounds;
    ALen := FullLen - (AStartIndex - 1);

    if AStartIndex = 1 then begin
      ASubStr := AsString;  // get the full string
      exit;
    end;
  end;

  if FullLen < 256 then
    AsString; // prefer to cache

  if FValueDone and (AStartIndex + ALen <= Length(FValue)) then begin
    ASubStr := Copy(FValue, AStartIndex, ALen);
    exit;
  end;

  if not CheckTypeAndGetAddr(Addr) then
    exit(False);


  if (MemManager.MemLimits.MaxStringLen > 0) and
     (QWord(ALen) > MemManager.MemLimits.MaxStringLen)
  then
    ALen := MemManager.MemLimits.MaxStringLen;

  if ALen <= 0 then
    exit;

  t := TypeInfo;
  if t.Kind = skWideString then begin
    {$PUSH}{$Q-}{$R-}
    Addr.Address := Addr.Address + (AStartIndex - 1) * 2;
    {$POP}
    if not ( (MemManager.SetLength(WResult, ALen)) and
             (Context.ReadMemory(Addr, SizeVal(ALen*2), @WResult[1])) )
    then
      SetLastError(Context.LastMemError)
    else
      ASubStr := WResult;
  end else
  if Addr.Address = Address.Address + 1 then begin
    // shortstring
    {$PUSH}{$Q-}{$R-}
    Addr.Address := Addr.Address + AStartIndex - 1;
    {$POP}
    if not ( (MemManager.SetLength(ASubStr, ALen)) and
             (Context.ReadMemory(Addr, SizeVal(ALen), @ASubStr[1])) )
    then begin
      ASubStr := '';
      SetLastError(Context.LastMemError);
    end;
  end
  else begin
    {$PUSH}{$Q-}{$R-}
    Addr.Address := Addr.Address + QWord(AStartIndex - 1);
    {$POP}
    if not ( (MemManager.SetLength(RResult, ALen)) and
             (Context.ReadMemory(Addr, SizeVal(ALen), @RResult[1])) )
    then begin
      SetLastError(Context.LastMemError);
    end else begin
      if ObtainDynamicCodePage(Addr, Codepage) then
        begin
        SetCodePage(RResult, Codepage, False);
        FDynamicCodePage:=Codepage;
        end;
      ASubStr := RResult;
    end;
  end;
end;

function TFpValueDwarfV3FreePascalString.GetSubWideString(AStartIndex,
  ALen: Int64; out ASubStr: WideString; AIgnoreBounds: Boolean): Boolean;
var
  WSubStr: AnsiString;
begin
  Result := GetSubString(AStartIndex, ALen, WSubStr, AIgnoreBounds);
  ASubStr := WSubStr;
end;

function TFpValueDwarfV3FreePascalString.GetAsString: AnsiString;
var
  t: TFpSymbol;
  Len: Int64;
  Addr: TFpDbgMemLocation;
  WResult: WideString;
  RResult: RawByteString;
  Codepage: TSystemCodePage;
begin
  if FValueDone then
    exit(FValue);

  // TODO: error handling
  FValue := '';
  Result := '';
  FValueDone := True;

  if not CheckTypeAndGetAddr(Addr) then
    exit;

  // get length
  CalcBounds;
  if FHighBound < FLowBound then
    exit; // empty string
  {$PUSH}{$Q-}{$R-}
  Len := FHighBound-FLowBound+1;
  {$POP}

  if (MemManager.MemLimits.MaxStringLen > 0) and
     (QWord(Len) > MemManager.MemLimits.MaxStringLen)
  then
      Len := MemManager.MemLimits.MaxStringLen;


  t := TypeInfo;
  if t.Kind = skWideString then begin
    if not ( (MemManager.SetLength(WResult, Len)) and
             (Context.ReadMemory(Addr, SizeVal(Len*2), @WResult[1])) )
    then
      SetLastError(Context.LastMemError)
    else
      Result := WResult;
  end else
  if Addr.Address = Address.Address + 1 then begin
    // shortstring
    if not ( (MemManager.SetLength(Result, Len)) and
             (Context.ReadMemory(Addr, SizeVal(Len), @Result[1])) )
    then begin
      Result := '';
      SetLastError(Context.LastMemError);
    end;
  end
  else begin
    if not ( (MemManager.SetLength(RResult, Len)) and
             (Context.ReadMemory(Addr, SizeVal(Len), @RResult[1])) )
    then begin
      SetLastError(Context.LastMemError);
    end else begin
      if ObtainDynamicCodePage(Addr, Codepage) then
        begin
        SetCodePage(RResult, Codepage, False);
        FDynamicCodePage:=Codepage;
        end;
      Result := RResult;
    end;
  end;

  FValue := Result;
end;

function TFpValueDwarfV3FreePascalString.GetAsWideString: WideString;
begin
  // todo: widestring, but currently that is encoded as PWideChar
  Result := GetAsString;
end;

procedure TFpValueDwarfV3FreePascalString.SetAsCardinal(AValue: QWord);
begin
  if not Context.WriteUnsignedInt(Address, SizeVal(AddressSize), AValue) then begin
    SetLastError(Context.LastMemError);
  end;
  FValueDone := False;
  FBoundsDone := False;
end;

function TFpValueDwarfV3FreePascalString.GetAsCardinal: QWord;
var
  d: TFpDbgMemLocation;
begin
  d := DataAddress;
  if IsTargetAddr(d) then
    Result := DataAddress.Address
  else
    Result := inherited GetAsCardinal;
end;

function TFpValueDwarfV3FreePascalString.GetMemberCount: Integer;
begin
  CalcBounds;
  Result := Max(0, FHighBound - FLowBound + 1);
end;

function TFpValueDwarfV3FreePascalString.GetFpcRefCount(out ARefCount: Int64
  ): Boolean;
var
  Addr: TFpDbgMemLocation;
begin
  ARefCount := 0;
  Result := (TypeInfo.Kind = skString);
  if not Result then
    exit;

  GetDwarfDataAddress(Addr);
  if (not IsValidLoc(Addr)) and
     (HasTypeCastInfo) and
     (svfOrdinal in TypeCastSourceValue.FieldFlags)
  then
    Addr := TargetLoc(TypeCastSourceValue.AsCardinal);

  Result := IsTargetNil(Addr);
  if Result then
    exit;

  if not IsReadableLoc(Addr) then
    exit;

  if TFpDwarfFreePascalSymbolClassMap(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030301
  then begin
    Addr:= Addr - AddressSize - 4;
    Result := Context.ReadSignedInt(Addr, SizeVal(4), ARefCount);
  end
  else begin
    Addr:= Addr - (AddressSize * 2);
    Result := Context.ReadSignedInt(Addr, SizeVal(AddressSize), ARefCount);
  end;
end;

function TFpValueDwarfV3FreePascalString.ObtainDynamicCodePage(Addr: TFpDbgMemLocation; out
  Codepage: TSystemCodePage): Boolean;
var
  CodepageOffset: SmallInt;
begin
  // Only call this function for non-empty strings!
  Result := False;
  if not IsTargetNotNil(Addr) then
    exit;

  // Only AnsiStrings in fpc 3.0.0 and higher have a dynamic codepage.
  if (TypeInfo.Kind = skString) and (TFpDwarfFreePascalSymbolClassMapDwarf3(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030000) then begin
    // Too bad the debug-information does not deliver this information. So we
    // use these hardcoded information, and hope that FPC does not change and
    // we never reach this point for a compilationunit that is not compiled by
    // fpc.
    if TFpDwarfFreePascalSymbolClassMapDwarf3(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion >= $030300 { $030301 } then
      CodepageOffset := AddressSize + SizeOf(Longint) + SizeOf(Word) + SizeOf(Word)
    else
      CodepageOffset := AddressSize * 3;
    Addr.Address := Addr.Address - CodepageOffset;
    if Context.ReadMemory(Addr, SizeVal(2), @Codepage) then
      Result := CodePageToCodePageName(Codepage) <> '';
  end;
end;

procedure TFpValueDwarfV3FreePascalString.CalcBounds;
var
  t, t2: TFpSymbol;
  i: Int64;
  Addr, Addr2: TFpDbgMemLocation;
  AttrData: TDwarfAttribData;
begin
  if FBoundsDone then
    exit;

  FBoundsDone := True;
  FLowBound := 0;
  FHighBound := -1;

  // get length
  t := TypeInfo;
  if t.NestedSymbolCount < 1 then // subrange type
    exit;

  t2 := t.NestedSymbol[0]; // subrange type
  if not( (t2 is TFpSymbolDwarfType) and TFpSymbolDwarfType(t2).GetValueBounds(self, FLowBound, FHighBound) )
  then
    exit;

  GetDwarfDataAddress(Addr);
  if (not IsValidLoc(Addr)) and
     (HasTypeCastInfo) and
     (svfOrdinal in TypeCastSourceValue.FieldFlags)
  then
    Addr := TargetLoc(TypeCastSourceValue.AsCardinal);
  if not IsReadableLoc(Addr) then
    exit;

  assert((TypeInfo <> nil) and (TypeInfo.CompilationUnit <> nil) and (TypeInfo.CompilationUnit.DwarfSymbolClassMap is TFpDwarfFreePascalSymbolClassMapDwarf3), 'TFpValueDwarfV3FreePascalString.GetAsString: (Owner <> nil) and (Owner.CompilationUnit <> nil) and (TypeInfo.CompilationUnit.DwarfSymbolClassMap is TFpDwarfFreePascalSymbolClassMapDwarf3)');
  if (TFpDwarfFreePascalSymbolClassMapDwarf3(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion > 0) and
     (TFpDwarfFreePascalSymbolClassMapDwarf3(TypeInfo.CompilationUnit.DwarfSymbolClassMap).FCompilerVersion < $030100)
  then begin
    if t.Kind = skWideString then begin
      if (t2 is TFpSymbolDwarfTypeSubRange) and (FLowBound = 1) then begin
        if (TFpSymbolDwarfTypeSubRange(t2).InformationEntry.GetAttribData(DW_AT_upper_bound, AttrData)) and
           (TFpSymbolDwarfTypeSubRange(t2).InformationEntry.AttribForm[AttrData.Idx] = DW_FORM_block1) and
           (IsReadableMem(Addr) and (LocToAddr(Addr) > AddressSize))
        then begin
          // fpc issue 0035359
          // read data and check for DW_OP_shr ?
          Addr2 := Addr;
          Addr2.Address := Addr2.Address - AddressSize;
          if Context.ReadSignedInt(Addr2, SizeVal(AddressSize), i) then begin
            if (i shr 1) = FHighBound then
              FHighBound := i;
          end
        end;
      end;
    end;
  end;
end;

function TFpValueDwarfV3FreePascalString.CheckTypeAndGetAddr(out
  AnAddr: TFpDbgMemLocation): boolean;
var
  t: TFpSymbolDwarfType;
begin
  Result := False;
  t := TypeInfo;
  if t.NestedSymbolCount < 1 then // subrange type
    exit;

  GetDwarfDataAddress(AnAddr);
  if (not IsValidLoc(AnAddr)) and
     (HasTypeCastInfo) and
     (svfOrdinal in TypeCastSourceValue.FieldFlags)
  then
    AnAddr := TargetLoc(TypeCastSourceValue.AsCardinal);
  if not IsReadableLoc(AnAddr) then
    exit;

  Result := True;
end;

{ TFpSymbolDwarfFreePascalDataProc }

function TFpSymbolDwarfFreePascalDataProc.GetLine: Cardinal;
begin
  if FOrigSymbol <> nil then
    Result := FOrigSymbol.GetLine
  else
    Result := inherited GetLine;
end;

function TFpSymbolDwarfFreePascalDataProc.GetColumn: Cardinal;
begin
  if FOrigSymbol <> nil then
    Result := FOrigSymbol.GetColumn
  else
    Result := inherited GetColumn;
end;

destructor TFpSymbolDwarfFreePascalDataProc.Destroy;
begin
  inherited Destroy;
  FOrigSymbol.ReleaseReference;
end;

function TFpSymbolDwarfFreePascalDataProc.ResolveInternalFinallySymbol(
  Process: Pointer): TFpSymbol;
{$IfDef WINDOWS}
var
  StartPC, EndPC: TDBGPtr;
  HelpSymbol, HelpSymbol2: TFpSymbolDwarf;
  AnAddresses: TDBGPtrArray;
  FndLine, i: Integer;
{$EndIf}
begin
  Result := Self;
  // TODO: FindProcSymbol - ideally we could go to the CU for finding the procsym => but that needs some code from TFpDwarfInfo.FindProcSymbol to be moved there

  {$IfDef WINDOWS}
  // On Windows: If in an SEH finally block, try to get the real procedure
  // Look for the line, before the finally statement.
  // TODO: This needs to move to a win-specific class, and ideally a FPC specific class too.
  if ('$fin' = copy(Name,1, 4) ) and
     CompilationUnit.GetProcStartEnd(ProcAddress, StartPC, EndPC) and
     (StartPC <> 0)
  then begin
    // TODO: use the assembler to skip the prologue
    StartPC := StartPC + 9; // fpc puts the first 9 bytes on "end"
    if EndPC < StartPC then
      EndPC := StartPC;

    if StartPC = ProcAddress then begin
      TFpSymbol(HelpSymbol) := Self;
      HelpSymbol.AddReference;
    end
    else
      TFpSymbol(HelpSymbol) := DbgInfo.FindProcSymbol(StartPC); // same proc as self, but will return the FIRTS line

    if (HelpSymbol = nil) or (HelpSymbol.CompilationUnit <> CompilationUnit) or
       (not HelpSymbol.InheritsFrom(TFpSymbolDwarfFreePascalDataProc))
    then begin
      HelpSymbol.ReleaseReference;
      exit
    end;

    AnAddresses := nil;
    if
       //(FndLine = HelpSymbol.Line) and
       HelpSymbol.CompilationUnit.GetLineAddresses(HelpSymbol.FileName, HelpSymbol.Line, AnAddresses, fsBefore, @FndLine) and
       (Length(AnAddresses) > 1)  // may be an internal finally on the begin/end line, sharing a line number
    then begin
      for i := 0 to Length(AnAddresses) - 1 do
        if (AnAddresses[i] < StartPC - 9) or (AnAddresses[i] > EndPC) then begin
          TFpSymbol(HelpSymbol2) := DbgInfo.FindProcSymbol(AnAddresses[i]);
          if (HelpSymbol2 <> nil) and (HelpSymbol2.CompilationUnit = CompilationUnit) and
             (HelpSymbol2.InheritsFrom(TFpSymbolDwarfFreePascalDataProc)) and
             ('$fin' <> copy(HelpSymbol2.Name,1, 4) )
          then begin
            Result := HelpSymbol2;
            // *** FOrigSymbol has now the reference that the caller had. ***
            TFpSymbolDwarfFreePascalDataProc(Result).FOrigSymbol := Self;
            HelpSymbol.ReleaseReference;
            exit;
          end;
          HelpSymbol2.ReleaseReference;
        end;
    end;

    AnAddresses := nil;
    if HelpSymbol.CompilationUnit.GetLineAddresses(HelpSymbol.FileName, HelpSymbol.Line-1, AnAddresses, fsBefore)
    then begin
      TFpSymbol(HelpSymbol2) := DbgInfo.FindProcSymbol(AnAddresses[0]);
      if (HelpSymbol2 <> nil) and (HelpSymbol2.CompilationUnit = CompilationUnit) and
         (HelpSymbol2.InheritsFrom(TFpSymbolDwarfFreePascalDataProc))
      then begin
        Result := HelpSymbol2;
        // *** FOrigSymbol has now the reference that the caller had. ***
        TFpSymbolDwarfFreePascalDataProc(Result).FOrigSymbol := Self;
        HelpSymbol.ReleaseReference;
        exit;
      end;
      HelpSymbol2.ReleaseReference;
    end;
    HelpSymbol.ReleaseReference;
  end;
  {$EndIf}
end;

{ TFpSymbolDwarfFreePascalDataParameter }

procedure TFpSymbolDwarfFreePascalDataParameter.NameNeeded;
begin
  inherited NameNeeded;
  if InformationEntry.IsArtificial and (Name = 'this') then
    SetName('self');
end;

initialization
  DwarfSymbolClassMapList.AddMap(TFpDwarfFreePascalSymbolClassMapDwarf2);
  DwarfSymbolClassMapList.AddMap(TFpDwarfFreePascalSymbolClassMapDwarf3);

  FPDBG_DWARF_VERBOSE       := DebugLogger.FindOrRegisterLogGroup('FPDBG_DWARF_VERBOSE' {$IFDEF FPDBG_DWARF_VERBOSE} , True {$ENDIF} );

  ParentFpLowerNameInfo := NameInfoForSearch('parentfp');
  ParentFp2LowerNameInfo := NameInfoForSearch('$parentfp');
end.

