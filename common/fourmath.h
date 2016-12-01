#pragma once

#include <float.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "../eigen/Eigen/Geometry"
#include "../eigen/Eigen/LU"
#include "mem_helpers.h"

namespace fd {

// derp
#define PI (3.141592653589793238462643383)

template <typename T>
class Vector4 {
protected:
  typedef Vector4<T> Vec;

public:
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
  Vector4(Eigen::Vector4f& v) : x(v.x()), y(v.y()), z(v.z()), w(v.w()) {}

	inline T* raw() { return (T*)this; }
	inline const T* raw() const { return (T*)this; }
	inline T& operator()(int index) { return d[index]; }
	inline T& operator[](int index) { return d[index]; }
	inline const T& operator[](int index) const { return d[index]; }
	inline void set(int index, T val) { d[index] = val; }
	inline void set(const Vec& rVal) {
	  x = rVal.x; y = rVal.y; z = rVal.z; w = rVal.w;
	}
  inline void set(T inX, T inY, T inZ, T inI) {
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
  Vec& operator *= (const Vec& r) {
    set(x * r.x, y * r.y, z * r.z, w * r.w);
    return *this;
  }
  Vec& operator /= (const Vec& r) {
    set(x / r.x, y / r.y, z / r.z, w / r.w);
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

  Vec operator / (const Vec& r) const {
    return Vec(x / r.x, y / r.y, z / r.z, w / r.w);
  }

	T dot(const Vec& v) const {
    return x * v.x + y * v.y + z * v.z + w * v.w;
	}

  inline T lengthSq() const {
    return (x*x + y*y + z*z + w*w);
  }

	T length() const {
	  return sqrt(lengthSq());
	}

	Vec& storeZero() {
	  set(0, 0, 0, 0);
    return *this;
	}

	Vec normalized() const {
	  Vec result(*this);
	  result.storeNormalized();
	  return result;
	}

	Vec& storeNormalized() {
	  T len = length();
	  if (len != (T)0) {
	    len = (T)1 / len;
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
	  static const T threshold = (T)0.00001;
    return approxEqual(c, threshold);
	}

  bool approxEqual(const Vec& c, T threshold) const {
	  return fabs(x - c.x) < threshold
	      && fabs(y - c.y) < threshold
        && fabs(z - c.z) < threshold
        && fabs(w - c.w) < threshold;
  }

	void printIt() const {
	  printf("%f\t%f\t%f\t%f", x, y, z, w);
	}
  
  public:
    static Vec s_ones;
};

typedef Vector4<float> Vec4f;

typedef Eigen::Vector4i Vec4i; // dunno bout eigen, I have to do x()? bleh
typedef Eigen::Vector2f Vec2f;
typedef Eigen::Vector2i Vec2i;

template <typename T>
class Quaternion;

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
  Matrix4() { storeIdentity(); }
  Matrix4(const Matrix4<T>& c) {
    d(0) = c.d(0); d(1) = c.d(1); d(2) = c.d(2); d(3) = c.d(3);
  }
  Matrix4(Vec x, Vec y, Vec z, Vec w) {
    d(0) = x; d(1) = y; d(2) = z; d(3) = w;
  }
  Matrix4(const T* vals) : e(vals) {}

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
  const Vec& operator[] (int index) const { return d(index); }
  void setRow(int index, const Vec& row) { d(index) = row; }

  bool operator ==(const FdMat& c) const {
    return d(0) == c.d(0) && d(1) == c.d(1) && d(2) == c.d(2) && d(3) == c.d(3);
  }
  bool operator !=(const FdMat& c) const {
    return !(operator ==(c));
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

  FdMat& storeRotation(T radians, int targetIndex, int sourceIndex) {
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

  FdMat& storeQuat3dRotation(Quaternion<T> quat);

  // Aspect is width/height of viewport. This is opengl style.
  FdMat& store3dProjection(T yFov, T aspect, T zNear, T zFar) {
    T yMax = tan(yFov * (T)PI / 360.0f);
    T xMax = yMax * aspect;

    T depth = zFar - zNear;
    T zScale = -(zFar + zNear) / depth;
    T zOffset = (T)-2.0 * (zFar * zNear) / depth;

    storeZero();
    raw()[0] = (T)1.0 / xMax;
    raw()[5] = (T)1.0 / yMax;
    raw()[10] = zScale;
    raw()[11] = (T)-1.0;
    raw()[14] = zOffset;
    return *this;
  }

  FdMat& storeScale(Vec scale) {
    storeZero();
    raw()[0] = scale.x;
    raw()[5] = scale.y;
    raw()[10] = scale.z;
    raw()[15] = scale.w;
    return *this;
  }

  FdMat& storeScale(T scale) {
    storeZero();
    raw()[0] = scale;
    raw()[5] = scale;
    raw()[10] = scale;
    raw()[15] = scale;
    return *this;
  }

  FdMat inverse() const {
    FdMat inv = transpose();
    EigMat inverse = inv.eigen().inverse().eval();
    inv.eigen() = inverse;
    return inv.transpose();
  }

  void storeFromTransposedArray(const T* raw) {
    FdMat temp(raw);
    *this = temp.transpose();
    //T* d = raw();
    //d[0] = raw[0];
    //d[1] = raw[4];
    //d[2] = raw[8];
    //d[3] = raw
  }

  FdMat transpose() const {
    FdMat r;
    r.d(0).set(d(0).x, d(1).x, d(2).x, d(3).x);
    r.d(1).set(d(0).y, d(1).y, d(2).y, d(3).y);
    r.d(2).set(d(0).z, d(1).z, d(2).z, d(3).z);
    r.d(3).set(d(0).w, d(1).w, d(2).w, d(3).w);
    return r;
  }

  void operator *= (const FdMat& r) {
    *this = *this * r;
  }

  FdMat operator * (const FdMat& m) const {
    FdMat r;
    // TODO: the briefest amount of energy toward optimization could be fruitful.
    FdMat t = m.transpose();
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        r[i][j] = d(i).dot(t[j]);
      }
    }
    return r;
  }

  Vec operator * (const Vec& r) const {
    return transform(r);
  }

  FdMat operator * (const T& s) const {
    FdMat r(*this);
    r.eigen() *= s;
    return r;
  }

  void operator *= (const T& s) {
    *this = *this * s;
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

  ALIGNED_ALLOC_NEW_DEL_OVERRIDE

public:
  static FdMat s_ident;

};

typedef Matrix4<float> Mat4f;

template <typename T>
class Pose4 {
public:
  typedef Matrix4<T> Mat;
  typedef Vector4<T> Vec;
  typedef Pose4<T> Pose;
  Mat rotation;
  Vec position;

  Pose4() {}
  Pose4(const Mat& rot, const Vec& pos) : rotation(rot), position(pos) {}

  void storeIdentity() {
    rotation.storeIdentity();
    position.storeZero();
  }

  // M * x = R * x + P
  // M * M * x = (R * (R * x + P) + P)
  // M * M * x = RRx + RP + P
  // (M * M) * x = RR * x + (RP + P)
  Pose operator * (const Pose& m) const {
    Pose r;
    r.rotation = rotation * m.rotation;
    r.position = rotation.transform(m.position) + position;
    return r;
  }

  // M * x = R * x + P
  Vec operator * (const Vec& r) const {
    return (rotation * r) + position;
  }

  // A 4x4 matrix is used for rotation, and a 4 vector for position.
  // Transforming a vector, x, by the pose, M, can be
  // M * x = (R * x) + P
  // where R is rotation and P is position.
  // For the inverse, 
  // M^-1 * M * x = x = M^-1 * ((R * x) + P)
  // x * ((R * x) + P)^-1 = M^-1
  Pose4& invert() {
    Mat rotInv = rotation.inverse();
    position = -rotInv.transform(position);
    rotation = rotInv;
    return *this;
  }

  // a stupid orthogonal projection to xyz
  Mat projectTo3Pose() const {
    Mat pose3 = rotation.convertToTransposed3d();
    pose3.d(3).set(position);
    pose3.d(3).w = 0;
    return pose3;
  }

  ALIGNED_ALLOC_NEW_DEL_OVERRIDE
};

typedef Pose4<float> Pose4f;

// So the premise is that you take sqrt(-1) = i, interpreted as rotation,
// and generalize the concept to 3d so you have 3 independent axis of rotation.
// So you have i,j,k, where
// i*j*k = -1, i*i = -1, j*j = -1, k*k = -1.
// To make this work, you give up commutivity so i*j != j*i.
// Actually i*j=k and j*i=-k so you can convert between different values
// You also need a real part, which gives 4 independent values.
// The nice thing is if these are "normalized", you can get a 3d rotation.
// The weird thing about that is you get two sets of r,i,j,k that equal the
// same rotation.
// If some of this doesn't make sense, understanding of i as a rotation helps.
// At least, that helped me. Why all the exposition? Haven't actually
// written a quat class before. Reinventing the wheel is a good idea, once, for
// sufficiently complex wheels.
template <typename T>
class Quaternion {
public:
  union {
    T d[4];
    struct {
      T i, j, k, r;
    };
    struct {
      T x, y, z, w;
    };
  };

  Quaternion() : r((T)1.0), i(0), j(0), k(0) {}
  Quaternion(T rIn, T iIn, T jIn, T kIn) : r(rIn), i(iIn), j(jIn), k(kIn) {}

  Quaternion& storeIdentity() {
    r = (T)1.0;
    i = 0;
    j = 0;
    k = 0;
    return *this;
  }

  Quaternion operator *(const Quaternion& q) const {
    Quaternion out;
    const Quaternion& p = *this; // lezy
    out.r = (p.r*q.r) - (p.i*q.i) - (p.j*q.j) - (p.k*q.k);
    out.i = (p.r*q.i) + (p.i*q.r) + (p.j*q.k) - (p.k*q.j);
    out.j = (p.r*q.j) - (p.i*q.k) + (p.j*q.r) + (p.k*q.i);
    out.k = (p.r*q.k) + (p.i*q.j) - (p.j*q.i) + (p.k*q.r);
    return out;
  }

  T length() const {
    return sqrt((r*r) + (i*i) + (j*j) + (k*k));
  }

  Quaternion& storeNormalized() {
    T len = length();
    if (len != 0) {
      T invLen = (T)1.0 / len;
      r *= invLen;
      i *= invLen;
      j *= invLen;
      k *= invLen;
    }
    return *this;
  }

  // TODO: doesn't handle double coverage, should it?
  bool approxEqual(const Quaternion& p, float threshold) const {
    return fabs(r - p.r) < threshold
        && fabs(i - p.i) < threshold
        && fabs(j - p.j) < threshold
        && fabs(k - p.k) < threshold;
  }
};

typedef Quaternion<float> Quatf;

// This comes from setting three "unit quaternions" like
// q_i = (0,1,0,0), q_j = (0,0,1,0), q_k = (0,0,0,1)
// and then multiplying through q * q_i * q_conj where
// q = (r, a*i, b*j, c*k) and q_conj = (r, -a*i, -b*j, -c*k).
// If you take q*q_i*q_conj as the first row,
// q*q_j*q_conj as second row, q*q_k*q_conj as third row
// (and really, q*q_identity*q as fourth row, where q_identity=(1,0,0,0))
// you get a 4x4 matrix where the top right 3x3 is actually the rotation.
// This ended up being openGL col/row convention.
template <typename T>
Matrix4<T>& Matrix4<T>::storeQuat3dRotation(Quaternion<T> q) {
  T rr = q.r * q.r;
  T ii = q.i * q.i;
  T jj = q.j * q.j;
  T kk = q.k * q.k;
  T ri = q.r * q.i;
  T rj = q.r * q.j;
  T rk = q.r * q.k;
  T ij = q.i * q.j;
  T ik = q.i * q.k;
  T jk = q.j * q.k;

  // Ugh my row/col conventions all over the fucking place.
  T* d = raw();
  d[0] = rr + ii - jj - kk;
  d[1] = (ij + rk) * (T)(2.0);
  d[2] = (ik - rj) * (T)(2.0);
  d[3] = (T)0;

  d[4] = (ij - rk) * (T)(2.0);
  d[5] = rr - ii + jj - kk;
  d[6] = (jk + ri) * (T)(2.0);
  d[7] = (T)0;

  d[8] = (ik + rj) * (T)(2.0);
  d[9] = (jk - ri) * (T)(2.0);
  d[10] = rr - ii - jj + kk;
  d[11] = (T)0;

  d[12] = (T)0;
  d[13] = (T)0;
  d[14] = (T)0;
  d[15] = (T)1.0;

  return *this;
}

} // namespace fd
