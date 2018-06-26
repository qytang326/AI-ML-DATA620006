#include "pisqpipe.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <iostream>
#include "myAI.h"

const char *infotext="name=\"Random\", author=\"Quanyin Tang\", version=\"0.1\", country=\"China\", www=\"https://qytang326.github.io\"";

#define MAX_BOARD 20
int board[MAX_BOARD][MAX_BOARD];
static unsigned seed;
int totalstep=0;

void brain_init()
{
  if(width<5 || height<5){
    pipeOut("ERROR size of the board");
    return;
  }
  if(width>MAX_BOARD || height>MAX_BOARD){
    pipeOut("ERROR Maximal board size is %d", MAX_BOARD);
    return;
  }
  restart(width,height);
  seed=start_time;
  pipeOut("OK");
}

void brain_restart()
{
  int x,y;
  for(x=0; x<width; x++){
    for(y=0; y<height; y++){
      board[x][y]=0;
    }
  }
  totalstep=0;
  restart(width,height);
  pipeOut("OK");
}

int isFree(int x, int y)
{
  return x>=0 && y>=0 && x<width && y<height && board[x][y]==0;
}

void brain_my(int x,int y)
{
  if(isFree(x,y)){
    board[x][y]=1;
    totalstep++;
    makemove(x+1,y+1,totalstep);
  }
  else{
    pipeOut("ERROR my move [%d,%d]",x,y);
  }
}

void brain_opponents(int x,int y)
{
  if(isFree(x,y)){
    board[x][y]=2;
    totalstep++;
    makemove(x+1,y+1,totalstep);
  }else{
    pipeOut("ERROR opponents's move [%d,%d]",x,y);
  }
}

void brain_block(int x,int y)
{
  if(isFree(x,y)){
    board[x][y]=3;
  }else{
    pipeOut("ERROR winning move [%d,%d]",x,y);
  }
}

int brain_takeback(int x,int y)
{
  if(x>=0 && y>=0 && x<width && y<height && board[x][y]!=0){
    board[x][y]=0;
    totalstep--;
    de_move(x+1,y+1,totalstep);
    return 0;
  }
  return 2;
}

unsigned rnd(unsigned n)
{
  seed=seed*367413989+174680251;
  return (unsigned)(UInt32x32To64(n,seed)>>32);
}

void brain_turn()
{
    set_engine_start_time(start_time);
    int x,y;
    set_engine_timeout_left(info_time_left);
    AI(x,y);
   // if(terminateAI) return;

  if(isFree(x-1,y-1)){do_mymove(x-1,y-1); }
  else{
    pipeOut("ERROR my move [%d,%d]",x,y);
  }

  //if(i>1) pipeOut("DEBUG %d coordinates didn't hit an empty field",i);

}

void brain_end()
{
    quit_game();
}

#ifdef DEBUG_EVAL
#include <windows.h>

void brain_eval(int x,int y)
{
  HDC dc;
  HWND wnd;
  RECT rc;
  char c;
  wnd=GetForegroundWindow();
  dc= GetDC(wnd);
  GetClientRect(wnd,&rc);
  c=(char)(board[x][y]+'0');
  TextOut(dc, rc.right-15, 3, &c, 1);
  ReleaseDC(wnd,dc);
}

#endif








/*DEBUG:::
    FILE *out;
    if((out = fopen("S:\\log.txt", "a")) != NULL)
    {
        fprintf(out,"Start thinking:totalstep=%d\n",totalstep);
    }
    fclose(out);
*/