#pragma once

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "../eigen/Eigen/Geometry"
#include "../eigen/Eigen/LU"

namespace fd {

// derp
#define PI (3.141592653589793238462643383)

//failed to compile in msvc, doesn't seem to be directly referenced in fourd anyway...
//worked fine in gcc
//template <typename Tmax>
//Tmax max(Tmax left, Tmax right) {
//  return (left > right) ? left : right;
//}

template <typename T>
class Vector4 {
protected:
  typedef Vector4<T> Vec;

  union {
    T d[4];
    struct {
      T x, y, z, w;
    };
  };

public:
  Vector4() : x(0), y(0), z(0), w(0) {}
	Vector4(T inX, T inY, T inZ, T inI) : x(inX), y(inY), z(inZ), w(inI) {}
	Vector4(const Vec& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	T* raw() { return (T*)this; }
	const T* raw() const { return (T*)this; }
	T& operator()(int index) { return d[index]; }
	T& operator[](int index) { return d[index]; }
	void set(int index, T val) { d[index] = val; }
	void set(const Vec& rVal) {
	  x = rVal.x; y = rVal.y; z = rVal.z; w = rVal.w;
	}
  void set(T inX, T inY, T inZ, T inI) {
    x = inX; y = inY; z = inZ; w = inI;
  }
	Vec& operator = (const Vec& rVal) {
	  set(rVal);
	  return *this;
	}
  Vec& operator += (const Vec& r) {
    set(x + r.x, y + r.y, z + r.z, w + r.w);
    return *this;
  }
  Vec& operator -= (const Vec& r) {
    set(x - r.x, y - r.y, z - r.z, w - r.w);
    return *this;
  }
  Vec& operator *= (const T& s) {
    set(x * s, y * s, z * s, w * s);
    return *this;
  }

	Vec operator + (const Vec& v) const {
	  return Vec(x + v.x, y + v.y, z + v.z, w + v.w);
	}

  Vec operator - (const Vec& v) const {
    return Vec(x - v.x, y - v.y, z - v.z, w - v.w);
  }
  Vec operator - () const {
    return Vec(-x, -y, -z, -w);
  }

	Vec operator * (T s) const {
	  return Vec(x * s, y * s, z * s, w * s);
	}

	Vec operator * (const Vec& v) const {
    return Vec(x * v.x, y * v.y, z * v.z, w * v.w);
	}

	T dot(const Vec& v) const {
    return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	T length() const {
	  return sqrt(x * x + y * y + z * z + w * w);
	}

	void storeZero() {
	  set(0, 0, 0, 0);
	}

	Vec normalized() {
	  Vec result(*this);
	  result.storeNormalized();
	  return result;
	}

	Vec& storeNormalized() {
	  T len = length();
	  if (len != 0) {
	    len = 1 / len;
	    set(x * len, y * len, z *len, w * len);
	  }
	  return *this;
	}

	Vec cross(const Vec& a, const Vec& b) const {
	  // e_ijk * a_i * b_j * c_k
	  const Vec& c = *this;
	  return Vec(
	      + a.y*b.z*c.w - a.y*b.w*c.z - a.z*b.y*c.w + a.z*b.w*c.y + a.w*b.y*c.z - a.w*b.z*c.y,
	      - a.x*b.z*c.w + a.x*b.w*c.z + a.z*b.x*c.w - a.z*b.w*c.x - a.w*b.x*c.z + a.w*b.z*c.x,
	      + a.x*b.y*c.w - a.x*b.w*c.y - a.y*b.x*c.w + a.y*b.w*c.x + a.w*b.x*c.y - a.w*b.y*c.x,
	      - a.x*b.y*c.z + a.x*b.z*c.y + a.y*b.x*c.z - a.y*b.z*c.x - a.z*b.x*c.y + a.z*b.y*c.x);
	}

	bool operator == (const Vec& c) const {
	  static const T threshold = 0.00001;
	  return abs(x - c.x) < threshold
	      && abs(y - c.y) < threshold
        && abs(z - c.z) < threshold
        && abs(w - c.w) < threshold;
	}

	void printIt() const {
	  printf("%f\t%f\t%f\t%f", x, y, z, w);
	}
};

typedef Vector4<float> Vec4f;


template <typename T>
class Matrix4 {
protected:
  typedef Vector4<T> Vec;
  typedef Matrix4<T> FdMat;
  typedef Eigen::Matrix<T, 4, 4> EigMat;

  // yay switching compilers is fun
  //union {
  //  Vec d[4];
    EigMat e;
  //};

public:
  Matrix4() { storeZero(); }
  Matrix4(const Matrix4<T>& c) {
    d(0) = c.d(0); d(1) = c.d(1); d(2) = c.d(2); d(3) = c.d(3);
  }
  Matrix4(Vec x, Vec y, Vec z, Vec w) {
    d(0) = x; d(1) = y; d(2) = z; d(3) = w;
  }
  //Matrix4(const T* vals) : e(vals) {}

  // this ugly is from when using gcc there was a union between Vec d[4] and EigMat e
  Vec& d(int index) { return (((Vec*)e.data())[index]); }
  const Vec& d(int index) const { return (((Vec*)e.data())[index]); }

  EigMat& eigen() { return e; }
  const EigMat& eigen() const { return e; }
  void setEigen(const EigMat& new_e) { e = new_e; }

  //EigMat& eigen() { return *(EigMat*)(&d[0]); }
  //const EigMat& eigen() const { return *(EigMat*)(&d[0]); }
  //void setEigen(const EigMat& new_e) { eigen() = new_e; }

  T* raw() { return (T*)&d(0); }
  const T* raw() const { return (T*)&d(0); }
  Vec& operator[] (int index) { return d(index); }
  void setRow(int index, const Vec& row) { d(index) = row; }

  bool operator ==(const FdMat& c) const {
    return d(0) == c.d(0) && d(1) == c.d(1) && d(2) == c.d(2) && d(3) == c.d(3);
  }

  FdMat& operator =(const FdMat& right) {
    setEigen(right.eigen());
    return *this;
  }

  FdMat&  storeZero() {
    for (int i = 0; i < 4; i++) {
      d(i).storeZero();
    }
    return *this;
  }

  FdMat& storeIdentity() {
    storeZero();
    d(0).x = d(1).y = d(2).z = d(3).w = 1;
    return *this;
  }

  FdMat& buildRotation(T radians, int targetIndex, int sourceIndex) {
    assert(targetIndex < 4 && targetIndex >= 0);
    assert(sourceIndex < 4 && sourceIndex >= 0);
    storeIdentity();
    T cosResult = cos(radians);
    T sinResult = sin(radians);
    Vec newTarget = d(targetIndex) * cosResult + d(sourceIndex) * sinResult;
    Vec newSource = d(targetIndex) * -sinResult + d(sourceIndex) * cosResult;
    d(targetIndex) = newTarget;
    d(sourceIndex) = newSource;
    return *this;
  }

  // Aspect is width/height of viewport. This is opengl style.
  FdMat& build3dProjection(T yFov, T aspect, T zNear, T zFar) {
    T yMax = tan(yFov * PI / 360.0f);
    T xMax = yMax * aspect;

    T depth = zFar - zNear;
    T zScale = -(zFar + zNear) / depth;
    T zOffset = -2.0 * (zFar * zNear) / depth;

    storeZero();
    raw()[0] = 1.0 / xMax;
    raw()[5] = 1.0 / yMax;
    raw()[10] = zScale;
    raw()[11] = -1.0;
    raw()[14] = zOffset;
    return *this;
  }

  FdMat inverse() const {
    FdMat inv = transpose();
    EigMat inverse = inv.eigen().inverse().eval();
    inv.eigen() = inverse;
    return inv.transpose();
  }

  FdMat transpose() const {
    FdMat r;
    r.d(0).set(d(0).x, d(1).x, d(2).x, d(3).x);
    r.d(1).set(d(0).y, d(1).y, d(2).y, d(3).y);
    r.d(2).set(d(0).z, d(1).z, d(2).z, d(3).z);
    r.d(3).set(d(0).w, d(1).w, d(2).w, d(3).w);
    return r;
  }

  FdMat operator * (const FdMat& m) const {
    FdMat r;
    FdMat t = m.transpose();
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        r[i][j] = d(i).dot(t[j]);
      }
    }
    return r;
  }

  Vec transform(const Vec& v) const {
    Vec result(v.dot(d(0)), v.dot(d(1)), v.dot(d(2)), v.dot(d(3)));
    return result;
  }

  void storeOrthognoal(int invariant, int variant, int secondary, int tertiary) {
    d(invariant).storeNormalized();
    d(variant) = d(invariant).cross(d(secondary), d(tertiary));
    d(variant).storeNormalized();
    d(tertiary) = d(invariant).cross(d(variant), d(secondary));
    d(tertiary).storeNormalized();
    d(secondary) = d(invariant).cross(d(tertiary), d(variant));
    d(secondary).storeNormalized();
  }

  // In the oculus sdk in 3d, this convention would be
  // X((-forward), right) = up and X(right, up) = -forward, X(u, -f) = r
  // where X is the cross operator applied in order to args
  // These follow a consistent levi-civita ordering:
  // (-3,1,2), (1,2,-3), (2,-3,1) -> -
  // which ties into the defintion of the cross product* in n-dimensions
  // (* I may be the only one who refers to this is a cross product in 4d)
  // Following the same logic, we label right=1, up=2, forward=3, in=4
  // So it seems like if the resulting levi-civita ordering is negative in 3d, it's "RH"
  // And if it's positive in 4d, it's "RH"
  static void lookAtRH(const Vec& eye, const Vec& at, const Vec& up, const Vec& in,
      FdMat& resultView, Vec& resultPos) {
    Vec newForward = (eye - at).storeNormalized(); // actually negative forward
    // X(u,-f,i)=r (2,-3,4,1) -> +
    Vec newRight = up.cross(newForward, in).storeNormalized();
    // X(-f,u,r)=i (-3,2,1,4) -> +
    Vec newIn = newForward.cross(up, newRight).storeNormalized();
    // X(r,i,-f)=u (1,4,-3,2) -> +
    Vec newUp = newRight.cross(newIn, newForward).storeNormalized();
    resultView = FdMat(newRight, newUp, newForward, newIn);

    resultPos.x = -(newRight.dot(eye));
    resultPos.y = -(newUp.dot(eye));
    resultPos.z = -(newForward.dot(eye));
    resultPos.w = -(newIn.dot(eye));
  }

  void printIt() const {
    for (int i = 0; i < 4; i++) {
      printf("(%d)\t", i);
      d(i).printIt();
      printf("\n");
    }
  }

  FdMat convertToTransposed3d() const {
    FdMat result = transpose();
    result.d(0).w = 0;
    result.d(1).w = 0;
    result.d(2).w = 0;
    result.d(3).set(0, 0, 0, 1);
    return result;
  }

  // Split this 4d matrix, interpreted as the usual 3d view + 3d position into a valid
  // 4d view + 4d position. Ok to pass self as viewTarget.
  void splice3dInto4d(FdMat& viewTarget, Vec& posTarget) const {
    posTarget = d(3);
    posTarget.w = 0;

    viewTarget = *this;
    viewTarget.d(0).w = 0;
    viewTarget.d(1).w = 0;
    viewTarget.d(2).w = 0;
    viewTarget.d(3).set(0, 0, 0, 1);
  }
};

typedef Matrix4<float> Mat4f;

} // namespace fd
