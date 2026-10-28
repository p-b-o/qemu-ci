#include<stdio.h>
#include<assert.h>

int main()
{
    int rd, rt;
    int result;

    __asm("wrdsp $zero, 0x3f\n\t");

    rt = 0x87654321;
    result = 0x00870043;

    __asm
        ("preceu.ph.qbla %0, %1\n\t"
         : "=r"(rd)
         : "r"(rt)
        );
    assert(result == rd);

    return 0;
}
