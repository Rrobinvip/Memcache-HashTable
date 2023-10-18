#ifndef UTILITY_MACROS_H
#define UTILITY_MACROS_H

#define EXIT_ON_VALUE(expression, value, message,exit_status) \
        if ((expression)==(value)) {\
            fprintf(stderr,(message));\
            exit((exit_status));\
        }

#define RETURN_ON_VALUE(expression, value, message,return_status) \
        if ((expression)==(value)) {\
            fprintf(stderr,(message));\
            return((return_status));\
        }

#define EXIT_NOT_ON_VALUE(expression, value, message,exit_status) \
        if ((expression)!=(value)) {\
            fprintf(stderr,(message));\
            exit((exit_status));\
        }

#define RETURN_AND_FREE_ON_VAL(expression, value, return_value, free1, free2, free3) \
        if ((expression)==(value)) {\
            if ((free1)!=NULL) free((free1));\
            if ((free2)!=NULL) free((free2));\
            if ((free3)!=NULL) free((free3));\
            return (return_value);\
        }

#define RETURN_AND_FREE_MEM(expression, value, message,return_value, mem, size) \
        if ((expression)==(value)) {\
            fprintf(stderr,(message));\
            munmap((mem), (size));\
            return (return_value);\
        }

#define FREE_ON_VAL(free1, free2, free3) \
        if ((free1)!=NULL) free((free1));\
        if ((free2)!=NULL) free((free2));\
        if ((free3)!=NULL) free((free3))

#define EXIT_AND_FREE_ON_VAL(expression, value, message, exit_status, free1, free2, free3) \
        if ((expression)==(value)) {\
            fprintf(stderr,(message));\
            if ((free1)!=NULL) free((free1));\
            if ((free2)!=NULL) free((free2));\
            if ((free3)!=NULL) free((free3));\
            exit((exit_status));\
        }

#define MILLION             1E6
#define FREE(p)             free(p); p = NULL
#define CLOSE(pipe)         close(pipe[0]); close(pipe[1])
// loop
#define FORONE(i,a)         for (int i = 0; i < (a); i++)
#define FORTWO(i,a,b)       for (int i = (b); i < (a); i++)
// prime number
#define PRIME               vector<bool> primes; \
                            primes.resize(32100, true); \
                            primes[0] = false; \
                            primes[1] = false; \
                            for(int i = 2; i < 32100; i++) { \
                                if(!primes[i]) { \
                                    continue; \
                                } \
                                for(int j = i+1; j < 32100; j++) { \
                                    if(j % i == 0) { \
                                        primes[j] = false; \
                                    } \
                                } \
                            }int __to_end__
// math
#define MIN(x, y)           (((x) < (y)) ?  (x) : (y))
#define MAX(x, y)           (((x) > (y)) ?  (x) : (y))
#define ABS(x)              (((x) <  0) ? -(x) : (x))
#define DIFF(a,b)           ABS((a)-(b))
#define SWAP(a, b)          do { a ^= b; b ^= a; a ^= b; } while ( 0 )
#define SORT(a, b)          do { if ((a) > (b)) SWAP((a), (b)); } while (0)
#define IS_ODD( num )       ((num) & 1)
#define IS_EVEN( num )      (!IS_ODD( (num) ))
#define IS_BETWEEN(n,L,H)   ((unsigned char)((n) >= (L) && (n) <= (H)))
// bit
#define BIT(x)              (1<<(x))
#define SETBIT(x,p)         ((x)|(1<<(p)))
#define CLEARBIT(x,p)       ((x)&(~(1<<(p))))
#define GETBIT(x,p)         (((x)>>(p))&1)
#define TOGGLEBIT(x,p)      ((x)^(1<<(p)))
// array
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define SET(d, n, v)        do{ size_t i_, n_; \
                            for ( n_ = (n), i_ = 0; n_ > 0; --n_, ++i_) \
                            (d)[i_] = (v); } while(0)
#define ZERO(d, n)          SET(d, n, 0)
#define IS_ARRAY(a)         ((void *)&a == (void *)a)

#define CLEARNL             while (getchar() != '\n')
#define MS_TIMEDIFF(time_struct1, time_struct2)     \
    ((time_struct2.tv_nsec-time_struct1.tv_nsec)/MILLION)+\
    ((time_struct2.tv_sec - time_struct1.tv_sec)*1000)

#endif /*UTILITY_MACROS_H */
