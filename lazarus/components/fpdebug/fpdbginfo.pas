unit FpDbgInfo;
(*
  About TFpValue and TFpSymbol

  * TFpSymbol
    Represents a Symbol or Identifier (stType or stValue)

  * TFpValue
    Holds the Value of a Symbol according to its type.

  TFpSymbol should not hold any Data, except for information that is in the
  debug info (dwarf/stabs/hardcoded/other..?).

  TFpSymbol however, might expose methods which can be used to obtain relevant
  data.

  All Data read from the target must be in TFpValue.
  Target data includes Address (can be indirect via ref or pointer, Size and
  Boundaries (Sub range / Array).

  This means that TFpSymbol (stType or stValue) should be re-usable. There can
  be multiple TFpValue for each TFpSymbol. (even for stValue, as in an
  Array the Symbol itself is repeated / Array of record: the same member occurs
  over and over)

  On the other hand, one generic TFpValue class should be used to access all
  kinds of TFpSymbol. In practice this is not possible, but it is something to
  strive for.

  ---
  A Variable value in the target typically consists of:
  - TFpSymbol (stValue)
  - TFpSymbol (stType)
  - TFpValue

*)
{$mode objfpc}{$H+}
{$TYPEDADDRESS on}
{$IFDEF INLINE_OFF}{$INLINE OFF}{$ENDIF}

interface

uses
  Classes, SysUtils, DbgIntfBaseTypes, FpDbgLoader, FpdMemoryTools, FpErrorMessages,
  {$ifdef FORCE_LAZLOGGER_DUMMY} LazLoggerDummy {$else} LazLoggerBase {$endif}, LazClasses, FpDbgCommon,
  // Register all image reader classes
  FpImgReaderWinPE, FpImgReaderElf, FpImgReaderMacho;

type

  TDbgSymbolType = (
    stNone,
    stValue,  // The symbol has a value (var, field, function, procedure (value is address of func/proc, so it can be called)
    stType    // The Symbol is a type (including proc/func declaration / without DW_AT_low_pc)
  );

  TDbgSymbolMemberVisibility =(
    svUnknown,
    svPrivate,
    svProtected,
    svPublic
  );

  TDbgSymbolFlag =(
    sfSubRange,     // This is a subrange, e.g 3..99
    sfDynArray,     // skArray is known to be a dynamic array
    sfStatArray,    // skArray is known to be a static array
    sfVirtual,      // skProcedure,skFunction:  virtual function (or overriden)
    sfParameter,    // Parameter to a function
    // unimplemented:
    sfInternalRef,  // TODO: (May not always be present) Internal ref/pointer e.g. var/constref parameters
    sfConst,         // The sym is a constant and cannot be modified
    sfVar,
    sfOut,
    sfpropGet,
    sfPropSet,
    sfPropStored,
    sfHasLine,
    sfHasLineAddrRng
  );
  TDbgSymbolFlags = set of TDbgSymbolFlag;

  TFpSymbolField = (
    sfiName, sfiKind, sfiSymType, sfiAddress, //sfiSize,
    sfiTypeInfo, sfiMemberVisibility,
    sfiForwardToSymbol
  );
  TFpSymbolFields = set of TFpSymbolField;

  TFpSymbol = class;

  TFpValueFieldFlag = (
    // svfAddress, svfDataAddress this symbol does have an address, but it may still be nil
    svfAddress, svfSize, svfSizeOfPointer,
    svfDataAddress, svfDataSize, svfDataSizeOfPointer,
    svfInteger, svfCardinal, svfFloat,
    svfString, svfWideString,
    svfBoolean,
    svfIdentifier,   // returned via AsString: a named value (enum, set-member)
    svfMembers,
    //svfParent, // TODO: for members, get the parent (object/record-fields, enum/set-members
    svfOrdinal       // AsCardinal ruturns an ordinal value, but the value is not represented as cardinal (e.g. bool, enum)
                     // if size > 8, then ordinal (if present) is based on a part only
  );
  TFpValueFieldFlags = set of TFpValueFieldFlag;

  TFpValueFlag = (
    vfVariant
  );
  TFpValueFlags = set of TFpValueFlag;

  { TFpValue }

  TFpValue = class(TRefCountedObject)
  private
    FEvalFlags: set of (efSizeDone, efSizeUnavail);
    FFlags: TFpValueFlags;
    FLastError: TFpError;
    FSize: TFpDbgValueSize;
    procedure SetAsString(AStartIndex, ALen: Int64; AValue: AnsiString);
  protected
    function GetKind: TDbgSymbolKind; virtual;
    function GetFieldFlags: TFpValueFieldFlags; virtual;

    function GetAsBool: Boolean;  virtual;
    function GetAsCardinal: QWord; virtual;
    function GetAsInteger: Int64; virtual;
    function GetAsString: AnsiString; virtual;
    function GetAsWideString: WideString; virtual;
    function GetAsFloat: Extended; virtual;

    procedure SetAsCardinal(AValue: QWord); virtual;
    procedure SetAsInteger(AValue: Int64); virtual;
    procedure SetAsBool(AValue: Boolean); virtual;
    procedure SetAsString(AValue: AnsiString); virtual;

    function GetAddress: TFpDbgMemLocation;  virtual;
    function DoGetSize(out ASize: TFpDbgValueSize): Boolean; virtual;
    function GetDataAddress: TFpDbgMemLocation;  virtual;
    function GetDerefAddress: TFpDbgMemLocation;  virtual;
    function GetEntryPCAddress: TFpDbgMemLocation; virtual;
    function GetDataSize: TFpDbgValueSize;  virtual;

    function GetHasBounds: Boolean; virtual;
    function GetOrdHighBound: Int64; virtual;
    function GetOrdLowBound: Int64; virtual;

    function GetMember({%H-}AIndex: Int64): TFpValue; virtual;
    function GetMemberByName(const AIndex: String): TFpValue; virtual;
    function GetMemberCount: Integer; virtual;
    function GetIndexType({%H-}AIndex: Integer): TFpSymbol; virtual;
    function GetIndexTypeCount: Integer; virtual;
    function GetMemberCountEx(const AIndex: array of Int64): Integer; virtual;
    function GetMemberEx(const AIndex: Array of Int64): TFpValue; virtual;

    function GetDbgSymbol: TFpSymbol; virtual;
    function GetTypeInfo: TFpSymbol; virtual;
    function GetParentTypeInfo: TFpSymbol; virtual;

    function GetLastError: TFpError; virtual;
  public
    constructor Create;
    property RefCount;
    procedure Reset; virtual; // keeps lastmember and structureninfo

    function GetSize(out ASize: TFpDbgValueSize): Boolean; inline;

    (* AsString[AStartIndex, ALen: Int64]
       - AStartIndex is 1-based
       - AIgnoreBounds may not be supported by all data types
       - If AStartIndex/ALen are out of bounds then
         - Result will be false / LastError will NOT be set
         - SubStr will contain any part that was in bounds
       - If Result is false, AND LastError is set: no data was retrieved
    *)
    function GetSubString(AStartIndex, ALen: Int64; out ASubStr: AnsiString; AIgnoreBounds: Boolean = False): Boolean; virtual;
    function GetSubWideString(AStartIndex, ALen: Int64; out ASubStr: WideString; AIgnoreBounds: Boolean = False): Boolean; virtual;

    // Kind: determines which types of value are available
    property Kind: TDbgSymbolKind read GetKind;
    property Flags: TFpValueFlags read FFlags write FFlags;
    property FieldFlags: TFpValueFieldFlags read GetFieldFlags;

    property AsInteger: Int64 read GetAsInteger write SetAsInteger;
    property AsCardinal: QWord read GetAsCardinal write SetAsCardinal;
    property AsBool: Boolean read GetAsBool write SetAsBool;
    property AsString: AnsiString read GetAsString write SetAsString;
    property AsWideString: WideString read GetAsWideString;
    property AsFloat: Extended read GetAsFloat;

    (* * Address/Size
         Address of the variable (as returned by the "@" address of operator
       * DataAddress/DataSize
         Address of Data, if avail and diff from Address (e.g. String, TObject, DynArray, ..., BUT NOT record)
         Otherwise same as Address/Size
         For pointers, this is the same as Address (Not DerefAddress)
    *)
    property Address: TFpDbgMemLocation read GetAddress;
    property DataAddress: TFpDbgMemLocation read GetDataAddress; //
    property DerefAddress: TFpDbgMemLocation read GetDerefAddress; //
    property EntryPCAddress: TFpDbgMemLocation read GetEntryPCAddress;
    property DataSize: TFpDbgValueSize read GetDataSize;

    property HasBounds: Boolean  read GetHasBounds;
    property OrdLowBound: Int64  read GetOrdLowBound;   // need typecast for QuadWord
    property OrdHighBound: Int64 read GetOrdHighBound;  // need typecast for QuadWord
    // memdump
  public
    function GetTypeCastedValue(ADataVal: TFpValue): TFpValue; virtual; // only if Symbol is a type

    function GetInstanceClassName(out AClassName; AParentClassIndex: integer = 0): boolean; virtual;
    function GetInstanceClassName(AClassName: PString; AUnitName: PString = nil; AParentClassIndex: integer = 0): boolean; virtual;
    function GetFpcRefCount(out ARefCount: Int64): Boolean; virtual;

// base class? Or Member includes member from base
    (* Member:
       * skClass, skStructure:
           stType: it excludes BaseClass (TODO: decide?)
           stValue: includes
       * skSet
           stType: all members
           stValue: only members set in value (Only impremented for DbgSymbolValue)
       * skArray: (differs from TFpSymbol)
         The values. The type of each Index-dimension is avail via IndexType
       * skPointer: deref the pointer, with index (0 = normal deref)
       NOTE: Values returned by Member/MemberByName are volatile.
             They maybe released or changed when Member is called again.
             To keep a returned Value a reference can be added (AddReference)
    *)
    property MemberCount: Integer read GetMemberCount;
    property Member[AIndex: Int64]: TFpValue read GetMember;
    property MemberByName[AIndex: String]: TFpValue read GetMemberByName; // Includes inheritance
    //  For Arrays (TODO pointers) only, the values stored in the array
    property IndexTypeCount: Integer read GetIndexTypeCount;
    property IndexType[AIndex: Integer]: TFpSymbol read GetIndexType;

    (* DbgSymbol: The TFpSymbol from which this value came, maybe nil.
                  Maybe a stType, then there is no Value *)
    property DbgSymbol: TFpSymbol read GetDbgSymbol;
    property TypeInfo: TFpSymbol read GetTypeInfo;
    property ParentTypeInfo: TFpSymbol read GetParentTypeInfo; // For members, the class in which this member is declared

    property LastError: TFpError read GetLastError;
    procedure SetLastError(ALastError: TFpError);
    procedure ResetError;
  end;

  { TFpValueConstWithType }

  TFpValueConstWithType = class(TFpValue)
  private
    FType: TFpSymbol;
    FValAddress: TFpDbgMemLocation;
  protected
    function GetTypeInfo: TFpSymbol; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAddress: TFpDbgMemLocation; override;
  public
    destructor Destroy; override;
    procedure SetTypeName(AName: String);
    procedure SetType(AType: TFpSymbol);
    procedure SetAddress(AnAddress: TFpDbgMemLocation);
  end;

  { TFpValueConstNumber }

  TFpValueConstNumber = class(TFpValueConstWithType)
  private
    FValue: QWord;
    FSigned: Boolean;
  protected
    property Value: QWord read FValue write FValue;
    property Signed: Boolean read FSigned write FSigned;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsCardinal: QWord; override;
    function GetAsInteger: Int64; override;
    function GetAsFloat: Extended; override;
  public
    constructor Create(AValue: QWord; ASigned: Boolean = True);
  end;

  { TFpValueConstChar }

  TFpValueConstChar = class(TFpValueConstWithType) // skChar / Not for strings
  private
    FValue: String;
  protected
    property Value: String read FValue write FValue;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsString: AnsiString; override;
    function GetAsWideString: WideString; override;
    function GetAsCardinal: QWord; override;
    function DoGetSize(out ASize: TFpDbgValueSize): Boolean; override;
  public
    constructor Create(const AValue: Char);
  end;

  { TFpValueConstWideChar }

  TFpValueConstWideChar = class(TFpValueConstWithType) // skChar / Not for strings
  private
    FValue: WideChar;
    function GetValue: String;
    procedure SetValue(AValue: String);
  protected
    property Value: String read GetValue write SetValue;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsString: AnsiString; override;
    function GetAsWideString: WideString; override;
    function GetAsCardinal: QWord; override;
    function DoGetSize(out ASize: TFpDbgValueSize): Boolean; override;
  public
    constructor Create(const AValue: WideChar);
  end;

  { TFpValueConstString }

  TFpValueConstString = class(TFpValueConstWithType) // skString
  private
    FValue: String;
  protected
    property Value: String read FValue write FValue;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsString: AnsiString; override;
    function GetAsWideString: WideString; override;
    function GetMemberCount: Integer; override;
  public
    constructor Create(const AValue: AnsiString);
  end;

  { TFpValueConstFloat }

  TFpValueConstFloat = class(TFpValueConstWithType)
  private
    FValue: Extended;
  protected
    property Value: Extended read FValue write FValue;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsFloat: Extended; override;
  public
    constructor Create(AValue: Extended);
  end;

  { TFpValueConstBool}

  TFpValueConstBool = class(TFpValueConstWithType)
  private
    FValue: Boolean;
  protected
    property Value: Boolean read FValue write FValue;
    function GetKind: TDbgSymbolKind; override;
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAsBool: Boolean; override;
    function GetAsCardinal: QWord; override;
  public
    constructor Create(AValue: Boolean);
  end;

  { TFpValueConstAddress }

  TFpValueConstAddress = class(TFpValueConstWithType)
  private
    FAddress: TFpDbgMemLocation;
  protected
    property Address: TFpDbgMemLocation read FAddress write FAddress;
    function GetKind: TDbgSymbolKind; override; // skAddress
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetAddress: TFpDbgMemLocation; override;
  public
    constructor Create(const AnAddress: TFpDbgMemLocation);
  end;

  { TFpValueTypeDefinition }

  TFpValueTypeDefinition = class(TFpValue)
  private
    FSymbol: TFpSymbol; // stType
  protected
    function GetKind: TDbgSymbolKind; override;
    function GetDbgSymbol: TFpSymbol; override;
  public
    constructor Create(ASymbol: TFpSymbol); // Only for stType
    destructor Destroy; override;
  end;

  { TFpValueConstEnumValue }

  TFpValueConstEnumValue = class(TFpValueConstWithType)
  private
    FName: String;
  protected
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetKind: TDbgSymbolKind; override;
    function GetAsString: AnsiString; override;
  public
    constructor Create(AName: String);
  end;

  { TFpValueConstSet }

  TFpValueConstSet = class(TFpValueConstWithType)
  private
    FNames: TStrings;
  protected
    function GetFieldFlags: TFpValueFieldFlags; override;
    function GetKind: TDbgSymbolKind; override;
    function GetMember(AIndex: Int64): TFpValue; override;
    function GetMemberCount: Integer; override;
  public
    constructor Create(ANames: TStrings);
    destructor Destroy; override;
  end;

  TFpDbgSymbolScope = class;

  { TFpSymbol }

  TFpSymbol = class(TRefCountedObject)
  private
    FEvaluatedFields: TFpSymbolFields;
    // Cached fields
    FName: String;
    FKind: TDbgSymbolKind;
    FSymbolType: TDbgSymbolType;
    FAddress: TFpDbgMemLocation;
    FTypeInfo: TFpSymbol;
    FMemberVisibility: TDbgSymbolMemberVisibility; // Todo: not cached

    function GetSymbolType: TDbgSymbolType; inline;
    function GetKind: TDbgSymbolKind; inline;
    function GetName: String; inline;
    function GetAddress: TFpDbgMemLocation; inline;
    function GetTypeInfo: TFpSymbol; inline;
    function GetMemberVisibility: TDbgSymbolMemberVisibility; inline;
  protected
    procedure SetLastError(AValueObj: TFpValue; ALastError: TFpError); inline;
    function  HasError(AValueObj: TFpValue): Boolean; inline;
    // NOT cached fields
    function GetChild({%H-}AIndex: Integer): TFpSymbol; virtual;
    function GetColumn: Cardinal; virtual;
    function GetFile: String; virtual;
    function GetFlags: TDbgSymbolFlags; virtual;
    function GetLine: Cardinal; virtual;
    function GetLineEndAddress: TDBGPtr; virtual;
    function GetLineStartAddress: TDBGPtr; virtual;
    function GetParent: TFpSymbol; virtual;

    function GetValueObject: TFpValue; virtual;
    function GetHasOrdinalValue: Boolean; virtual;
    function GetOrdinalValue: Int64; virtual;

    function GetNestedSymbol({%H-}AIndex: Int64): TFpSymbol; virtual;
    function GetNestedSymbolByName(const AIndex: String): TFpSymbol; virtual;
    function GetNestedSymbolCount: Integer; virtual;
  protected
    property EvaluatedFields: TFpSymbolFields read FEvaluatedFields write FEvaluatedFields;
    // Cached fields
    procedure SetName(const AValue: String); inline;
    procedure SetKind(AValue: TDbgSymbolKind); inline;
    procedure SetSymbolType(AValue: TDbgSymbolType); inline;
    procedure SetAddress(const AValue: TFpDbgMemLocation); inline;
    procedure SetTypeInfo(ASymbol: TFpSymbol); inline;
    procedure SetMemberVisibility(AValue: TDbgSymbolMemberVisibility); inline;

    function GetInternalTypeInfo: TFpSymbol; virtual;
    procedure KindNeeded; virtual;
    procedure NameNeeded; virtual;
    procedure SymbolTypeNeeded; virtual;
    procedure AddressNeeded; virtual;
    function  DoReadSize(const AValueObj: TFpValue; out ASize: TFpDbgValueSize): Boolean; virtual;
    procedure TypeInfoNeeded; virtual;
    procedure MemberVisibilityNeeded; virtual;
    //procedure Needed; virtual;
  public
    constructor Create(const AName: String);
    constructor Create(const AName: String; AKind: TDbgSymbolKind; const AAddress: TFpDbgMemLocation);
    destructor Destroy; override;
    // Basic info
    property Name:       String read GetName;
    property SymbolType: TDbgSymbolType read GetSymbolType;
    property Kind:       TDbgSymbolKind read GetKind;
    // Memory; Size is also part of type (byte vs word vs ...)
    property Address:    TFpDbgMemLocation read GetAddress;    // used by Proc/func
    // ReadSize: Return False means no value available, and an error may or may not have occurred
    function ReadSize(const AValueObj: TFpValue; out ASize: TFpDbgValueSize): Boolean; inline;
    // TypeInfo used by
    // stValue (Variable): Type
    // stType: Pointer: type pointed to / Array: Element Type / Func: Result / Class: itheritance
    property TypeInfo: TFpSymbol read GetTypeInfo;
    property InternalTypeInfo: TFpSymbol read GetInternalTypeInfo; // Not modified by any dwarf modifier
    // Location
    property FileName: String read GetFile;
    property Line: Cardinal read GetLine;
    property LineStartAddress: TDBGPtr read GetLineStartAddress;
    property LineEndAddress: TDBGPtr read GetLineEndAddress;
    property Column: Cardinal read GetColumn;
    // Methods for structures (record / class / enum)
    //         array: each member represents an index (enum or subrange) and has low/high bounds
    property MemberVisibility: TDbgSymbolMemberVisibility read GetMemberVisibility;
    property NestedSymbolCount: Integer read GetNestedSymbolCount;
    (* Member:
       * skClass, skStructure:
           stType: it excludes BaseClass (TODO: decide?)
           includes
       * skSet
           stType: all members
           stValue: only members set in value (Only impremented for DbgSymbolValue)
       * skArray:
         The type of each Index-dimension
         The count is the amount of dimensions
       NOTE: Values returned by Member/MemberByName are volatile.
             They maybe released or changed when Member is called again.
             To keep a returned Value a reference can be added (AddReference)
    *)
    property NestedSymbol[AIndex: Int64]: TFpSymbol read GetNestedSymbol;
    property NestedSymbolByName[AIndex: String]: TFpSymbol read GetNestedSymbolByName; // Includes inheritance
    //
    property Flags: TDbgSymbolFlags read GetFlags;
    property Parent: TFpSymbol read GetParent; deprecated;
    function GetInstanceClassName(AValueObj: TFpValue; AClassName, AUnitName: PString; AParentClassIndex: integer = 0): boolean; virtual;

    // for Subranges  // Type-Symbols only?
    // TODO: flag bounds as cardinal if needed
    function GetValueBounds(AValueObj: TFpValue; out ALowBound, AHighBound: Int64): Boolean; virtual;
    function GetValueLowBound(AValueObj: TFpValue; out ALowBound: Int64): Boolean; virtual;
    function GetValueHighBound(AValueObj: TFpValue; out AHighBound: Int64): Boolean; virtual;

    // VALUE
    property Value: TFpValue read GetValueObject; //deprecated 'rename / create';
    property HasOrdinalValue: Boolean read GetHasOrdinalValue;
    property OrdinalValue: Int64 read GetOrdinalValue;   //deprecated 'xxxx'; // need typecast for QuadWord

    // TypeCastValue| only fon stType symbols, may return nil
    // Returns a reference to caller / caller must release
    function TypeCastValue({%H-}AValue: TFpValue): TFpValue; virtual;

    function CreateSymbolScope(ALocationContext: TFpDbgLocationContext): TFpDbgSymbolScope; virtual;
  end;

  TFpSymbolArray = array of TFpSymbol;

  { TFpSymbolForwarder }

  TFpSymbolForwarder = class(TFpSymbol)
  private
    FForwardToSymbol: TFpSymbol;
  protected
    procedure SetForwardToSymbol(AValue: TFpSymbol); inline;
    procedure ForwardToSymbolNeeded; virtual;
    function  GetForwardToSymbol: TFpSymbol; inline;
  protected
    procedure KindNeeded; override;
    procedure NameNeeded; override;
    procedure SymbolTypeNeeded; override;
    function  DoReadSize(const AValueObj: TFpValue; out ASize: TFpDbgValueSize): Boolean; override;
    procedure TypeInfoNeeded; override;
    procedure MemberVisibilityNeeded; override;

    function GetFlags: TDbgSymbolFlags; override;
    function GetValueObject: TFpValue; override;
    function GetHasOrdinalValue: Boolean; override;
    function GetOrdinalValue: Int64; override;
    function GetNestedSymbol(AIndex: Int64): TFpSymbol; override;
    function GetNestedSymbolByName(const AIndex: String): TFpSymbol; override;
    function GetNestedSymbolCount: Integer; override;
  public
    function GetInstanceClassName(AValueObj: TFpValue; AClassName, AUnitName: PString; AParentClassIndex: integer = 0): boolean; override;
    function GetValueBounds(AValueObj: TFpValue; out ALowBound, AHighBound: Int64): Boolean; override;
    function GetValueLowBound(AValueObj: TFpValue; out ALowBound: Int64): Boolean; override;
    function GetValueHighBound(AValueObj: TFpValue; out AHighBound: Int64): Boolean; override;
  end;

  { TFpDbgSymbolScope }

  TFpDbgSymbolScope = class(TRefCountedObject)
  private
    FLocationContext: TFpDbgLocationContext;
  protected
    function GetSymbolAtAddress: TFpSymbol; virtual;
    function GetProcedureAtAddress: TFpValue; virtual;
    function GetMemManager: TFpDbgMemManager; virtual;
    function GetSizeOfAddress: Integer; virtual;
  public
    constructor Create(ALocationContext: TFpDbgLocationContext);
    destructor Destroy; override;
    property SymbolAtAddress: TFpSymbol read GetSymbolAtAddress;
    property ProcedureAtAddress: TFpValue read GetProcedureAtAddress;
    // search this, and all parent context
    function FindSymbol(const {%H-}AName: String; const OnlyUnitName: String = ''): TFpValue; virtual;
    property MemManager: TFpDbgMemManager read GetMemManager;
    property SizeOfAddress: Integer read GetSizeOfAddress;
    property LocationContext: TFpDbgLocationContext read FLocationContext;
  end;

  { TFpDbgSimpleLocationContext }

  TFpDbgSimpleLocationContext = class(TFpDbgLocationContext)
  private
    FMemManager: TFpDbgMemManager;
    FAddress: TDbgPtr;
    FThreadId: Integer;
    FStackFrame: Integer;
    FSizeOfAddr: Integer;
  protected
    function GetMemManager: TFpDbgMemManager; override;
    function GetAddress: TDbgPtr; override;
    function GetThreadId: Integer; override;
    function GetStackFrame: Integer; override;
    function GetSizeOfAddress: Integer; override;
  public
    constructor Create(AMemManager: TFpDbgMemManager; AnAddress: TDbgPtr; AnSizeOfAddr, AThreadId: Integer; AStackFrame: Integer);
  end;

  { TFpDbgCallMemReader }

  // This is basically a wrapper on another TFpDbgMemReaderBase. But with the
  // possibility to override the value of some registers.
  // It is used to evaluate function-results.
  TFpDbgCallMemReader = class(TFpDbgMemReaderBase)
  private
    type TRegisterValue = record IsSet: boolean; Value: TDBGPtr end;
  private
    FRegisterCache: array of TRegisterValue;
    FBaseMemReader: TFpDbgMemReaderBase;
  public
    constructor Create(ABaseMemReader: TFpDbgMemReaderBase);
    function ReadMemory(AnAddress: TDbgPtr; ASize: Cardinal; ADest: Pointer): Boolean; override;
    function ReadMemory(AnAddress: TDbgPtr; ASize: Cardinal; ADest: Pointer; out ABytesRead: Cardinal): Boolean; override;
    function ReadMemoryEx(AnAddress, AnAddressSpace: TDbgPtr; ASize: Cardinal; ADest: Pointer): Boolean; override;
    function ReadRegister(ARegNum: Cardinal; out AValue: TDbgPtr; AContext: TFpDbgLocationContext): Boolean; override;
    function WriteRegister(ARegNum: Cardinal; const AValue: TDbgPtr; AContext: TFpDbgLocationContext): Boolean; override;
    function RegisterSize(ARegNum: Cardinal): Integer; override;
    procedure SetRegisterValue(ARegNum: Cardinal; AValue: TDbgPtr);
  end;

  { TFpDbgInfoAbstractCallContext }

  // This class is used to represent the context, just after the debugger made
  // the debugee call some function.
  // The special addition to make this work is that it is possible to set a
  // register-value by calling SetRegisterValue. Further this class is an empty
  // wrapper.

  { TFpDbgAbstractCallContext }

  TFpDbgAbstractCallContext = class(TFpDbgLocationContext)
  private
    FBaseContext: TFpDbgLocationContext;
    FMemManager: TFpDbgMemManager;
    FMemReader: TFpDbgCallMemReader;
    FIsValid: Boolean;
    FMessage: string;
  protected
    function GetMemManager: TFpDbgMemManager; override;
    function GetAddress: TDbgPtr; override;
    function GetThreadId: Integer; override;
    function GetStackFrame: Integer; override;
    function GetSizeOfAddress: Integer; override;
  public
    constructor Create(const ABaseContext: TFpDbgLocationContext; AMemReader: TFpDbgMemReaderBase; AMemConverter: TFpDbgMemConvertor);
    destructor Destroy; override;

    procedure SetRegisterValue(ARegNum: Cardinal; AValue: TDbgPtr);
    procedure SetError(const Message: string);
    property IsValid: Boolean read FIsValid;
    property Message: string read FMessage;
  end;

  { TDbgInfo }
  TGetLineAddrFindSibling = (fsNone, fsBefore, fsNext);

  TDbgInfo = class(TObject)
  private
    FHasInfo: Boolean;
    FMemManager: TFpDbgMemManager;
  protected
    FTargetInfo: TTargetDescriptor;
    procedure SetHasInfo;
  public
    constructor Create({%H-}ALoaderList: TDbgImageLoaderList; AMemManager: TFpDbgMemManager); virtual;
    (* Context should be searched by Thread, and StackFrame. The Address can be
       derived from this.
       However a different Address may be froced.
       TODO: for now address may be needed, as stack decoding is not done yet
    *)
    function FindSymbolScope(ALocationContext: TFpDbgLocationContext; {%H-}AAddress: TDbgPtr = 0): TFpDbgSymbolScope; virtual;
    function FindProcSymbol(AAddress: TDbgPtr): TFpSymbol; virtual; overload;
    function FindProcSymbol(const {%H-}AName: String): TFpSymbol; virtual; overload;
    function FindLineInfo(AAddress: TDbgPtr): TFpSymbol; virtual;

    function  FindProcStartEndPC(const AAddress: TDbgPtr; out AStartPC, AEndPC: TDBGPtr): boolean; virtual;

    property HasInfo: Boolean read FHasInfo;
    function GetLineAddresses(const AFileName: String; ALine: Cardinal; var AResultList: TDBGPtrArray;
      AFindSibling: TGetLineAddrFindSibling = fsNone; AFoundLine: PInteger = nil; AFoundFilename: PBoolean = nil): Boolean; virtual;
    //property MemManager: TFpDbgMemReaderBase read GetMemManager write SetMemManager;
    property TargetInfo: TTargetDescriptor read FTargetInfo write FTargetInfo;
    property MemManager: TFpDbgMemManager read FMemManager;
  end;

function dbgs(ADbgSymbolKind: TDbgSymbolKind): String; overload;

implementation

var
  FPDBG_FUNCCALL: PLazLoggerLogGroup;

function dbgs(ADbgSymbolKind: TDbgSymbolKind): String;
begin
  Result := '';
  WriteStr(Result, ADbgSymbolKind);
end;

{ TFpDbgCallMemReader }

constructor TFpDbgCallMemReader.Create(ABaseMemReader: TFpDbgMemReaderBase);
begin
  FBaseMemReader := ABaseMemReader;
end;

function TFpDbgCallMemReader.ReadMemory(AnAddress: TDbgPtr; ASize: Cardinal; ADest: Pointer): Boolean;
begin
  Result := FBaseMemReader.ReadMemory(AnAddress, ASize, ADest);
end;

function TFpDbgCallMemReader.ReadMemory(AnAddress: TDbgPtr; ASize: Cardinal; ADest: Pointer; out ABytesRead: Cardinal): Boolean;
begin
  Result := FBaseMemReader.ReadMemory(AnAddress, ASize, ADest, ABytesRead);
end;

function TFpDbgCallMemReader.ReadMemoryEx(AnAddress, AnAddressSpace: TDbgPtr; ASize: Cardinal; ADest: Pointer): Boolean;
begin
  Result := FBaseMemReader.ReadMemoryEx(AnAddress, AnAddressSpace, ASize, ADest);
end;

function TFpDbgCallMemReader.ReadRegister(ARegNum: Cardinal; out AValue: TDbgPtr; AContext: TFpDbgLocationContext): Boolean;
begin
  if (ARegNum < Length(FRegisterCache)) and (FRegisterCache[ARegNum].IsSet) then
    begin
    AValue := FRegisterCache[ARegNum].Value;
    Result := True;
    end
  else
    Result := FBaseMemReader.ReadRegister(ARegNum, AValue, AContext);
end;

function TFpDbgCallMemReader.RegisterSize(ARegNum: Cardinal): Integer;
begin
  Result := FBaseMemReader.RegisterSize(ARegNum);
end;

procedure TFpDbgCallMemReader.SetRegisterValue(ARegNum: Cardinal; AValue: TDbgPtr);
var
  OldSize, i: Integer;
begin
  if High(FRegisterCache) < ARegNum then
    begin
    OldSize := Length(FRegisterCache);
    SetLength(FRegisterCache, ARegNum +1);
    for i := OldSize to High(FRegisterCache) do
      FRegisterCache[i].IsSet := False;
    end;
  FRegisterCache[ARegNum].IsSet := True;
  FRegisterCache[ARegNum].Value := AValue;
end;

function TFpDbgCallMemReader.WriteRegister(ARegNum: Cardinal; const AValue: TDbgPtr; AContext: TFpDbgLocationContext): Boolean;
begin
  Result := FBaseMemReader.WriteRegister(ARegNum, AValue, AContext);
end;

constructor TFpDbgAbstractCallContext.Create(
  const ABaseContext: TFpDbgLocationContext; AMemReader: TFpDbgMemReaderBase;
  AMemConverter: TFpDbgMemConvertor);
begin
  FBaseContext:=ABaseContext;
  FBaseContext.AddReference;

  FMemReader := TFpDbgCallMemReader.Create(AMemReader);
  FMemManager := TFpDbgMemManager.Create(FMemReader, AMemConverter);

  FIsValid := True;

  Inherited Create;
end;

destructor TFpDbgAbstractCallContext.Destroy;
begin
  FMemManager.Free;
  FMemReader.Free;
  FBaseContext.ReleaseReference;
  inherited Destroy;
end;

function TFpDbgAbstractCallContext.GetAddress: TDbgPtr;
begin
  Result := FBaseContext.Address;
end;

function TFpDbgAbstractCallContext.GetMemManager: TFpDbgMemManager;
begin
  Result := FMemManager;
end;

function TFpDbgAbstractCallContext.GetSizeOfAddress: Integer;
begin
  Result := FBaseContext.SizeOfAddress;
end;

function TFpDbgAbstractCallContext.GetStackFrame: Integer;
begin
  Result := FBaseContext.StackFrame;
end;

function TFpDbgAbstractCallContext.GetThreadId: Integer;
begin
  Result := FBaseContext.ThreadId;
end;

procedure TFpDbgAbstractCallContext.SetRegisterValue(ARegNum: Cardinal; AValue: TDbgPtr);
begin
  FMemReader.SetRegisterValue(ARegNum, AValue);
end;

procedure TFpDbgAbstractCallContext.SetError(const Message: string);
begin
  debugln(FPDBG_FUNCCALL, ['CallRoutine -- !!! Ctx.SetError: ', Message]);
  FIsValid := False;
  FMessage := Message;
end;

{ TFpValueConstString }

function TFpValueConstString.GetKind: TDbgSymbolKind;
begin
  Result := skString;
end;

function TFpValueConstString.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfString] + inherited GetFieldFlags;
end;

function TFpValueConstString.GetAsString: AnsiString;
begin
  Result := Value;
end;

function TFpValueConstString.GetAsWideString: WideString;
begin
  Result := GetAsString;
end;

function TFpValueConstString.GetMemberCount: Integer;
begin
  Result := Length(FValue);
end;

constructor TFpValueConstString.Create(const AValue: AnsiString);
begin
  inherited Create;
  FValue := AValue;
end;

{ TFpValueConstChar }

function TFpValueConstChar.GetKind: TDbgSymbolKind;
begin
  Result := skChar;
end;

function TFpValueConstChar.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfString, svfSize] + inherited GetFieldFlags;
  if Length(FValue) = 1 then
    Result := Result + [svfOrdinal];
end;

function TFpValueConstChar.GetAsString: AnsiString;
begin
  Result := Value;
end;

function TFpValueConstChar.GetAsWideString: WideString;
begin
  Result := GetAsString;
end;

function TFpValueConstChar.GetAsCardinal: QWord;
begin
  if Length(FValue) = 1 then
    Result := ord(FValue[1])
  else
    Result := 0;
end;

function TFpValueConstChar.DoGetSize(out ASize: TFpDbgValueSize): Boolean;
begin
  ASize := SizeVal(1);
  Result := True;
end;

constructor TFpValueConstChar.Create(const AValue: Char);
begin
  inherited Create;
  FValue := AValue;
end;

{ TFpValueConstWideChar }

function TFpValueConstWideChar.GetValue: String;
begin
  Result := FValue;
end;

procedure TFpValueConstWideChar.SetValue(AValue: String);
var
  w: WideString;
begin
  w := AValue;
  if w <> '' then
    FValue := w[1]
  else
    FValue := #0;
end;

function TFpValueConstWideChar.GetKind: TDbgSymbolKind;
begin
  Result := skChar;
end;

function TFpValueConstWideChar.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfWideString, svfSize, svfOrdinal];
  Result := Result + inherited GetFieldFlags;
end;

function TFpValueConstWideChar.GetAsString: AnsiString;
begin
  Result := Value;
end;

function TFpValueConstWideChar.GetAsWideString: WideString;
begin
  Result := FValue;
end;

function TFpValueConstWideChar.GetAsCardinal: QWord;
begin
  Result := ord(FValue);
end;

function TFpValueConstWideChar.DoGetSize(out ASize: TFpDbgValueSize): Boolean;
begin
  ASize := SizeVal(2);
  Result := True;
end;

constructor TFpValueConstWideChar.Create(const AValue: WideChar);
begin
  inherited Create;
  FValue := AValue;
end;

{ TDbgSymbolValue }

function TFpValue.GetAsString: AnsiString;
begin
  Result := '';
end;

function TFpValue.GetAsWideString: WideString;
begin
  Result := '';
end;

function TFpValue.GetDbgSymbol: TFpSymbol;
begin
  Result := nil;
end;

constructor TFpValue.Create;
begin
  inherited Create;
  AddReference;
end;

function TFpValue.GetTypeCastedValue(ADataVal: TFpValue): TFpValue;
begin
  assert(False, 'TFpValue.GetTypeCastedValue: False');
  Result := nil;
end;

function TFpValue.GetInstanceClassName(out AClassName;
  AParentClassIndex: integer): boolean;
begin
  Result := GetInstanceClassName(@AClassName, nil);
end;

function TFpValue.GetInstanceClassName(AClassName: PString; AUnitName: PString;
  AParentClassIndex: integer): boolean;
var
  ti: TFpSymbol;
begin
  ti := TypeInfo;
  Result := ti <> nil;
  if Result then
    Result := ti.GetInstanceClassName(Self, AClassName, AUnitName, AParentClassIndex);
end;

function TFpValue.GetFpcRefCount(out ARefCount: Int64): Boolean;
begin
  Result := False;
end;

procedure TFpValue.ResetError;
begin
  FLastError := NoError;
end;

function TFpValue.GetTypeInfo: TFpSymbol;
begin
  if (DbgSymbol <> nil) and (DbgSymbol.SymbolType = stValue) then
    Result := DbgSymbol.TypeInfo
  else
    Result := nil;
end;

function TFpValue.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [];
end;

function TFpValue.GetIndexType(AIndex: Integer): TFpSymbol;
begin
  Result := nil;
end;

function TFpValue.GetIndexTypeCount: Integer;
begin
  Result := 0;
end;

function TFpValue.GetMemberEx(const AIndex: array of Int64): TFpValue;
begin
  Result := nil;
end;

function TFpValue.GetMemberCountEx(const AIndex: array of Int64): Integer;
begin
  Result := 0;
end;

function TFpValue.GetAsFloat: Extended;
begin
  Result := 0;
end;

function TFpValue.GetParentTypeInfo: TFpSymbol;
begin
  Result := nil;
end;

function TFpValue.GetLastError: TFpError;
begin
  Result := FLastError;
end;

function TFpValue.GetHasBounds: Boolean;
begin
  Result := False;
end;

function TFpValue.GetOrdHighBound: Int64;
begin
  Result := 0;
end;

function TFpValue.GetOrdLowBound: Int64;
begin
  Result := 0;
end;

function TFpValue.GetEntryPCAddress: TFpDbgMemLocation;
begin
  Result := InvalidLoc;
end;

function TFpValue.GetSubString(AStartIndex, ALen: Int64; out
  ASubStr: AnsiString; AIgnoreBounds: Boolean): Boolean;
begin
  Result := AIgnoreBounds;
  ASubStr := '';
  If (ALen < 1) or (AStartIndex < 1) then
    exit;

  ASubStr := AsString;
  If ALen = 1 then begin
    Result := AStartIndex <= Length(ASubStr);
    if Result then
      ASubStr := ASubStr[AStartIndex]
    else
      ASubStr := '';
  end
  else begin
    Result := AStartIndex + ALen <= Length(ASubStr);
    ASubStr := Copy(ASubStr, AStartIndex, ALen);
  end;
  if AIgnoreBounds then
    Result := True;
end;

function TFpValue.GetSubWideString(AStartIndex, ALen: Int64; out
  ASubStr: WideString; AIgnoreBounds: Boolean): Boolean;
begin
  Result := AIgnoreBounds;
  ASubStr := '';
  If (ALen < 1) or (AStartIndex < 1) then
    exit;

  ASubStr := AsWideString;
  If ALen = 1 then begin
    Result := AStartIndex <= Length(ASubStr);
    if Result then
      ASubStr := ASubStr[AStartIndex]
    else
      ASubStr := '';
  end
  else begin
    Result := AStartIndex + ALen <= Length(ASubStr);
    ASubStr := Copy(ASubStr, AStartIndex, ALen);
  end;
  if AIgnoreBounds then
    Result := True;
end;

procedure TFpValue.SetAsString(AStartIndex, ALen: Int64; AValue: AnsiString);
begin

end;

procedure TFpValue.Reset;
begin
  FEvalFlags := [];
  FLastError := NoError;
end;

procedure TFpValue.SetLastError(ALastError: TFpError);
begin
  if not IsError(ALastError) then
    exit;
  FLastError := ALastError;
end;

function TFpValue.GetKind: TDbgSymbolKind;
begin
  Result := skNone;
end;

function TFpValue.GetMember(AIndex: Int64): TFpValue;
begin
  Result := nil;
end;

function TFpValue.GetMemberByName(const AIndex: String): TFpValue;
begin
  Result := nil;
end;

function TFpValue.GetMemberCount: Integer;
begin
  Result := 0;
end;

function TFpValue.GetAddress: TFpDbgMemLocation;
begin
  Result := InvalidLoc;
end;

function TFpValue.DoGetSize(out ASize: TFpDbgValueSize): Boolean;
var
  ti: TFpSymbol;
begin
  Result := False;
  ti := TypeInfo;
  if ti = nil then
    exit;

  Result := ti.ReadSize(Self, ASize);
end;

function TFpValue.GetDataAddress: TFpDbgMemLocation;
begin
  Result := Address;
end;

function TFpValue.GetDerefAddress: TFpDbgMemLocation;
begin
  Result := InvalidLoc;
end;

function TFpValue.GetDataSize: TFpDbgValueSize;
begin
  if not GetSize(Result) then
    Result := ZeroSize;
end;

function TFpValue.GetSize(out ASize: TFpDbgValueSize): Boolean;
begin
  Result := False;
  if (efSizeUnavail in FEvalFlags) then // If there was an error, then LastError should still be set
    exit;

  Result := efSizeDone in FEvalFlags;
  if Result then begin
    ASize := FSize;
    exit;
  end;

  Result := DoGetSize(ASize);
  FSize := ASize;
  if Result then
    Include(FEvalFlags, efSizeDone)
  else
    Include(FEvalFlags, efSizeUnavail);
end;

function TFpValue.GetAsBool: Boolean;
begin
  Result := False;
end;

function TFpValue.GetAsCardinal: QWord;
begin
  Result := 0;
end;

function TFpValue.GetAsInteger: Int64;
begin
  Result := 0;
end;

procedure TFpValue.SetAsCardinal(AValue: QWord);
begin
  SetLastError(CreateError(fpErrChangeVariableNotSupported));
end;

procedure TFpValue.SetAsInteger(AValue: Int64);
begin
  SetLastError(CreateError(fpErrChangeVariableNotSupported));
end;

procedure TFpValue.SetAsBool(AValue: Boolean);
begin
  SetLastError(CreateError(fpErrChangeVariableNotSupported));
end;

procedure TFpValue.SetAsString(AValue: AnsiString);
begin
  SetLastError(CreateError(fpErrChangeVariableNotSupported));
end;

{ TFpValueConstWithType }

function TFpValueConstWithType.GetTypeInfo: TFpSymbol;
begin
  Result := FType;
end;

function TFpValueConstWithType.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [];
  if IsValidLoc(FValAddress) then
    Result := [svfAddress];
end;

function TFpValueConstWithType.GetAddress: TFpDbgMemLocation;
begin
  Result := InvalidLoc;
  if IsInitializedLoc(FValAddress) then
    Result := FValAddress;
end;

destructor TFpValueConstWithType.Destroy;
begin
  inherited Destroy;
  FType.ReleaseReference;
end;

procedure TFpValueConstWithType.SetTypeName(AName: String);
begin
  if FType = nil then
    FType := TFpSymbol.Create(AName)
  else
    FType.SetName(AName);
end;

procedure TFpValueConstWithType.SetType(AType: TFpSymbol);
begin
  assert(FType=nil, 'TFpValueConstWithType.SetType: FType=nil');
  FType := AType;
  if FType <> nil then
    FType.AddReference;
end;

procedure TFpValueConstWithType.SetAddress(AnAddress: TFpDbgMemLocation);
begin
  FValAddress := AnAddress;
end;

{ TPasParserConstNumberSymbolValue }

function TFpValueConstNumber.GetKind: TDbgSymbolKind;
begin
  if FSigned then
    Result := skInteger
  else
    Result := skCardinal;
end;

function TFpValueConstNumber.GetFieldFlags: TFpValueFieldFlags;
begin
  if FSigned then
    Result := [svfOrdinal, svfInteger]
  else
    Result := [svfOrdinal, svfCardinal];
  Result := Result + inherited GetFieldFlags;
end;

function TFpValueConstNumber.GetAsCardinal: QWord;
begin
  Result := FValue;
end;

function TFpValueConstNumber.GetAsInteger: Int64;
begin
  Result := Int64(FValue);
end;

function TFpValueConstNumber.GetAsFloat: Extended;
begin
  Result := GetAsInteger;
end;

constructor TFpValueConstNumber.Create(AValue: QWord; ASigned: Boolean);
begin
  inherited Create;
  FValue := AValue;
  FSigned := ASigned;
end;

{ TFpValueConstFloat }

function TFpValueConstFloat.GetKind: TDbgSymbolKind;
begin
  Result := skFloat;
end;

function TFpValueConstFloat.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfFloat];
  Result := Result + inherited GetFieldFlags;
end;

function TFpValueConstFloat.GetAsFloat: Extended;
begin
  Result := FValue;
end;

constructor TFpValueConstFloat.Create(AValue: Extended);
begin
  inherited Create;
  FValue := AValue;
end;

{ TFpValueConstBool }

function TFpValueConstBool.GetKind: TDbgSymbolKind;
begin
  Result := skBoolean;
end;

function TFpValueConstBool.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfOrdinal, svfBoolean];
  Result := Result + inherited GetFieldFlags;
end;

function TFpValueConstBool.GetAsBool: Boolean;
begin
  Result := FValue;
end;

function TFpValueConstBool.GetAsCardinal: QWord;
begin
  if FValue then
    Result := 1
  else
    Result := 0;
end;

constructor TFpValueConstBool.Create(AValue: Boolean);
begin
  inherited Create;
  FValue := AValue;
end;

{ TDbgSymbolValueConstAddress }

function TFpValueConstAddress.GetKind: TDbgSymbolKind;
begin
  Result := skAddress;
end;

function TFpValueConstAddress.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := [svfAddress]
end;

function TFpValueConstAddress.GetAddress: TFpDbgMemLocation;
begin
  Result := FAddress;
end;

constructor TFpValueConstAddress.Create(const AnAddress: TFpDbgMemLocation);
begin
  inherited Create;
  FAddress := AnAddress;
end;

{ TFpValueTypeDeclaration }

function TFpValueTypeDefinition.GetKind: TDbgSymbolKind;
begin
  Result := skType;
end;

function TFpValueTypeDefinition.GetDbgSymbol: TFpSymbol;
begin
  Result := FSymbol;
end;

constructor TFpValueTypeDefinition.Create(ASymbol: TFpSymbol);
begin
  inherited Create;
  FSymbol := ASymbol;
  FSymbol.AddReference{$IFDEF WITH_REFCOUNT_DEBUG}(@FSymbol, 'TFpValueTypeDeclaration'){$ENDIF};
end;

destructor TFpValueTypeDefinition.Destroy;
begin
  inherited Destroy;
  FSymbol.ReleaseReference{$IFDEF WITH_REFCOUNT_DEBUG}(@FSymbol, 'TFpValueTypeDeclaration'){$ENDIF};
end;

{ TFpValueConstEnumValue }

function TFpValueConstEnumValue.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := inherited GetFieldFlags;
  Result := Result + [{svfOrdinal,} svfIdentifier];
end;

function TFpValueConstEnumValue.GetKind: TDbgSymbolKind;
begin
  Result := skEnumValue;
end;

function TFpValueConstEnumValue.GetAsString: AnsiString;
begin
  Result := FName;
end;

constructor TFpValueConstEnumValue.Create(AName: String);
begin
  inherited Create;
  FName := AName;
end;

{ TFpValueConstSet }

function TFpValueConstSet.GetFieldFlags: TFpValueFieldFlags;
begin
  Result := inherited GetFieldFlags;
  Result := Result + [svfMembers];
end;

function TFpValueConstSet.GetKind: TDbgSymbolKind;
begin
  Result := skSet;
end;

function TFpValueConstSet.GetMember(AIndex: Int64): TFpValue;
begin
  Result := TFpValueConstEnumValue.Create(FNames[AIndex]);
end;

function TFpValueConstSet.GetMemberCount: Integer;
begin
  Result := FNames.Count;
end;

constructor TFpValueConstSet.Create(ANames: TStrings);
begin
  inherited Create;
  FNames := ANames;
end;

destructor TFpValueConstSet.Destroy;
begin
  inherited Destroy;
  FNames.Free;
end;

{ TDbgInfoAddressContext }

function TFpDbgSymbolScope.GetMemManager: TFpDbgMemManager;
begin
  Result := LocationContext.MemManager;
end;

constructor TFpDbgSymbolScope.Create(ALocationContext: TFpDbgLocationContext);
begin
  FLocationContext := ALocationContext;
  FLocationContext.AddReference;
  inherited Create;
  AddReference;
end;

destructor TFpDbgSymbolScope.Destroy;
begin
  inherited Destroy;
  FLocationContext.ReleaseReference;
end;

function TFpDbgSymbolScope.GetProcedureAtAddress: TFpValue;
var
  Sym: TFpSymbol;
begin
  Result := nil;
  Sym := SymbolAtAddress;
  if Sym <> nil then
    Result := Sym.Value;
end;

function TFpDbgSymbolScope.GetSizeOfAddress: Integer;
begin
  Result := LocationContext.SizeOfAddress;
end;

function TFpDbgSymbolScope.GetSymbolAtAddress: TFpSymbol;
begin
  Result := nil;
end;

function TFpDbgSymbolScope.FindSymbol(const AName: String;
  const OnlyUnitName: String): TFpValue;
begin
  Result := nil;
end;

function TFpDbgSimpleLocationContext.GetMemManager: TFpDbgMemManager;
begin
  Result := FMemManager;
end;

function TFpDbgSimpleLocationContext.GetAddress: TDbgPtr;
begin
  Result := fAddress;
end;

function TFpDbgSimpleLocationContext.GetThreadId: Integer;
begin
  Result := fThreadId;
end;

function TFpDbgSimpleLocationContext.GetStackFrame: Integer;
begin
  Result := fStackFrame;
end;

function TFpDbgSimpleLocationContext.GetSizeOfAddress: Integer;
begin
  Result := FSizeOfAddr;
end;

constructor TFpDbgSimpleLocationContext.Create(AMemManager: TFpDbgMemManager;
  AnAddress: TDbgPtr; AnSizeOfAddr, AThreadId: Integer; AStackFrame: Integer);
begin
  inherited Create;
  AddReference;
  FMemManager := AMemManager;
  FAddress := AnAddress;
  FSizeOfAddr := AnSizeOfAddr;
  FThreadId := AThreadId;
  FStackFrame := AStackFrame;
end;

{ TFpSymbol }

constructor TFpSymbol.Create(const AName: String);
begin
  inherited Create;
  AddReference;
  if AName <> '' then
    SetName(AName);
end;

constructor TFpSymbol.Create(const AName: String; AKind: TDbgSymbolKind;
  const AAddress: TFpDbgMemLocation);
begin
  Create(AName);
  SetKind(AKind);
  FAddress := AAddress;
end;

destructor TFpSymbol.Destroy;
begin
  SetTypeInfo(nil);
  inherited Destroy;
end;

function TFpSymbol.ReadSize(const AValueObj: TFpValue; out
  ASize: TFpDbgValueSize): Boolean;
begin
  Result := DoReadSize(AValueObj, ASize);
end;

function TFpSymbol.GetInstanceClassName(AValueObj: TFpValue; AClassName,
  AUnitName: PString; AParentClassIndex: integer): boolean;
begin
  if AClassName <> nil then AClassName^ := '';
  if AUnitName  <> nil then AUnitName^ := '';
  Result := False;
end;

function TFpSymbol.GetValueBounds(AValueObj: TFpValue; out ALowBound,
  AHighBound: Int64): Boolean;
begin
  Result := GetValueLowBound(AValueObj, ALowBound); // TODO: ond GetValueHighBound() // but all callers must check result;
  if not GetValueHighBound(AValueObj, AHighBound) then
    Result := False;
end;

function TFpSymbol.GetValueLowBound(AValueObj: TFpValue; out
  ALowBound: Int64): Boolean;
begin
  Result := False;
end;

function TFpSymbol.GetValueHighBound(AValueObj: TFpValue; out
  AHighBound: Int64): Boolean;
begin
  Result := False;
end;

function TFpSymbol.TypeCastValue(AValue: TFpValue): TFpValue;
begin
  Result := nil;
end;

function TFpSymbol.CreateSymbolScope(ALocationContext: TFpDbgLocationContext
  ): TFpDbgSymbolScope;
begin
  Result := nil;
end;

function TFpSymbol.GetAddress: TFpDbgMemLocation;
begin
  if not(sfiAddress in FEvaluatedFields) then
    AddressNeeded;
  Result := FAddress;
end;

function TFpSymbol.GetTypeInfo: TFpSymbol;
begin
  if not(sfiTypeInfo in FEvaluatedFields) then
    TypeInfoNeeded;
  Result := FTypeInfo;
end;

function TFpSymbol.GetMemberVisibility: TDbgSymbolMemberVisibility;
begin
  if not(sfiMemberVisibility in FEvaluatedFields) then
    MemberVisibilityNeeded;
  Result := FMemberVisibility;
end;

procedure TFpSymbol.SetLastError(AValueObj: TFpValue; ALastError: TFpError);
begin
  if AValueObj <> nil then
    AValueObj.SetLastError(ALastError);
end;

function TFpSymbol.HasError(AValueObj: TFpValue): Boolean;
begin
  Result := (AValueObj <> nil) and IsError(AValueObj.LastError);
end;

function TFpSymbol.GetValueObject: TFpValue;
begin
  Result := nil;
end;

function TFpSymbol.GetKind: TDbgSymbolKind;
begin
  if not(sfiKind in FEvaluatedFields) then
    KindNeeded;
  Result := FKind;
end;

function TFpSymbol.GetName: String;
begin
  if not(sfiName in FEvaluatedFields) then
    NameNeeded;
  Result := FName;
end;

function TFpSymbol.GetSymbolType: TDbgSymbolType;
begin
  if not(sfiSymType in FEvaluatedFields) then
    SymbolTypeNeeded;
  Result := FSymbolType;
end;

function TFpSymbol.GetInternalTypeInfo: TFpSymbol;
begin
  Result := Self;
end;

function TFpSymbol.GetLineEndAddress: TDBGPtr;
begin
  Result := 0;
end;

function TFpSymbol.GetLineStartAddress: TDBGPtr;
begin
  Result := 0;
end;

function TFpSymbol.GetHasOrdinalValue: Boolean;
begin
  Result := False;
end;

function TFpSymbol.GetOrdinalValue: Int64;
begin
  Result := 0;
end;

function TFpSymbol.GetNestedSymbol(AIndex: Int64): TFpSymbol;
begin
  Result := nil;
end;

function TFpSymbol.GetNestedSymbolByName(const AIndex: String): TFpSymbol;
begin
  Result := nil;
end;

function TFpSymbol.GetNestedSymbolCount: Integer;
begin
  Result := 0;
end;

procedure TFpSymbol.SetAddress(const AValue: TFpDbgMemLocation);
begin
  FAddress := AValue;
  Include(FEvaluatedFields, sfiAddress);
end;

procedure TFpSymbol.SetKind(AValue: TDbgSymbolKind);
begin
  FKind := AValue;
  Include(FEvaluatedFields, sfiKind);
end;

procedure TFpSymbol.SetSymbolType(AValue: TDbgSymbolType);
begin
  FSymbolType := AValue;
  Include(FEvaluatedFields, sfiSymType);
end;

procedure TFpSymbol.SetTypeInfo(ASymbol: TFpSymbol);
begin
  if FTypeInfo <> nil then begin
    //Assert((FTypeInfo.Reference = self) or (FTypeInfo.Reference = nil), 'FTypeInfo.Reference = self|nil');
    FTypeInfo.ReleaseReference{$IFDEF WITH_REFCOUNT_DEBUG}(@FTypeInfo, ClassName+'.SetTypeInfo');{$ENDIF}
  end;
  FTypeInfo := ASymbol;
  Include(FEvaluatedFields, sfiTypeInfo);
  if FTypeInfo <> nil then begin
    FTypeInfo.AddReference{$IFDEF WITH_REFCOUNT_DEBUG}(@FTypeInfo, ClassName+'.SetTypeInfo'){$ENDIF};
  end;
end;

procedure TFpSymbol.SetMemberVisibility(AValue: TDbgSymbolMemberVisibility);
begin
  FMemberVisibility := AValue;
  Include(FEvaluatedFields, sfiMemberVisibility);
end;

procedure TFpSymbol.SetName(const AValue: String);
begin
  FName := AValue;
  Include(FEvaluatedFields, sfiName);
end;

function TFpSymbol.GetChild(AIndex: Integer): TFpSymbol;
begin
  result := nil;
end;

function TFpSymbol.GetColumn: Cardinal;
begin
  Result := 0;
end;

function TFpSymbol.GetFile: String;
begin
  Result := '';
end;

function TFpSymbol.GetFlags: TDbgSymbolFlags;
begin
  Result := [];
end;

function TFpSymbol.GetLine: Cardinal;
begin
  Result := 0;
end;

function TFpSymbol.GetParent: TFpSymbol;
begin
  Result := nil;
end;

procedure TFpSymbol.KindNeeded;
begin
  SetKind(skNone);
end;

procedure TFpSymbol.NameNeeded;
begin
  SetName('');
end;

procedure TFpSymbol.SymbolTypeNeeded;
begin
  SetSymbolType(stNone);
end;

procedure TFpSymbol.AddressNeeded;
begin
  SetAddress(InvalidLoc);
end;

function TFpSymbol.DoReadSize(const AValueObj: TFpValue; out
  ASize: TFpDbgValueSize): Boolean;
begin
  ASize := ZeroSize;
  Result := False;
end;

procedure TFpSymbol.TypeInfoNeeded;
begin
  SetTypeInfo(nil);
end;

procedure TFpSymbol.MemberVisibilityNeeded;
begin
  SetMemberVisibility(svPrivate);
end;

{ TFpSymbolForwarder }

procedure TFpSymbolForwarder.SetForwardToSymbol(AValue: TFpSymbol);
begin
  FForwardToSymbol := AValue;
  EvaluatedFields :=  EvaluatedFields + [sfiForwardToSymbol];
end;

procedure TFpSymbolForwarder.ForwardToSymbolNeeded;
begin
  SetForwardToSymbol(nil);
end;

function TFpSymbolForwarder.GetForwardToSymbol: TFpSymbol;
begin
  if not(sfiForwardToSymbol in EvaluatedFields) then begin
    if TMethod(@ForwardToSymbolNeeded).Code = Pointer(@TFpSymbolForwarder.ForwardToSymbolNeeded) then
      exit(nil);

    ForwardToSymbolNeeded;
  end;
  Result := FForwardToSymbol;
end;

procedure TFpSymbolForwarder.KindNeeded;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    SetKind(p.Kind)
  else
    SetKind(skNone);  //  inherited KindNeeded;
end;

procedure TFpSymbolForwarder.NameNeeded;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    SetName(p.Name)
  else
    SetName('');  //  inherited NameNeeded;
end;

procedure TFpSymbolForwarder.SymbolTypeNeeded;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    SetSymbolType(p.SymbolType)
  else
    SetSymbolType(stNone);  //  inherited SymbolTypeNeeded;
end;

function TFpSymbolForwarder.DoReadSize(const AValueObj: TFpValue; out
  ASize: TFpDbgValueSize): Boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.DoReadSize(AValueObj, ASize)
  else
    Result := inherited DoReadSize(AValueObj, ASize);
end;

procedure TFpSymbolForwarder.TypeInfoNeeded;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    SetTypeInfo(p.TypeInfo)
  else
    SetTypeInfo(nil);  //  inherited TypeInfoNeeded;
end;

procedure TFpSymbolForwarder.MemberVisibilityNeeded;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    SetMemberVisibility(p.MemberVisibility)
  else
    SetMemberVisibility(svPrivate);  //  inherited MemberVisibilityNeeded;
end;

function TFpSymbolForwarder.GetFlags: TDbgSymbolFlags;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.Flags
  else
    Result := [];  //  Result := inherited GetFlags;
end;

function TFpSymbolForwarder.GetValueObject: TFpValue;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.Value
  else
    Result := nil;  //  Result := inherited Value;
end;

function TFpSymbolForwarder.GetHasOrdinalValue: Boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.HasOrdinalValue
  else
    Result := False;  //  Result := inherited GetHasOrdinalValue;
end;

function TFpSymbolForwarder.GetOrdinalValue: Int64;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.OrdinalValue
  else
    Result := 0;  //  Result := inherited GetOrdinalValue;
end;

function TFpSymbolForwarder.GetInstanceClassName(AValueObj: TFpValue;
  AClassName, AUnitName: PString; AParentClassIndex: integer): boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.GetInstanceClassName(AValueObj, AClassName, AUnitName, AParentClassIndex)
  else
    Result := inherited GetInstanceClassName(AValueObj, AClassName, AUnitName, AParentClassIndex);
end;

function TFpSymbolForwarder.GetValueBounds(AValueObj: TFpValue; out
  ALowBound, AHighBound: Int64): Boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.GetValueBounds(AValueObj, ALowBound, AHighBound)
  else
    Result := inherited GetValueBounds(AValueObj, ALowBound, AHighBound);
end;

function TFpSymbolForwarder.GetValueLowBound(AValueObj: TFpValue; out
  ALowBound: Int64): Boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.GetValueLowBound(AValueObj, ALowBound)
  else
    Result := inherited GetValueLowBound(AValueObj, ALowBound);
end;

function TFpSymbolForwarder.GetValueHighBound(AValueObj: TFpValue; out
  AHighBound: Int64): Boolean;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.GetValueHighBound(AValueObj, AHighBound)
  else
    Result := inherited GetValueHighBound(AValueObj, AHighBound);
end;

function TFpSymbolForwarder.GetNestedSymbol(AIndex: Int64): TFpSymbol;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.NestedSymbol[AIndex]
  else
    Result := nil;  //  Result := inherited GetMember(AIndex);
end;

function TFpSymbolForwarder.GetNestedSymbolByName(const AIndex: String
  ): TFpSymbol;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.NestedSymbolByName[AIndex]
  else
    Result := nil;  //  Result := inherited GetMemberByName(AIndex);
end;

function TFpSymbolForwarder.GetNestedSymbolCount: Integer;
var
  p: TFpSymbol;
begin
  p := GetForwardToSymbol;
  if p <> nil then
    Result := p.NestedSymbolCount
  else
    Result := 0;  //  Result := inherited GetMemberCount;
end;

{ TDbgInfo }

constructor TDbgInfo.Create(ALoaderList: TDbgImageLoaderList;
  AMemManager: TFpDbgMemManager);
begin
  FMemManager := AMemManager;
  inherited Create;
end;

function TDbgInfo.FindSymbolScope(ALocationContext: TFpDbgLocationContext;
  AAddress: TDbgPtr): TFpDbgSymbolScope;
begin
  Result := nil;
end;

function TDbgInfo.FindProcSymbol(AAddress: TDbgPtr): TFpSymbol;
begin
  Result := nil;
end;

function TDbgInfo.FindProcSymbol(const AName: String): TFpSymbol;
begin
  Result := nil;
end;

function TDbgInfo.FindLineInfo(AAddress: TDbgPtr): TFpSymbol;
begin
  Result := nil;
end;

function TDbgInfo.FindProcStartEndPC(const AAddress: TDbgPtr; out AStartPC,
  AEndPC: TDBGPtr): boolean;
begin
  AStartPC := 0;
  AEndPC := 0;
  Result := false;
end;

function TDbgInfo.GetLineAddresses(const AFileName: String; ALine: Cardinal;
  var AResultList: TDBGPtrArray; AFindSibling: TGetLineAddrFindSibling;
  AFoundLine: PInteger; AFoundFilename: PBoolean): Boolean;
begin
  Result := False;
end;

procedure TDbgInfo.SetHasInfo;
begin
  FHasInfo := True;
end;

initialization
  FPDBG_FUNCCALL := DebugLogger.FindOrRegisterLogGroup('FPDBG_FUNCCALL' {$IFDEF FPDBG_FUNCCALL} , True {$ENDIF} );
end.

