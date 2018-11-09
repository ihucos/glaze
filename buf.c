#include<stdio.h>


#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
#define IDX_MASK(buffer) ((size_t)NELEMS(buffer) - 1)
#define IDX(buffer, index) buffer[(size_t)index & IDX_MASK(buffer)]
#define IDX_INDEX(buffer) buffer[NELEMS(buffer)-1]

#define IDX_RESET(buffer) {IDX_INDEX(buffer) = 0;}
#define IDX_PUSH(buffer, element) {buffer[IDX_INDEX(buffer)++] = element;}
#define IDX_POP(buffer) a[NELEMS(a)-1]--;
#define IDX_NEXT(buffer) buffer[IDX_INDEX(buffer)++]



int main() {
        char a[16];
        IDX_RESET(a)

        for (int i=0; i <= 14; i++){
	        IDX_PUSH(a, i)
        }
        IDX_POP();
        IDX_POP();
        IDX_POP();
        IDX_POP();
        IDX_POP();
        IDX_POP();
        IDX_POP();
	IDX_PUSH(a, (char)99)

        for (int i=0; i <= 15; i++){
	        printf("%u ", (char)IDX(a, i));
        }
	printf("\n");
	return 0;
}
