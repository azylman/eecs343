/* -*-C-*-
*******************************************************************************
*
* File:         life2.c
* RCS:          $Id: $
* Description:  
* Author:       Fabian E. Bustamante
*               AquaLab Research Group
*               Department of Electrical Engineering and Computer Science
*               Northwestern University
* Created:      Wed Sep 14, 2011 at 16:47:00
* Modified:     Thu Sep 15, 2011 at 09:50:00 fabianb@eecs.northwestern.edu
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (C) Copyright 2011, Northwestern University, all rights reserved.
*
*******************************************************************************
*/

/*
 * The Game of Life is a cellular automaton devised by the British
 * mathematician John Horton Conway in 1970. It is the best-known
 * example of a cellular automaton.
 *
 * Conway's game of life is described here:
 *   http://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
 *
 * A cell C is represented by a 1 when alive or 0 when dead, in an
 * m-by-m square array of cells. We calculate N - the sum of live cells
 * in C's eight-location neighbourhood, then cell C is alive or dead in
 * the next generation based on the following table:
 *
 *  C   N                 new C
 *  1   0,1             ->  0  # Lonely
 *  1   4,5,6,7,8       ->  0  # Overcrowded
 *  1   2,3             ->  1  # Lives
 *  0   3               ->  1  # It takes three to give birth!
 *  0   0,1,2,4,5,6,7,8 ->  0  # Barren
 *
 * Assume cells beyond the boundary are always dead.
 *
 * The "game" is actually a zero-player game, meaning that its
 * evolution is determined by its initial state, needing no input from
 * human players. One interacts with the Game of Life by creating an
 * initial configuration and observing how it evolves.
 *
 * This works on a random array but you should test your implementation
 * on more interesting things. Look at the game's page.
 *
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "tools/tools.h"

#define SLEEPT 200000

int keep_playing = 1;

/*
 * handler:
 *   Deal with SIGINTs.
 *
 */
void handler(int signo) {
    switch(signo) {
        case SIGINT:
            keep_playing = 0;
            break;
    }
} /* handler */

/*
 * game:
 *   All the action is here; creates and manipulates the array
 *   
 */
void 
game(int w, int h)
{
  int x, y, rounds, round;
  unsigned univ[h][w];
  
  for (x = 0; x < w; x++) {
    for (y = 0; y < h; y++) {
      rounds = rand() % 200000;
      for (round = 0; round < rounds; round++) {
        univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
      }
    }
  }

  while (keep_playing) {
    show(univ, w, h);
    evolve(univ, w, h);
    usleep(SLEEPT);
  }
} /* game */
 
/*
 *
 *
 */
int 
main(int argc, char **argv)
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &handler;
  sigaction(SIGINT, &sa, NULL);

  srand(time(NULL));

  int w = 0, h = 0;

  if (argc > 1) w = atoi(argv[1]);
  if (argc > 2) h = atoi(argv[2]);
  if (w <= 0) w = 40;
  if (h <= 0) h = 40;
  game(w, h);
  exit(EXIT_SUCCESS);
} /* main */
