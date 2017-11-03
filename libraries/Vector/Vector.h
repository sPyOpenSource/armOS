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

    static double dot(const VectorDouble *a, const VectorDouble *b)
    {
      return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
    }

    double length() {
	     return sqrt(x * x + y * y + z * z);
    }

    void normalize(){
        double l = length();
        if (l > 0){
            x /= l;
            y /= l;
            z /= l;
        }
    }
};
