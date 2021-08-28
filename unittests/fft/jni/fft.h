#ifndef _fft_h_
#define _fft_h_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct complex_f {
    float re;
    float im;
} complex_f;

typedef struct complex_d {
    double re;
    double im;
} complex_d;


#define complex_mul_re(a_re, a_im, b_re, b_im)  (a_re * b_re - a_im * b_im)
#define complex_mul_im(a_re, a_im, b_re, b_im)  (a_re * b_im + a_im * b_re)

typedef enum fft_dir {
    FFT_FORWARD,    /* kernel uses "-1" sign */
    FFT_INVERSE     /* kernel uses "+1" sign */
} fft_dir;


/* In-place FFT */
void ffti_f(complex_f data[], unsigned log2_N, fft_dir direction);

/* ... In-place FFT stage functions ... */
void ffti_copy_shuffle_f(complex_f src[], complex_f dst[], unsigned log2_N);
void ffti_shuffle_f(complex_f data[], unsigned log2_N);
void ffti_evaluate_f(complex_f data[], unsigned log2_N, fft_dir direction);

/* Recursive FFT */
void fftr_f(complex_f data[], unsigned log2_N, fft_dir direction);

/* Recursive FFT, user-supplied scratchpad buffer */
void fftrb_f(complex_f data[], unsigned log2_N, fft_dir direction, complex_f scratch[]);


#ifdef __cplusplus
}
#endif


#endif
