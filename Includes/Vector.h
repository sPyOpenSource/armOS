#ifndef Vector_h
#define Vector_h

#include "Arduino.h"
#include "Kalman.h"

template <class T>
class Vector {
   public:
      T x, y, z;
      void setValues(T X, T Y, T Z){
          x = X;
          y = Y;
          z = Z;
      };
};

class VectorDouble: public Vector<double> {
  public:
    static void cross(const VectorDouble * a, const VectorDouble * b, VectorDouble * out){
    	out->x = (a->y * b->z) - (a->z * b->y);
    	out->y = (a->z * b->x) - (a->x * b->z);
    	out->z = (a->x * b->y) - (a->y * b->x);
    };

    double dot(const VectorDouble * a){
      return (a->x * x) + (a->y * y) + (a->z * z);
    };

    double length(){
	    return sqrt(x * x + y * y + z * z);
    };

    void minus(const VectorDouble * a){
      x -= a->x;
      y -= a->y;
      z -= a->z;
    };

    void normalize(){
      double l = length();
      if (l > 0){
        x /= l;
        y /= l;
        z /= l;
      }
    };
};

class VectorKalman: public Vector<Kalman> {
  public:
    void setParameters(const VectorDouble * Q, const VectorDouble * Qbias, const VectorDouble * R){
      x.setParameters(Q->x, Qbias->x, R->x);
      y.setParameters(Q->y, Qbias->y, R->y);
      z.setParameters(Q->z, Qbias->z, R->z);
    };

    void set(const VectorDouble * a){
      x.set(a->x);
      y.set(a->y);
      z.set(a->z);
    };

    void update(VectorDouble * a, const VectorDouble * b, double dt){
      a->x = x.get(a->x, b->x, dt);
      a->y = y.get(a->y, b->y, dt);
      a->z = z.get(a->z, b->z, dt);
    };
};

#endif
