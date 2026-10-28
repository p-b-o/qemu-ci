#include<stdio.h>
#include<assert.h>

int main()
{
    int acli, aclo;
    int result;

    __asm("wrdsp $zero, 0x3f\n\t");

    acli   = 0x004433;
    result = 0x004433;

    __asm
        ("mthi %1, $ac1\n\t"
         "mfhi %0, $ac1\n\t"
         : "=r"(aclo)
         : "r"(acli)
        );
    assert(result == aclo);

    return 0;
}
