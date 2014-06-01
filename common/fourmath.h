#pragma once

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "../eigen/Eigen/Geometry"
#include "../eigen/Eigen/LU"

namespace fd {

// derp
#define PI (3.141592653589793238462643383)

template <typename T>
T max(T left, T right) {
  return (left > right) ? left : right;
}

template <typename T>
class Vector4 {
protected:
  union {
    T d[4];
    struct {
      T x, y, z, w;
    };
  };

public:
  Vector4() : x(0), y(0), z(0), w(0) {}
	Vector4(T inX, T inY, T inZ, T inI) : x(inX), y(inY), z(inZ), w(inI) {}
	Vector4(const Vector4<T>& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	T* raw() { return (T*)this; }
	const T* raw() const { return (T*)this; }
	T& operator()(int index) { return d[index]; }
	T& operator[](int index) { return d[index]; }
	void set(int index, T val) { d[index] = val; }
	void set(const Vector4<T>& rVal) {
	  x = rVal.x; y = rVal.y; z = rVal.z; w = rVal.w;
	}
  void set(T inX, T inY, T inZ, T inI) {
    x = inX; y = inY; z = inZ; w = inI;
  }
	Vector4<T>& operator = (const Vector4<T>& rVal) {
	  set(rVal);
	  return *this;
	}
  Vector4<T>& operator += (const Vector4<T>& r) {
    set(x + r.x, y + r.y, z + r.z, w + r.w);
    return *this;
  }

	Vector4<T> operator + (const Vector4<T>& v) const {
	  return Vector4<T>(x + v.x, y + v.y, z + v.z, w + v.w);
	}

  Vector4<T> operator - (const Vector4<T>& v) const {
    return Vector4<T>(x - v.x, y - v.y, z - v.z, w - v.w);
  }

	Vector4<T> operator * (T s) const {
	  return Vector4<T>(x * s, y * s, z * s, w * s);
	}

	Vector4<T> operator * (const Vector4<T>& v) const {
    return Vector4<T>(x * v.x, y * v.y, z * v.z, w * v.w);
	}

	T dot(const Vector4<T>& v) const {
    return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	T length() const {
	  return sqrt(x * x + y * y + z * z + w * w);
	}

	void storeZero() {
	  set(0, 0, 0, 0);
	}

	Vector4<T> normalized() {
	  Vector4<T> result(*this);
	  result.storeNormalized();
	  return result;
	}

	void storeNormalized() {
	  T len = length();
	  if (len != 0) {
	    len = 1 / len;
	    set(x * len, y * len, z *len, w * len);
	  }
	}

	Vector4<T> cross(const Vector4<T>& a, const Vector4<T>& b) const {
	  // e_ijk * a_i * b_j * c_k
	  const Vector4<T>& c = *this;
	  return Vector4<T>(
	      + a.y*b.z*c.w - a.y*b.w*c.z - a.z*b.y*c.w + a.z*b.w*c.y + a.w*b.y*c.z - a.w*b.z*c.y,
	      - a.x*b.z*c.w + a.x*b.w*c.z + a.z*b.x*c.w - a.z*b.w*c.x - a.w*b.x*c.z + a.w*b.z*c.x,
	      + a.x*b.y*c.w - a.x*b.w*c.y - a.y*b.x*c.w + a.y*b.w*c.x + a.w*b.x*c.y - a.w*b.y*c.x,
	      - a.x*b.y*c.z + a.x*b.z*c.y + a.y*b.x*c.z - a.y*b.z*c.x - a.z*b.x*c.y + a.z*b.y*c.x);
	}

	bool operator == (const Vector4<T>& c) const {
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

  union {
    Vec d[4];
    EigMat e;
  };

public:
  Matrix4() { storeZero(); }
  Matrix4(const Matrix4<T>& c) {
    d[0] = c.d[0]; d[1] = c.d[1]; d[2] = c.d[2]; d[3] = c.d[3];
  }
  Matrix4(Vec x, Vec y, Vec z, Vec w) {
    d[0] = x; d[1] = y; d[2] = z; d[3] = w;
  }
  Matrix4(const T* vals) : e(vals) {}

  EigMat& eigen() { return e; }
  const EigMat& eigen() const { return e; }
  void setEigen(const EigMat& new_e) { e = new_e; }

  T* raw() { return (T*)&d[0]; }
  const T* raw() const { return (T*)&d[0]; }
  Vec& operator[] (int index) { return d[index]; }
  void setRow(int index, const Vec& row) { d[index] = row; }

  bool operator ==(const FdMat& c) const {
    return d[0] == c.d[0] && d[1] == c.d[1] && d[2] == c.d[2] && d[3] == c.d[3];
  }

  FdMat& operator =(const FdMat& right) {
    e = right.e;
    return *this;
  }

  FdMat&  storeZero() {
    for (int i = 0; i < 4; i++) {
      d[i].storeZero();
    }
    return *this;
  }

  FdMat& storeIdentity() {
    storeZero();
    d[0].x = d[1].y = d[2].z = d[3].w = 1;
    return *this;
  }

  FdMat& buildRotation(float radians, int targetIndex, int sourceIndex) {
    assert(targetIndex < 4 && targetIndex >= 0);
    assert(sourceIndex < 4 && sourceIndex >= 0);
    storeIdentity();
    T cosResult = cos(radians);
    T sinResult = sin(radians);
    Vec newTarget = d[targetIndex] * cosResult + d[sourceIndex] * sinResult;
    Vec newSource = d[targetIndex] * -sinResult + d[sourceIndex] * cosResult;
    d[targetIndex] = newTarget;
    d[sourceIndex] = newSource;
    return *this;
  }

  FdMat inverse() const {
    FdMat inv = transpose();
    EigMat inverse = inv.e.inverse().eval();
    inv.e = inverse;
    return inv.transpose();
  }

  FdMat transpose() const {
    FdMat r;
    r.d[0].set(d[0].x, d[1].x, d[2].x, d[3].x);
    r.d[1].set(d[0].y, d[1].y, d[2].y, d[3].y);
    r.d[2].set(d[0].z, d[1].z, d[2].z, d[3].z);
    r.d[3].set(d[0].w, d[1].w, d[2].w, d[3].w);
    return r;
  }

  FdMat operator * (const FdMat& m) const {
    FdMat r;
    FdMat t = m.transpose();
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        r[i][j] = d[i].dot(t[j]);
      }
    }
    return r;
  }

  Vec transform(const Vec& v) const {
    Vec result(v.dot(d[0]), v.dot(d[1]), v.dot(d[2]), v.dot(d[3]));
    return result;
  }

  void storeOrthognoal(int invariant, int variant, int secondary, int tertiary) {
    d[invariant].storeNormalized();
    d[variant] = d[invariant].cross(d[secondary], d[tertiary]);
    d[variant].storeNormalized();
    d[tertiary] = d[invariant].cross(d[variant], d[secondary]);
    d[tertiary].storeNormalized();
    d[secondary] = d[invariant].cross(d[tertiary], d[variant]);
    d[secondary].storeNormalized();
  }

  void printIt() const {
    for (int i = 0; i < 4; i++) {
      printf("(%d)\t", i);
      d[i].printIt();
      printf("\n");
    }
  }

  // Split this 4d matrix, interpreted as the usual 3d view + 3d position into a valid
  // 4d view + 4d position. Ok to pass self as viewTarget.
  void splice3dInto4d(FdMat& viewTarget, Vec& posTarget) const {
    posTarget = d[3];
    posTarget.w = 0;

    viewTarget = *this;
    viewTarget.d[0].w = 0;
    viewTarget.d[1].w = 0;
    viewTarget.d[2].w = 0;
    viewTarget.d[3].set(0, 0, 0, 1);
  }
};

typedef Matrix4<float> Mat4f;

} // namespace fd
