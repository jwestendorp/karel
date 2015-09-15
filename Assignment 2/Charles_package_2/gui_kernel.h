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

#include <windows.h>

//	WINARGS are mandatory in the GUI run methods.
struct WINARGS
{	HINSTANCE	hInstance;
	HINSTANCE	hPrevInstance;
	PSTR		szCmdLine;
	int			iCmdShow;

	WINARGS (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
		: hInstance (hInstance), hPrevInstance (hPrevInstance), szCmdLine (szCmdLine), iCmdShow (iCmdShow)
	{}
	WINARGS ()
		: hInstance (NULL), hPrevInstance (NULL), szCmdLine (0), iCmdShow (0)
	{}
} ;

//	GPOINT extends POINT with validity check and equality operation.
struct GPOINT : public POINT
{
	GPOINT (int nx = 0, int ny = 0)
	{	x = nx; y = ny; }
	
	bool valid (); // \p -> isbetween (p.x,MINCOORD,MAXCOORD) && isbetween (p.y,MINCOORD,MAXCOORD)
	bool operator== (const GPOINT&);
} ;

//	GSIZE extends SIZE with validity check and equality operation
struct GSIZE : public SIZE
{
	GSIZE (int ncx = 0, int ncy = 0)
	{	cx = ncx; cy = ncy; }
	
	bool valid (); // \s -> s.cx>0 && s.cy>0
	bool operator== (const GSIZE&);
} ;

//	x,y coordinate values must be between MINCOORD and MAXCOORD
const int MINCOORD = -32768;
const int MAXCOORD =  32767;

//	Constants for special keys. These are passed via the keyCode field in the KEYINFO structure below.
enum SpecialKey
	{	WinUpKey	= 0
	,	WinDownKey
	,	WinLeftKey
	,	WinRightKey
	,	WinPgUpKey
	,	WinPgDownKey
	,	WinEndKey
	,	WinBeginKey
	,	WinBackSpKey
	,	WinDelKey
	,	WinTabKey
	,	WinReturnKey
	,	WinEscapeKey
	,	WinHelpKey
	,	WinF1Key
	,	WinF2Key
	,	WinF3Key
	,	WinF4Key
	,	WinF5Key
	,	WinF6Key
	,	WinF7Key
	,	WinF8Key
	,	WinF9Key
	,	WinF10Key
	,	WinF11Key
	,	WinF12Key
	,	NrOfWinKeys
	} ;

struct KEYINFO
{	bool	isASCII;		//	TRUE iff keyCode is ASCII; FALSE then one of the special key codes
	int		keyCode;		//	If isASCII: the ASCII code; otherwise: a special key code
};

enum MouseState
{	MouseDown				//	The mouse button goes down
,	MouseTrack				//	The mouse button is still down
,	MouseUp					//	The mouse button goes up
}	;

struct MOUSEINFO
{	MouseState	mouseState;	//	State of the mouse
	GPOINT		mousePos;	//	Current mouse position
}	;

struct RGBCOLOUR
{
	int r;					//	The amount of red   in the colour (MINRGB <= x <= MAXRGB)
	int g;					//	The amount of green in the colour (MINRGB <= x <= MAXRGB)
	int b;					//	The amount of blue  in the colour (MINRGB <= x <= MAXRGB)
	
	enum { MINRGB = 0, MAXRGB = 255};
	
	RGBCOLOUR (int nr = MINRGB, int ng = MINRGB, int nb = MINRGB)
		: r (nr), g (ng), b (nb) {}
	
	RGBCOLOUR (const RGBCOLOUR& rgb)
		: r (rgb.r), g (rgb.g), b (rgb.b) {}
		
	bool valid ();	//  \c -> isbetween (c.r,MINRGB,MAXRGB)
					//     && isbetween (c.g,MINRGB,MAXRGB)
					//     && isbetween (c.b,MINRGB,MAXRGB)
	
	bool operator== (const RGBCOLOUR&);
} ;

extern const RGBCOLOUR BlackRGB;	// The colour black
extern const RGBCOLOUR WhiteRGB;	// The colour white
extern const RGBCOLOUR RedRGB;		// The colour red
extern const RGBCOLOUR GreenRGB;	// The colour green
extern const RGBCOLOUR BlueRGB;		// The colour blue


//	Forward declaration of the (virtual) GUI class and the clas Canvas

class GUI;
class Canvas;
class Dialog;


/*****************************************************************************************
	Font class
*****************************************************************************************/

class GFONT
{
	friend class Canvas;

	enum DefaultSize { DSIZE = 12 };

public:
	enum FStyle { Plain=0, Bold=1, Italic=2, Underline=4 };
	
	GFONT (const char name [], int size = DSIZE, FStyle style=Plain);
	GFONT ();
	
private:
	int		size;
	HFONT   theFont;
	FStyle	style;

	void setFont	(HDC drawingContext);
} ;


/*****************************************************************************************
	Menu class
*****************************************************************************************/

//	The prototypes of the program defined menu callback procedures.

typedef void (*MenuCallBack) ();
typedef struct MenuItem *MenuItems;
typedef class Menu *Menus;

/*	Menu:	Constructor that creates a new menu with the indicated name
	add:	Appends the item to the argument menu at the end.
			When the argument is not specified a separator is added
*/
	
class Menu	
{
	friend class GUI;
public:
				Menu	(char *);
	Menu&		add		(char *, MenuCallBack);
	Menu&		add		(void);
private:
	MenuItems		menu_call_backs; 	//  a list of callback procedures
	char			*menu_title;		//	the title
	Menus			menu_next;
	static Menus	menu_bar;
};


/*****************************************************************************************
	The class GUI is a virtual class that must be inherited in order to be used.
	The derived clas can redefine the virtual event handlers
	
		* Window			(handles all window updates)
		* Keyboard			(handles all keyboard input in the window)
		* Mouse				(handles all mouse input in the window)
		* Timer				(handles all timer events in the window)
	
	There is always one window associated with a GUI in which one can indicate
	the desired origin.
	
	A GUI-application needs to be triggered by invoking the 'Run' method. Two versions
	of 'Run' are available: one for an application that contains a menu and one
	for a menu-less application.
	
	Termination operations:

	Stop: Terminates the gui application.


	GSIZE GetWindowSize ()

	Actions:
	Retrieve the current size of the window viewing area.

	Result:
	*	GSIZE:			the size of the window viewing area.

	
	Timing operations:

	startTimer starts the timer so that it calls the timer procedure every dt milliseconds.
	If a timer is already open then dt is the new interval and timing starts from this
	moment on. 
	stopTimer stops the evaluation of the timer.
	
	Parameters:
	*	dt:				The timer interval. If negative it will be set to zero. It is
						not guaranteed that the timer will be called at exactly the
						indicated interval.
	 
*****************************************************************************************/

typedef struct DialogRepr *DialogHandle; 

class GUI
{
public:
				GUI					(GSIZE, char *title);

	void		Run					(WINARGS winArgs);
	void		Run					(Menu& menu, WINARGS winArgs);

	GSIZE		getWindowSize		();
	void		startTimer			(int);
	void 		stopTimer			();
	void		Activate			();
	void		Stop				();

	static void		Beep			(int);

	virtual void	Window		(const RECT& area)				{}
	virtual void	Mouse		(const MOUSEINFO& mouse_info)	{}
	virtual void	Keyboard	(const KEYINFO& key_info)		{}
	virtual void	Timer		(const int dt)					{}


	friend class Canvas;
	friend class Dialog;
private:
	GUI(const GUI&);					// empty copy-constructor; prevents duplication
	const GUI& operator=(const GUI&);	// empty assignment-operator; prevents duplication
	struct TimerDef
	{
		UINT	timer_period;
		UINT	timerID;

				TimerDef ()
					: timer_period(0),timerID(0)
				{}
	} ;

//	HDC				drawingContext;
	HWND			the_window;
	DialogHandle	the_dialogs;

	GPOINT		canvas_origin;
	RGBCOLOUR	pen_colour;
	COLORREF	penColour;
	bool		in_normal_mode;
	GFONT		font;
	GSIZE		windowSize;
	char		*windowTitle;
	
	HPEN		thePen;
	HBRUSH		theBrush;
	HGDIOBJ		blackPen;
	HGDIOBJ		whitePen;
	HGDIOBJ		nullPen;
	HGDIOBJ		blackBrush;
	HGDIOBJ		whiteBrush;
	HGDIOBJ		nullBrush;
	
	MenuItems	*gui_menus;
	int			gui_nr_of_menus;
	TimerDef	gui_timer;
	bool		mouse_is_down;
	bool		quitRequested;

	void InitialiseDrawingLibrary (WINARGS winArgs);
	static VOID    CALLBACK EvaluateTimer     (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	static LRESULT CALLBACK DrawingWindowProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	static bool drawing_library_initialised;
	static bool gui_created;

	
	void 		RunGUI 				(void);
	void 		insertMenus 		(Menus menus);

};


/******************************************************************************************
	Dialogs class
*******************************************************************************************/

typedef WORD	DialogId;
typedef int		ControlId;

//	The prototypes of the program defined control callback procedures.
typedef void (*ButtonCallBack)		(ControlId, Dialog&);
typedef void (*CheckCallBack)		(ControlId, bool, Dialog&);
typedef void (*RadioButtonCallBack) (ControlId, Dialog&);

class Dialog
{
	friend class GUI;
public:
			Dialog					(DialogId, GUI&);
	Dialog&	addButtonCallBack		(ControlId, ButtonCallBack);
	Dialog&	addCheckCallBack		(ControlId, CheckCallBack);
	Dialog&	addRadioButtonCallBack	(ControlId, RadioButtonCallBack);
	
	void	closeDialog				();
	
	void	getEditValue			(ControlId, char *, int);
	bool	getRadioValue			(ControlId);
	bool	getCheckValue			(ControlId);

	void	setEditValue			(ControlId, const char *);
	void	setRadioValue			(ControlId);
	void	setCheckValue			(ControlId, bool);
	void	setStaticValue			(ControlId, const char *);

private:
			Dialog					(DialogHandle, GUI&);
	DialogHandle	dialog_handle;
	GUI&			dialog_gui;

	static BOOL CALLBACK DialogProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
} ;


/*****************************************************************************************
	FileSelector class.
*****************************************************************************************/

class FileSelector
{
	friend class GUI;
public:
	enum { MAXFileName = MAX_PATH};

		 FileSelector (GUI&);
	bool get	(char *);
	bool put	(char *);
private:
	GUI&	file_select_gui;
} ;


/*****************************************************************************************
	For drawing one needs a 'Canvas' object. The constructor expects a GUI object as
	argument.

	---------------------------------------------------------------------------------------
	
	GPOINT getOrigin ()

	Actions:
	Retrieve current orientation (origin) of the window viewing area.

	Result:
	*	GPOINT:			the current origin of the window viewing area.

	---------------------------------------------------------------------------------------

	void setPenColour (RGBCOLOUR colour)
	RGBCOLOUR  getPenColour ()

	Actions:
	Set the current colour and return the current colour. 
	The new colour is set only if colour.valid().

	Parameters:
	*	colour:			the new colour be used in all subsequent drawing operations.

	Result:
	*	the colour that was used before the procedure was called.

	---------------------------------------------------------------------------------------

	void  setPenPos (GPOINT pos)
	GPOINT getPenPos ()

	Actions:
	Set the current pen position and return the current pen position.
	The new position is set only if pos.valid().

	Parameters:
	*	pos:			the new position of the pen.

	Result:
	*	the position of the pen before the procedure was called. 

	---------------------------------------------------------------------------------------
	
	void toggleXORmode ()

	Actions:
	Toggles drawing mode of the canvas to XOR mode and normal mode.

*****************************************************************************************/

class Canvas
{
	friend class GUI;

public:
			Canvas			(GUI &gui);
			~Canvas			();

	void		setPenColour 	(RGBCOLOUR);
	RGBCOLOUR 	getPenColour	();
	void 		setPenPos		(GPOINT);
	GPOINT		getPenPos		();
	void		toggleXORmode	();
	
	void		setFont 		(GFONT);
	GFONT		getFont			();
	int			getFontHeight 	();
	int			getTextWidth 	(const char *s);

	void		drawPOINT		();
	void		drawLineTo		(GPOINT);
	void		drawRectangle	(GPOINT, GPOINT);
	void		drawText		(const char *s);
	void		drawOval		(GPOINT, GPOINT);
	void 		drawPolygon		(GPOINT points [], int size);
	
	void		fillRectangle	(GPOINT, GPOINT);
	void		fillOval		(GPOINT, GPOINT);
	void		fillPolygon		(GPOINT points [], int size);
	GPOINT		getOrigin		()
				{	return canvas_gui.canvas_origin; }
private:
	GUI		&canvas_gui;
	HDC		 drawingContext;
	static bool canvas_created;

} ;


/*****************************************************************************************
	Exception class
*****************************************************************************************/

void makeAlert (const char alert_text []);

class GUIException
{
public:
	virtual void report () const;
} ;

class GUITerminated : public GUIException
{
public:
	enum GUIFailureKind
	{
		GFK_InitializationError, GFK_MultipleGUIs, GFK_Active, GFK_MultipleCanvas
	};
	GUITerminated (GUIFailureKind st)
		: status (st)
	{}
	
	void report () const;
private:
	GUIFailureKind status;
} ;


class DialogFailure : public GUIException
{
public:
	enum DialogFailureKind
	{
		DFK_MissingResource, DFK_UnknownItemType, DFK_IllegaltemID, DFK_WrongItemType
	};

	DialogFailure (DialogFailureKind st)
		: status (st)
	{}
	void report () const;
private:
	DialogFailureKind status;
} ;
