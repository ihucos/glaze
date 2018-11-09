#include<stdio.h>


#define IDX_SIZE(x) ((sizeof(x) / sizeof((x)[0])) - 1)
#define IDX_MASK(buffer) ((size_t)IDX_SIZE(buffer) - 1)
#define IDX(buffer, index) buffer[(size_t)index & IDX_MASK(buffer)]
#define IDX_INDEX(buffer) buffer[IDX_SIZE(buffer)]
#define IDX_RESET(buffer) {IDX_INDEX(buffer) = 0;}
#define IDX_PUSH(buffer, element) {IDX_NEXT(buffer) = element;}
#define IDX_POP(buffer) buffer[IDX_SIZE(buffer)]--;
#define IDX_CURRENT(buffer) buffer[IDX_INDEX(buffer)]
#define IDX_NEXT(buffer) buffer[IDX_INDEX(buffer)++]



int main() {
        char a[16+1];
        IDX_RESET(a)

        for (int i=0; i <= 14; i++){
	        IDX_PUSH(a, i+10)
        }
        
        IDX_RESET(a)
        for (int i=0; i <= 14; i++){
	        printf("%u ", IDX_NEXT(a));
        }
	printf("\n");



        IDX_POP(a);
        IDX_POP(a);
        IDX_POP(a);
        IDX_POP(a);
        IDX_POP(a);
        IDX_POP(a);
        IDX_POP(a);
	IDX_PUSH(a, (char)99)

        for (int i=0; i <= 15; i++){
	        printf("%u ", (char)IDX(a, i));
        }
	printf("\n");
	return 0;
}
