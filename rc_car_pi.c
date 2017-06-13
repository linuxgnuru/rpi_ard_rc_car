#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // unsure
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h> // unsure
#include <libgen.h> // unsure
#include <errno.h>
#include <signal.h>
#include <ncurses.h>

#include <wiringPi.h>

#include "rc_car_pi.h"

static const char *devName = "/dev/i2c-1";
int i2c_file;

void printCommand();
void clearLine(int row);
static void die(int sig);

int main(int argc, char **argv)
{
  int key;
  unsigned char cmd[16];

  // note: we're assuming BSD-style reliable signals here
  (void)signal(SIGINT, die);
  (void)signal(SIGHUP, die);
  (void)signal(SIGTERM, die);
  if ((i2c_file = open(devName, O_RDWR)) < 0)
  {
    fprintf(stderr, "[%d] [%s] [%s] I2C: Failed to access %s\n", __LINE__, __FILE__, __func__, devName);
    exit(1);
  }
  if (ioctl(i2c_file, I2C_SLAVE, ADDRESS) < 0)
  {
    fprintf(stderr, "[%d] [%s] [%s] I2C: Failed to acquire bus access/talk to slave 0x%x\n", __LINE__, __FILE__, __func__, ADDRESS);
    exit(1);
  }
  if (wiringPiSetup() == -1)
  {
    fprintf(stdout, "[%d] [%s] [%s] Error trying to setup wiringPi - oops: %s\n", __LINE__, __FILE__, __func__, strerror(errno));
    exit(1);
  }
  initscr();
  if (has_colors() == FALSE)
  {
    endwin();
    fprintf(stdout, "[%d] [%s] [%s] Your terminal does not support color", __LINE__, __FILE__, __func__);
    exit(1);
  }
  use_default_colors();
  // Start color
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  noecho();
  cbreak();
  nodelay(stdscr, true);
  curs_set(0);
  move(0, 2);
  printw("I2C: Acquiring bus to 0x%x", ADDRESS);
  printCommand();
  cmd[0] = 255;
  while (1)
  {
    key = getch();
    if (key > -1)
    {
      switch (key)
      {
        case '0': cmd[0] = CHECK_SONAR; break;
        case '1': cmd[0] = CHECK_SONAR_LEFT; break;
        case '2': cmd[0] = CHECK_SONAR_CENTER; break;
        case '3': cmd[0] = CHECK_SONAR_RIGHT; break;
        case '4': cmd[0] = GO_RIGHT; break;
        case '5': cmd[0] = GO_CENTER; break;
        case '6': cmd[0] = GO_LEFT; break;
        case '7': cmd[0] = GET_DIR; break;
                  //case '8': cmd[0] = CHECK_BATT; break;
        default: cmd[0] = 255; break;
      }
      if (key == 'q')
        break;
    }
    if (cmd[0] != 255 && key != 'q')
    {
      if (write(i2c_file, cmd, 1) == 1)
      {
        char buf[1];
        if (read(i2c_file, buf, 1) == 1)
        {
          int temp = (int)buf[0];
          switch (temp)
          {
            case BAD_:         clearLine(14); printw("bad"); break;
            case OK_:          clearLine(15); printw("OK"); break;
            case SONAR_LEFT:   clearLine(16); printw("left sonar hit something"); break;
            case SONAR_CENTER: clearLine(17); printw("center sonar hit something"); break;
            case SONAR_RIGHT:  clearLine(18); printw("right sonar hit something"); break;
            case NOCHANGE_:    clearLine(19); printw("position no change"); break;
            case RIGHT_:       clearLine(20); printw("position right"); break;
            case CENTER_:      clearLine(22); printw("position center"); break;
            case LEFT_:        clearLine(21); printw("position left"); break;
            case FAR_RIGHT_:   clearLine(23); printw("position far right"); break;
            case FAR_LEFT_:    clearLine(24); printw("position far left"); break;
            default:           clearLine(26); printw("unknown [%d]", temp); break;
          }
        } // end read
      } // end write
      usleep(10000);
      //move(27, 2);
      attron(COLOR_PAIR(1));
      mvprintw(27, 2, "command sent: [%d]", cmd[0]);
      attroff(COLOR_PAIR(1));
      refresh();
    } // end if cmd[0] != 255
  } // end while(1)
  close(i2c_file);
  endwin();
  return EXIT_SUCCESS;
}

static void die(int sig)
{
  if (sig != 0 && sig != 2)
    (void)fprintf(stderr, "caught signal %s\n", strsignal(sig));
  if (sig == 2)
    (void)fprintf(stderr, "Exiting due to Ctrl + C\n");
  close(i2c_file);
  endwin();
  exit(0);
}

void printCommand()
{
  int r = 2;

  mvaddstr(r++, 2, "Commands:");
  mvaddstr(r++, 2, "0 - check sonar");
  mvaddstr(r++, 2, "1 - check sonar 0");
  mvaddstr(r++, 2, "2 - check sonar 1");
  mvaddstr(r++, 2, "3 - check sonar 2");
  mvaddstr(r++, 2, "4 - go right");
  mvaddstr(r++, 2, "5 - go center");
  mvaddstr(r++, 2, "6 - go left");
  mvaddstr(r++, 2, "7 - get direction");
//  mvaddstr(r++, 2, "8 - check battery");
  mvaddstr(r++, 2, "q - quit");
  mvaddstr(r++, 2, "Data");
  refresh();
}

void clearLine(int row)
{
  int i;

  if (row < 0)
    return;
  for (i = 14; i < LAST_LINE; i++)
  {
    if (i != row)
      mvaddstr(i, 2, "                            ");
  }
  //refresh();
  move(row, 2);
}

