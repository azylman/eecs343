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

#include <stdio.h>
#include <stdlib.h>

/*
 * ASCII escape sequences used:
 *  Move cursor to home      \033[H
 *  Blue                     \033[44m
 *  Turn off attributes      \033[m
 *  Move cursor to next line \033[E
 * Color escape sequences should be followed by 'm'
 *
 */ 
void 
show(void *u, int w, int h)
{
  int x, y;
  int (*univ)[w] = u;
  printf("\033[H");
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++)
      printf(univ[y][x] ? "\033[44m  \033[m" : "  ");
    printf("\033[E");
  }
  fflush(stdout);
} /* show */
 
/*
 * evolve:
 *  Twisted implementation of game logic
 *
 */
void 
evolve(void *u, int w, int h)
{
  int x, y, x1, y1;
  unsigned (*univ)[w] = u;
  unsigned new[h][w];
  
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int alive = univ[y][x];
      int n = 0;
      for (y1 = 0; y1 < h; y1++) {
        for (x1 = 0; x1 < w; x1++) {
          if (y1 == y && x1 == x) continue;
          if (abs(y1 - y) <= 1 && abs(x1 - x) <= 1) {
            if (univ[y1][x1]) {
              n++;
            }
          }
        }
      }
      
      if (alive) {
        if (n < 2 || n > 3) {
          new[y][x] = 0;
        } else {
          new[y][x] = 1;
        }
      } else {
        if (n == 3) {
          new[y][x] = 1;
        } else {
          new[y][x] = 0;
        }
      }
    }
  }

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) 
      univ[y][x] = new[y][x];
} /* evolve */

