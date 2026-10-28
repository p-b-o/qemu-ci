#include<stdio.h>
#include<assert.h>

int main()
{
    int achi, acho;
    int result;

    __asm("wrdsp $zero, 0x3f\n\t");

    achi   = 0x004433;
    result = 0x004433;

    __asm
        ("mthi %1, $ac1\n\t"
         "mfhi %0, $ac1\n\t"
         : "=r"(acho)
         : "r"(achi)
        );
    assert(result == acho);

    return 0;
}
