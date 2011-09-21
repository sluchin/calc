/*
  calc.h

  typedef struct _ut_calc {
  int (*get_digit)(const dbl val, const char *fmt);
  } ut_calc;

  void ut_calc_initialize(ut_calc *ts);

  calc.c

  void ut_calc_initialize(ut_calc *ts)
  {
  ts->get_digit = &get_digit;
  }

*/
#include <stdio.h>  /* printf */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <float.h>  /* LDBL_MAX */
#include <libgen.h> /* basename */

#include "calc.h"
#include "def.h"



int main(int argc, char *argv[])
{
    ut_calc ts;

    ut_calc_initialize(&ts);

    int result = 0;
    dbl val = DBL_MAX;
    char *fmt = "%.18Lg";

    result = ts.get_digit(val, fmt);

    printf("result=%d\n", result);

    return EXIT_SUCCESS;
}

