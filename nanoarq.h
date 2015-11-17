#ifndef NANOARQ_H_INCLUDED
#define NANOARQ_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ARQ_SUCCESS = 0
} arq_error_t;

typedef struct arq_t arq_t;

arq_error_t arq_required_size(int *out_required_size);


#ifdef __cplusplus
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

arq_error_t arq_required_size(int *out_required_size)
{
    *out_required_size = 0;
    return ARQ_SUCCESS;
}

#endif

