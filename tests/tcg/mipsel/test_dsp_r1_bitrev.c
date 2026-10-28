#include<stdio.h>
#include<assert.h>

int main()
{
    int rd, rt;
    int result;

    __asm("wrdsp $zero, 0x3f\n\t");

    rt     = 0x12345678;
    result = 0x00001E6A;

    __asm
        ("bitrev %0, %1\n\t"
         : "=r"(rd)
         : "r"(rt)
        );
    assert(rd == result);

    return 0;
}
