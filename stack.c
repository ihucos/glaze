

static void *buffer[255];
static char buffer_counter;
#define PUSH(element) (buffer[buffer_counter++] = element)
#define POP() buffer[buffer_counter--]
#define ANCHOR() XXX
#define STRSINCEANCHOR() XXX

void copyoutstring(char bytes) {
        void *out[bytes + 1];
        out[bytes + 1] = 0;
        for(; bytes; bytes--){
                out[bytes] = buffer[buffer_counter + bytes];
        }
}

main(){



}
