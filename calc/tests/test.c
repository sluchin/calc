#include <stdio.h>  /* printf */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <float.h>  /* LDBL_MAX */
#include <libgen.h> /* basename */

//#include "calc.h"
#include "def.h"

static dbl get_pi(void);
static dbl get_ln(dbl x, dbl y);

typedef union union_func {
    dbl (*get_pi)(void);
    dbl (*get_ln)(dbl x, dbl y);
} union_func;

dbl get_pi(void)
{
    dbl x = 0;
    return x;
}

dbl get_ln(dbl x, dbl y)
{
    return x;
}

void
init_func(union_func *f[])
{
    f[0]->get_pi = get_pi;
    f[1]->get_ln = get_ln;
}

int main(int argc, char *argv[])
{

    return EXIT_SUCCESS;
}

