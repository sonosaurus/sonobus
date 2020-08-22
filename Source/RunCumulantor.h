#ifndef __RUNCUMULANTOR__
#define __RUNCUMULANTOR__

/// Function for updating the cumulants, 1-D version
extern void push_aggregate( float* Za, float* xa, float* sigma2a, float Zb, float xb, float sigma2b);

/// Function for updating the cumulants, 2-D version
extern void push_aggregate_2d( float* Za, float* xa, float* ya, float* xxa, float* xya, float* yya,
                    float Zb, float xb, float yb, float xxb, float xyb, float yyb );


/// Function for updating the cumulants, 1-D version
extern void push_aggregateD( double* Za, double* xa, double* sigma2a, double Zb, double xb, double sigma2b);

/// Function for updating the cumulants, 2-D version
extern void push_aggregate_2dD( double* Za, double* xa, double* ya, double* xxa, double* xya, double* yya,
                              double Zb, double xb, double yb, double xxb, double xyb, double yyb );


namespace stats {


class RunCumulantor1D{
public:

    typedef float value_type;

    RunCumulantor1D(){
        Z=xbar=s2xx=0.0f;
    }
    
    // default copy constructor OK

    virtual ~RunCumulantor1D(){
        // pass
    }

    /// Append an additional sample to the ensemble that we are estimating over.
    ///
    /// For indiviudal data samples, typically sigma2==0.0 (unless we knew that there
    ///     was an intrinsic uncertainty in the values).
    /// The weights are often just 1.0 (in order to get the usual mean and standard deviation).
    /// but can also vary from sample to sample.
    void push( value_type x, value_type w=1.0, value_type sigma2=0.0){
        push_aggregate( &Z, &xbar, &s2xx, w, x, sigma2 );
    } 

    /// Postcondition: this contains an estimate of mean & standard deviation
    /// derived from all of the data (already) pushed into this as well as
    /// all of the that that have been pushed into that.
    ///
    /// In some cases we may have CumulantorA, CumulantorB
    /// and decide that we want the cumulants for all of the data that have
    /// been added to A and B; this does this *in place*
    void merge( const RunCumulantor1D& that ){
        push_aggregate( &Z, &xbar, &s2xx, that.Z, that.xbar, that.s2xx );
    }

    void reset() {
        Z=xbar=s2xx=0.0f;
    }

    void resetInitVal(float val) {
        Z = 1;
        xbar = val;
        s2xx = 0.0f;
    }
    
    
    value_type Z; ///< total weight of data used to construct estimate
    value_type xbar; ///< current estimate of mean
    value_type s2xx; ///< current estimate of standard-deviation

};

    
class RunCumulantor1DD{
    public:
        
        typedef double value_type;
        
        RunCumulantor1DD(){
            Z=xbar=s2xx=0.0;
        }
        
        // default copy constructor OK
        
        virtual ~RunCumulantor1DD(){
            // pass
        }
        
        /// Append an additional sample to the ensemble that we are estimating over.
        ///
        /// For indiviudal data samples, typically sigma2==0.0 (unless we knew that there
        ///     was an intrinsic uncertainty in the values).
        /// The weights are often just 1.0 (in order to get the usual mean and standard deviation).
        /// but can also vary from sample to sample.
        void push( value_type x, value_type w=1.0, value_type sigma2=0.0){
            push_aggregateD( &Z, &xbar, &s2xx, w, x, sigma2 );
        } 
        
        /// Postcondition: this contains an estimate of mean & standard deviation
        /// derived from all of the data (already) pushed into this as well as
        /// all of the that that have been pushed into that.
        ///
        /// In some cases we may have CumulantorA, CumulantorB
        /// and decide that we want the cumulants for all of the data that have
        /// been added to A and B; this does this *in place*
        void merge( const RunCumulantor1D& that ){
            push_aggregateD( &Z, &xbar, &s2xx, that.Z, that.xbar, that.s2xx );
        }
        
        void reset() {
            Z=xbar=s2xx=0.0;
        }
        
        void resetInitVal(double val) {
            Z = 1;
            xbar = val;
            s2xx = 0.0;
        }
        
        
        value_type Z; ///< total weight of data used to construct estimate
        value_type xbar; ///< current estimate of mean
        value_type s2xx; ///< current estimate of standard-deviation
        
    };


class RunCumulantor2D{
public:
   typedef float value_type;

    RunCumulantor2D(){
        Z=xbar=s2xx=ybar=s2yy=s2xy=0.0f;
    }
    
    // default copy constructor OK

    virtual ~RunCumulantor2D(){
        // pass
    }

    /// Append an additional sample to the ensemble that we are estimating over.
    ///
    /// For indiviudal data samples, typically sigma2==0.0 (unless we knew that there
    ///     was an intrinsic uncertainty in the values).
    /// The weights are often just 1.0 (in order to get the usual mean and standard deviation).
    /// but can also vary from sample to sample.
    void push( value_type xnew, value_type ynew, value_type w=1.0, 
                value_type s2xxnew=0.0, value_type s2xynew=0.0, value_type s2yynew=0.0){
        push_aggregate_2d( &Z, &xbar, &ybar, &s2xx, &s2xy, &s2yy, w, xnew, ynew, s2xxnew, s2xynew, s2yynew );
    } 

    /// Postcondition: this contains an estimate of mean & standard deviation
    /// derived from all of the data (already) pushed into this as well as
    /// all of the that that have been pushed into that.
    ///
    /// In some cases we may have CumulantorA, CumulantorB
    /// and decide that we want the cumulants for all of the data that have
    /// been added to A and B; this does this *in place*
    void merge( const RunCumulantor2D& that ){
        push_aggregate_2d( &Z, &xbar, &ybar, &s2xx, &s2xy, &s2yy,  that.Z, that.xbar, that.ybar,
                    that.s2xx, that.s2xy, that.s2yy  );
    }

    value_type Z; ///< total weight of data used to construct estimate
    value_type xbar,ybar; ///< current estimate of mean
    value_type s2xx, s2xy, s2yy; ///< current estimate of standard-deviation
};


} // end namespace vri::stats

#endif // double-inclusion guard

