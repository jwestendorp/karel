//#include <strstream>
#include <fstream>

/*
	Robot is a library to draw and manipulate a simple robot in a window.
	It uses provides the class ROBOT. Implementation is built on the gui_kernel.
	Last Modified:	September 16 2006, by Pieter Koopman, Radboud University Nijmegen
*/

#include "Robot.h"

using namespace std;

enum Veld { Leeg, Bal, Muur };

enum Richting { Noord, West, Zuid, Oost };

void rechthoek (int lx, int ly, int rx, int ry, Veld veld,
				Veld w[WereldBreedte][WereldHoogte] )
{
	for (int b = lx; b < rx; b++)
		w [b][ly] = w [b][ry-1] = veld;
	for (int h = ly + 1; h < ry - 1; h++)
		w [lx][h] = w [rx-1][h] = veld;
}

void IllegaleActie :: report ()
{
	switch (_actie)
	{
	case IA_Karel:
		makeAlert ("Charles is not allowed at this position.");
		break;
	case IA_PakBal:
		makeAlert ("Charles can not pick up a ball because there is no ball at this position.");
		break;
	case IA_LegBal:
		makeAlert ("Charles can not put a ball here because there is already a ball.");
		break;
	case IA_Stap:
		makeAlert ("Charles bumped into a wall...");
		break;
	case IA_Open:
		makeAlert ("It was not possible to open the labyrinth.\nProbably it is in the wrong directory!");
		break;
	}
}

class ROBOT
{
public:
	void stap			();
	void linksom 		();
	void rechtsom 		();

	bool bovenopBal		();

	void pakBal			();
	void legBal			();

	bool muurVoor		();
	bool noord			();

	void rust			(int);

	void maakWereldLeeg ();
	void nieuweWereld	(const char wereld []);
	void reset			();
	void herteken		(Canvas&);
	void herteken		();
	ROBOT (int st, Richting r, int x, int y);
	void tekenWereld	(Canvas&);
	void tekenRobot		(Canvas&);
	void tekenRobot		();

	void plaatsRechthoek (int links, int boven, int breedte, int hoogte) ;
	void maakMuur ( int links, int onder, int aantal, bool horizontaal ) ;
	void maakBallenSnoer () ;
	void maakBallenChaos () ;
	void maakMurenChaos  (bool horizontaal, bool verticaal) ;  // toegevoegd door Peter
	void maakBallenPad   () ;                                  // toegevoegd door Peter
	void grotwand        (Richting richting) ;                 // toegevoegd door Peter
	void maakGrot        () ;                                  // toegevoegd door Peter
	void creeerBal       (int x, int y);                       // toegevoegd door Peter

protected:
	Richting	huidige_richting;
	int			x_pos;
	int			y_pos;
	int			stap_tijd;
private:
	void tekenWereldDeel	(Canvas &canvas, int from_x, int from_y, int to_x, int to_y);
	void wisWereldDeel		(Canvas &canvas, int from_x, int from_y, int to_x, int to_y);

	void tekenMuur			(Canvas &canvas, int x, int y);
	void tekenSteen			(Canvas &canvas, int x, int y);
	void tekenLeegVeld		(Canvas &canvas, int x, int y);
	void tekenBal			(Canvas &canvas, int x, int y);


	void pauze 				();
	GPOINT schermPos (int sin_richting, int cos_richting, int hoek_nr);

	Veld 		Wereld [WereldBreedte][WereldHoogte];
	double		hoeken [4][2];
} ;

const int xInit = 1;
const int yInit = WereldHoogte-2;
const Richting rInit = Oost;

void rechthoek (int lx, int ly, int rx, int ry, Veld veld,
				Veld w[WereldBreedte][WereldHoogte] );


extern int kies3 ();

class Robot : public ROBOT
{
public:
	Robot (int st = dInit, Richting r = rInit, int x = xInit, int y = yInit)
		: ROBOT (st, r, x, y) {}
} ;

void ROBOT :: maakWereldLeeg ()
{
	for (int b = 0; b < WereldBreedte; b++)
		for (int h = 0; h < WereldHoogte; h++)
			 Wereld [b][h] = Leeg;
}

ROBOT :: ROBOT (int st, Richting r, int x, int y)
	: huidige_richting (r), x_pos (x), y_pos (y), stap_tijd (st)
{
	maakWereldLeeg ();

	rechthoek (0,0,WereldBreedte,WereldHoogte,Muur,Wereld);

	const double EDWD	= 0.5773502691896;	// EDWD = Een derde wortel drie
	const double staart_grootte = (EDWD + 0.2) * RobotGrootte / 2;

	hoeken [0][0] = - RobotGrootte / 2;
	hoeken [0][1] = - staart_grootte;
	hoeken [1][0] = 0;
	hoeken [1][1] = 0;
	hoeken [2][0] = RobotGrootte / 2;
	hoeken [2][1] = - staart_grootte;
	hoeken [3][0] = 0;
	hoeken [3][1] = EDWD * RobotGrootte;

	if (x_pos < 1 || y_pos < 1 || x_pos >= WereldBreedte - 1 || y_pos >= WereldHoogte - 1 || Wereld [1][y_pos] == Muur)
		throw IllegaleActie (IA_Karel);
}

void ROBOT ::  pauze ()
{
	DWORD eind_tijd = GetTickCount() + stap_tijd;

	while (GetTickCount() < eind_tijd)
		;
}

void ROBOT :: linksom ()
{
	huidige_richting = (Richting) ((huidige_richting + 1) % 4);
	herteken ();
}

void ROBOT :: rechtsom ()
{
	huidige_richting = (Richting) ((huidige_richting + 3) % 4);
	herteken ();
}

inline bool ROBOT :: bovenopBal ()
{
	return (Wereld [x_pos][y_pos] == Bal);
}

void ROBOT :: pakBal ()
{
	if (bovenopBal())
		Wereld [x_pos][y_pos] = Leeg;
	else
		throw IllegaleActie (IA_PakBal);
	herteken ();
}

void ROBOT :: legBal ()
{
	if (Wereld [x_pos][y_pos] == Leeg)
		Wereld [x_pos][y_pos]  = Bal;
	else
		throw IllegaleActie (IA_LegBal);
	herteken ();
}

void ROBOT :: stap	()
{
	Canvas canvas (karelsWereld());

	wisWereldDeel 	(canvas, x_pos - 1, y_pos - 1, x_pos + 1, y_pos + 1);
	tekenWereldDeel	(canvas, x_pos - 1, y_pos - 1, x_pos + 1, y_pos + 1);

	switch (huidige_richting)
	{
		case Noord:
			if (Wereld [x_pos][y_pos+1] == Muur)
				throw IllegaleActie (IA_Stap);
			else
				y_pos++;
			break;
		case Oost:
			if (Wereld [x_pos+1][y_pos] == Muur)
				throw IllegaleActie (IA_Stap);
			else
				x_pos++;
			break;
		case Zuid:
			if (Wereld [x_pos][y_pos-1] == Muur)
				throw IllegaleActie (IA_Stap);
			else
				y_pos--;
			break;
		case West:
			if (Wereld [x_pos-1][y_pos] == Muur)
				throw IllegaleActie (IA_Stap);
			else
				x_pos--;
			break;
	}

	tekenRobot		(canvas);
	pauze			();
}


bool ROBOT :: muurVoor	()
{
	switch (huidige_richting)
	{
		case Noord:
			return (Wereld [x_pos][y_pos+1] == Muur);
		case Oost:
			return (Wereld [x_pos+1][y_pos] == Muur);
		case Zuid:
			return (Wereld [x_pos][y_pos-1] == Muur);
		case West:
			return (Wereld [x_pos-1][y_pos] == Muur);
		default :				// dit stukje is toegevoegd door Ger
			return false ;		// om compiler-warnings te voorkomen
	}
}

bool ROBOT :: noord		()
{
	return huidige_richting == Noord;
}

void ROBOT :: rust (int r)
{
	stap_tijd = r;
}

GPOINT naarGPOINT (int x, int y)
{
	const int x_verpl = 12, y_verpl = 12;
	return GPOINT (x + x_verpl, SCREENHEIGHT - y - y_verpl);
}

GPOINT ROBOT :: schermPos (int sin_richting, int cos_richting, int hoek_nr)
{
	double x = x_pos * 2 * SteenGrootte + cos_richting * hoeken[hoek_nr][0] - sin_richting * hoeken[hoek_nr][1];
	double y = y_pos * 2 * SteenGrootte + sin_richting * hoeken[hoek_nr][0] + cos_richting * hoeken[hoek_nr][1];
	return naarGPOINT (int (x), int (y));
}

void ROBOT :: herteken (Canvas &canvas)
{
	wisWereldDeel 	(canvas, x_pos - 1, y_pos - 1, x_pos + 1, y_pos + 1);
	tekenWereldDeel	(canvas, x_pos - 1, y_pos - 1, x_pos + 1, y_pos + 1);
	tekenRobot		(canvas);
	pauze			();
}

void ROBOT :: herteken ()
{
	Canvas canvas (karelsWereld());
	herteken (canvas);
}

void ROBOT :: tekenRobot (Canvas &canvas)
{
	int sin_richting = 99, cos_richting =99 ;   // door Ger P: onmogelijke beginwaarden ivm compiler-warning

	switch (huidige_richting)
	{
		case Oost:
			sin_richting = -1;
			cos_richting = 0;
			break;
		case Zuid:
			sin_richting = 0;
			cos_richting = -1;
			break;
		case West:
			sin_richting = 1;
			cos_richting = 0;
			break;
		case Noord:
			sin_richting = 0;
			cos_richting = 1;
			break;
	}

	GPOINT ghoeken [4];

	for (int i = 0; i < 4; i++)
		ghoeken [i] = schermPos (sin_richting, cos_richting, i);

	canvas.setPenColour 	(RedRGB);
	canvas.fillPolygon		(ghoeken, 4);
}

void  ROBOT :: tekenRobot ()
{
	Canvas canvas (karelsWereld());
	tekenRobot (canvas);
}

void  ROBOT :: tekenSteen (Canvas &canvas, int x, int y)
{
	const GPOINT lo = naarGPOINT (x * SteenGrootte - SteenGrootte / 2, y * SteenGrootte - SteenGrootte / 2);
	canvas.fillRectangle (lo, GPOINT (lo.x + SteenGrootte,lo.y - SteenGrootte));
}

void  ROBOT :: tekenMuur (Canvas &canvas, int x, int y)
{
	canvas.setPenColour 	(BlueRGB);

	tekenSteen (canvas, x*2, y*2);
	if (x < WereldBreedte - 1 && Wereld [x+1][y] == Muur)
		tekenSteen (canvas, x*2+1, y*2);
	if (y < WereldHoogte - 1 && Wereld [x][y+1] == Muur)
		tekenSteen (canvas, x*2, y*2+1);
}

void  ROBOT :: tekenLeegVeld (Canvas &canvas, int x, int y)
{
	canvas.setPenColour (BlackRGB);
	canvas.setPenPos	(naarGPOINT (x*2*SteenGrootte, y*2*SteenGrootte));
	canvas.drawPOINT	();
}

void  ROBOT :: tekenBal (Canvas &canvas, int x, int y)
{
	canvas.setPenColour (GreenRGB);

	const GPOINT lo = naarGPOINT (x * 2 * SteenGrootte - BalGrootte / 2, y * 2 * SteenGrootte - BalGrootte / 2);
	canvas.fillOval (lo, GPOINT (lo.x + BalGrootte,lo.y - BalGrootte));
}

void ROBOT :: tekenWereld (Canvas& canvas)
{
	tekenWereldDeel (canvas, 0, 0, WereldBreedte-1, WereldHoogte-1);
}

void ROBOT :: wisWereldDeel (Canvas &canvas, int from_x, int from_y, int to_x, int to_y)
{
	const GPOINT lo = naarGPOINT (from_x * 2 * SteenGrootte, from_y * 2 * SteenGrootte - SteenGrootte / 2);
	const GPOINT rb = naarGPOINT (to_x * 2 * SteenGrootte, to_y * 2 * SteenGrootte + SteenGrootte / 2);

	canvas.setPenColour	(WhiteRGB);
	canvas.fillRectangle (lo, rb);
}

void ROBOT :: tekenWereldDeel (Canvas &canvas, int from_x, int from_y, int to_x, int to_y)
{

	for (int b = from_x; b <= to_x; b++)
	{	for (int h = from_y; h <= to_y; h++)
		{	switch (Wereld [b][h])
			{
				case Muur:
					tekenMuur		(canvas, b, h);
					break;
				case Leeg:
					tekenLeegVeld	(canvas, b, h);
					break;
				default:
					tekenBal		(canvas, b, h);
					break;
			}
		}
	}
}

void ROBOT :: nieuweWereld (const char wereld [])
{
	ifstream doolhof;

	doolhof.open (wereld);

	if (doolhof.fail ())
		throw IllegaleActie (IA_Open);
	else
	{
		int i, r, x_bal, y_bal, hori_muren, verti_muren;

		doolhof >> x_pos >> y_pos >> r >> x_bal >> y_bal >> hori_muren >> verti_muren;

		huidige_richting = Richting (r);

		maakWereldLeeg ();
		rechthoek (0,0,WereldBreedte,WereldHoogte,Muur,Wereld);

		Wereld [x_bal][y_bal] = Bal;

		for (i = 0; i < hori_muren; i++)
		{	int x, y, d;
			doolhof >> x >> y >> d;
			for (int j = 0; j < d; j++)
				Wereld [x+j][y] = Muur;
		}
		for (i = 0; i < verti_muren; i++)
		{	int x, y, d;
			doolhof >> x >> y >> d;
			for (int j = 0; j < d; j++)
				Wereld [x][y+j] = Muur;
		}

		doolhof.close ();

		Canvas canvas (karelsWereld());
		wisWereldDeel	(canvas, 0, 0, WereldBreedte-1, WereldHoogte-1);

		tekenWereld	(canvas);
		tekenRobot	(canvas);
	}
}

void ROBOT :: reset()
{
	huidige_richting = Oost;
	x_pos = xInit;
	y_pos = yInit;

	maakWereldLeeg ();
	rechthoek (0,0,WereldBreedte,WereldHoogte,Muur,Wereld);

	Canvas canvas (karelsWereld());
	wisWereldDeel	(canvas, 0, 0, WereldBreedte-1, WereldHoogte-1);

	tekenWereld	(canvas);
	tekenRobot	(canvas);
}

Robot& Karel ()			// delaratie van Karel als robot
{
	static Robot karel;
	return karel;
}

void RobotGUI :: Window (const RECT& area)
{
	Canvas canvas(karelsWereld());
	Karel().tekenWereld	(canvas);
	Karel().tekenRobot	(canvas);
}

RobotGUI :: RobotGUI()
	: GUI (GSIZE (SCREENWIDTH, SCREENHEIGHT), (char*)"Charles does his first steps")
{
	Karel();
}

RobotGUI& karelsWereld()
{
	static RobotGUI gui;
	return gui;
}

void stap			() { Karel().stap(); }
void linksom 		() { Karel().linksom(); }
void rechtsom 		() { Karel().rechtsom(); }

bool op_bal			() { return Karel().bovenopBal(); }

void pak_bal		() { Karel().pakBal(); }
void leg_bal		() { Karel().legBal(); }

bool muur_voor		() { return Karel().muurVoor(); }
bool noord			() { return Karel().noord(); }

void rust       	(int n) { Karel().rust(n); }

void maakWereldLeeg () { Karel().maakWereldLeeg(); }
void nieuweWereld	(const char wereld [])  { Karel().nieuweWereld(wereld); }
void reset			() { Karel().reset(); }
void stop 			() { karelsWereld().Stop();}


// For English version:
void step			() { Karel().stap(); }
void turn_left 		() { Karel().linksom(); }
void turn_right		() { Karel().rechtsom(); }
bool on_ball		() { return Karel().bovenopBal(); }
void get_ball		() { Karel().pakBal(); }
void put_ball		() { Karel().legBal(); }

bool in_front_of_wall    () { return Karel().muurVoor(); }
bool north			() { return Karel().noord(); }

void rest			(int n) { Karel().rust(n); }
void makeWorldEmpty () { Karel().maakWereldLeeg(); }
void newWorld	    (const char wereld [])  { Karel().nieuweWereld(wereld); }


int kies3 ()
{
	return rand () % 3 + 1;
}

// void veeg_schoon ()        { reset(); }

// toegevoegd door Ger Paulussen:
void stappen (int aantal_stappen)
{
	for (int teller = 1 ; teller <= aantal_stappen ; teller = teller + 1)
		stap () ;
}

void teken_ballenlijn ( int aantal_ballen )	// toegevoegd door Ger
{
	for (int teller = 1; teller<= aantal_ballen; teller++)
	{
		leg_bal () ;
		stap () ;
	}
}

void ROBOT :: plaatsRechthoek ( int links, int onder, int breedte, int hoogte )
{
	for (int hor = links; hor<=links+breedte; hor++)
	{
		Wereld [hor][onder]        = Muur ;
		Wereld [hor][onder+hoogte] = Muur ;
	};

	for (int vert = onder; vert<=onder+hoogte; vert++)
	{
		Wereld [links][vert]         = Muur ;
		Wereld [links+breedte][vert] = Muur ;
	};

	Canvas canvas (karelsWereld());
	tekenWereld (canvas);
}

void ROBOT :: maakMuur ( int links, int onder, int aantal, bool horizontaal )
{
    if (horizontaal)
    {
        for (int x = links ; x <= links + aantal ; x++)
        {
            Wereld [x][onder] = Muur ;
        }
    }
    else
    {
        for (int y = onder; y <= onder+aantal ; y++)
        {
            Wereld[links][y] = Muur ;
        }
    }
    Canvas canvas (karelsWereld()) ;
    tekenWereld (canvas) ;
    tekenRobot  (canvas) ;
}

void ROBOT :: maakBallenSnoer ()	// toegevoegd door Ger
{
	for (int hor = 1; hor<= WereldBreedte - 2; hor++)
	{
		Wereld [hor][1] = Bal ;
		Wereld [hor][WereldHoogte - 2] = Bal ;
	};

	for (int vert = 2; vert<=WereldHoogte - 2; vert++)
	{
		Wereld [1][vert] = Bal ;
		Wereld [WereldBreedte - 2][vert] = Bal ;
	};

	Canvas canvas (karelsWereld());
	tekenWereld (canvas);
}

bool voorkeurLinks ()
{
	return kies3 () == 1;
}

bool voorkeurRechts ()
{
	return kies3 () == 3;
}

void ROBOT :: maakBallenChaos ()	// toegevoegd door Ger
{
	const int aantal_keuzes	= 9;
	const int aantal_rijen		= aantal_keuzes * 2 + 1;

	int ballen [aantal_rijen];
	int i;
	for (i = 0; i < aantal_rijen; i++)
		ballen [i] = 0;

	for (i = 0; i < 40; i++)
	{	int bal = aantal_keuzes;
		for (int j = 0; j < aantal_keuzes ; j++)
			if (voorkeurLinks ())
				bal--;
			else if (voorkeurRechts ())
				bal++;
		ballen [bal]++;
	}

	for (i = 0; i < aantal_rijen; i++)
		for (int j = 0; j < ballen [i]; j++)
			Wereld [WereldBreedte - j - 2][WereldHoogte - i] = Bal;

	Canvas canvas (karelsWereld());
	tekenWereld (canvas);
}

void ROBOT :: maakBallenPad ()			// toegevoegd door Peter
{
	maakWereldLeeg ();
	rechthoek (0,0,WereldBreedte,WereldHoogte,Muur,Wereld);

	for (int ix = 1; ix < WereldBreedte-8; ix++)					Wereld[ix][WereldHoogte-2]  = Bal;
	for (int iy = WereldHoogte-2; iy > WereldHoogte/2; iy--)		Wereld[WereldBreedte-8][iy] = Bal;
	for (int ix = WereldBreedte-8; ix >= 1; ix--)	                Wereld[ix][WereldHoogte/2]  = Bal;
	for (int iy = WereldHoogte/2; iy >= 4; iy--)					Wereld[1][iy] = Bal;
	for (int ix = 1; ix <= WereldBreedte/3; ix++)		    		Wereld[ix][4]               = Bal;
	for (int iy = 4; iy <= WereldHoogte * 2 / 3; iy++)				Wereld[WereldBreedte/3][iy] = Bal;

	x_pos = 1;
	y_pos = WereldHoogte-2;

	Canvas canvas (karelsWereld());
	wisWereldDeel (canvas, 0, 0, WereldBreedte-1, WereldHoogte-1);
	tekenWereld (canvas);
	tekenRobot  (canvas);
}

void ROBOT :: grotwand (Richting richting)    // toegevoegd door Peter
{
    const int marge       = WereldHoogte / 3 ;
    const int max_breedte = WereldBreedte / 10 ;

    for (int ix = 2; ix <= WereldBreedte-3; )
    {
        const int breedte = rand() % (min (max_breedte, WereldBreedte - ix - 2)) + 1 ;
        const int dy = rand() % marge + 2 ;
        int y ;
        if (richting == Noord)
        {
            y = dy ;
        }
        else
        {
            y = WereldHoogte - dy - 1 ;
        }
        for (int i=1; i <= breedte; i++)
        {
            Wereld[ix][y] = Muur ;
            ix++ ;
        }
    }
}

void ROBOT :: maakGrot ()               // toegevoegd door Peter
{
    maakWereldLeeg () ;
    rechthoek (0,0, WereldBreedte,WereldHoogte,Muur,Wereld);
    grotwand (Noord) ;
    grotwand (Zuid) ;
	x_pos = 1;
	y_pos = WereldHoogte-2;
    Canvas canvas (karelsWereld());
    wisWereldDeel (canvas, 0,0, WereldBreedte-1, WereldHoogte-1);
    tekenWereld (canvas) ;
    tekenRobot  (canvas) ;
}

void ROBOT :: creeerBal (int x, int y)
{
    if (x >= 0 && x < WereldBreedte && y >= 0 && y < WereldHoogte)
    {
        Wereld[x][y] = Bal;
        Canvas canvas (karelsWereld());
        tekenWereld (canvas);
        tekenRobot  (canvas);
    }
}

// For English version:

void place_rectangle        (int left, int bottom, int width, int height)  { Karel().plaatsRechthoek(left,bottom,width,height); }
void place_walls            (int left, int bottom, int nr_of_walls, bool horizontal) { Karel().maakMuur (left,bottom,nr_of_walls,horizontal); }
void make_string_with_balls ()  { Karel().maakBallenSnoer (); }
void make_chaos_with_balls  ()  { Karel().maakBallenChaos (); }
void make_path_with_balls   ()  { Karel().maakBallenPad (); }
void make_cave              ()  { Karel().maakGrot (); }
void make_labyrinth         ()  { Karel().nieuweWereld ( "labyrinth" ) ; }
void create_ball            (int x, int y) { Karel().creeerBal(x,y); }

void steps (int number_of_steps) { stappen ( number_of_steps ) ; }
void draw_line_with_balls ( int number_of_steps )  { teken_ballenlijn ( number_of_steps ); }	// toegevoegd door Ger
