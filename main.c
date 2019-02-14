#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "skeem.h"
#include "refcnt.h"

static char *readfile(const char *fname);

static void add_io_functions(Env *global);

int main(int argc, char *argv[]) {

    int rv = 0;

    rc_init();

    Env *global = global_env();

    add_io_functions(global);

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
                write(stdout, result);
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
            } else if(!is_null(result)) {
                write(stdout, result);
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

/* Not invoked directly, but rather used as the destructor
 * I could've just used `fclose` directly and cast it to
 * a `cdata_dtor`, but I prefer this way.
 */
static void bif_fclose(void *p) {
    FILE *f = p;
    assert(p);
    fclose(f);
}

static Expr *bif_fopen(Env *env, Expr *e) {
    const char *filename = get_text(car(e));
    const char *filemode = get_text(car(cdr(e)));
    if(!filename[0] || !filemode[0])
        return error("'fopen' expects a filename and mode");

    FILE *f = fopen(filename, filemode);
    if(!f)
        return errorf("unable to open %s (%s): %s", filename, filemode, strerror(errno));
    return cdata(f, bif_fclose);
}

static Expr *bif_fputs(Env *env, Expr *e) {
    Expr *file = car(e);
    const char *text = get_text(car(cdr(e)));

    /* You can check the cdtor of the CDATA object to make sure it's
        of the correct type.
        Technically the `is_cdata()` call here is not necessary because
        `get_cdtor()` will also return NULL if it's not given a CDATA
    */
    if(is_null(file) || !is_cdata(file) || get_cdtor(file) != bif_fclose)
        return error("'fputs' expects a file as its first parameter");
    assert(get_cdata(file)); /* shouldn't be null because of how `bif_fopen` works */
    FILE *f = get_cdata(file);
    if(fputs(text, f) == EOF || fputs("\n",f) == EOF)
        return errorf("unable to write to file: %s", strerror(errno));
    return NULL;
}

static Expr *bif_fgets(Env *env, Expr *e) {
    Expr *file = car(e);
    if(is_null(file) || !is_cdata(file) || get_cdtor(file) != bif_fclose)
        return error("'fgets' expects a file as its first parameter");
    assert(get_cdata(file));
    FILE *f = get_cdata(file);
    if(feof(f))
        return NULL;

    char buffer[256];
    if(!fgets(buffer, sizeof buffer, f))
        return feof(f) ? NULL : errorf("unable to read from file: %s", strerror(errno));

    /* Trim trailing space: */
    int l = strlen(buffer);
    while((l > 0) && strchr("\r\n", buffer[l - 1]))
        buffer[--l] = '\0';

    return value(buffer);
}

static Expr *bif_feof(Env *env, Expr *e) {
    Expr *file = car(e);
    if(is_null(file) || !is_cdata(file) || get_cdtor(file) != bif_fclose)
        return error("'feof' expects a file as its first parameter");
    assert(get_cdata(file));
    FILE * f = get_cdata(file);
    return boolean(feof(f));
}

static void add_io_functions(Env *global) {
    env_put(global, "fopen", cfun(bif_fopen));
    env_put(global, "fputs", cfun(bif_fputs));
    env_put(global, "fgets", cfun(bif_fgets));
    env_put(global, "feof", cfun(bif_feof));
}
