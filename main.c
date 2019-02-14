#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skeem.h"
#include "refcnt.h"

static char *readfile(const char *fname);

int main(int argc, char *argv[]) {

    int rv = 0;

    rc_init();

    Env *global = global_env();

    if(argc > 1) {

        char *text = readfile(argv[1]);
        if(!text) {
            fprintf(stderr, "error: reading %s: %s\n", argv[1], strerror(errno));
            rv = 1;
        } else {
            
			Expr *result = eval_str(global, text);
			if(is_error(result)) {
				fprintf(stderr, "error: %s\n", get_text(result));
			} else {
				printf("Result:\n");
				write(result);
				fputs("\n", stdout);
			}
            if(result) rc_release(result);
            
			free(text);
        }
    } else {

        while(1) {
            char buffer[512];
            fputs(">>> ", stdout);
            if(!fgets(buffer, sizeof buffer, stdin))
                break;

            Expr *result = eval_str(global, buffer);
            if(is_error(result)) {
		        fprintf(stderr, "error: %s\n", get_text(result));
			} else {
                write(result);
            	fputs("\n", stdout);
			}
            if(result) rc_release(result);
        }
    }

	rc_release(global);

    return rv;
}

static char *readfile(const char *fname) {
    FILE *f;
    long len,r;
    char *str;

    if(!(f = fopen(fname, "rb")))
        return NULL;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    if(!(str = malloc(len+2)))
        return NULL;
    r = fread(str, 1, len, f);

    if(r != len) {
        free(str);
        return NULL;
    }

    fclose(f);
    str[len] = '\0';
    return str;
}

