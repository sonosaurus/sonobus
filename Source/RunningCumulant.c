
#ifndef __VRI_STATS_RUNNING_CUMULANT_INLINE__
#define __VRI_STATS_RUNNING_CUMULANT_INLINE__

#ifdef __SPU__
#define FAST_CUMULANT // Have the SPUs always use the fast version
#endif

#ifdef FAST_CUMULANT // define this label if you want to inline this C file

// lets make this C file such that it can be included straight up as opposed to including the .h file (necessary for inlining)

#define INLINE_ME static inline // use a name slightly different from INLINE in case some other header file used that commonly named label
INLINE_ME float sigma2_increment( float r, float sigma2a, float sigma2b, float scaled_dx, float scaled_dy);
INLINE_ME void push_sample(float * Z, float * xbar, float * sigma2, float w, float x);
INLINE_ME void push_aggregate(float * Za, float * xa, float * sigma2a, float Zb, float xb, float sigma2b);
INLINE_ME void push_aggregate_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb, float xxb, float xyb, float yyb );
INLINE_ME void push_sample_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb );

#else

//#include "vri/common/stats/RunningCumulant.h"
#define INLINE_ME // use a name slightly different from INLINE in case some other header file used that commonly named label

#endif


// returns the amount to change the variance estimate.
// q = ratio of the weight of the current sample to the net weight prior to its introduction.
//      i.e. q= Zb/Za where Za is the weight of the sample used to construct the estimate sigma2a, etc.
// scaled_dx, scaled_dy are the increments to the mean that will be applied.
INLINE_ME float sigma2_increment( float q, float sigma2a, float sigma2b, float scaled_dx, float scaled_dy)
{
    return (sigma2b-sigma2a)/(1.0f+q)+q*scaled_dx*scaled_dy;
}

// 1-D functions

// push as single (0 variance) sample into the estimate.
// the first three arguments are in/out.
INLINE_ME void push_sample( float* Z, float* xbar, float* sigma2, float w, float x ){
    float q=(*Z)/w;
    float scaled_dx=(x-(*xbar))/(1.0f+q);
    *sigma2+=sigma2_increment( q, *sigma2, 0.0f, scaled_dx, scaled_dx);
    *xbar+= scaled_dx;
    *Z+=w;
}

// Push an already aggregated sample (non-zero variance) into the estimate.
INLINE_ME void push_aggregate( float* Za, float* xa, float* sigma2a, float Zb, float xb, float sigma2b){
    float q=(*Za)/Zb;
    float scaled_dx=(xb-*xa)/(1.0f+q);
    *sigma2a+=sigma2_increment( q, *sigma2a, sigma2b, scaled_dx, scaled_dx);
    *xa+= scaled_dx ;
    *Za+=Zb;
}

// 2-D functions
INLINE_ME void push_aggregate_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb, float xxb, float xyb, float yyb ){
    float q=*Za/Zb;
    float scaled_dx=(xb-*xa)/(1.0f+q);
    float scaled_dy=(yb-*ya)/(1.0f+q);

    *xxa+=sigma2_increment( q, *xxa, xxb, scaled_dx, scaled_dx);
    *yya+=sigma2_increment( q, *yya, yyb, scaled_dy, scaled_dy);
    *xya+=sigma2_increment( q, *xya, xyb, scaled_dx, scaled_dy );

    *xa+=scaled_dx ;
    *ya+=scaled_dy ;

    *Za+=Zb;

}

INLINE_ME void  push_sample_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb ){

    float q=*Za/Zb;
    float scaled_dx=(xb-*xa)/(1.0f+q);
    float scaled_dy=(yb-*ya)/(1.0f+q);

    *xxa+=sigma2_increment( q, *xxa, 0.0f, scaled_dx, scaled_dx);
    *yya+=sigma2_increment( q, *yya, 0.0f, scaled_dy, scaled_dy);
    *xya+=sigma2_increment( q, *xya, 0.0f, scaled_dx, scaled_dy );

    *xa+=scaled_dx ;
    *ya+=scaled_dy ;

    *Za+=Zb;
}



#ifdef FAST_CUMULANT // define this label if you want to inline this C file
//#define INLINE_ME static inline // use a name slightly different from INLINE in case some other header file used that commonly named label
INLINE_ME double sigma2_incrementD( double r, double sigma2a, double sigma2b, double scaled_dx, double scaled_dy);
INLINE_ME void push_sampleD(double * Z, double * xbar, double * sigma2, double w, double x);
INLINE_ME void push_aggregateD(double * Za, double * xa, double * sigma2a, double Zb, double xb, double sigma2b);
INLINE_ME void push_aggregate_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                                 double Zb, double xb, double yb, double xxb, double xyb, double yyb );
INLINE_ME void push_sample_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                              double Zb, double xb, double yb );

#else

//#include "vri/common/stats/RunningCumulant.h"
#define INLINE_ME // use a name slightly different from INLINE in case some other header file used that commonly named label

#endif


// returns the amount to change the variance estimate.
// q = ratio of the weight of the current sample to the net weight prior to its introduction.
//      i.e. q= Zb/Za where Za is the weight of the sample used to construct the estimate sigma2a, etc.
// scaled_dx, scaled_dy are the increments to the mean that will be applied.
INLINE_ME double sigma2_incrementD( double q, double sigma2a, double sigma2b, double scaled_dx, double scaled_dy)
{
    return (sigma2b-sigma2a)/(1.0f+q)+q*scaled_dx*scaled_dy;
}

// 1-D functions

// push as single (0 variance) sample into the estimate.
// the first three arguments are in/out.
INLINE_ME void push_sampleD( double* Z, double* xbar, double* sigma2, double w, double x ){
    double q=(*Z)/w;
    double scaled_dx=(x-(*xbar))/(1.0f+q);
    *sigma2+=sigma2_incrementD( q, *sigma2, 0.0f, scaled_dx, scaled_dx);
    *xbar+= scaled_dx;
    *Z+=w;
}

// Push an already aggregated sample (non-zero variance) into the estimate.
INLINE_ME void push_aggregateD( double* Za, double* xa, double* sigma2a, double Zb, double xb, double sigma2b){
    double q=(*Za)/Zb;
    double scaled_dx=(xb-*xa)/(1.0f+q);
    *sigma2a+=sigma2_incrementD( q, *sigma2a, sigma2b, scaled_dx, scaled_dx);
    *xa+= scaled_dx ;
    *Za+=Zb;
}

// 2-D functions
INLINE_ME void push_aggregate_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                                 double Zb, double xb, double yb, double xxb, double xyb, double yyb ){
    double q=*Za/Zb;
    double scaled_dx=(xb-*xa)/(1.0f+q);
    double scaled_dy=(yb-*ya)/(1.0f+q);
    
    *xxa+=sigma2_incrementD( q, *xxa, xxb, scaled_dx, scaled_dx);
    *yya+=sigma2_incrementD( q, *yya, yyb, scaled_dy, scaled_dy);
    *xya+=sigma2_incrementD( q, *xya, xyb, scaled_dx, scaled_dy );
    
    *xa+=scaled_dx ;
    *ya+=scaled_dy ;
    
    *Za+=Zb;
    
}

INLINE_ME void  push_sample_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                               double Zb, double xb, double yb ){
    
    double q=*Za/Zb;
    double scaled_dx=(xb-*xa)/(1.0f+q);
    double scaled_dy=(yb-*ya)/(1.0f+q);
    
    *xxa+=sigma2_incrementD( q, *xxa, 0.0f, scaled_dx, scaled_dx);
    *yya+=sigma2_incrementD( q, *yya, 0.0f, scaled_dy, scaled_dy);
    *xya+=sigma2_incrementD( q, *xya, 0.0f, scaled_dx, scaled_dy );
    
    *xa+=scaled_dx ;
    *ya+=scaled_dy ;
    
    *Za+=Zb;
}





#endif
