#ifndef DECLAREUSERALG_H_
#define DECLAREUSERALG_H_

#define DECLARE_USER_ALG(x) \
Base* x##_create() { \
	return new x;  \
} \
\
void x##_destroy(Base* p) \
{\
	delete p;\
}

#endif /*DECLAREUSERALG_H_*/
