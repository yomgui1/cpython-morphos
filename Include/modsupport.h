/* Module support interface */

struct methodlist {
	char *ml_name;
	method ml_meth;
};

extern object *findmethod PROTO((struct methodlist *, object *, char *));
extern object *initmodule PROTO((char *, struct methodlist *));
extern int err_badargs PROTO((void));
extern object *err_nomem PROTO((void));
