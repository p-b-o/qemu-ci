#include<stdio.h>
#include<assert.h>

int main()
{
    int rd, result;

    __asm("wrdsp $zero, 0x3f\n\t");

    result = 0xBFBFBFBF;
    __asm
        ("repl.qb %0, 0xBF\n\t"
         : "=r"(rd)
        );
    assert(rd == result);

    return 0;
}
