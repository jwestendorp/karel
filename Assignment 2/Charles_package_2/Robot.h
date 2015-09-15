#include "gui_kernel.h"

/*
	Robot is a library to draw and manipulate a simple robot in a window.
	It uses provides the class ROBOT. Implementation is built on the gui_kernel.
	Last Modified:	September 16 2006, by Pieter Koopman, Radboud University Nijmegen
*/

extern void stap			();
extern void linksom 		();
extern void rechtsom 		();

extern bool op_bal			();

extern void pak_bal		();
extern void leg_bal		();

extern bool muur_voor		();
extern bool noord			();

extern void rust			(int n);

extern void maakWereldLeeg ();
extern void nieuweWereld	(const char wereld []);
extern void reset			();
extern void stop ();

// For English version:
extern void step		() ;
extern void turn_left 	() ;
extern void turn_right	() ;
extern bool on_ball	    () ;
extern void get_ball	() ;
extern void put_ball	() ;

extern bool in_front_of_wall    () ;
extern bool north		() ;

extern void rest		(int n) ;
extern void makeWorldEmpty () ;
extern void newWorld	(const char wereld []) ;

extern void steps (int number_of_steps) ;
extern void draw_line_with_balls ( int number_of_steps ) ;



const int SteenGrootte	= 6;
const int BalGrootte	= 11;
const int RobotGrootte	= 18;
const int RandGrootte	= 12;
const int WereldBreedte = 50;
const int WereldHoogte	= 30;

const int SCREENWIDTH	= WereldBreedte * 2 * SteenGrootte + RandGrootte;
const int SCREENHEIGHT	= WereldHoogte * 2 * SteenGrootte + RandGrootte;

const int dInit = 60;

class RobotGUI : public GUI
{
public:
	virtual void Window (const RECT& area);
	RobotGUI();
};

extern RobotGUI& karelsWereld();

enum IllegaleActies { IA_Karel, IA_Stap, IA_PakBal, IA_LegBal, IA_Open} ;

class IllegaleActie
{
public:
	IllegaleActie (IllegaleActies ia) : _actie (ia) {}
	void report () ;
private:
	IllegaleActies _actie;
} ;

// For English version:
extern void steps ( int ) ;
extern void draw_line_with_balls ( int ) ;
extern void place_rectangle (int, int, int, int) ;
extern void place_walls (int, int, int, bool) ;
extern void make_string_with_balls () ;
extern void make_chaos_with_balls () ;
extern void make_labyrinth () ;
extern void make_path_with_balls () ;			// toegevoegd door Peter Achten 8 sept 09
extern void create_ball (int x, int y) ;        // toegevoegd door Peter Achten 21 aug 13
extern void make_cave () ;                      // toegevoegd door Peter Achten 10 sept 14
