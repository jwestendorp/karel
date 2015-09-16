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

// Jelmar Gerritsen / s4636686 / study student 1 :
// Jonas Westendorp / student number / study student 2 :

/////////////////////////////////////////////////////////////////////////////
//
//
//    Code is down below in separate voids
//
//
/////////////////////////////////////////////////////////////////////////////

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


///////////////////////////////////
// Own code starts here
// Assignment voids are Opdracht1, Opdracht2 and Bonus
// located in submenu "OPDRACHTEN"
//////////////////////////////////
void nieuwe_bal_zoeken ()
{
    //check left
    turn_left();
    //if there is no wall, take a step
    if (!in_front_of_wall())
    {
        step();
        //then check if we are on a ball
        if (!on_ball())
        {
        //if we aren't, do a 180 degree turn and take two steps
        turn_left();
        turn_left();
        step(); step();
        //if there's no ball there, we checked east north and west. the trail has ended.
        if (!on_ball())
        {
            //we are done
            //terminate the process
            exit(0);
        }
        }
    }
    //if there is a wall to the left, do a 180 and take a step
    else
    {
        turn_left();
        turn_left();
        step();
        //if there is no ball there
        if (!on_ball())
        {
        turn_left();
        turn_left();
        step();
        }

        if (!on_ball()){
            //we are done
            //terminate the process
            exit(0);
        }


    }
}

///////////////////////////////////////////////
// this is the main functon for assignment 1 //
///////////////////////////////////////////////

void Opdracht1()
{
    // if we are on a ball
    if (on_ball())
    {
        if (in_front_of_wall())
        {
            //if there's a wall, we need to check for the next ball which is not in front of us,
            //after which we will check again
            nieuwe_bal_zoeken();
            Opdracht1();
        }
        //and not in front of a wall
        else
        {
            //take a step and check again
            step();
            Opdracht1();
        }
    }
    //while we are not on a ball
    while (!on_ball())
    {
      //do a 180 and take a step back, then look for the next ball
        turn_left();
        turn_left();
        step();
        nieuwe_bal_zoeken();
        //if we find it, start stepping again
        if (on_ball()){
            Opdracht1();
        }
        else{
            //we can't find the next ball, the trail has ended
            //process will be terminated
        }
    }
}

// below are the functions for assignment 2

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
  //move and place balls
  move_and_place();
  //until we hit a wall
  if (in_front_of_wall())
  {
    //place final ball and do a 180
    put_ball();
    turn_left();
    turn_left();
    //get back
    just_move();
    //then if we hit a wall again
    if (in_front_of_wall())
    {
      //get into position again
      turn_right();
      step();
      //if there's a wall there, we are at a side wall (east or west)
      if (in_front_of_wall())
      {
        //which means we turn right and just walk down/up
        turn_right();
        just_move();
        //after which we get into position again
        turn_right();
        step();
        //if we find a ball, we are back at the starting position, which means
        if (on_ball())
        {
          //we are done
        }
        else
        {
          //if there is no ball, we haven't been here before. get into position and start filling
            turn_right();
            compact();
        }
      }
      else
      {
        turn_right();
        compact();
      }
    }
  }
}

void enter_cave()
{
  //get into the right position
  step();
  turn_right();
}

///////////////////////////////////////////////
// this is the main functon for assignment 2 //
///////////////////////////////////////////////

void Opdracht2()
{
  enter_cave();
  compact();
}

//below are the bonus functions

void rondjes()
{
  //while we are not at the reference ball
    while (!on_ball())
    {
      //and in front of a wall
        while (in_front_of_wall())
        {
          //check if there is a wall to the right
            turn_right();
            //if there isn't, step, and turn left to check for a wall again
            if (!in_front_of_wall())
            {
                step();
                turn_left();
                rondjes();
            }
            //if there is a wall, step to the next tile and check again
            if (in_front_of_wall())
            {
                turn_right();
                step();
                turn_left();
            }
        }
        while (!in_front_of_wall())
        {
          //if we find a tile which is not adjacent to a wall, step and look to the left for a wall
            step();
            turn_left();
            rondjes();
        }

    }
    //if we hit the reference ball, initiate leaving procedure
    if (on_ball())
    {
      //move until we find the western wall
        turn_right();
        just_move();
      //then move until we find the top wall
        turn_right();
        just_move();
      //we are done
    }

}

///////////////////////////////////////////////////////
// this is the main functon for the bonus assignment //
///////////////////////////////////////////////////////

void Bonus()
{
  //while we are not yet at the first ball that indicates the location of the church
    while (!on_ball())
    {
      //keep stepping
        step();
    }
    //when we hit it, turn right
    if (on_ball())
    {
        turn_right();
        //and head down until we hit the church
        while (!in_front_of_wall())
        {
            step();
        }
        //if we hit it, put a ball for reference and start circling the church
        if (in_front_of_wall())
        {
            put_ball();
            //take one step to not trigger the leaving procedure immediately
            turn_right();
            step();
            turn_left();
            //start circling
            rondjes();
        }
    }
}


// end of part with code to be completed by students
// =========================================================================


void quick  () { rest(0.5); };
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
	       .add ((char*)"Opdracht 1",Opdracht2);

	Menu sn_menu ((char*)"Velocity");
	sn_menu.add ((char*)"Quick", quick)
		   .add ((char*)"Normal",normal)
		   .add ((char*)"Slow",slow)
		   .add ((char*)"Very slow",very_slow);

    Menu o1_menu ((char*)"OPDRACHTEN");
	o1_menu.add ((char*)"Opdracht 1", Opdracht1)
		   .add ((char*)"Opdracht 2", Opdracht2)
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
