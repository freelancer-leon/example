/*
 * Build with PLT need to compile with:
 *     gcc -Wall -g -z lazy -o plt plt.c -lm
 *
 * Build by default will use full RELRO linker keyword
 * https://codeantenna.com/a/l2FxBmeZdA
 */

#include <stdio.h>
#include <math.h>

int main()
{
  double d = 9;

  d = sqrt(d);
  printf("d = %f\n", d);

  return 0;
}
