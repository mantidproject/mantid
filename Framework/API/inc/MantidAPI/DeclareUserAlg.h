#ifndef DECLAREUSERALG_H_
#define DECLAREUSERALG_H_

#define DECLARE_USER_ALG(x)                                                    \
  Algorithm *x##_create() { return new x; }                                    \
                                                                               \
  void x##_destroy(Algorithm *p) { delete p; }

#endif /*DECLAREUSERALG_H_*/
