#include "Robot.h"
/*
	Karel is a framework for manipulating with Karel the robot.
	Karel is a subclass of ROBOT from the library Robot.
	Last Modified:	September 16 2006, by Pieter Koopman, RU Nijmegen
	With some additions: September 12 2007, by Ger Paulussen & Peter Achten
	More additions: August 21 2013, by Peter Achten
*/


// =========================================================================

// make_church creates a Church-like shape of walls
void make_church ()
{
    const int lane   = 2 + rand () % (WereldHoogte / 5) ;
    const int street = 2 + rand () % (WereldBreedte / 5) ;
    const int width  = 3 + rand () % (WereldBreedte / 2) ;
    const int height = 2 + rand () % (WereldHoogte / 3) ;
    place_rectangle (street,lane,width,height) ;
    place_rectangle (street + 2,lane + height, 2 + width / 3, 2 + height / 3) ;
    place_walls (street + 3 + width / 6,lane + height + height / 3 + 3,4,false) ;
    place_walls (street + 2 + width / 6,lane + height + height / 3 + 5,2,true) ;
    create_ball (street, WereldHoogte - 2) ;
}


// here starts the part with code to be completed by the students

// Name / student number / study student 1 :
// Name / student number / study student 2 :

// give one or more comment lines about what will happen in this function
void follow_path ()
{
    // give your own code completion
}

// give one or more comment lines about what will happen in this function
void hansl_and_gretl ()
{
	make_path_with_balls() ;
	follow_path () ;
}

// give one or more comment lines about what will happen in this function
// note that you are allowed to add formal parameters to fill_cave_with_balls if that is necessary for your solution
void fill_cave_with_balls ()
{
    // give your own code completion
}

// give one or more comment lines about what will happen in this function
void cave ()
{
	// if necessary for your solution, you are allowed to give actual parameters to fill_cave_with_balls
	fill_cave_with_balls () ;
	fill_cave_with_balls () ;
}

// give one or more comment lines about what will happen in this function
void start_cave ()
{
    make_cave () ;
    cave () ;
}

// give one or more comment lines about what will happen in this function
void rondje_om_de_kerk ()
{
    make_church () ;
    // give your own code completion

}


// For testing purposes, you can define your own function here:
void test(){
}



void nieuwe_bal_zoeken ()
{
    //1e poging
    turn_left();

    if (!in_front_of_wall()){
        step();
        //2e poging
        if (!on_ball()){
        turn_left();
        turn_left();
        step(); step();

        if (!on_ball()){
            exit(1);
            //return;
        }
        }
    }

    else{
        turn_left();
        turn_left();
        step();

        if (!on_ball())
        {
        turn_left();
        turn_left();
        step();
        }

        if (!on_ball()){
            stop();
        }


    }
}


void Opdracht1 ()
{
        // enter your Charles code here
    if (on_ball()){
        if (in_front_of_wall()){
            nieuwe_bal_zoeken();
            Opdracht1();
        }

        else{
            step();
            Opdracht1();
        }
    }

    while (!on_ball()){
        turn_left();
        turn_left();
        step();
        nieuwe_bal_zoeken();
        if (on_ball()){
            Opdracht1();
        }
        else{
            stop();
        }
    }
}

void move_and_place()
{
  while (!in_front_of_wall())
  {
    //moving and placing
    put_ball();
    step();
  }
}
void just_move()
{
  //charles is just moving
  while (!in_front_of_wall())
  {
    step();
  }
}
void compact()
{
  move_and_place();
  if (in_front_of_wall())
  {
    //place final ball and do a 180
    place_ball();
    turn_left();
    turn_left();
    //get back
    just_move();
    if (in_front_of_wall())
    {
      turn_right();
      if (in_front_of_wall())
      {
        turn_right();
        just_move();
        if ((in_front_of_wall()) && (!on_ball()))
        {
          turn_right();
          step();
          turn_right();
          compact();
        }
        if ((in_front_of_wall()) && (on_ball()))
        {
          //we are done
        }
      }
      else
      {
        turn_right();
        step();
        turn_right();
        compact();
      }
    }
  }
}

void enter_cave()
{
  step();
  turn_right();
}

void execution()
{
  enter_cave();
  compact();
}

void kerk_zoeken(){
    while (!in_front_of_wall()){
            step();
        }
        if (in_front_of_wall()){
            stop();
        }
}

void rondje_kerk(){
    //is muur rechts van karel?

    while (in_front_of_wall()){
        turn_left();
    }
    while (!in_front_of_wall()){
        step();
        turn_left();
    }
}

void Bonus(){

    //karel zoekt bal
    while (!on_ball){
        step();
    }

    //karel zoekt kerk
    if (on_ball()){
        turn_right();
        kerk_zoeken();
        rondje_kerk();
        stop();
    }
}

// end of part with code to be completed by students
// =========================================================================


void quick  () { rest(    1); };
void normal () { rest(0.5); };
void slow   () { rest(  250); };
void very_slow  () { rest( 1000); };

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    Menu charles_menu ((char*)"Charles") ;
    charles_menu.add ((char*)"Clean", reset)
                .add ((char*)"Stop",  stop) ;

	Menu a1_menu ((char*)"Assignment 2");
	a1_menu.add ((char*)"Hansl and Gretl", hansl_and_gretl )
		   .add ((char*)"Cave", start_cave )
		   .add ((char*)"Bonus: rondje om de kerk...", rondje_om_de_kerk )
	       .add ((char*)"Test a function",test)
	       .add ((char*)"Opdracht 1",execution);

	Menu sn_menu ((char*)"Velocity");
	sn_menu.add ((char*)"Quick", quick)
		   .add ((char*)"Normal",normal)
		   .add ((char*)"Slow",slow)
		   .add ((char*)"Very slow",very_slow);

    Menu o1_menu ((char*)"OPDRACHTEN");
	o1_menu.add ((char*)"Opdracht 1", Opdracht1 )
		   .add ((char*)"Opdracht 2", execution )
		   .add ((char*)"BONUS", Bonus )
	       .add ((char*)"Test a function",test);

	try
	{
		karelsWereld().Run (charles_menu,WINARGS(hInstance, hPrevInstance, szCmdLine, iCmdShow));
	}
	catch (IllegaleActie dezeIllegaleActie )
	{
		dezeIllegaleActie.report ();
	}
	catch (...)
	{
		makeAlert ("Something went terribly wrong!");
	}

	return 0;
}
