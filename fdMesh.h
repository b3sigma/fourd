#pragma once

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include <assert.h>
#include "fdTypes.h"
#include "fdMath.h"

// Windings are a mess and should be considered meaningless.
class Mesh {
public:

  typedef std::vector<Vec4f> VertList;
  VertList _verts;
  typedef std::vector<int> IndexList;
  IndexList _indices;

  Mesh() {}

  int getNumberTriangles() {
    return (int)_indices.size() / 3;
  }

  void getTriangle(int index, Vec4f& a, Vec4f& b, Vec4f& c);
  void printIt();

  void buildQuad(float size, Vec4f offset, Vec4f step);
  void buildCube(float size, Vec4f offset, Vec4f step);
  void buildTesseract(float size, Vec4f offset, Vec4f step);
  void buildReferenceTesseract(float size, Vec4f offset, Vec4f step);
  void buildFourTetrad(float size, Vec4f offset);
  void buildCircle(float radius, Vec4f center, Vec4f right, Vec4f up, int faceCount);
  void buildCylinder(float radius, float length, int faceCount);
  void buildFourCylinder(float radius, float length, float inside, int faceCount);
  void projectIntoFour(float insideDist);
  void merge(const Mesh& other);

private:
  void populateVerts(float size, int dim, const Vec4f& offset, const Vec4f& step);
  void buildNormals(const VertList& verts, const IndexList& indices, VertList& normals);
  void addTri(int a, int b, int c);
  void addQuad(int a, int b, int c, int d);
  void addTri(int a, int b, int c, IndexList& indices);
  void addQuad(int a, int b, int c, int d, IndexList& indices);
  void addCube(int a, int b, int c, int d, int e, int f, int g, int h);

  typedef std::pair<int, int> Edge;
  typedef std::vector<Edge> Edges;
  class Shape;
  typedef std::vector<Shape*> Shapes;

  class Shape {
   private:
    // vertex storage elsewhere
    const VertList& _globalVerts;
    // index storage elsewhere
    const IndexList& _globalIndices;

    IndexList _triangles;

    typedef std::map<int64, int> EdgeMap;
    EdgeMap _allEdges; // edge codes to count of edges
    Edges _interiors;
    Edges _exteriors;

   public:
    Shape(const VertList& verts, const IndexList& indices) : _globalVerts(verts), _globalIndices(indices) {}
    const Edges& getExteriors() const { return _exteriors; }
    const IndexList& getTriangles() const { return _triangles; }
    void addTriangle(int a, int b, int c);
    void analyzeEdges();
    void printIt();

    static void cleanupShapes(Shapes& shapes);

   private:
    void addEdge(int a, int b);
  };
  typedef std::map<int64, int> TriHash;


  class Connectivity {
    // vertex storage elsewhere
    const VertList& _verts;
    // index storage elsewhere
    const IndexList& _indices;

   public:
    Connectivity(const VertList& verts, const IndexList& indices) : _verts(verts), _indices(indices) {}
    ~Connectivity();

    Shape* buildEmptyShape() const { return new Shape(_verts, _indices); }
    void buildGraph();

    class Triangle {
    public:
      typedef std::list<Triangle*> TriangleList;
      typedef TriangleList::iterator iterator;
      typedef std::set<int64> EdgeSet;

     private:
      IndexList _indices;
      const VertList& _globalVerts;
      TriangleList _connections;
      EdgeSet _edges;
      int64 _triCode;
      int _triIndex;

     public:
      Triangle(int startIndex, const VertList& verts, const IndexList& indices);

      iterator begin() { return _connections.begin(); }
      iterator end() { return _connections.end(); }
      int getTriIndex() const { return _triIndex; }
      int64 getTriCode() const { return _triCode; }
      int getIndex(int which) const { return _indices[which]; }
      void addConnection(Triangle* tri) { _connections.push_back(tri); }

      bool isNextTo(const Triangle* other) const;
      bool isCoplanarWith(const Triangle* other) const;
    };
    Triangle* getTriangle(int triIndex);

   private:
    typedef std::pair<int, Triangle*> TriangleHashPair;
    typedef std::map<int, Triangle*> TriangleHash; // by triangle index in context of all triangles
    TriangleHash  _triHash;
    typedef std::map<int64, Triangle*> TriangleCodeHash; // by sorted 3 indices into unique tri hash
    TriangleCodeHash _triCodeHash;
    typedef std::pair<int, Triangle*> IndexHashPair;
    typedef std::multimap<int, Triangle*> TriangleIndexHash; // by each individual index list
    typedef std::pair<TriangleIndexHash::iterator, TriangleIndexHash::iterator> IndexHashRange;
    TriangleIndexHash _triIndexHash;


  };

  static int64 makeUniqueTriCode(int a, int b, int c);
  static const int c_edgeShiftAmount = 32;
  static const int c_edgeIndexMask = 0xffffffff;
  static int64 makeUniqueEdgeCode(int a, int b);
  static Edge decodeEdgeCode(int64 edgeCode);

  Shape* collectCoPlanar(Connectivity& connectivity, int triangle, TriHash& unique);
  void buildShapes(const VertList& verts, const IndexList& indices, Shapes& outShapes);
};
