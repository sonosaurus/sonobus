#ifndef RUNINGCUMULANT_H_
#define RUNINGCUMULANT_H_

#ifdef __cplusplus
extern "C" {
#endif

    
float sigma2_increment( float r, float sigma2a, float sigma2b, float scaled_dx, float scaled_dy);
void push_sample(float * Z, float * xbar, float * sigma2, float w, float x);
void push_aggregate(float * Za, float * xa, float * sigma2a, float Zb, float xb, float sigma2b);
void push_aggregate_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb, float xxb, float xyb, float yyb );
void push_sample_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb );

    
    double sigma2_incrementD( double r, double sigma2a, double sigma2b, double scaled_dx, double scaled_dy);
    void push_sampleD(double * Z, double * xbar, double * sigma2, double w, double x);
    void push_aggregateD(double * Za, double * xa, double * sigma2a, double Zb, double xb, double sigma2b);
    void push_aggregate_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                           double Zb, double xb, double yb, double xxb, double xyb, double yyb );
    void push_sample_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                        double Zb, double xb, double yb );

    
#ifdef __cplusplus
}
#endif

#endif /* RUNINGCUMULANT_H_ */
