#include<stdio.h>
#include<assert.h>

int main()
{
    int rd, rt;
    int result;

    __asm("wrdsp $zero, 0x3f\n\t");

    rt = 0x87654321;
    result = 0x32801080;

    __asm
        ("precequ.ph.qbra %0, %1\n\t"
         : "=r"(rd)
         : "r"(rt)
        );
    assert(result == rd);

    return 0;
}
