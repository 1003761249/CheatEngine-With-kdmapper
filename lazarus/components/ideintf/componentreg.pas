{  $Id$  }
{
 /***************************************************************************
                            componentreg.pas
                            ----------------

 ***************************************************************************/

 *****************************************************************************
  See the file COPYING.modifiedLGPL.txt, included in this distribution,
  for details about the license.
 *****************************************************************************

  Author: Mattias Gaertner, Juha Manninen

  Abstract:
    Interface to the component palette and the registered component classes.
    Supports reordering of pages and components by user settings in environment options.
}
unit ComponentReg;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, typinfo, Contnrs, Laz_AVL_Tree, fgl,
  // LCL
  Controls,
  // LazUtils
  LazUtilities, LazLoggerBase, Laz2_XMLCfg, LazMethodList, LazUTF8;

type
  TComponentPriorityCategory = (
    cpBase,
    cpUser,            // User has changed the order using options GUI.
    cpRecommended,
    cpNormal,
    cpOptional
    );
    
  TComponentPriority = record
    Category: TComponentPriorityCategory;
    Level: integer; // higher level means higher priority (range: -1000 to 1000)
  end;
    
const
  ComponentPriorityNormal: TComponentPriority = (Category: cpNormal; Level: 0);

  LCLCompPriority: TComponentPriority = (Category: cpBase; Level: 10);
  FCLCompPriority: TComponentPriority = (Category: cpBase; Level: 9);
  IDEIntfCompPriority: TComponentPriority = (Category: cpBase; Level: 8);

type
  TBaseComponentPage = class;
  TBaseComponentPalette = class;
  TRegisteredComponent = class;
  TOnGetCreationClass = procedure(Sender: TObject;
                              var NewComponentClass: TComponentClass) of object;

  { TRegisteredCompList }

  TRegisteredCompList = class(specialize TFPGList<TRegisteredComponent>)
  public
    function Equals(Obj: TObject): Boolean; override;
  end;

  { TBaseCompPaletteOptions }

  TBaseCompPaletteOptions = class
  protected
    FPageNames: TStringList;    // Pages reordered by user.
  public
    constructor Create;
    destructor Destroy; override;
    procedure Clear;
    procedure Assign(Source: TBaseCompPaletteOptions);
    function Equals(Obj: TObject): boolean; override;
  public
    property PageNames: TStringList read FPageNames;
  end;

  { TCompPaletteOptions }

  TCompPaletteOptions = class(TBaseCompPaletteOptions)
  private
    FName: string;
    // List of page names with component names.
    // Object holds another TStringList for the components.
    FPageNamesCompNames: TStringList;
    // Pages removed or renamed. They must be hidden in the palette.
    FHiddenPageNames: TStringList;
    FVisible: boolean;
    procedure AddPageCompName(aPageName, aCompName: string);
  public
    constructor Create;
    destructor Destroy; override;
    procedure Clear;
    procedure Assign(Source: TCompPaletteOptions);
    procedure AssignPageCompNames(aPageName: string; aList: TStringList);
    function IsDefault: Boolean;
    procedure Load(XMLConfig: TXMLConfig; Path: String);
    procedure Save(XMLConfig: TXMLConfig; Path: String);
    function Equals(Obj: TObject): boolean; override;
  public
    property Name: string read FName write FName;
    property PageNamesCompNames: TStringList read FPageNamesCompNames;
    property HiddenPageNames: TStringList read FHiddenPageNames;
    property Visible: boolean read FVisible write FVisible;
  end;

  { TCompPaletteUserOrder }

  // Only used by the component palette options to show all available pages.
  // It's like TCompPaletteOptions but collects all pages and components,
  //  including the original ones and the newly installed ones.
  //  The active palette is later synchronized with this.
  TCompPaletteUserOrder = class(TBaseCompPaletteOptions)
  private
    fPalette: TBaseComponentPalette;
    // List of page names with component contents.
    // Object holds TRegisteredCompList for the components.
    FComponentPages: TStringList;
    // Reference to either EnvironmentOptions.ComponentPaletteOptions or a copy of it.
    fOptions: TCompPaletteOptions;
    procedure AddRecentlyInstalledComps;
    function FindComp(aCompClassName: string): TRegisteredComponent;
  public
    constructor Create(aPalette: TBaseComponentPalette);
    destructor Destroy; override;
    procedure Assign(Source: TCompPaletteUserOrder);
    procedure AssignCompPage(aPageName: string; aList: TRegisteredCompList);
    procedure Clear;
    function Equals(Obj: TObject): boolean; override;
    function SortPagesAndCompsUserOrder: Boolean;
  public
    // all pages, ordered first by Options, then by default priority
    property ComponentPages: TStringList read FComponentPages;
    property Options: TCompPaletteOptions read fOptions write fOptions;
  end;

  { TRegisteredComponent }

  TRegisteredComponent = class
  private
    FComponentClass: TComponentClass;
    FOnGetCreationClass: TOnGetCreationClass;
    FOrigPageName: string;
    FRealPage: TBaseComponentPage;
    FVisible: boolean;
  protected
    FNextSameName: TRegisteredComponent;
    FPrevSameName: TRegisteredComponent;
  public
    constructor Create(TheComponentClass: TComponentClass; const ThePageName: string);
    destructor Destroy; override;
    procedure ConsistencyCheck; virtual;
    function GetUnitName: string; virtual; abstract;
    function GetPriority: TComponentPriority; virtual;
    procedure AddToPalette; virtual;
    function CanBeCreatedInDesigner: boolean; virtual;
    function GetCreationClass: TComponentClass; virtual;
    function HasAmbiguousClassName: boolean; virtual; // true if another component class with same name (different unit) is registered
    function GetFullClassName(const UnitSep: char = '/'): string; virtual;
  public
    property ComponentClass: TComponentClass read FComponentClass; // Beware of ambiguous classnames
    property OnGetCreationClass: TOnGetCreationClass read FOnGetCreationClass
                                                     write FOnGetCreationClass;
    property OrigPageName: string read FOrigPageName; // case sensitive
    property RealPage: TBaseComponentPage read FRealPage write FRealPage;
    property Visible: boolean read FVisible write FVisible;
    property NextSameName: TRegisteredComponent read FNextSameName;
    property PrevSameName: TRegisteredComponent read FPrevSameName;
  end;

  { TBaseComponentPage }

  TBaseComponentPage = class
  private
    FPageName: string;
    FPalette: TBaseComponentPalette;
    FVisible: boolean;
  protected
    FIndex: Integer;           // Index in the Pages container.
  public
    constructor Create(const ThePageName: string);
  public
    property PageName: string read FPageName;
    property Palette: TBaseComponentPalette read FPalette write FPalette;
    property Visible: boolean read FVisible write FVisible;
  end;

  TBaseComponentPageClass = class of TBaseComponentPage;

  { TBaseComponentPalette }
  
  TComponentPaletteHandlerType = (
    cphtVoteVisibility,  // Visibility of component palette icons is recomputed
    cphtComponentAdded,  // Typically selection is changed after component was added.
    cphtSelectionChanged,
    cphtUpdated
    );

  TComponentSelectionMode = (
    csmSingle, // reset selection on component add
    csmMulty   // don't reset selection on component add
  );

  TEndUpdatePaletteEvent = procedure(Sender: TObject; PaletteChanged: boolean) of object;
  TGetComponentClassEvent = procedure(const AClass: TComponentClass) of object;
  TUpdateCompVisibleEvent = procedure(AComponent: TRegisteredComponent;
                      var VoteVisible: integer { Visible>0 }  ) of object;
  TPaletteHandlerEvent = procedure of object;
  TComponentAddedEvent = procedure(ALookupRoot, AComponent: TComponent; ARegisteredComponent: TRegisteredComponent) of object;
  RegisterUnitComponentProc = procedure(const Page, UnitName: ShortString;
                                        ComponentClass: TComponentClass);
  TBaseComponentPageList = specialize TFPGList<TBaseComponentPage>;
  TPagePriorityList = specialize TFPGMap<String, TComponentPriority>;

  TBaseComponentPalette = class
  private
    // List of pages, created based on user ordered and original pages.
    fPages: TBaseComponentPageList;
    // List of all components in all pages.
    fComps: TRegisteredCompList;
    // New pages added and their priorities, ordered by priority.
    fOrigPagePriorities: TPagePriorityList;
    // User ordered + original pages and components
    fUserOrder: TCompPaletteUserOrder;
    // Component cache, a tree of TRegisteredComponent sorted for componentclass
    fComponentCache: TAVLTree;
    // Two page caches, one for original pages, one for user ordered pages,
    // containing page names. Object holds TRegisteredCompList for components.
    fOrigComponentPageCache: TStringList;  // Original
    fUserComponentPageCache: TStringList;  // User ordered
    // Used to find names that differ in character case only.
    fOrigPageHelper: TStringListUTF8Fast;
    fHandlers: array[TComponentPaletteHandlerType] of TMethodList;
    fComponentPageClass: TBaseComponentPageClass;
    fSelected: TRegisteredComponent;
    fSelectionMode: TComponentSelectionMode;
    fHideControls: boolean;
    fChangeStamp: integer;
    fOnClassSelected: TNotifyEvent;
    fLastFoundCompClassName: String;
    fLastFoundRegComp: TRegisteredComponent;
    procedure AddHandler(HandlerType: TComponentPaletteHandlerType;
                         const AMethod: TMethod; AsLast: boolean = false);
    procedure RemoveHandler(HandlerType: TComponentPaletteHandlerType;
                            const AMethod: TMethod);
    procedure CacheOrigComponentPages;
    function CreatePagesFromUserOrder: Boolean;
    procedure DoPageAddedComponent(Component: TRegisteredComponent);
    procedure DoPageRemovedComponent(Component: TRegisteredComponent);
    procedure SetHideControls(AValue: boolean);
    function VoteCompVisibility(AComponent: TRegisteredComponent): Boolean;
    function GetSelected: TRegisteredComponent;
    function GetMultiSelect: boolean;
    procedure SetSelected(const AValue: TRegisteredComponent);
    procedure SetMultiSelect(AValue: boolean);
  protected
    FChanged: boolean;
    procedure DoChange; virtual; abstract;
  public
    constructor Create(EnvPaletteOptions: TCompPaletteOptions);
    destructor Destroy; override;
    procedure Clear;
    function AssignOrigCompsForPage(PageName: string;
                                    DestComps: TRegisteredCompList): Boolean;
    function AssignOrigVisibleCompNames(PageName: string;
                                    DestCompNames: TStringList): Boolean;
    function RefUserCompsForPage(PageName: string): TRegisteredCompList;
    procedure BeginUpdate; virtual; abstract;
    procedure EndUpdate; virtual; abstract;
    procedure IncChangeStamp;
    function IndexOfPageName(const APageName: string; ACaseSensitive: Boolean): integer;
    function GetPage(const APageName: string; ACaseSensitive: Boolean=False): TBaseComponentPage;
    procedure AddRegComponent(NewComponent: TRegisteredComponent);
    procedure RemoveRegComponent(AComponent: TRegisteredComponent);
    function FindRegComponent(ACompClass: TClass): TRegisteredComponent;
    function FindRegComponent(const ACompClassName: string): TRegisteredComponent; // can be UnitName/ClassName
    function CreateNewClassName(const Prefix: string): string;
    procedure Update({%H-}ForceUpdateAll: Boolean); virtual;
    procedure IterateRegisteredClasses(Proc: TGetComponentClassEvent);
    procedure SetSelectedComp(AComponent: TRegisteredComponent; AMulti: Boolean);
    // Registered handlers
    procedure DoAfterComponentAdded(ALookupRoot, AComponent: TComponent;
                                    ARegisteredComponent: TRegisteredComponent);
    procedure DoAfterSelectionChanged;
    procedure DoAfterUpdate;
    procedure RemoveAllHandlersOfObject(AnObject: TObject);
    procedure AddHandlerVoteVisibility(const OnUpdateCompVisibleEvent: TUpdateCompVisibleEvent;
                                       AsLast: boolean = false);
    procedure RemoveHandlerVoteVisibility(OnUpdateCompVisibleEvent: TUpdateCompVisibleEvent);
    procedure AddHandlerComponentAdded(OnComponentAddedEvent: TComponentAddedEvent);
    procedure RemoveHandlerComponentAdded(OnComponentAddedEvent: TComponentAddedEvent);
    procedure AddHandlerSelectionChanged(OnSelectionChangedEvent: TPaletteHandlerEvent);
    procedure RemoveHandlerSelectionChanged(OnSelectionChangedEvent: TPaletteHandlerEvent);
    procedure AddHandlerUpdated(OnUpdateEvent: TPaletteHandlerEvent);
    procedure RemoveHandlerUpdated(OnUpdateEvent: TPaletteHandlerEvent);
  public
    property Pages: TBaseComponentPageList read fPages;
    property Comps: TRegisteredCompList read fComps;
    property OrigPagePriorities: TPagePriorityList read fOrigPagePriorities;
    property ComponentPageClass: TBaseComponentPageClass read FComponentPageClass
                                                        write FComponentPageClass;
    property ChangeStamp: integer read fChangeStamp;
    property HideControls: boolean read FHideControls write SetHideControls;
    property Selected: TRegisteredComponent read GetSelected write SetSelected;
    property MultiSelect: boolean read GetMultiSelect write SetMultiSelect;
    property SelectionMode: TComponentSelectionMode read FSelectionMode write FSelectionMode;
    // User ordered + original pages and components.
    property UserOrder: TCompPaletteUserOrder read fUserOrder;
    property OnClassSelected: TNotifyEvent read fOnClassSelected write fOnClassSelected;
  end;
  

var
  IDEComponentPalette: TBaseComponentPalette = nil;

function ComponentPriority(Category: TComponentPriorityCategory; Level: integer): TComponentPriority;
function ComparePriority(const p1,p2: TComponentPriority): integer;
function CompareIDEComponentByClass(Data1, Data2: pointer): integer;
function dbgs(const c: TComponentPriorityCategory): string; overload;
function dbgs(const p: TComponentPriority): string; overload;

implementation

const
  BasePath = 'ComponentPaletteOptions/';

function ComponentPriority(Category: TComponentPriorityCategory; Level: integer
  ): TComponentPriority;
begin
  Result.Category:=Category;
  Result.Level:=Level;
end;

function ComparePriority(const p1, p2: TComponentPriority): integer;
begin
  // lower category is better
  Result:=ord(p2.Category)-ord(p1.Category);
  if Result<>0 then exit;
  // higher level is better
  Result:=p1.Level-p2.Level;
end;

function CompareIDEComponentByClass(Data1, Data2: Pointer): integer;
var
  Comp1: TRegisteredComponent absolute Data1;
  Comp2: TRegisteredComponent absolute Data2;
begin
  // The same compare function must be used here and in CompareClassWithIDEComponent.
  Result:=ComparePointers(Comp1.ComponentClass, Comp2.ComponentClass);
end;

function CompareClassWithIDEComponent(Key, Data: Pointer): integer;
var
  AClass: TComponentClass absolute Key;
  RegComp: TRegisteredComponent absolute Data;
begin
  Result:=ComparePointers(AClass, RegComp.ComponentClass);
end;

function dbgs(const c: TComponentPriorityCategory): string;
begin
  Result:=GetEnumName(TypeInfo(TComponentPriorityCategory),ord(c));
end;

function dbgs(const p: TComponentPriority): string;
begin
  Result:='Cat='+dbgs(p.Category)+',Lvl='+IntToStr(p.Level);
end;

{ TRegisteredCompList }

function TRegisteredCompList.Equals(Obj: TObject): Boolean;
Var
  i: Longint;
  Source: TRegisteredCompList;
begin
  if Obj is TRegisteredCompList then
  begin
    Source:=TRegisteredCompList(Obj);
    if Count<>Source.Count then exit(False);
    For i:=0 to Count-1 do
      If Items[i]<>Source[i] then exit(False);
    Result:=True;
  end else
    Result:=inherited Equals(Obj);
end;

{ TBaseCompPaletteOptions }

constructor TBaseCompPaletteOptions.Create;
begin
  inherited Create;
  FPageNames := TStringList.Create;
end;

destructor TBaseCompPaletteOptions.Destroy;
begin
  FreeAndNil(FPageNames);
  inherited Destroy;
end;

procedure TBaseCompPaletteOptions.Clear;
begin
  FPageNames.Clear;
end;

procedure TBaseCompPaletteOptions.Assign(Source: TBaseCompPaletteOptions);
begin
  FPageNames.Assign(Source.FPageNames);
end;

function TBaseCompPaletteOptions.Equals(Obj: TObject): boolean;
var
  Source: TBaseCompPaletteOptions;
begin
  if Obj is TBaseCompPaletteOptions then
  begin
    Source:=TBaseCompPaletteOptions(Obj);
    Result:=FPageNames.Equals(Source.FPageNames);
  end else
    Result:=inherited Equals(Obj);
end;

{ TCompPaletteOptions }

constructor TCompPaletteOptions.Create;
begin
  inherited Create;
  FPageNamesCompNames := TStringListUTF8Fast.Create;
  FPageNamesCompNames.OwnsObjects := True;
  FHiddenPageNames := TStringListUTF8Fast.Create;
  FVisible := True;
end;

destructor TCompPaletteOptions.Destroy;
begin
  FHiddenPageNames.Free;
  FPageNamesCompNames.Free;
  inherited Destroy;
end;

procedure TCompPaletteOptions.Clear;
begin
  inherited Clear;
  FPageNamesCompNames.Clear;
  FHiddenPageNames.Clear;
end;

procedure TCompPaletteOptions.AddPageCompName(aPageName, aCompName: string);
var
  i: Integer;
  sl: TStringList;
begin
  i := FPageNamesCompNames.IndexOf(aPageName);
  if i >= 0 then begin
    sl := TStringList(FPageNamesCompNames.Objects[i]);
    sl.Add(aCompName);
  end
  else
    DebugLn(['TCompPaletteOptions.AddPageCompName: Page "',aPageName,'" not found.']);
end;

procedure TCompPaletteOptions.Assign(Source: TCompPaletteOptions);
var
  i: Integer;
  sl: TStringList;
begin
  inherited Assign(Source);
  // Name: do not assign name
  FPageNamesCompNames.Clear;
  for i:=0 to Source.FPageNamesCompNames.Count-1 do
  begin
    sl := TStringList.Create;
    sl.Assign(TStringList(Source.FPageNamesCompNames.Objects[i]));
    FPageNamesCompNames.AddObject(Source.FPageNamesCompNames[i], sl);
  end;
  FHiddenPageNames.Assign(Source.FHiddenPageNames);
  FVisible := Source.FVisible;
end;

procedure TCompPaletteOptions.AssignPageCompNames(aPageName: string; aList: TStringList);
var
  sl: TStringList;
begin
  sl := TStringList.Create;
  sl.Assign(aList);
  FPageNamesCompNames.AddObject(aPageName, sl);
end;

function TCompPaletteOptions.IsDefault: Boolean;
begin
  Result := (FPageNames.Count = 0)
    and (FPageNamesCompNames.Count = 0)
    and (FHiddenPageNames.Count = 0);
end;

procedure TCompPaletteOptions.Load(XMLConfig: TXMLConfig; Path: String);
var
  CompNames: TStringList;
  SubPath, CompPath: String;
  PageName, CompName: String;
  PageCount, CompCount: Integer;
  i, j: Integer;
begin
  Path := Path + BasePath;
  try
    FName:=XMLConfig.GetValue(Path+'Name/Value','');
    FVisible:=XMLConfig.GetValue(Path+'Visible/Value',true);

    // Pages
    FPageNames.Clear;
    SubPath:=Path+'Pages/';
    PageCount:=XMLConfig.GetValue(SubPath+'Count', 0);
    for i:=1 to PageCount do begin
      PageName:=XMLConfig.GetValue(SubPath+'Item'+IntToStr(i)+'/Value', '');
      if PageName <> '' then
        FPageNames.Add(PageName);
    end;

    // HiddenPages
    FHiddenPageNames.Clear;
    SubPath:=Path+'HiddenPages/';
    PageCount:=XMLConfig.GetValue(SubPath+'Count', 0);
    for i:=1 to PageCount do begin
      PageName:=XMLConfig.GetValue(SubPath+'Item'+IntToStr(i)+'/Value', '');
      if PageName <> '' then
        FHiddenPageNames.Add(PageName);
    end;

    // ComponentPages
    FPageNamesCompNames.Clear;
    SubPath:=Path+'ComponentPages/';
    PageCount:=XMLConfig.GetValue(SubPath+'Count', 0);
    for i:=1 to PageCount do begin
      CompPath:=SubPath+'Page'+IntToStr(i)+'/';
      PageName:=XMLConfig.GetValue(CompPath+'Value', '');
      CompNames:=TStringList.Create;
      CompCount:=XMLConfig.GetValue(CompPath+'Components/Count', 0);
      for j:=1 to CompCount do begin
        CompName:=XMLConfig.GetValue(CompPath+'Components/Item'+IntToStr(j)+'/Value', '');
        CompNames.Add(CompName);
      end;                                // CompNames is owned by FComponentPages
      FPageNamesCompNames.AddObject(PageName, CompNames);
    end;
  except
    on E: Exception do begin
      DebugLn('ERROR: TCompPaletteOptions.Load: ',E.Message);
      exit;
    end;
  end;
end;

procedure TCompPaletteOptions.Save(XMLConfig: TXMLConfig; Path: String);
var
  CompNames: TStringList;
  SubPath, CompPath: String;
  i, j: Integer;
begin
  try
    Path := Path + BasePath;
    XMLConfig.SetDeleteValue(Path+'Name/Value', FName,'');
    XMLConfig.SetDeleteValue(Path+'Visible/Value', FVisible,true);

    SubPath:=Path+'Pages/';
    XMLConfig.DeletePath(SubPath);
    XMLConfig.SetDeleteValue(SubPath+'Count', FPageNames.Count, 0);
    for i:=0 to FPageNames.Count-1 do
      XMLConfig.SetDeleteValue(SubPath+'Item'+IntToStr(i+1)+'/Value', FPageNames[i], '');

    SubPath:=Path+'HiddenPages/';
    XMLConfig.DeletePath(SubPath);
    XMLConfig.SetDeleteValue(SubPath+'Count', FHiddenPageNames.Count, 0);
    for i:=0 to FHiddenPageNames.Count-1 do
      XMLConfig.SetDeleteValue(SubPath+'Item'+IntToStr(i+1)+'/Value', FHiddenPageNames[i], '');

    SubPath:=Path+'ComponentPages/';
    XMLConfig.DeletePath(SubPath);
    XMLConfig.SetDeleteValue(SubPath+'Count', FPageNamesCompNames.Count, 0);
    for i:=0 to FPageNamesCompNames.Count-1 do begin
      CompNames:=TStringList(FPageNamesCompNames.Objects[i]);
      CompPath:=SubPath+'Page'+IntToStr(i+1)+'/';
      XMLConfig.SetDeleteValue(CompPath+'Value', FPageNamesCompNames[i], '');
      XMLConfig.SetDeleteValue(CompPath+'Components/Count', CompNames.Count, 0);
      for j:=0 to CompNames.Count-1 do
        XMLConfig.SetDeleteValue(CompPath+'Components/Item'+IntToStr(j+1)+'/Value',
                                 CompNames[j], '');
    end;
  except
    on E: Exception do begin
      DebugLn('ERROR: TCompPaletteOptions.Save: ',E.Message);
      exit;
    end;
  end;
end;

function TCompPaletteOptions.Equals(Obj: TObject): boolean;
var
  Source: TCompPaletteOptions;
begin
  Result:=inherited Equals(Obj);
  if not Result then exit;
  if Obj is TCompPaletteOptions then
  begin
    Source:=TCompPaletteOptions(Obj);
    // Name: do not check Name
    if Visible<>Source.Visible then exit(false);
    if not FHiddenPageNames.Equals(Source.FHiddenPageNames) then exit(false);
  end;
end;

{ TCompPaletteUserOrder }

constructor TCompPaletteUserOrder.Create(aPalette: TBaseComponentPalette);
begin
  inherited Create;
  fPalette:=aPalette;
  FComponentPages := TStringListUTF8Fast.Create;
  FComponentPages.OwnsObjects := True;
end;

destructor TCompPaletteUserOrder.Destroy;
begin
  Clear;
  FreeAndNil(FComponentPages);
  inherited Destroy;
end;

procedure TCompPaletteUserOrder.AddRecentlyInstalledComps;
// Add comps that are not found in user config for any page
// but are found in original config for a certain page.
// They are components that a user installed after configuring the palette.
var
  PageI, i: Integer;
  PgName, CompName: String;
  PalComps, UserComps: TRegisteredCompList;
  RegComp: TRegisteredComponent;
begin
  for PageI := 0 to FComponentPages.Count-1 do
  begin
    PgName := FComponentPages[PageI];
    UserComps := TRegisteredCompList(FComponentPages.Objects[PageI]);
    if fPalette.fOrigComponentPageCache.Find(PgName, i) then begin
      PalComps := TRegisteredCompList(fPalette.fOrigComponentPageCache.Objects[i]);
      for i := PalComps.Count-1 downto 0 do  // Iterate backwards.
      begin
        CompName := PalComps[i].ComponentClass.ClassName;
        if FindComp(CompName)<>Nil then // New installed components are always at
          Break                         // the end of the list -> OK to skip rest.
        else begin
          // Not in user config.
          RegComp := fPalette.FindRegComponent(CompName);
          DebugLn(['Adding recently installed component "',
                   RegComp.ComponentClass.ClassName,'" to user config.']);
          UserComps.Add(RegComp);
          fOptions.AddPageCompName(PgName, CompName);
        end;
      end;
    end;
  end;
end;

procedure TCompPaletteUserOrder.Assign(Source: TCompPaletteUserOrder);
var
  i: Integer;
  nm: String;
  cl: TRegisteredCompList;
begin
  inherited Assign(Source);
  FComponentPages.Clear;
  for i:=0 to Source.FComponentPages.Count-1 do
  begin
    nm := Source.FComponentPages[i];
    cl := TRegisteredCompList(Source.FComponentPages.Objects[i]);
    AssignCompPage(nm, cl);
  end;
end;

procedure TCompPaletteUserOrder.AssignCompPage(aPageName: string; aList: TRegisteredCompList);
var
  rcl: TRegisteredCompList;
begin
  rcl := TRegisteredCompList.Create;
  rcl.Assign(aList);
  FComponentPages.AddObject(aPageName, rcl);
end;

procedure TCompPaletteUserOrder.Clear;
begin
  inherited Clear;
  FComponentPages.Clear;
end;

function TCompPaletteUserOrder.Equals(Obj: TObject): boolean;
var
  Source: TCompPaletteUserOrder;
  i: Integer;
  MyList, SrcList: TRegisteredCompList;
begin
  Result:=inherited Equals(Obj);
  if not Result then exit;
  if Obj is TCompPaletteUserOrder then
  begin
    Source:=TCompPaletteUserOrder(Obj);
    if FComponentPages.Count<>Source.FComponentPages.Count then exit(false);
    for i:=0 to Source.FComponentPages.Count-1 do
    begin
      MyList:=TRegisteredCompList(FComponentPages.Objects[i]);
      SrcList:=TRegisteredCompList(Source.FComponentPages.Objects[i]);
      if not MyList.Equals(SrcList) then exit(false);
    end;
    Result:=true;
  end;
end;

function TCompPaletteUserOrder.FindComp(aCompClassName: string): TRegisteredComponent;
var
  i, j: Integer;
  CompList: TRegisteredCompList;
begin
  for i := 0 to FComponentPages.Count-1 do
  begin
    //PgName := FComponentPages[i];
    CompList := TRegisteredCompList(FComponentPages.Objects[i]);
    Assert(Assigned(CompList), 'TCompPaletteUserOrder.FindComp: CompList=Nil.');
    for j := 0 to CompList.Count-1 do
    begin
      Result := CompList[j];
      if Result.ComponentClass.ClassName = aCompClassName then
        Exit;   // Found
    end;
  end;
  Result := Nil;
end;

function TCompPaletteUserOrder.SortPagesAndCompsUserOrder: Boolean;
// Calculate page order using user config and default order. User config takes priority.
// This order will finally be shown in the palette.
var
  DstComps: TRegisteredCompList;
  RegComp: TRegisteredComponent;
  sl: TStringList;
  PgName: String;
  PageI, i: Integer;
begin
  Result:=True;
  Clear;
  fPalette.CacheOrigComponentPages;
  // First add user defined page order from EnvironmentOptions,
  FComponentPages.Assign(fOptions.FPageNames);
  // then add other pages which don't have user configuration
  for PageI := 0 to fPalette.OrigPagePriorities.Count-1 do
  begin
    PgName:=fPalette.OrigPagePriorities.Keys[PageI];
    if (FComponentPages.IndexOf(PgName) = -1)
    and (fOptions.FHiddenPageNames.IndexOf(PgName) = -1) then
      FComponentPages.Add(PgName);
  end;
  // Map components with their pages
  for PageI := 0 to FComponentPages.Count-1 do
  begin
    PgName := FComponentPages[PageI];
    DstComps := TRegisteredCompList.Create;
    FComponentPages.Objects[PageI] := DstComps;
    i := fOptions.FPageNamesCompNames.IndexOf(PgName);
    if i >= 0 then begin                // Add components reordered by user.
      sl := TStringList(fOptions.FPageNamesCompNames.Objects[i]);
      i := 0;
      while i < sl.Count do
      begin
        RegComp := fPalette.FindRegComponent(sl[i]);
        if Assigned(RegComp) then begin
          DstComps.Add(RegComp);
          Inc(i);
        end
        else begin                    // Component can be uninstalled, not found
          DebugLn(['TCompPaletteUserOrder.SortPagesAndCompsUserOrder: Not found "',sl[i],'". Removing.']);
          sl.Delete(i);               // Delete from configuration.
        end;
      end;
    end
    else                              // Add components that were not reordered.
      fPalette.AssignOrigCompsForPage(PgName, DstComps);
  end;
end;

{ TRegisteredComponent }

constructor TRegisteredComponent.Create(TheComponentClass: TComponentClass;
  const ThePageName: string);
begin
  FComponentClass:=TheComponentClass;
  FOrigPageName:=ThePageName;
  FVisible:=true;
end;

destructor TRegisteredComponent.Destroy;
begin
  if FPrevSameName<>nil then
    FPrevSameName.FNextSameName:=FNextSameName;
  if FNextSameName<>nil then
    FNextSameName.FPrevSameName:=FPrevSameName;
  FPrevSameName:=nil;
  FNextSameName:=nil;
  if Assigned(FRealPage) and Assigned(FRealPage.Palette) then
    FRealPage.Palette.RemoveRegComponent(Self);
  inherited Destroy;
end;

procedure TRegisteredComponent.ConsistencyCheck;
begin
  if (FComponentClass=nil) then
    raise Exception.Create('TRegisteredComponent.ConsistencyCheck FComponentClass=nil');
  if not IsValidIdent(FComponentClass.ClassName) then
    raise Exception.Create('TRegisteredComponent.ConsistencyCheck not IsValidIdent(FComponentClass.ClassName)');
end;

function TRegisteredComponent.GetPriority: TComponentPriority;
begin
  Result:=ComponentPriorityNormal;
end;

procedure TRegisteredComponent.AddToPalette;
begin
  IDEComponentPalette.AddRegComponent(Self);
end;

function TRegisteredComponent.CanBeCreatedInDesigner: boolean;
begin
  Result:=true;
end;

function TRegisteredComponent.GetCreationClass: TComponentClass;
begin
  Result:=FComponentClass;
  if Assigned(OnGetCreationClass) then
    OnGetCreationClass(Self,Result);
end;

function TRegisteredComponent.HasAmbiguousClassName: boolean;
begin
  Result:=(FPrevSameName<>nil) or (FNextSameName<>nil);
end;

function TRegisteredComponent.GetFullClassName(const UnitSep: char): string;
begin
  Result:=ComponentClass.UnitName+UnitSep+ComponentClass.ClassName;
end;

{ TBaseComponentPage }

constructor TBaseComponentPage.Create(const ThePageName: string);
begin
  FPageName:=ThePageName;
  FVisible:=FPageName<>'';
end;

{ TBaseComponentPalette }

constructor TBaseComponentPalette.Create(EnvPaletteOptions: TCompPaletteOptions);
begin
  fSelectionMode:=csmSingle;
  fPages:=TBaseComponentPageList.Create;
  fComps:=TRegisteredCompList.Create;
  fOrigPagePriorities:=TPagePriorityList.Create;
  fUserOrder:=TCompPaletteUserOrder.Create(Self);
  fUserOrder.Options:=EnvPaletteOptions; // EnvironmentOptions.ComponentPaletteOptions;
  fComponentCache:=TAVLTree.Create(@CompareIDEComponentByClass);
  fOrigComponentPageCache:=TStringList.Create;
  fOrigComponentPageCache.OwnsObjects:=True;
  {$IF FPC_FULLVERSION>=30200}fOrigComponentPageCache.UseLocale:=False;{$ENDIF}
  fOrigComponentPageCache.CaseSensitive:=True;
  fOrigComponentPageCache.Sorted:=True;
  fUserComponentPageCache:=TStringList.Create;
  fUserComponentPageCache.OwnsObjects:=True;
  {$IF FPC_FULLVERSION>=30200}fUserComponentPageCache.UseLocale:=False;{$ENDIF}
  fUserComponentPageCache.CaseSensitive:=True;
  fUserComponentPageCache.Sorted:=True;
  fOrigPageHelper:=TStringListUTF8Fast.Create; // Note: CaseSensitive = False
  fOrigPageHelper.Sorted:=True;
  fLastFoundCompClassName:='';
  fLastFoundRegComp:=Nil;
end;

destructor TBaseComponentPalette.Destroy;
var
  HandlerType: TComponentPaletteHandlerType;
begin
  Clear;
  FreeAndNil(fOrigPageHelper);
  FreeAndNil(fUserComponentPageCache);
  FreeAndNil(fOrigComponentPageCache);
  FreeAndNil(fComponentCache);
  FreeAndNil(fUserOrder);
  FreeAndNil(fOrigPagePriorities);
  FreeAndNil(fComps);
  FreeAndNil(fPages);
  for HandlerType:=Low(HandlerType) to High(HandlerType) do
    FHandlers[HandlerType].Free;
  inherited Destroy;
end;

procedure TBaseComponentPalette.Clear;
var
  i: Integer;
begin
  for i:=0 to fPages.Count-1 do
    fPages[i].Free;
  fPages.Clear;
  for i:=0 to fComps.Count-1 do
    fComps[i].RealPage:=nil;
  fComps.Clear;
  fOrigPagePriorities.Clear;
  fOrigPageHelper.Clear;
end;

procedure TBaseComponentPalette.CacheOrigComponentPages;
var
  PageI, CompI: Integer;
  PgName: string;
  RegComp: TRegisteredComponent;
  RegComps: TRegisteredCompList;
begin
  if fOrigComponentPageCache.Count > 0 then Exit;  // Fill cache only once.
  for PageI := 0 to fOrigPagePriorities.Count-1 do
  begin
    PgName:=fOrigPagePriorities.Keys[PageI];
    Assert((PgName <> '') and not fOrigComponentPageCache.Find(PgName, CompI),
                  Format('CacheComponentPages: %s already cached.', [PgName]));
    // Add a cache StringList for this page name.
    RegComps := TRegisteredCompList.Create;
    fOrigComponentPageCache.AddObject(PgName, RegComps);
    // Find all components for this page and add them to cache.
    for CompI := 0 to fComps.Count-1 do begin
      RegComp := fComps[CompI];
      if RegComp.OrigPageName = PgName then // case sensitive!
        RegComps.Add(RegComp);
    end;
  end;
end;

function TBaseComponentPalette.CreatePagesFromUserOrder: Boolean;
var
  UserPageI, CurPgInd, CompI: Integer;
  aVisibleCompCnt: integer;
  PgName: String;
  Pg: TBaseComponentPage;
  RegiComps, UserRegComps: TRegisteredCompList;
  RegComp: TRegisteredComponent;
begin
  Result := True;
  fUserComponentPageCache.Clear;
  for UserPageI := 0 to fUserOrder.ComponentPages.Count-1 do
  begin
    PgName := fUserOrder.ComponentPages[UserPageI];
    CurPgInd := IndexOfPageName(PgName, True);
    if CurPgInd = -1 then begin
      // Create a new page
      Pg := ComponentPageClass.Create(PgName);
      fPages.Insert(UserPageI, Pg);
      Pg.Palette := Self;
    end
    else if CurPgInd <> UserPageI then
      fPages.Move(CurPgInd, UserPageI); // Move page to right place.
    Pg := fPages[UserPageI];
    Pg.FIndex := UserPageI;
    Assert(PgName = Pg.PageName,
      Format('TComponentPalette.CreatePagesFromUserOrder: Page names differ, "%s" and "%s".',
             [PgName, Pg.PageName]));
    // New cache page
    UserRegComps := TRegisteredCompList.Create;
    fUserComponentPageCache.AddObject(PgName, UserRegComps);
    // Associate components belonging to this page
    aVisibleCompCnt := 0;
    RegiComps := TRegisteredCompList(fUserOrder.ComponentPages.Objects[UserPageI]);
    for CompI := 0 to RegiComps.Count-1 do
    begin
      RegComp := RegiComps[CompI];
      if RegComp = nil then Continue;
      RegComp.RealPage := Pg;
      UserRegComps.Add(RegComp);
      if VoteCompVisibility(RegComp) then
        inc(aVisibleCompCnt);
    end;
    Pg.Visible := (CompareText(PgName,'Hidden')<>0) and (aVisibleCompCnt>0);
  end;
  // Remove left-over pages.
  while fPages.Count > fUserOrder.ComponentPages.Count do begin
    Pg := fPages[fPages.Count-1];
    fPages.Delete(fPages.Count-1);
    Pg.Free;
  end;
end;

function TBaseComponentPalette.AssignOrigCompsForPage(PageName: string;
  DestComps: TRegisteredCompList): Boolean;
// Returns True if the page was found.
var
  rcl: TRegisteredCompList;
  i: Integer;
begin
  Result := fOrigComponentPageCache.Find(PageName, i);
  if Result then begin
    rcl := TRegisteredCompList(fOrigComponentPageCache.Objects[i]);
    DestComps.Assign(rcl);
  end
  else
    DestComps.Clear;
    //raise Exception.Create(Format('AssignOrigCompsForPage: %s not found in cache.', [PageName]));
end;

function TBaseComponentPalette.AssignOrigVisibleCompNames(PageName: string;
  DestCompNames: TStringList): Boolean;
// Returns True if the page was found.
var
  rcl: TRegisteredCompList;
  i: Integer;
begin
  DestCompNames.Clear;
  Result := fOrigComponentPageCache.Find(PageName, i);
  if not Result then Exit;
  rcl := TRegisteredCompList(fOrigComponentPageCache.Objects[i]);
  for i := 0 to rcl.Count-1 do
    if rcl[i].Visible then
      DestCompNames.Add(rcl[i].ComponentClass.ClassName);
end;

function TBaseComponentPalette.RefUserCompsForPage(PageName: string): TRegisteredCompList;
var
  i: Integer;
begin
  if fUserComponentPageCache.Find(PageName, i) then
    Result := TRegisteredCompList(fUserComponentPageCache.Objects[i])
  else
    Result := Nil;
end;

function TBaseComponentPalette.GetSelected: TRegisteredComponent;
begin
  Result := fSelected;
end;

function TBaseComponentPalette.GetMultiSelect: boolean;
begin
  Result := FSelectionMode = csmMulty;
end;

procedure TBaseComponentPalette.SetSelected(const AValue: TRegisteredComponent);
begin
  if fSelected=AValue then exit;
  fSelected:=AValue;
  if fSelected<>nil then begin
    if (fSelected.RealPage=nil) or (fSelected.RealPage.Palette<>Self)
    or (not fSelected.Visible)
    or (not fSelected.CanBeCreatedInDesigner) then
      fSelected:=nil;
  end;
  DoAfterSelectionChanged;
end;

procedure TBaseComponentPalette.SetMultiSelect(AValue: boolean);
begin
  if AValue then
    FSelectionMode := csmMulty
  else
    FSelectionMode := csmSingle;
end;

procedure TBaseComponentPalette.AddHandler(HandlerType: TComponentPaletteHandlerType;
  const AMethod: TMethod; AsLast: boolean);
begin
  if FHandlers[HandlerType]=nil then
    FHandlers[HandlerType]:=TMethodList.Create;
  FHandlers[HandlerType].Add(AMethod,AsLast);
end;

procedure TBaseComponentPalette.RemoveHandler(HandlerType: TComponentPaletteHandlerType;
  const AMethod: TMethod);
begin
  FHandlers[HandlerType].Remove(AMethod);
end;

procedure TBaseComponentPalette.DoPageAddedComponent(Component: TRegisteredComponent);
begin
  fComponentCache.Add(Component);
  DoChange;
end;

procedure TBaseComponentPalette.DoPageRemovedComponent(Component: TRegisteredComponent);
begin
  fComponentCache.Remove(Component);
  DoChange;
end;

procedure TBaseComponentPalette.SetHideControls(AValue: boolean);
begin
  if FHideControls=AValue then Exit;
  FHideControls:=AValue;
  FChanged:=True;
end;

procedure TBaseComponentPalette.IncChangeStamp;
begin
  Inc(fChangeStamp);
end;

function TBaseComponentPalette.IndexOfPageName(const APageName: string;
  ACaseSensitive: Boolean): integer;
begin
  Result:=Pages.Count-1;
  if ACaseSensitive then
  begin                          // Case sensitive search
    while (Result>=0) and (Pages[Result].PageName <> APageName) do
      dec(Result);
  end
  else begin                     // Case in-sensitive search
    while (Result>=0) and (AnsiCompareText(Pages[Result].PageName,APageName)<>0) do
      dec(Result);
  end;
end;

function TBaseComponentPalette.GetPage(const APageName: string;
  ACaseSensitive: Boolean=False): TBaseComponentPage;
var
  i: Integer;
begin
  i:=IndexOfPageName(APageName, ACaseSensitive);
  if i>=0 then
    Result:=Pages[i]
  else
    Result:=nil;
end;

procedure TBaseComponentPalette.AddRegComponent(NewComponent: TRegisteredComponent);
var
  NewPriority: TComponentPriority;
  InsertIndex, i: Integer;
  OtherComp: TRegisteredComponent;
  CurClassName: String;
begin
  // check for ambiguous classname
  CurClassName:=NewComponent.ComponentClass.ClassName;
  for i:=0 to fComps.Count-1 do
  begin
    OtherComp:=fComps[i];
    if SameText(OtherComp.ComponentClass.ClassName,CurClassName) then
    begin
      while OtherComp.NextSameName<>nil do
        OtherComp:=OtherComp.NextSameName;
      OtherComp.FNextSameName:=NewComponent;
      NewComponent.FPrevSameName:=OtherComp;
      break;
    end;
  end;

  // Store components to fComps, sorting them by priority.
  NewPriority:=NewComponent.GetPriority;
  InsertIndex:=0;
  while (InsertIndex<fComps.Count)
  and (ComparePriority(NewPriority,fComps[InsertIndex].GetPriority)<=0) do
    inc(InsertIndex);
  fComps.Insert(InsertIndex,NewComponent);

  DoPageAddedComponent(NewComponent);

  if NewComponent.FOrigPageName = '' then Exit;

  // See if page was added with different char case. Use the first version always.
  if fOrigPageHelper.Find(NewComponent.FOrigPageName, InsertIndex) then begin
    NewComponent.FOrigPageName := fOrigPageHelper[InsertIndex]; // Possibly different case
    Assert(fOrigPagePriorities.IndexOf(NewComponent.FOrigPageName) >= 0,
           'TBaseComponentPalette.AddComponent: FOrigPageName not found!');
  end
  else begin
    fOrigPageHelper.Add(NewComponent.FOrigPageName);
    Assert(fOrigPagePriorities.IndexOf(NewComponent.FOrigPageName) = -1,
           'TBaseComponentPalette.AddComponent: FOrigPageName exists but it should not!');
    // Store a list of page names and their priorities.
    InsertIndex:=0;
    while (InsertIndex<fOrigPagePriorities.Count)
    and (ComparePriority(NewPriority, fOrigPagePriorities.Data[InsertIndex])<=0) do
      inc(InsertIndex);
    fOrigPagePriorities.InsertKeyData(InsertIndex, NewComponent.FOrigPageName, NewPriority);
  end;
end;

procedure TBaseComponentPalette.RemoveRegComponent(AComponent: TRegisteredComponent);
begin
  fComps.Remove(AComponent);
  AComponent.RealPage:=nil;
  //ToDo: fix DoPageRemovedComponent(AComponent);
end;

function TBaseComponentPalette.FindRegComponent(ACompClass: TClass): TRegisteredComponent;
// Return registered component based on LCL component class type.
// Optimized with balanced tree fComponentCache.
var
  ANode: TAVLTreeNode;
begin
  ANode:=fComponentCache.FindKey(ACompClass, @CompareClassWithIDEComponent);
  if ANode<>nil then
    Result:=TRegisteredComponent(ANode.Data)
  else
    Result:=nil;
end;

function TBaseComponentPalette.FindRegComponent(const ACompClassName: string): TRegisteredComponent;
// Return registered component based on LCL component class name.
var
  i: Integer;
  HasUnitName: Boolean;
  CurClassName: String;
begin
  // A small optimization. If same type is asked many times, return it quickly.
  if ACompClassName = fLastFoundCompClassName then
    Exit(fLastFoundRegComp);
  // Linear search. Can be optimized if needed.
  HasUnitName:=Pos('/',ACompClassName)>0;
  for i := 0 to fComps.Count-1 do
  begin
    Result:=fComps[i];
    if HasUnitName then
      CurClassName:=Result.GetUnitName+'/'+Result.ComponentClass.ClassName
    else
      CurClassName:=Result.ComponentClass.ClassName;
    if SameText(CurClassName, ACompClassName) then
    begin
      if not HasUnitName then
        while Result.PrevSameName<>nil do
          Result:=Result.PrevSameName;
      fLastFoundCompClassName := ACompClassName;
      fLastFoundRegComp := Result;
      exit;
    end;
  end;
  Result:=nil;
end;

function TBaseComponentPalette.CreateNewClassName(const Prefix: string): string;
var
  i: Integer;
begin
  if FindRegComponent(Prefix)=nil then begin
    Result:=Prefix;
  end else begin
    i:=1;
    repeat
      Result:=Prefix+IntToStr(i);
      inc(i);
    until FindRegComponent(Result)=nil;
  end;
end;

procedure TBaseComponentPalette.Update(ForceUpdateAll: Boolean);
begin
  fUserOrder.SortPagesAndCompsUserOrder;
  CreatePagesFromUserOrder;
  fUserOrder.AddRecentlyInstalledComps;
  DoAfterUpdate;
end;

procedure TBaseComponentPalette.IterateRegisteredClasses(Proc: TGetComponentClassEvent);
var
  i: Integer;
begin
  for i:=0 to fComps.Count-1 do
    Proc(fComps[i].ComponentClass);
end;

procedure TBaseComponentPalette.SetSelectedComp(AComponent: TRegisteredComponent; AMulti: Boolean);
begin
  MultiSelect := AMulti;
  Selected := AComponent;
end;

// Execute handlers

function TBaseComponentPalette.VoteCompVisibility(AComponent: TRegisteredComponent): Boolean;
var
  i, Vote: Integer;
begin
  Vote:=1;
  if HideControls and AComponent.ComponentClass.InheritsFrom(TControl) then
    Dec(Vote);
  i:=FHandlers[cphtVoteVisibility].Count;
  while FHandlers[cphtVoteVisibility].NextDownIndex(i) do
    TUpdateCompVisibleEvent(FHandlers[cphtVoteVisibility][i])(AComponent,Vote);
  Result:=Vote>0;
  AComponent.Visible:=Result;
end;

procedure TBaseComponentPalette.DoAfterComponentAdded(ALookupRoot,
  AComponent: TComponent; ARegisteredComponent: TRegisteredComponent);
var
  i: Integer;
begin
  i:=FHandlers[cphtComponentAdded].Count;
  while FHandlers[cphtComponentAdded].NextDownIndex(i) do
    TComponentAddedEvent(FHandlers[cphtComponentAdded][i])(ALookupRoot, AComponent, ARegisteredComponent);
end;

procedure TBaseComponentPalette.DoAfterSelectionChanged;
var
  i: Integer;
begin
  i:=FHandlers[cphtSelectionChanged].Count;
  while FHandlers[cphtSelectionChanged].NextDownIndex(i) do
    TPaletteHandlerEvent(FHandlers[cphtSelectionChanged][i])();
end;

procedure TBaseComponentPalette.DoAfterUpdate;
var
  i: Integer;
begin
  i:=FHandlers[cphtUpdated].Count;
  while FHandlers[cphtUpdated].NextDownIndex(i) do
    TPaletteHandlerEvent(FHandlers[cphtUpdated][i])();
end;

procedure TBaseComponentPalette.RemoveAllHandlersOfObject(AnObject: TObject);
var
  HandlerType: TComponentPaletteHandlerType;
begin
  for HandlerType:=Low(HandlerType) to High(HandlerType) do
    FHandlers[HandlerType].RemoveAllMethodsOfObject(AnObject);
end;

// Add / Remove handlers

// UpdateVisible
procedure TBaseComponentPalette.AddHandlerVoteVisibility(
  const OnUpdateCompVisibleEvent: TUpdateCompVisibleEvent; AsLast: boolean);
begin
  AddHandler(cphtVoteVisibility,TMethod(OnUpdateCompVisibleEvent),AsLast);
end;

procedure TBaseComponentPalette.RemoveHandlerVoteVisibility(
  OnUpdateCompVisibleEvent: TUpdateCompVisibleEvent);
begin
  RemoveHandler(cphtVoteVisibility,TMethod(OnUpdateCompVisibleEvent));
end;

// ComponentAdded
procedure TBaseComponentPalette.AddHandlerComponentAdded(
  OnComponentAddedEvent: TComponentAddedEvent);
begin
  AddHandler(cphtComponentAdded,TMethod(OnComponentAddedEvent));
end;

procedure TBaseComponentPalette.RemoveHandlerComponentAdded(
  OnComponentAddedEvent: TComponentAddedEvent);
begin
  RemoveHandler(cphtComponentAdded,TMethod(OnComponentAddedEvent));
end;

// SelectionChanged
procedure TBaseComponentPalette.AddHandlerSelectionChanged(
  OnSelectionChangedEvent: TPaletteHandlerEvent);
begin
  AddHandler(cphtSelectionChanged,TMethod(OnSelectionChangedEvent));
end;

procedure TBaseComponentPalette.RemoveHandlerSelectionChanged(
  OnSelectionChangedEvent: TPaletteHandlerEvent);
begin
  RemoveHandler(cphtSelectionChanged,TMethod(OnSelectionChangedEvent));
end;

procedure TBaseComponentPalette.AddHandlerUpdated(
  OnUpdateEvent: TPaletteHandlerEvent);
begin
  AddHandler(cphtUpdated,TMethod(OnUpdateEvent));
end;

procedure TBaseComponentPalette.RemoveHandlerUpdated(
  OnUpdateEvent: TPaletteHandlerEvent);
begin
  RemoveHandler(cphtUpdated,TMethod(OnUpdateEvent));
end;

end.

