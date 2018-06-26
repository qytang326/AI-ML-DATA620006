#ifndef MYAI_H
#define MYAI_H
void restart(int width,int height);
void makemove(int x,int y,int totalstep);
void de_move(int x,int y,int totalstep);
void quit_game();
void set_timeout_match(int info_timeout_match);
void set_timeout_turn(int info_timeout_turn);
void set_timeout_left(int info_timeout_left);
void set_engine_start_time(int start_time);
void set_engine_timeout_left(int timeout_left);
void count_timeout_left(int timeelapsed);
void AI(int &move_x,int &move_y);
#endif
