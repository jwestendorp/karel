/*****************************************************************************************
	gui_kernel is a framework for small interactive window based applications.
	It creates a resizable, scollable window.
	A number of drawing functions are provided to draw lines, text, rectangles, polygons,
	and circles.
	A simple timing mechanism is supported.
	Several menus can be created, as well as dialogs.

	Date:			May 9 2000
	Modified:		April 12 2002
	Authors:		Peter Achten & Sjaak Smetsers
					Katholieke Universiteit Nijmegen
					Nijmegen Institute for Information and Computing
					Department of Software Technology
	Last Modified:	September 16 2003, by Pieter Koopman, KUN
*****************************************************************************************/

#include "gui_kernel.h"

#include <algorithm>
#include <cstdlib>

using namespace std;


bool isbetween (int x, int low, int up)
{
	return (low<=x && x<=up);
} /* isbetween */

int Maximum (int x, int y)
{
	return x > y ? x : y;
}

int setbetween (int x, int low, int up)
{
	if (x < low)
		return low;
	else if (x > up)
		return up;
	else
		return x;
} /* setbetween */

bool GPOINT :: valid ()
{
	return (isbetween (x,MINCOORD,MAXCOORD) && isbetween (y,MINCOORD,MAXCOORD));
}

bool GPOINT :: operator== (const GPOINT& p)
{
	return x == p.x && y == p.y;
}

bool GSIZE :: valid ()
{
	return (cx>0 && cy>0);
}

bool GSIZE :: operator== (const GSIZE& s)
{
	return cx == s.cx && cy == s.cy;
}

bool RGBCOLOUR :: valid ()
{	return (
		isbetween (r,MINRGB,MAXRGB) &&
		isbetween (g,MINRGB,MAXRGB) &&
		isbetween (b,MINRGB,MAXRGB));
}

bool RGBCOLOUR :: operator== (const RGBCOLOUR& col)
{
	return r == col.r && g == col.g && b == col.b;
}

//	pointsToRect converts two corner points to a rectangle.
void pointsToRect (GPOINT orig, GPOINT corner1, GPOINT corner2, RECT *rect)
{
	if (corner1.x < corner2.x)
	{
		rect->left  = corner1.x + orig.x;
		rect->right = corner2.x + orig.x;
	}
	else
	{
		rect->left  = corner2.x + orig.x;
		rect->right = corner1.x + orig.x;
	}
	if (corner1.y < corner2.y)
	{
		rect->top    = corner1.y + orig.y;
		rect->bottom = corner2.y + orig.y;
	}
	else
	{
		rect->top    = corner2.y + orig.y;
		rect->bottom = corner1.y + orig.y;
	}
} /* pointsToRect */

//	boxToRect converts two corner points to a rectangle.
void boxToRect (GPOINT orig, GPOINT left_corner, GSIZE size, RECT& rect)
{
	rect.left	= left_corner.x + orig.x;
	rect.right 	= rect.left + size.cx;
	rect.top	= left_corner.y + orig.y;
	rect.bottom	= rect.top + size.cy;

} /* boxToRect */


/*	Set and get the GWL_USERDATA of a windowhandle.
*/
void SetGWL_USERDATA (LONG data, HWND hwnd)
{
	SetWindowLong (hwnd, GWL_USERDATA, data);
}	/* SetGWL_USERDATA */

LONG GetGWL_USERDATA (HWND hwnd)
{
	return GetWindowLong (hwnd, GWL_USERDATA);
}	/* GetGWL_USERDATA */




/*****************************************************************************************
	Global data:
*****************************************************************************************/

static int dwScrollUnit = 50;		//	dwScrollUnit defines the amount of stepwise scrolling (horizontally and vertically)
static WINARGS theWINARGS;			//	the global WINARGS (needed to create Dialogs).
static HWND activeDialog = NULL;	//	NULL iff no active dialog; handle of dialog otherwise.
static LONG stdEditCallback = 0;	// The standard internal Windows callback routine of edit controls.

//	Drawing stuff:
const RGBCOLOUR BlackRGB	= RGBCOLOUR (RGBCOLOUR :: MINRGB, RGBCOLOUR :: MINRGB, RGBCOLOUR :: MINRGB);
const RGBCOLOUR WhiteRGB	= RGBCOLOUR (RGBCOLOUR :: MAXRGB, RGBCOLOUR :: MAXRGB, RGBCOLOUR :: MAXRGB);
const RGBCOLOUR RedRGB		= RGBCOLOUR (RGBCOLOUR :: MAXRGB, RGBCOLOUR :: MINRGB, RGBCOLOUR :: MINRGB);
const RGBCOLOUR GreenRGB	= RGBCOLOUR (RGBCOLOUR :: MINRGB, RGBCOLOUR :: MAXRGB, RGBCOLOUR :: MINRGB);
const RGBCOLOUR BlueRGB		= RGBCOLOUR (RGBCOLOUR :: MINRGB, RGBCOLOUR :: MINRGB, RGBCOLOUR :: MAXRGB);

static GPOINT canvas_penpos = GPOINT (0,0);	// The pen-position


/*****************************************************************************************
	Menu stuff:
*****************************************************************************************/
struct MenuItem
{
	MenuCallBack	mi_action;		//	the callback procedure
	MenuItems 	 	mi_next;		//	the next callback procedure
	char			*mi_title;

					MenuItem (char *title, MenuCallBack action);
} ;

HMENU	menuBar;					//	the handle to the top-level menu in which pull-down menus are created
Menus	Menu :: menu_bar	= NULL;


/*****************************************************************************************
	Window class stuff:
*****************************************************************************************/
static char      szDrawingWindowClass[] = "DrawingWindow";			// The window class name of the drawing window.

LRESULT CALLBACK DrawingWindowProc (HWND, UINT, WPARAM, LPARAM);	// The callback routine of the drawing window.
BOOL    CALLBACK DialogProc        (HWND, UINT, WPARAM, LPARAM);	// The callback routine of the dialogs.


/*****************************************************************************************
	Canvas stuff:
*****************************************************************************************/

bool Canvas :: canvas_created = false;

Canvas :: Canvas (GUI &gui)
	: canvas_gui (gui)
{
	if (canvas_created)
		throw GUITerminated (GUITerminated :: GFK_MultipleCanvas);
	canvas_created = true;
	drawingContext = GetDC (gui.the_window);
//	drawingContext = GetDC (gui.the_window);
	setPenPos (canvas_penpos);		// penpos must be set
}

Canvas :: ~Canvas (void)
{
//	ReleaseDC (canvas_gui.the_window,drawingContext);
	ReleaseDC (canvas_gui.the_window,drawingContext);
	canvas_created = false;
}


/*****************************************************************************************
	GUI stuff:
*****************************************************************************************/

bool GUI :: gui_created = false;

GUI :: GUI (GSIZE windowSize, char *title)
	: canvas_origin (GPOINT(0,0)), pen_colour (BlackRGB), in_normal_mode (true), windowSize (windowSize), windowTitle (title), gui_nr_of_menus (0), gui_timer (), mouse_is_down (false), quitRequested (false)
{
	if (gui_created)
		throw GUITerminated (GUITerminated :: GFK_MultipleGUIs);
	else
		gui_created = true;

}

bool GUI :: drawing_library_initialised = false;

void GUI :: InitialiseDrawingLibrary (WINARGS winArgs)
{
	if (drawing_library_initialised)
		throw GUITerminated (GUITerminated :: GFK_Active);

	drawing_library_initialised = true;
	theWINARGS = winArgs;

	WNDCLASSEX  wndclass;

	// Register the window class:
	wndclass.cbSize        = sizeof (wndclass);
	wndclass.style         = CS_OWNDC;
	wndclass.lpfnWndProc   = DrawingWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof (HANDLE);	// Allocate local memory for reference to GUI instance
	wndclass.hInstance     = winArgs.hInstance;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szDrawingWindowClass;
	wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);

	RegisterClassEx (&wndclass);

	//	Create the menubar.
	menuBar = CreateMenu ();

	//	Create the window:
	DWORD wndstyle = WS_OVERLAPPED	|
					 WS_CAPTION		|
					 WS_SYSMENU		|
					 WS_MINIMIZEBOX	|
					 WS_MAXIMIZEBOX	|
					 WS_BORDER		|
					 WS_THICKFRAME	|
					 WS_HSCROLL		|
					 WS_VSCROLL;
	int wwidth     = Maximum (windowSize.cx,0);
	int wheight    = Maximum (windowSize.cy,0);
	wwidth  = wwidth  + 2 * GetSystemMetrics (SM_CXSIZEFRAME)
		              +     GetSystemMetrics (SM_CXVSCROLL);
	wheight = wheight + 2 * GetSystemMetrics (SM_CYSIZEFRAME)
		              +     GetSystemMetrics (SM_CYHSCROLL)
					  +     GetSystemMetrics (SM_CYCAPTION)
					  +     GetSystemMetrics (SM_CYMENU);

	the_window = CreateWindow (
					szDrawingWindowClass,		// window class name
					windowTitle,				// window caption
					wndstyle,					// window style
					CW_USEDEFAULT,				// initial x position
					CW_USEDEFAULT,				// initial y position
					wwidth,						// initial x size
					wheight,					// initial y size
					NULL,						// parent window handle
					menuBar,					// window menu handle
					winArgs.hInstance,			// program instance handle
					NULL						// creation parameters
					);
	SetGWL_USERDATA ((LONG)this, the_window);	// store reference to GUI object in the window

	//	Initialise the drawing context of the window:
	blackPen   = GetStockObject (BLACK_PEN);	// Initialise the pens
	whitePen   = GetStockObject (WHITE_PEN);
	nullPen    = GetStockObject (NULL_PEN);
	blackBrush = GetStockObject (BLACK_BRUSH);	// Initialise the brushes
	whiteBrush = GetStockObject (WHITE_BRUSH);
	nullBrush  = GetStockObject (NULL_BRUSH);
	penColour  = RGB (0,0,0);					// Set colour to black

	HDC dc = GetDC (the_window);		// Initialise the drawing context of the window to filling
	SelectObject (dc, blackPen);
	SelectObject (dc, blackBrush);
	SetTextAlign (dc, TA_UPDATECP);	// Drawing text with TextOut now uses current pen position and updates it
	SetBkMode (dc, TRANSPARENT);	// Drawing text will not erase background
	ReleaseDC (the_window,dc);

	//	Set the range of both scrollbars
	SetScrollRange (the_window,SB_HORZ,MINCOORD,MAXCOORD-windowSize.cx,true);
	SetScrollRange (the_window,SB_VERT,MINCOORD,MAXCOORD-windowSize.cy,true);

	ShowWindow (the_window, winArgs.iCmdShow);

	the_dialogs = NULL;
}


/******************************************************************************************
	Dialog stuff
*******************************************************************************************/

//	DialogArgs are passed as extra parameter to CreateDialogParam in order to allow dialogs to obtain the GUI class object.
struct DialogArgs
{
	GUI*	ref_to_dialog_gui;		// The reference to the GUI class object

	DialogArgs ()					// Initialise class object to NULL if no arguments
		: ref_to_dialog_gui (NULL) {}
	DialogArgs (GUI *gui)			// Initialise class object
		: ref_to_dialog_gui (gui) {}
} ;

/*********************************************************************************************
	FixEditControlProcedure.
	This routine subclasses standard windows edit controls. It filters WM_CHAR events in order
	to prevent duplicated character output.
*********************************************************************************************/
static LRESULT CALLBACK FixEditControlProcedure (HWND hwnd,UINT uMess,WPARAM wParam,LPARAM lParam)
{
	static bool is_first_WM_CHAR = true;
	TCHAR currentChar = 0;

	switch (uMess)
	{
		case WM_CHAR:
			{
				if (is_first_WM_CHAR || (lParam & 0x40000000) || currentChar != (TCHAR) wParam)
				{
					is_first_WM_CHAR = false;
					currentChar = (TCHAR) wParam;
					return CallWindowProc ((WNDPROC) stdEditCallback, hwnd, uMess, wParam, lParam);
				}
				else
				{
					currentChar = (TCHAR) wParam;
					return 0;
				}
			}
		case WM_KEYUP:
			{
				is_first_WM_CHAR = true;
				currentChar = 0;
				return CallWindowProc ((WNDPROC) stdEditCallback, hwnd, uMess, wParam, lParam);
			}
	}
	return CallWindowProc ((WNDPROC) stdEditCallback, hwnd, uMess, wParam, lParam);
}	/* FixEditControlProcedure */

enum DialogItemType
{
	kButtonDialogItem,
	kCheckBoxDialogItem,
	kRadioButtonDialogItem,
	kEditTextDialogItem,
	kStaticTextDialogItem,
	kUnknownDialogItem
};

struct ControlRepr
{
	ControlId			control_id;
	DialogItemType		control_type;
	ControlRepr *		control_next;

	union
	{
		ButtonCallBack		cbf_button;
		CheckCallBack		cbf_check;
		RadioButtonCallBack	cbf_radio;
	} control_callback;

	ControlRepr (ControlId, DialogItemType);

	void clearCallBack ();

} ;

ControlRepr :: ControlRepr (ControlId id, DialogItemType type)
	: control_id (id), control_type (type), control_next (NULL)
{
	clearCallBack ();
}

void ControlRepr :: clearCallBack ()
{
	switch (control_type)
	{
		case kButtonDialogItem:
			control_callback.cbf_button = NULL;
			return;
		case kCheckBoxDialogItem:
			control_callback.cbf_check = NULL;
			return;
		case kRadioButtonDialogItem:
			control_callback.cbf_radio = NULL;
			return;
		case kEditTextDialogItem:
		case kStaticTextDialogItem:
			return;
		default:
			throw DialogFailure (DialogFailure :: DFK_UnknownItemType);
	}
}

//	MAC compatibility types...
typedef HWND ControlRef;
typedef HWND Handle;
typedef HWND DialogPtr;
typedef HWND DialogItemPtr;
//	...up to here.

struct DialogRepr
{
	DialogId		diar_id;
	DialogPtr		diar_ptr;
	int				diar_nr_of_items;
	ControlRepr	*	diar_controls;

	DialogHandle	diar_next;

					DialogRepr (DialogId did, DialogPtr dptr, int nr_of_items, DialogHandle next)
						: diar_id (did), diar_ptr (dptr), diar_nr_of_items (nr_of_items),
						  diar_controls (NULL), diar_next (next)
					{}

	ControlRepr* 	searchControl	(ControlId);
	ControlRepr&	getControl		(ControlId, DialogItemType);
	ControlRef		getControlRef	(ControlId, DialogItemType);

} ;

struct GetDialogItemInfo				// Struct passed through EnumGetDialogItem
{
	ControlId		gdi_id;				// the id of the control which type is searched for
	DialogItemType	gdi_type;			// the type of the control
	DialogItemPtr	gdi_hwnd;			// the handle to the control
	int				gdi_nr_of_items;	// the number of controls

					GetDialogItemInfo (ControlId id,DialogItemType type)	// use to search for control
						: gdi_id (id), gdi_type (type), gdi_hwnd (NULL), gdi_nr_of_items (0)
					{}
					GetDialogItemInfo ()									// use to search all controls
						: gdi_id (0), gdi_type (kUnknownDialogItem), gdi_hwnd (NULL), gdi_nr_of_items (0)
					{}
} ;

struct CreateControlRefInfo				// Struct passed through EnumCreateControlRef
{
	ControlRepr **last_control_ptr;		// the ptr to the last control element in the list
	bool          no_active_radio_button;
										// keep track of the first radio button control in the list and select it if found

	              CreateControlRefInfo (ControlRepr ** ptr)
					  : last_control_ptr (ptr), no_active_radio_button (true)
				  {}
} ;

ControlRepr* DialogRepr :: searchControl (ControlId id)
{
	for (ControlRepr *next_control = diar_controls; next_control != NULL; next_control = next_control -> control_next)
		if (next_control -> control_id == id)
			return next_control;
	return NULL;
}

ControlRepr& DialogRepr :: getControl (ControlId id, DialogItemType type)
{
	ControlRepr *con_ptr = searchControl (id);

	if (con_ptr != NULL)
	{	if (con_ptr -> control_type == type)
			return *con_ptr;
		else
			throw DialogFailure (DialogFailure :: DFK_WrongItemType);
	}
	else
		throw DialogFailure (DialogFailure :: DFK_IllegaltemID);
}

void makeAlert (const char alert_text [])
{
	MessageBox( NULL, alert_text, NULL, MB_APPLMODAL );
}

DialogItemType GetDialogItemType (HWND hwndControl)
{
	char classname[] = "123456";
	LONG hwndStyle = GetWindowLong (hwndControl, GWL_STYLE);

	GetClassName (hwndControl, classname, 7);

	if (strncmp (classname,"Button",6) == 0)
	{
		switch (hwndStyle & 0xF)
		{
			case BS_DEFPUSHBUTTON:
				return kButtonDialogItem;
			case BS_PUSHBUTTON:				// it's a button
				return kButtonDialogItem;	// PA: for some mysterious reason this test is not correct!
			case BS_AUTOCHECKBOX:			// it's a check box
				return kCheckBoxDialogItem;
			case BS_AUTORADIOBUTTON:		// it's a radio button
				return kRadioButtonDialogItem;
			default:
				return kButtonDialogItem;	// PA: actually incorrect, but otherwise can't find ordinary buttons
		}
	}
/*		if (hwndStyle & BS_DEFPUSHBUTTON)								// it's the default button
			return kButtonDialogItem;
		else if (hwndStyle & BS_PUSHBUTTON)								// it's a button
			return kButtonDialogItem;	// PA: for some mysterious reason this test is not correct!
		else if (hwndStyle & BS_AUTOCHECKBOX)							// it's a check box
			return kCheckBoxDialogItem;
		else if (hwndStyle & BS_AUTORADIOBUTTON)						// it's a radio button
			return kRadioButtonDialogItem;
		else
		//	return kUnknownDialogItem;
			return kButtonDialogItem;	// PA: actually incorrect, but otherwise can't find ordinary buttons

*/
	else if (strncmp (classname,"Edit",4) == 0)							// it's an edit text
		return kEditTextDialogItem;
	else if (strncmp (classname,"Static", 6) == 0)							// it's a static text
		return kStaticTextDialogItem;
	else
		return kUnknownDialogItem;
}	/* GetDialogItemType */

BOOL CALLBACK EnumGetDialogItem (HWND hwndControl, LPARAM lParam)
{
	int hwndId = (int)GetWindowLong (hwndControl, GWL_ID);
	GetDialogItemInfo *info = (GetDialogItemInfo *)lParam;

	info->gdi_nr_of_items += 1;	// increment nr of controls

	if (hwndId == info->gdi_id)	// item found
	{
		info->gdi_type = GetDialogItemType (hwndControl);
		info->gdi_hwnd = hwndControl;
		return FALSE;			// stop looking
	}
	else
		return TRUE;			// continue looking
}	/* EnumGetDialogItem */

void GetDialogItem (DialogPtr the_dialog, ControlId item, DialogItemType *item_type, HWND& handle)
{
	GetDialogItemInfo info (item,kUnknownDialogItem);
	EnumChildWindows (the_dialog, (WNDENUMPROC) EnumGetDialogItem, (LPARAM) &info);
	*item_type = info.gdi_type;
	handle     = info.gdi_hwnd;
}	/* GetDialogItem */

DialogItemType GetDialogItemAndType (DialogPtr the_dialog, ControlId item, HWND& handle)
{
	DialogItemType	item_type;

	GetDialogItem (the_dialog, item, & item_type, handle);
	return item_type;
}	/* GetDialogItemAndType */

//	CountDITL counts the number of dialog items in the dialog.
int CountDITL (DialogPtr the_dialog)
{
	GetDialogItemInfo *info = new GetDialogItemInfo ();
	if (EnumChildWindows (the_dialog, (WNDENUMPROC) EnumGetDialogItem, (LPARAM) info))
		return info->gdi_nr_of_items;
	else
		return 0;
}	/* CountDITL */

BOOL CALLBACK EnumCreateControlRef (HWND hwndControl, LPARAM lParam)
{
	CreateControlRefInfo *info = (CreateControlRefInfo *)lParam;

	DialogItemType item_type = GetDialogItemType (hwndControl);
	ControlId      item_id   = (ControlId) GetWindowLong (hwndControl, GWL_ID);

	ControlRepr * new_repr = new ControlRepr (item_id, item_type);
	*(info->last_control_ptr) = new_repr;
	info->last_control_ptr = & new_repr -> control_next;

	if (info->no_active_radio_button && item_type == kRadioButtonDialogItem)
	{
		SendMessage (hwndControl, BM_SETCHECK, (WPARAM) 1, 0);
		info->no_active_radio_button = false;
	}
	return TRUE;		// continue looking
}	/* EnumCreateControlRef */

//	CreateControlRefs creates the ControlRepr list of a dialog.
void CreateControlRefs (DialogPtr the_dialog, ControlRepr **last_control_ptr)
{
	CreateControlRefInfo *info = new CreateControlRefInfo (last_control_ptr);

	EnumChildWindows (the_dialog, (WNDENUMPROC) EnumCreateControlRef, (LPARAM) info);
}	/* CreateControlRefs */

ControlRef DialogRepr :: getControlRef (ControlId control_id, DialogItemType item_type)
{
	getControl (control_id, item_type);
	Handle the_control;
	GetDialogItemAndType (diar_ptr, control_id, the_control);
	return (ControlRef) the_control;
} /* getControlRef */

/*	PA: EnumFixEditControl repairs standard windows edit control behaviour to display to many keyboard input event.
*/
BOOL CALLBACK EnumFixEditControl (HWND hwndControl, LPARAM lParam)
{
	if (GetDialogItemType (hwndControl) == kEditTextDialogItem)
		stdEditCallback = SetWindowLong (hwndControl, GWL_WNDPROC, (LONG) FixEditControlProcedure);
	return TRUE;		// continue repairing
}	/* EnumFixEditControl */

void FixEditControls (DialogPtr the_dialog)
{
	EnumChildWindows (the_dialog, (WNDENUMPROC) EnumFixEditControl, (LPARAM)NULL);
}	/*	FixEditControls */


// The Dialog constructors

// First the public one:
Dialog :: Dialog (DialogId dialog_id, GUI& gui)
	: dialog_gui (gui)
{
	DialogArgs dialogArgs (&dialog_gui);

	//	We first look whether the dialog has already been administered.
	for (DialogHandle next_dh = gui.the_dialogs; next_dh != NULL; next_dh = next_dh -> diar_next)
	{
		if (next_dh -> diar_id == dialog_id)
		{	if (next_dh -> diar_ptr == NULL)
			{
				next_dh -> diar_ptr = CreateDialogParam ( theWINARGS.hInstance,
					                                      MAKEINTRESOURCE (dialog_id),
														  gui.the_window,
														  (DLGPROC) DialogProc,
														  (LPARAM)&dialogArgs
														);

				if (next_dh -> diar_ptr == NULL)
					throw DialogFailure (DialogFailure :: DFK_MissingResource);
				FixEditControls (next_dh -> diar_ptr);

				// Show the dialog
				ShowWindow (next_dh -> diar_ptr, SW_SHOW);

				// Remove previously defined callbacks;
				bool no_active_radio_button = true;

				for (ControlRepr *next_control = next_dh -> diar_controls; next_control != NULL; next_control = next_control -> control_next)
				{
					next_control -> clearCallBack ();
					if (no_active_radio_button && next_control -> control_type == kRadioButtonDialogItem)
					{
						HWND	radio_control;
						GetDialogItemAndType (next_dh -> diar_ptr, next_control -> control_id, radio_control);
						SendMessage (radio_control, BM_SETCHECK, (WPARAM) 1, 0);
						no_active_radio_button = false;
					}
				}
				dialog_handle = next_dh;
			}
			else
				SetActiveWindow (next_dh -> diar_ptr);
			return;
		}
	}

	DialogPtr new_dialog = CreateDialogParam ( theWINARGS.hInstance,
		                                       MAKEINTRESOURCE (dialog_id),
											   gui.the_window,
											   (DLGPROC) DialogProc,
											   (LPARAM)&dialogArgs
											 );

	if (new_dialog != NULL)
	{
		// So we have a new one; display it first
		ShowWindow (new_dialog, SW_SHOW);

		// Get the number of controls
		const int nr_of_items = CountDITL (new_dialog);

		dialog_handle = gui.the_dialogs = new DialogRepr (dialog_id, new_dialog, nr_of_items, gui.the_dialogs);

		ControlRepr **last_control_ptr = & dialog_handle -> diar_controls;

		CreateControlRefs (new_dialog, last_control_ptr);
		FixEditControls (new_dialog);
	}
	else
		throw DialogFailure (DialogFailure :: DFK_MissingResource);
}

// And now the private one:
Dialog :: Dialog (DialogHandle handle, GUI& gui)
	: dialog_handle (handle), dialog_gui (gui)
{}


/*	The methods for adding the controlhandlers
*/
Dialog&	Dialog :: addButtonCallBack	(ControlId control_id, ButtonCallBack call_back)
{
	ControlRepr &control = dialog_handle -> getControl (control_id, kButtonDialogItem);
	control.control_callback.cbf_button = call_back;
	return *this;
}


Dialog&	Dialog :: addCheckCallBack	(ControlId control_id, CheckCallBack call_back)
{
	ControlRepr &control = dialog_handle -> getControl (control_id, kCheckBoxDialogItem);
	control.control_callback.cbf_check = call_back;
	return *this;
}

Dialog&	Dialog :: addRadioButtonCallBack (ControlId control_id, RadioButtonCallBack call_back)
{
	ControlRepr &control = dialog_handle -> getControl (control_id, kRadioButtonDialogItem);
	control.control_callback.cbf_radio = call_back;
	return *this;

}

/*	The methods for retrieving control values
*/
bool Dialog :: getRadioValue (ControlId control_id)
{
	ControlRef the_control = dialog_handle -> getControlRef (control_id, kRadioButtonDialogItem);
	LRESULT cv = SendMessage (the_control, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);

	return cv == BST_CHECKED;
}

bool Dialog :: getCheckValue (ControlId control_id)
{
	ControlRef the_control = dialog_handle -> getControlRef (control_id, kCheckBoxDialogItem);
	LRESULT cv = SendMessage (the_control, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);

	return cv == BST_CHECKED;
}

void Dialog :: getEditValue (ControlId control_id, char *result_text, int maxNrChars)
{
	ControlRef the_control = dialog_handle -> getControlRef (control_id, kEditTextDialogItem);

	// fetch the string
	char *text = (char *)malloc (maxNrChars * sizeof (char));
	int nrChars = GetWindowText (the_control, text, maxNrChars);

	// convert it to a C-string
	int i;
	for (i = 0; i < nrChars; i++)
		result_text [i] = text [i];
	result_text [i] = '\0';
	free (text);
}

/*	The methods for setting control values
*/
// An auxiliary function to deselect all radio buttons except the active one
void resetRadioButtons (ControlId active_item, ControlRepr *controls, DialogPtr dialog_ptr)
{
	for (ControlRepr *next_control = controls; next_control != NULL; next_control = next_control -> control_next)
	{	if (next_control -> control_type == kRadioButtonDialogItem && next_control -> control_id != active_item)
		{	Handle radio_control;
			GetDialogItemAndType (dialog_ptr, next_control -> control_id, radio_control);
			SendMessage (radio_control, BM_SETCHECK, (WPARAM) BST_UNCHECKED, (LPARAM) 0);
		}
	}
}

void Dialog :: setRadioValue (ControlId control_id)
{
	ControlRef radio_control = dialog_handle -> getControlRef (control_id, kRadioButtonDialogItem);

	SendMessage (radio_control, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
	resetRadioButtons (control_id, dialog_handle -> diar_controls, dialog_handle -> diar_ptr);
}

void Dialog :: setCheckValue (ControlId control_id, bool checked)
{
	ControlRef check_control = dialog_handle -> getControlRef (control_id, kCheckBoxDialogItem);
	SendMessage (check_control, BM_SETCHECK, (WPARAM) checked ? BST_CHECKED : BST_UNCHECKED, (LPARAM) 0);
}

void Dialog :: setEditValue (ControlId control_id, const char *text)
{
	ControlRef text_control = dialog_handle -> getControlRef (control_id, kEditTextDialogItem);
	SetWindowText (text_control, text);
}

void Dialog :: setStaticValue (ControlId control_id, const char *text)
{
	ControlRef text_control = dialog_handle -> getControlRef (control_id, kStaticTextDialogItem);
	SetWindowText (text_control, text);
}

/*	The method for closing a dialog
*/
void Dialog :: closeDialog	()
{
	DestroyWindow (dialog_handle -> diar_ptr);
	dialog_handle -> diar_ptr = NULL;
}


// An auxiliary function for finding a dialog, given a pointer to the (internal) dialog record
DialogHandle searchForDialog (DialogPtr dialog, DialogHandle dialogs)
{
	for (;  dialogs != NULL; dialogs = dialogs -> diar_next)
		if (dialogs -> diar_ptr == dialog)
			return dialogs;
	return NULL;

} /* searchForDialog */


/*	RunGUI implements the event loop of the interactive process.
*/
void GUI :: RunGUI (void)
{
	MSG msg;
	while (!quitRequested && GetMessage (&msg, NULL, 0, 0))
	{
		if (activeDialog == NULL || !IsDialogMessage (activeDialog, &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage  (&msg);
		}
	}
};

//	Run without menus
void GUI :: Run (WINARGS winArgs)
{
	InitialiseDrawingLibrary (winArgs);
	RunGUI ();
}

//	Run with menus
void GUI :: Run (Menu &menu, WINARGS winArgs)
{
	InitialiseDrawingLibrary (winArgs);

	gui_nr_of_menus = 0;
	int gui_nr_of_items = 0;

	Menus next_menu;
	// First count the number of menu items so that we can distribute correct numbers.
	for (next_menu = menu.menu_bar; next_menu != NULL; next_menu = next_menu -> menu_next)
	{
		for (MenuItems next_item = next_menu -> menu_call_backs; next_item != NULL; next_item = next_item -> mi_next)
		{
			gui_nr_of_items++;
		}
	}
	// Now create menu(items) with correct numbers [gui_nr_of_items..1]:
	for (next_menu = menu.menu_bar; next_menu != NULL; next_menu = next_menu -> menu_next)
	{
		HMENU menu_handle = CreatePopupMenu ();

		// Count nr of items
		int nrItems = 0;
		MenuItems next_item;
		for (next_item = next_menu -> menu_call_backs; next_item != NULL; next_item = next_item -> mi_next)
		{
			nrItems++;
		}
		gui_nr_of_items -= nrItems;
		int itemnr = gui_nr_of_items;

		for (next_item = next_menu -> menu_call_backs; next_item != NULL; next_item = next_item -> mi_next)
		{
			if (strcmp (next_item -> mi_title,"-(") == 0)
				InsertMenu (menu_handle,0xFFFFFFFF,MF_BYPOSITION | MF_SEPARATOR,(UINT)itemnr,0);
			else
				InsertMenu (menu_handle,0xFFFFFFFF,MF_BYPOSITION | MF_STRING,   (UINT)itemnr,next_item -> mi_title);
			itemnr++;
		}

		InsertMenu (menuBar, 0, MF_BYPOSITION | MF_POPUP, (UINT)menu_handle,next_menu->menu_title);
		gui_nr_of_menus++;
	}

	gui_menus = new MenuItems [gui_nr_of_menus];

	int this_menu = gui_nr_of_menus - 1;

	for (next_menu = menu.menu_bar; next_menu != NULL; next_menu = next_menu -> menu_next)
	{
		gui_menus [this_menu] = next_menu -> menu_call_backs;
		this_menu--;
	}

	DrawMenuBar (the_window);
	RunGUI ();
}


/*****************************************************************************************
	Timing operations:
*****************************************************************************************/

VOID CALLBACK GUI :: EvaluateTimer (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	GUI *gui = (GUI*)GetGWL_USERDATA (hwnd);	// Retrieve reference to GUI object

	gui->Timer (gui->gui_timer.timer_period);	// Simply call the timer callback,
	if (gui->gui_timer.timerID != 0)			// and restart the timer if not stopped by callback
	{
		KillTimer (NULL, gui->gui_timer.timerID);
		gui->gui_timer.timerID = SetTimer (hwnd, (UINT)1, gui->gui_timer.timer_period, EvaluateTimer);
	}
}

void GUI :: startTimer (int dt)
{
	gui_timer.timer_period		= (dt < 0)  ? 0 : (UINT)dt;

	if (gui_timer.timerID != 0)			// Timer is already open, close it first
	{
		KillTimer (NULL, gui_timer.timerID);
	}
	gui_timer.timerID = SetTimer (the_window, (UINT)1, gui_timer.timer_period, EvaluateTimer);
}

void GUI :: stopTimer ()
{
	if (gui_timer.timerID != 0)
	{
		KillTimer (the_window,gui_timer.timerID);
		gui_timer.timerID = 0;
	}
}


/*****************************************************************************************
	General operations:
*****************************************************************************************/

void GUI :: Activate ()
{
	SetActiveWindow (the_window);
}

void GUI :: Beep (int duration)
{
	MessageBeep (MB_ICONASTERISK);
}


/*****************************************************************************************
	Termination operations:
*****************************************************************************************/

void GUI :: Stop ()
{
//	throw GUITerminated (0);
	quitRequested = true;
}


/*****************************************************************************************
	The window operations:
*****************************************************************************************/

GSIZE GUI :: getWindowSize ()
{
	RECT clientRect;

	GetClientRect (the_window,&clientRect);
	return GSIZE (clientRect.right,clientRect.bottom);
}



/*****************************************************************************************
	The menu operations:
*****************************************************************************************/

MenuItem :: MenuItem (char *title, MenuCallBack action)
	: mi_action (action), mi_next (NULL)
{
	mi_title = title;
}

Menu :: Menu (char *title)
	: menu_call_backs (NULL)
{
	menu_title		= title;
	menu_next 		= menu_bar;
	menu_bar		= this;

}

MenuItems& lastItem (MenuItems& first)
{
	MenuItems *last_item;

	for (last_item = & first; *last_item != NULL;
		 last_item = & (*last_item) -> mi_next)
		 ;
	return *last_item;

}

Menu& Menu :: add (char *item_title, MenuCallBack action)
{
	lastItem (menu_call_backs) =  new MenuItem (item_title, action);
	return *this;

}

Menu& Menu :: add ()
{
	lastItem (menu_call_backs) =  new MenuItem ((char*)"-(", NULL);
	return *this;

}


/*****************************************************************************************
	Canvas operations:
*****************************************************************************************/

void Canvas :: toggleXORmode (void)
{
	if (canvas_gui.in_normal_mode)
	{
		SetROP2 (drawingContext, R2_NOT);
//		SetROP2 (canvas_gui.drawingContext, R2_NOT);
		canvas_gui.in_normal_mode = false;
	}
	else
	{
		SetROP2 (drawingContext, R2_COPYPEN);
		canvas_gui.in_normal_mode = true;
	}
}	/* toggleXORMode */

void Canvas :: setPenColour (RGBCOLOUR newColour)
{
	if (newColour.valid () && ! (canvas_gui.pen_colour == newColour))
	{
		LOGBRUSH logicalBrush;

		canvas_gui.pen_colour = newColour;

		canvas_gui.penColour = RGB (newColour.r,newColour.g,newColour.b);
		//	Text will be drawn with the new colour
		SetTextColor (drawingContext, canvas_gui.penColour);
		//	Line images will be outlined with the new colour
		logicalBrush.lbStyle = BS_SOLID;
		logicalBrush.lbColor = canvas_gui.penColour;
		logicalBrush.lbHatch = 0;
//		canvas_gui.thePen  = ExtCreatePen (PS_GEOMETRIC | PS_INSIDEFRAME, 1, &logicalBrush, 0, NULL);
//		canvas_gui.thePen  = ExtCreatePen (PS_GEOMETRIC | PS_SOLID, 1, &logicalBrush, 0, NULL);
		canvas_gui.thePen  = ExtCreatePen (PS_COSMETIC | PS_SOLID, 1, &logicalBrush, 0, NULL);
		DeleteObject (SelectObject (drawingContext,canvas_gui.thePen));
		canvas_gui.theBrush = CreateSolidBrush (canvas_gui.penColour);
		//	Line images will be filled with the new colour
		DeleteObject (SelectObject (drawingContext,canvas_gui.theBrush));
	}
}	/* setPenColour */

RGBCOLOUR Canvas :: getPenColour ()
{
	return canvas_gui.pen_colour;
}	/* getPenColour */

void Canvas :: setPenPos (GPOINT newPos)
{
	if (newPos.valid ())
	{
		canvas_penpos = newPos;
		GPOINT origin = getOrigin ();
		MoveToEx (drawingContext,newPos.x - origin.x, newPos.y - origin.y, NULL);
	}
}	/* setPenPos */

GPOINT Canvas :: getPenPos ()
{
	return canvas_penpos;
}	/* getPenPos */

void Canvas :: drawPOINT ()
{
	GPOINT newpoint (canvas_penpos.x + 1, canvas_penpos.y);
	GPOINT origin = getOrigin ();

	SetPixelV (drawingContext, canvas_penpos.x-origin.x, canvas_penpos.y-origin.y, canvas_gui.penColour);
	if (newpoint.valid ())
		MoveToEx (drawingContext, newpoint.x-origin.x, newpoint.y-origin.y, NULL);
	else
		MoveToEx (drawingContext, canvas_penpos.x-origin.x, canvas_penpos.y-origin.y, NULL);
}	/* drawPOINT */

void Canvas :: drawLineTo (GPOINT end)
{
	if (end.valid ())
	{
		BOOL ok;
		GPOINT origin = getOrigin ();

		setPenPos (canvas_penpos);
		ok = LineTo (drawingContext, end.x-origin.x, end.y-origin.y);
		SetPixelV (drawingContext, end.x-origin.x, end.y-origin.y, canvas_gui.penColour);
		if (!ok)
			MoveToEx (drawingContext, end.x-origin.x, end.y-origin.y, NULL);
		setPenPos (end);
	}
} /* drawLineTo */

void Canvas :: drawRectangle (GPOINT corner1,  GPOINT corner2)
{
	if (corner1.valid () && corner2.valid ())
	{
		HGDIOBJ prevBrush;
		RECT rect;
		GPOINT origin = getOrigin ();

		pointsToRect (origin, corner1, corner2, &rect);

		prevBrush = SelectObject (drawingContext, canvas_gui.nullBrush);
		Rectangle (drawingContext, rect.left-origin.x, rect.top-origin.y, rect.right-origin.x, rect.bottom-origin.y);
		SelectObject (drawingContext, prevBrush);
	}

} /* drawRectangle */

void Canvas :: drawText (const char *s)
{
	int len = strlen (s);
	GPOINT origin = getOrigin ();

	if (len > 0)
	{
		TextOut (drawingContext, canvas_penpos.x-origin.x, canvas_penpos.y-origin.y, s, len);
	}
}	/* drawText */

void Canvas :: drawOval (GPOINT corner1, GPOINT corner2)
{
	if (corner1.valid () && corner2.valid ())
	{
		HGDIOBJ prevBrush;
		RECT rect;
		GPOINT origin = getOrigin ();

		pointsToRect (origin, corner1, corner2, &rect);

		prevBrush = SelectObject (drawingContext, canvas_gui.nullBrush);
		Ellipse (drawingContext, rect.left-origin.x, rect.top-origin.y, rect.right-origin.x, rect.bottom-origin.y);
		SelectObject (drawingContext, prevBrush);
	}

} /* drawOval */

void Canvas :: fillRectangle (GPOINT corner1, GPOINT corner2)
{
	if (corner1.valid () && corner2.valid ())
	{
		RECT rect;
		GPOINT origin = getOrigin ();

		pointsToRect (origin, corner1, corner2, &rect);
		Rectangle (drawingContext, rect.left-origin.x, rect.top-origin.y, rect.right-origin.x, rect.bottom-origin.y);
	}

} /* fillRectangle */

void Canvas :: fillOval (GPOINT corner1,  GPOINT corner2)
{
	if (corner1.valid () && corner2.valid ())
	{
		RECT rect;
		GPOINT origin = getOrigin ();

		pointsToRect (origin, corner1, corner2, &rect);
		Ellipse (drawingContext, rect.left-origin.x, rect.top-origin.y, rect.right-origin.x, rect.bottom-origin.y);
	}
} /* fillOval */

void Canvas :: drawPolygon (GPOINT points [], int size)
{
	HGDIOBJ prevBrush;
	GPOINT origin = getOrigin ();
	GPOINT p;

	for (int i = 0; i<size; i++)
	{
		points [i].x = points [i].x - origin.x;
		points [i].y = points [i].y - origin.y;
	}

	prevBrush = SelectObject (drawingContext, canvas_gui.nullBrush);
	Polygon (drawingContext, points, size);
	SelectObject (drawingContext, prevBrush);

} /* drawPolygon */

void Canvas :: fillPolygon	(GPOINT points [], int size)
{
	GPOINT origin = getOrigin ();
	GPOINT p;

	for (int i = 0; i<size; i++)
	{
		points [i].x = points [i].x - origin.x;
		points [i].y = points [i].y - origin.y;
	}

	Polygon (drawingContext, points, size);

} /* fillPolygon */

void Canvas :: setFont (GFONT font)
{
	canvas_gui.font = font;
	font.setFont (drawingContext);
}

GFONT Canvas :: getFont ()
{
	return canvas_gui.font;
}

int	Canvas :: getFontHeight ()
{
	TEXTMETRIC metrics;

	GetTextMetrics (drawingContext, &metrics);
	return metrics.tmAscent + metrics.tmDescent + metrics.tmInternalLeading + metrics.tmExternalLeading;
}

int	Canvas :: getTextWidth 	(const char *s)
{
	SIZE sz;
	const int len = strlen (s);

	GetTextExtentPoint32 (drawingContext, s, len, &sz);
	return sz.cx;
}

/*****************************************************************************************
	Alerts:
*****************************************************************************************/


void GUIException :: report () const
{
	makeAlert ("Unknown GUI Exception occurred");
}

void GUITerminated :: report () const
{
	switch (status)
	{
		case GUITerminated :: GFK_InitializationError:
			makeAlert ("GUI initialization error");
			return;
		case GUITerminated :: GFK_MultipleGUIs:
			makeAlert ("Multiple GUI objects declared");
			return;
		case GUITerminated :: GFK_Active:
			makeAlert ("GUI is already active");
			return;
		case GUITerminated :: GFK_MultipleCanvas:
			makeAlert ("More than one active canvas");
			return;
		default:
			makeAlert ("An unknown GUI error.");
			return;
	}
}

void DialogFailure :: report () const
{
	switch (status)
	{
		case DialogFailure :: DFK_MissingResource:
			makeAlert ("Error in opening dialog: resource missing.");
			return;
		case DialogFailure :: DFK_UnknownItemType:
			makeAlert ("Error in opening dialog: unknown control.");
			return;
		case DialogFailure :: DFK_IllegaltemID:
			makeAlert ("Error in dialog: specified control could not be found.");
			return;
		case DialogFailure :: DFK_WrongItemType:
			makeAlert ("Error in dialog: specified control has wrong type.");
			return;
		default:
			return;
	}
}

/*****************************************************************************************
	Fonts:
*****************************************************************************************/
GFONT :: GFONT (const char name [], int si, FStyle st)
	: size (si), style (st)
{
	LOGFONT fontDescription;
	HFONT newFont;

	fontDescription.lfHeight         = -si;
	fontDescription.lfWidth          = 0;
	fontDescription.lfEscapement     = 0;
	fontDescription.lfOrientation    = 0;
	fontDescription.lfWeight         = (st & Bold) ? FW_BOLD : FW_NORMAL;
	fontDescription.lfItalic         = (style & Italic) ? TRUE : FALSE;
	fontDescription.lfUnderline      = (style & Underline) ? TRUE : FALSE;
	fontDescription.lfStrikeOut      = FALSE;
	fontDescription.lfCharSet        = DEFAULT_CHARSET;
	fontDescription.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	fontDescription.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	fontDescription.lfQuality        = DEFAULT_QUALITY;
	fontDescription.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	strcpy (fontDescription.lfFaceName, name);

	newFont = CreateFontIndirect (&fontDescription);

	if (newFont != NULL)
		theFont = newFont;
}

GFONT :: GFONT ()
{
	GFONT ("Times New Roman", 10, Plain);		// PA: Tijdelijk voor vast font gekozen. Kan vast beter.
}

void GFONT :: setFont (HDC drawingContext)
{
        SelectObject (drawingContext,theFont);
}



/*****************************************************************************************
	File selectors:
*****************************************************************************************/

FileSelector :: FileSelector (GUI& gui)
	: file_select_gui (gui)
{
}

bool FileSelector :: get (char * name)
{
	bool fileSelected = false;
	OPENFILENAME ofn;

	ofn.lStructSize       = sizeof (OPENFILENAME);
	ofn.hwndOwner         = NULL;
	ofn.hInstance         = NULL;
	ofn.lpstrFilter       = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = (LPSTR) malloc (MAX_PATH);
	ofn.lpstrFile[0]      = '\0';
	ofn.nMaxFile          = MAX_PATH;
	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;
	ofn.lpstrInitialDir   = NULL;
	ofn.lpstrTitle        = NULL;
	ofn.Flags             = OFN_EXPLORER
						  | OFN_FILEMUSTEXIST
						  |	OFN_HIDEREADONLY
						  | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt       = NULL;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	fileSelected = GetOpenFileName (&ofn) != 0;
	if (fileSelected)
		strncpy (name, ofn.lpstrFile, MAX_PATH);
	else
		name [0] = '\0';
	free (ofn.lpstrFile);
	return fileSelected;
}

bool FileSelector :: put (char *name)
{
	bool fileSelected = false;
	OPENFILENAME ofn;

	ofn.lStructSize       = sizeof (OPENFILENAME);
	ofn.hwndOwner         = NULL;
	ofn.lpstrFilter       = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = (LPSTR) malloc (MAX_PATH);
	ofn.lpstrFile[0]      = '\0';
	ofn.nMaxFile          = MAX_PATH;
	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;
	ofn.lpstrInitialDir   = NULL;
	ofn.lpstrTitle        = NULL;
	ofn.Flags             = OFN_EXPLORER
	                      | OFN_OVERWRITEPROMPT
	                      | OFN_HIDEREADONLY;
	ofn.lpstrDefExt       = NULL;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	fileSelected = GetSaveFileName (&ofn) != 0;
	if (fileSelected)
		strncpy (name, ofn.lpstrFile, MAX_PATH);
	else
		name [0] = '\0';
	free (ofn.lpstrFile);
	return fileSelected;
}


/******************************************************************************************
	The callback procedures (DrawingWindowProc, DialogProc).
*******************************************************************************************/

//	Translate virtual key codes to special key codes (used in DrawingWindowProc).
static int CheckVirtualKeyCode (int keycode)
{
	int c = NrOfWinKeys + 1;
	switch (keycode)
	{
		case VK_UP:			return WinUpKey;
		case VK_DOWN:		return WinDownKey;
		case VK_LEFT:		return WinLeftKey;
		case VK_RIGHT:		return WinRightKey;
		case VK_PRIOR:		return WinPgUpKey;
		case VK_NEXT:		return WinPgDownKey;
		case VK_END:		return WinEndKey;
		case VK_HOME:		return WinBeginKey;
		case VK_BACK:		return WinBackSpKey;
		case VK_DELETE:		return WinDelKey;
		case VK_TAB:		return WinTabKey;
		case VK_RETURN:		return WinReturnKey;
		case VK_ESCAPE:		return WinEscapeKey;
		case VK_HELP:		return WinHelpKey;
		case VK_F1:			return WinF1Key;
		case VK_F2:			return WinF2Key;
		case VK_F3:			return WinF3Key;
		case VK_F4:			return WinF4Key;
		case VK_F5:			return WinF5Key;
		case VK_F6:			return WinF6Key;
		case VK_F7:			return WinF7Key;
		case VK_F8:			return WinF8Key;
		case VK_F9:			return WinF9Key;
		case VK_F10:		return WinF10Key;
		case VK_F11:		return WinF11Key;
		case VK_F12:		return WinF12Key;
		default:			return c;
	}
}

/******************************************************************************************
	The callback function of the window.
*******************************************************************************************/
LRESULT CALLBACK GUI :: DrawingWindowProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	GUI *gui = (GUI*)GetGWL_USERDATA (hwnd);	// Retrieve reference to GUI object

	switch (iMsg)
	{
		case WM_COMMAND:
			{
				int wNotifyCode = HIWORD(wParam);
				HWND hwndCtl = (HWND) lParam;

				if (wNotifyCode == 0 && hwndCtl == NULL)
				{	// WM_COMMAND generated from a menu
					MenuItems *menus = gui->gui_menus;
					int index = (int)LOWORD(wParam);	// Menu elements are numbered from 1
					int searchedMenus = 0;

					while (index >= 0 && searchedMenus < gui->gui_nr_of_menus)
					{
						MenuItem *items = menus[searchedMenus];
						while (index > 0 && items->mi_next != NULL)
						{
							index--;
							items = items->mi_next;
						};
						if (index > 0)
						{
							searchedMenus++;
						}
						else
						{
							items->mi_action ();
						}
						index--;
					}
				}
				return 0;
			}
		case WM_CHAR:
			{
				KEYINFO keyInfo;
				keyInfo.isASCII = TRUE;
				keyInfo.keyCode = (int) wParam;

				gui->Keyboard (keyInfo);
				return 0;
			}
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				KEYINFO keyInfo;
				keyInfo.isASCII = FALSE;
				keyInfo.keyCode = CheckVirtualKeyCode ((int) wParam);

				if (keyInfo.keyCode==NrOfWinKeys+1)
				/* Ignore non-virtual keys, because they arrive as WM_SYSCHAR and WM_CHAR. */
				{
					return DefWindowProc (hwnd, iMsg, wParam, lParam);
				}
				else
				{
				/* Handle virtual keys analogously to keys received as WM_SYSCHAR and WM_CHAR. */
					gui->Keyboard (keyInfo);
					return 0;
				}
			}
		case WM_LBUTTONDOWN:
			{
				MOUSEINFO mouseInfo;
				int x,y;

				gui->mouse_is_down = true;

				x = gui->canvas_origin.x + (int) LOWORD(lParam);
				y = gui->canvas_origin.y + (int) HIWORD(lParam);
				mouseInfo.mouseState = MouseDown;
				mouseInfo.mousePos.x = x;
				mouseInfo.mousePos.y = y;

				gui->Mouse (mouseInfo);
				return 0;
			}
		case WM_MOUSEMOVE:
			{
				MOUSEINFO mouseInfo;
				int x,y;

				if (gui->mouse_is_down)
				{
					x = gui->canvas_origin.x + (int) LOWORD(lParam);
					y = gui->canvas_origin.y + (int) HIWORD(lParam);
					mouseInfo.mouseState = MouseTrack;
					mouseInfo.mousePos.x = x;
					mouseInfo.mousePos.y = y;

					gui->Mouse (mouseInfo);
				}
				return 0;
			}
		case WM_LBUTTONUP:
			{
				MOUSEINFO mouseInfo;
				int x,y;

				gui->mouse_is_down = false;

				x = gui->canvas_origin.x + (int) LOWORD(lParam);
				y = gui->canvas_origin.y + (int) HIWORD(lParam);
				mouseInfo.mouseState = MouseUp;
				mouseInfo.mousePos.x = x;
				mouseInfo.mousePos.y = y;

				gui->Mouse (mouseInfo);
				return 0;
			}
		case WM_PAINT:
			{
//				HDC hdc;
				PAINTSTRUCT ps;
				RECT updateRect;

				BeginPaint (hwnd, &ps);
				updateRect.left   = ps.rcPaint.left   + gui->canvas_origin.x;
				updateRect.top    = ps.rcPaint.top    + gui->canvas_origin.y;
				updateRect.right  = ps.rcPaint.right  + gui->canvas_origin.x;
				updateRect.bottom = ps.rcPaint.bottom + gui->canvas_origin.y;
				gui->Window (updateRect);
				EndPaint (hwnd, &ps);
				return 0;
			}
		case WM_SIZE:
			{
				int newWidth, newHeight;
				int newX, newY;
				int maxRange;
				SCROLLINFO scrollInfo;

				scrollInfo.cbSize = sizeof (SCROLLINFO);

				newWidth  = (int)LOWORD(lParam);
				newHeight = (int)HIWORD(lParam);
				scrollInfo.fMask = SIF_POS;					// Retrieve thumb
				GetScrollInfo (hwnd,SB_HORZ,&scrollInfo);
				gui->canvas_origin.x = scrollInfo.nPos;
				GetScrollInfo (hwnd,SB_VERT,&scrollInfo);
				gui->canvas_origin.y = scrollInfo.nPos;

				//	Adjust horizontal scrollbar settings:
				maxRange = Maximum (MINCOORD,MAXCOORD-newWidth);
				scrollInfo.fMask = SIF_RANGE;				// Set range
				scrollInfo.nMin  = MINCOORD;
				scrollInfo.nMax  = maxRange;
				SetScrollInfo (hwnd, SB_HORZ,&scrollInfo,true);
				newX = setbetween (gui->canvas_origin.x,MINCOORD,maxRange);
				if (newX!=gui->canvas_origin.x)
				{
					scrollInfo.fMask  = SIF_POS | SIF_PAGE;
					scrollInfo.nPos   = newX;
					scrollInfo.nPage  = newWidth;
					SetScrollInfo (hwnd,SB_HORZ,&scrollInfo,true);
					gui->canvas_origin.x = newX;
				}
				//	Adjust vertical scrollbar settings:
				maxRange = Maximum (MINCOORD,MAXCOORD-newHeight);
				scrollInfo.fMask = SIF_RANGE;				// Set range
				scrollInfo.nMin  = MINCOORD;
				scrollInfo.nMax  = maxRange;
				SetScrollInfo (hwnd, SB_VERT, &scrollInfo, true);
				newY = setbetween (gui->canvas_origin.y,MINCOORD,maxRange);
				if (newY!=gui->canvas_origin.y)
				{
					scrollInfo.fMask  = SIF_POS | SIF_PAGE;
					scrollInfo.nPos   = newY;
					scrollInfo.nPage  = newHeight;
					SetScrollInfo (hwnd,SB_VERT,&scrollInfo,true);
					gui->canvas_origin.y = newY;
				}

				return DefWindowProc (hwnd, iMsg, wParam, lParam);
			}
		case WM_HSCROLL:
			{
				int curX, newX, curWidth, maxRange;
				int nScrollCode = (int) LOWORD(wParam);
				int nPos        = (int) HIWORD(wParam);
				RECT clientRect;
				SCROLLINFO scrollInfo;

				scrollInfo.cbSize = sizeof (SCROLLINFO);

				GetClientRect (hwnd,&clientRect);
				curWidth = clientRect.right;
				scrollInfo.fMask = SIF_POS;					// Retrieve thumb
				GetScrollInfo (hwnd, SB_HORZ, &scrollInfo);
				gui->canvas_origin.x = scrollInfo.nPos;

				curX     = gui->canvas_origin.x;
				maxRange = Maximum (MINCOORD,MAXCOORD-curWidth);
				newX     = curX;
				switch (nScrollCode)
				{
					case SB_BOTTOM:			// Scrolls to the lower right.
						{
							newX = maxRange;
							break;
						}
					case SB_ENDSCROLL:		// Ends scroll.
						{
							break;
						}
					case SB_LINELEFT:		// Scrolls left by one unit.
						{
							newX = setbetween (curX - dwScrollUnit,MINCOORD,maxRange);
							break;
						}
					case SB_LINERIGHT:		// Scrolls right by one unit.
						{
							newX = setbetween (curX + dwScrollUnit,MINCOORD,maxRange);
							break;
						}
					case SB_PAGELEFT:		// Scrolls left by the width of the window.
						{
							newX = setbetween (curX - curWidth,MINCOORD,maxRange);
							break;
						}
					case SB_PAGERIGHT:		// Scrolls right by the width of the window.
						{
							newX = setbetween (curX + curWidth,MINCOORD,maxRange);
							break;
						}
					case SB_THUMBPOSITION:	// Scrolls to the absolute position. The current position is specified by the nPos parameter.
					case SB_THUMBTRACK:		// Drags scroll box to the specified position. The current position is specified by the nPos parameter.
						{
							newX = setbetween (nPos,MINCOORD,maxRange);
							break;
						}
					case SB_TOP:			// Scrolls to the upper left.
						{
							newX = MINCOORD;
							break;
						}
				}
				gui->canvas_origin.x = newX;
				if (newX != curX)
				{
					scrollInfo.fMask = SIF_POS;
					scrollInfo.nPos = newX;
					SetScrollInfo (hwnd, SB_HORZ, &scrollInfo, true);
					ScrollWindowEx(	hwnd,			// handle of window to scroll
									curX-newX,		// amount of horizontal scrolling
									0,				// amount of vertical scrolling
									NULL,			// address of structure with scroll rectangle
									NULL,			// address of structure with clip rectangle
									NULL,			// handle of update region
									NULL,			// address of structure for update rectangle
									SW_ERASE | SW_INVALIDATE 	// scrolling flags
								  );
					UpdateWindow (hwnd);
				}
				return 0;
			}
		case WM_VSCROLL:
			{
				int curY, newY, curHeight, maxRange;
				int nScrollCode = (int) LOWORD(wParam);
				int nPos        = (int) HIWORD(wParam);
				RECT clientRect;
				SCROLLINFO scrollInfo;

				scrollInfo.cbSize = sizeof (SCROLLINFO);

				GetClientRect (hwnd,&clientRect);
				curHeight= clientRect.bottom;
				scrollInfo.fMask = SIF_POS;					// Retrieve thumb
				GetScrollInfo (hwnd, SB_VERT, &scrollInfo);
				gui->canvas_origin.y = scrollInfo.nPos;

				curY     = gui->canvas_origin.y;
				maxRange = Maximum (MINCOORD,MAXCOORD-curHeight);
				newY     = curY;
				switch (nScrollCode)
				{
					case SB_BOTTOM:			// Scrolls to the lower right.
						{
							newY = maxRange;
							break;
						}
					case SB_ENDSCROLL:		// Ends scroll.
						{
							break;
						}
					case SB_LINELEFT:		// Scrolls left by one unit.
						{
							newY = setbetween (curY - dwScrollUnit,MINCOORD,maxRange);
							break;
						}
					case SB_LINERIGHT:		// Scrolls right by one unit.
						{
							newY = setbetween (curY + dwScrollUnit,MINCOORD,maxRange);
							break;
						}
					case SB_PAGELEFT:		// Scrolls left by the width of the window.
						{
							newY = setbetween (curY - curHeight,MINCOORD,maxRange);
							break;
						}
					case SB_PAGERIGHT:		// Scrolls right by the width of the window.
						{
							newY = setbetween (curY + curHeight,MINCOORD,maxRange);
							break;
						}
					case SB_THUMBPOSITION:	// Scrolls to the absolute position. The current position is specified by the nPos parameter.
					case SB_THUMBTRACK:		// Drags scroll box to the specified position. The current position is specified by the nPos parameter.
						{
							newY = setbetween (nPos,MINCOORD,maxRange);
							break;
						}
					case SB_TOP:			// Scrolls to the upper left.
						{
							newY = MINCOORD;
							break;
						}
				}
				gui->canvas_origin.y = newY;
				if (newY != curY)
				{
					scrollInfo.fMask = SIF_POS;
					scrollInfo.nPos = newY;
					SetScrollInfo (hwnd, SB_VERT, &scrollInfo, true);
					ScrollWindowEx(	hwnd,			// handle of window to scroll
									0,				// amount of horizontal scrolling
									curY-newY,		// amount of vertical scrolling
									NULL,			// address of structure with scroll rectangle
									NULL,			// address of structure with clip rectangle
									NULL,			// handle of update region
									NULL,			// address of structure for update rectangle
									SW_ERASE | SW_INVALIDATE 	// scrolling flags
								  );
					UpdateWindow (hwnd);
				}
				return 0;
			}
		case WM_DESTROY:
			DeleteObject (gui->thePen);
			DeleteObject (gui->theBrush);
			PostQuitMessage (0);
			return 0;
	}

	return DefWindowProc (hwnd, iMsg, wParam, lParam);
}	/* DrawingWindowProc */


/******************************************************************************************
	The callback function of the dialogs.
*******************************************************************************************/

BOOL CALLBACK Dialog :: DialogProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static DialogArgs *args = NULL;
	static GUI *the_dialog_gui = NULL;

	switch (iMsg)
	{
	//	WM_INITDIALOG assumes that dialogs are created with CreateDialogParam(_,_,_,_,(LPARAM)DialogArgs)
		case WM_INITDIALOG:
			{
				args = (DialogArgs*)lParam;
				the_dialog_gui = args->ref_to_dialog_gui;
				return TRUE;
				break;
			}
		case WM_COMMAND:
			{
				DialogHandle active_dialog = searchForDialog (hwnd, the_dialog_gui->the_dialogs);
				int item_hit = 0;

				if (LOWORD (wParam) == IDOK)		// First check if OK or CANCEL button has been pressed
				{
					item_hit = IDOK;				// OK button has been pressed
				}
				else if (LOWORD (wParam) == IDCANCEL)
				{
					item_hit = IDCANCEL;			// Cancel button has been pressed
				}
				else if (HIWORD (wParam) == BN_CLICKED)
				{
					if (lParam != 0)
					{
						item_hit = (int)GetWindowLong ((HWND)lParam, GWL_ID);
					}
					else if (LOWORD (wParam) == 2)
					{
						DestroyWindow (hwnd);
						return TRUE;
					}
				}
				else
				{
					item_hit = (int)LOWORD (wParam);
				}
				Dialog dialog (active_dialog, *the_dialog_gui);

				HWND the_control;
				GetDialogItemAndType (active_dialog -> diar_ptr, item_hit, the_control);

				ControlRepr& control = * active_dialog -> searchControl (item_hit);

				switch (control.control_type)
				{
					case kButtonDialogItem:
						if (control.control_callback.cbf_button != NULL)
							control.control_callback.cbf_button (item_hit, dialog);
						break;
					case kCheckBoxDialogItem:
					{
						LRESULT cv = SendMessage (the_control, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
						SendMessage (the_control, BM_SETCHECK, (WPARAM) (cv==BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED, (LPARAM) 0);

						if (control.control_callback.cbf_check != NULL)
							control.control_callback.cbf_check (item_hit, cv == BST_CHECKED, dialog);
						break;
					}
					case kRadioButtonDialogItem:
					{
						LRESULT cv = SendMessage (the_control, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);

						if (cv == BST_CHECKED)
						{
							// reset all the other radio buttons
							resetRadioButtons (item_hit, active_dialog -> diar_controls, active_dialog -> diar_ptr);

							// set the selected one
							SendMessage (the_control, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);

							if (control.control_callback.cbf_radio != NULL)
								control.control_callback.cbf_radio (item_hit, dialog);
						}
						break;
					}
					case kEditTextDialogItem:
					default:
						break;
				}
				return TRUE;
			} break;
		/*	WM_ACTIVATE must set the activeDialog value to this dialog if active; and to NULL otherwise.
		*/
		case WM_ACTIVATE:
			{
				if (LOWORD(wParam) == WA_INACTIVE)
				{
					activeDialog = NULL;
				}
				else
				{
					activeDialog = hwnd;
				}
				return FALSE;
			}
			break;
		case WM_CLOSE:
			{
				DialogHandle active_dialog = searchForDialog (hwnd, the_dialog_gui->the_dialogs);
				DestroyWindow (active_dialog -> diar_ptr);
				active_dialog -> diar_ptr = NULL;
				return TRUE;
				break;
			}
		default:
			return FALSE;
			break;
	}
}	/* DialogProc */
