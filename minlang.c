#include <stdlib.h>
#include <stdio.h>

size_t VAL=0;
size_t SUB=1;
size_t args[99];

size_t add(size_t *arg){
        return arg[0] + arg[1];
}

size_t mul(size_t *arg){
        return arg[0] * arg[1];
}

size_t eval(size_t[] code){
        size_t count = code[0];
        size_t (*fc)();
        fc = (void*)code[1];
        for(size_t c; c < count; c++){
               (code + 2)[(c * 2) - 1] 
        }
        fc()
}


int main(){

        size_t sub[] = {2, (size_t)mul, VAL, 5, VAL, 8};
        size_t ast[] = {2, (size_t)add, VAL, 4, SUB, (size_t)sub};

        //size_t (*fc)();
        //fc = (void*)ast[1];
        //size_t r = fc(ast+2);
        //printf("xx %ld\n", r);

}


