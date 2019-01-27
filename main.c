#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skeem.h"
#include "refcnt.h"

extern int level, max_level;

int main(int argc, char *argv[]) {

    int rv = 0, verbose = 0;

    rc_init();

    Env *global = global_env();

    if(argc > 1) {

        char *text = readfile(argv[1]);
        if(!text) {
            fprintf(stderr, "error: reading %s: %s\n", argv[1], strerror(errno));
            rv = 1;
        } else {
            Expr *program = parse_stmts(text);
            if(is_error(program)) {
                fprintf(stderr, "error [parse]: %s\n", get_text(program));
                rv = 1;
            } else {
                if(verbose) {
                    write(program);
                    fputs("\n", stdout);
                }
                Expr *result = eval(global, program);
                if(is_error(result)) {
                    fprintf(stderr, "error [eval]: %s\n", get_text(result));
                } else {
                    printf("Result:\n");
                    write(result);
                    fputs("\n", stdout);
                }
                if(result) rc_release(result);
            }
            if(program) rc_release(program);
            free(text);
        }
    } else {

        while(1) {
            char buffer[512];
            fputs(">>> ", stdout);
            if(!fgets(buffer, sizeof buffer, stdin))
                break;

            Expr *program = parse(buffer);
			if(is_error(program)) {
		        fprintf(stderr, "error [parse]: %s\n", get_text(program));
				rc_release(program);
				continue;
			}

            if(verbose) {
                write(program);
                fputs("\n", stdout);
            }

            Expr *result = eval(global, program);
            if(is_error(result)) {
		        fprintf(stderr, "error [eval]: %s\n", get_text(result));
			} else {
                write(result);
            	fputs("\n", stdout);
			}
            if(result) rc_release(result);

			if(program) rc_release(program);
        }
    }

	rc_release(global);
	
	printf("max_level == %d\n", max_level);

    return rv;
}