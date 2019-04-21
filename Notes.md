Functions I meant to use to iterate through hash tables...

```
static void sk_env_flatten_r(SkEnv *env, SkEnv *into) {
    unsigned int h;
    hash_element *e;
    if(!env) return;
    sk_env_flatten_r(env->parent, into);
    for(h = 0; h <= env->mask; h++) {
        for(e = env->table[h]; e; e = e->next)
            sk_env_put(into, e->name, rc_retain(e->ex));
    }
}
SkEnv *sk_env_flatten(SkEnv *env) {
    SkEnv *into = sk_env_create(NULL);
    sk_env_flatten_r(env, into);
    return into;
}

const char *sk_env_next(SkEnv *env, const char *name) {
    unsigned int h;
    hash_element *e;
    if(!env) return NULL;
    if(!name) {
        for(h = 0; h <= env->mask; h++) {
            if((e = env->table[h]))
                return e->name;
        }
    } else {
        h = hash(name) & env->mask;
        for(e = env->table[h]; e; e = e->next) {
            if(!strcmp(e->name, name)) {
                if(e->next)
                    return e->next->name;
                for(h++; h <= env->mask; h++) {
                    if(env->table[h])
                        return env->table[h]->name;
                }
            }
        }
    }
    return NULL;
    
    // cant do this: What if the key is in the child _and_ the parent?
    // Althrough I really want to:
    //return sk_env_next(env->parent, name); 
}
```