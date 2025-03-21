unit strpas2jscomponents;

{$mode objfpc}{$H+}

interface

Resourcestring
  rsActionListComponentEditor = 'HTM&L Element Actionlist Editor...';
  rsActionListCreateMissing = 'Create &actions for HTML tags...';
  rsErrNoHTMLFileNameForComponent = 'No HTML filename found for component %s';
  rsAllTagsHaveAction = 'All HTML tags with IDs already have a corresponding Action component.';
  rsHTMLActionsCreated = '%d HTML Element Action components were created';
  rsHTMLFragment = 'Pas2JS HTML Fragment Module';
  rsHTMLFragmentDescr = 'A Pas2JS datamodule to load and show a HTML fragment in the browser.';
  rsHTMLFIleFilters = 'HTML Files |*.html;*.htm|All files|*.*';
  rsPas2JSRest = 'Pas2JS REST';
  rsMenuRestShowData = 'Show data';
  rsMenuRestCreateFieldDefs = 'Create field defs';
  rsNoMetaDataResource = 'No metadata resource present';
  rsNoResource = 'No resource present';
  rsNoMetaDataResourceCannotCreateFieldDefs = 'No metadata resource present, cannot get fielddefs';
  rsNoResourceCannotCreateFieldDefs = 'No resource present, cannot get fielddefs';
  rsNoResourceCannotShowData = 'No resource present, cannot show data';
  rsServerRequestFailedCannotCreateFieldDefs = 'Server request failed, cannot update fielddefs';
  rsCreateFieldDefsCount = 'Added %d fielddefs';
  rsCreateFieldDefsNoNew = 'Fielddefs are up-to-date, no new fielddefs were added';

  rsEditingHTMLProp = 'Editing HTML property: %s';
  rsEditTemplate = 'Edit Template';

  rsStandardHTMLAction = 'Standard HTML Element action.';
  rsDBEditHTMLAction = 'Standard Data-Aware HTML Element action.';
  rsDBHTMLAction = 'Standard Data-Aware HTML read-only Element action.';
  rsDBButtonHTMLAction = 'Data-aware HTML button action.';

  rsActionListEditorNewAction = 'New Element Action';
  rsActionListEditorNewStdAction = 'New Standard Element Action';
  rsActionListEditorMoveDownAction = 'Move Down';
  rsActionListEditorMoveUpAction = 'Move Up';
  rsActionListEditorDeleteActionHint = 'Delete Element Action';
  rsActionListEditorDeleteAction = 'Delete';
  rsActionListEditorPanelDescrriptions = 'Show Description Panel';
  rsActionListEditorPanelToolBar = 'Show Toolbar';
  rsActionListEditor = 'HTML Element Action list editor';
  rsElementAction = 'Element action';
  rsErrorDeletingAction = 'Error when deleting element action';
  rsErrorWhileDeletingAction = 'An error occurred when deleting element action:%s%s';
  rsAddHTMLElementActions = 'Add HTML Element actions';
  rsUseDBAwareActions = 'Use Data-Aware actions';
  rsCreateServiceClient = 'Create Service client component';
  rsInvalidAPIReturned = 'The service URL "%s" returned an invalid API: %s';

implementation

end.

