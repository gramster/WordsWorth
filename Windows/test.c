#include <stdio.h>

#define X	1

#define Y	(X)
#undef X
#define X	(Y+1)
#undef Y

#define Y	(X)
#undef X
#define X	(Y+1)
#undef Y

#define Y	(X)
#undef X
#define X	(Y+1)
#undef Y

main()
{
    printf("X is %d\n", X);
}

