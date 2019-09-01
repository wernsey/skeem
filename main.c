#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "skeem.h"

#ifdef SK_USE_EXTERNAL_REF_COUNTER
#  include "refcnt.h"
#endif

static char *readfile(const char *fname);

/* See the bottom of this file. It adds some
file I/O functions that are not included in
the Skeem built-in functions. */
static void add_io_functions(SkEnv *global);

int main(int argc, char *argv[]) {

    int rv = 0;

#ifdef SK_USE_EXTERNAL_REF_COUNTER
    rc_init();
#endif

    /* Create a global environment where functions and
    variables are stored. `sk_global_env()` also adds the
    built-in functions to the new environment. */
    SkEnv *global = sk_global_env();

    /* We can add our own domain specific functions
    to the interpreter */
    add_io_functions(global);

    if(argc > 1) {
        /* Executing a file */
        char *text = readfile(argv[1]);
        if(!text) {
            fprintf(stderr, "error: reading %s: %s\n", argv[1], strerror(errno));
            rv = 1;
        } else {
            /* Evaluate the input string against the
            global environment */
            SkObj *result = sk_eval_str(global, text);

            /* Check for pars/evaluation errors and print the
            result */
            if(sk_is_error(result)) {
                fprintf(stderr, "error: %s\n", sk_get_text(result));
            } else {
                char *text = sk_serialize(result);
                printf("Result: %s\n", text);
                free(text);
            }
            if(result) rc_release(result);

            free(text);
        }
    } else {
        /* REPL */
        while(1) {
            char buffer[512];
            fputs(">>> ", stdout);
            if(!fgets(buffer, sizeof buffer, stdin))
                break;

            /* Evaluate the input string against the
            global environment */
            SkObj *result = sk_eval_str(global, buffer);

            /* Check for pars/evaluation errors and print the
            result */
            if(sk_is_error(result)) {
                fprintf(stderr, "error: %s\n", sk_get_text(result));
            } else if(!sk_is_null(result)) {
                char *text = sk_serialize(result);
                puts(text);
                free(text);
            }

            /* Tell the reference counter to release the
            result object */
            rc_release(result);
        }
    }

    /* Release the global object through the  */
    rc_release(global);

    return rv;
}

/* Reads an entire file into a heap allocated buffer */
static char *readfile(const char *fname) {
    FILE *f;
    long len,r;
    char *str;

    if(!(f = fopen(fname, "rb")))
        return NULL;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    if(!(str = malloc(len+1)))
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

static SkObj *bif_import(SkEnv *env, SkObj *e) {
    const char *filename = sk_get_text(sk_car(e));
    if(!filename[0])
        return sk_error("'import' expects a filename");
    char *text = readfile(filename);
    if(!text)
        return sk_errorf("unable to import %s: %s", filename, strerror(errno));
    SkObj *result = sk_eval_str(env, text);
    free(text);
    return result;
}

/* Not invoked directly, but rather used as the destructor
 * I could've just used `fclose` directly and cast it to
 * a `ref_dtor`, but I prefer it this because it is clearer
 * in the later checks for `sk_get_cdtor(obj) != file_dtor`
 * that you're actually doing a type check.
 */
static void file_dtor(void *p) {
    FILE *f = p;
    assert(p);
    fclose(f);
}

static SkObj *bif_fopen(SkEnv *env, SkObj *e) {
    const char *filename = sk_get_text(sk_car(e));
    const char *filemode = sk_get_text(sk_car(sk_cdr(e)));
    if(!filename[0] || !filemode[0])
        return sk_error("'fopen' expects a filename and mode");

    FILE *f = fopen(filename, filemode);
    if(!f)
        return sk_errorf("unable to open %s (%s): %s", filename, filemode, strerror(errno));
    return sk_cdata(f, file_dtor);
}

static SkObj *bif_readfile(SkEnv *env, SkObj *e) {
    const char *filename = sk_get_text(sk_car(e));
    if(!filename[0])
        return sk_error("'readfile' expects a filename");
    char *text = readfile(filename);
    if(!text)
        return sk_errorf("unable to read %s: %s", filename, strerror(errno));
    return sk_value_o(text);
}

static SkObj *bif_fputs(SkEnv *env, SkObj *e) {
    SkObj *file = sk_car(e);
    const char *text = sk_get_text(sk_car(sk_cdr(e)));

    /* You can check the cdtor of the CDATA object to make sure it's
        of the correct type.
        Technically the `sk_is_cdata()` call here is not necessary because
        `sk_get_cdtor()` will also return NULL if it's not given a CDATA.
        I do it here for completeness, but the other functions don't.
    */
    if(sk_is_null(file) || !sk_is_cdata(file) || sk_get_cdtor(file) != file_dtor)
        return sk_error("'fputs' expects a file as its first parameter");
    assert(sk_get_cdata(file)); /* shouldn't be null because of how `bif_fopen` works */
    FILE *f = sk_get_cdata(file);
    if(fputs(text, f) == EOF || fputs("\n",f) == EOF)
        return sk_errorf("unable to write to file: %s", strerror(errno));
    return NULL;
}

static SkObj *bif_fgets(SkEnv *env, SkObj *e) {
    SkObj *file = sk_car(e);
    if(sk_get_cdtor(file) != file_dtor)
        return sk_error("'fgets' expects a file as its first parameter");
    assert(sk_get_cdata(file));
    FILE *f = sk_get_cdata(file);
    if(feof(f))
        return NULL;

    char buffer[256];
    if(!fgets(buffer, sizeof buffer, f))
        return feof(f) ? NULL : sk_errorf("unable to read from file: %s", strerror(errno));

    /* Trim trailing space: */
    int l = strlen(buffer);
    while((l > 0) && strchr("\r\n", buffer[l - 1]))
        buffer[--l] = '\0';

    return sk_value(buffer);
}

static SkObj *bif_feof(SkEnv *env, SkObj *e) {
    SkObj *file = sk_car(e);
    if(sk_get_cdtor(file) != file_dtor)
        return sk_error("'feof' expects a file as its first parameter");
    assert(sk_get_cdata(file));
    FILE *f = sk_get_cdata(file);
    return sk_boolean(feof(f));
}

static SkObj *bif_is_file(SkEnv *env, SkObj *e) {
    SkObj *file = sk_car(e);
    return sk_boolean(sk_get_cdtor(file) == file_dtor);
}

/* Register the functions. For each function, create a
CFun object and adds it to the global environment under the
specific name */
static void add_io_functions(SkEnv *global) {
    sk_env_put(global, "import", sk_cfun(bif_import));
    sk_env_put(global, "readfile", sk_cfun(bif_readfile));
    sk_env_put(global, "fopen", sk_cfun(bif_fopen));
    sk_env_put(global, "fputs", sk_cfun(bif_fputs));
    sk_env_put(global, "fgets", sk_cfun(bif_fgets));
    sk_env_put(global, "feof", sk_cfun(bif_feof));
    sk_env_put(global, "file?", sk_cfun(bif_is_file));
}
