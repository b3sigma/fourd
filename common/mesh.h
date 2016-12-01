#pragma once

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include <assert.h>
#include "types.h"
#include "fourmath.h"

namespace fd {

class MeshConverter {
public:
  MeshConverter() {}
};

// Windings are a mess and should be considered meaningless.
class Mesh {
public:
  friend MeshConverter;

  typedef std::vector<Vec4f> VecList;
  VecList _verts;
  typedef std::vector<int> IndexList;
  IndexList _indices;
  VecList _colors;

  Mesh() {}

  int getNumberTriangles() {
    return (int)_indices.size() / 3;
  }

  void getTriangle(int index, Vec4f& a, Vec4f& b, Vec4f& c);
  void getColors(int index, Vec4f& a, Vec4f& b, Vec4f& c);
  void printIt();

  // Schlafli description explaination.
  // Read from left to right, recursively up a dimension per comma.
  // Eg {4,3,3} tesseract:
  // So the first 4 means 4 edges in 2d (square) to make a face.
  // Second 3 means 3 faces per vertex (cube) to make a cell.
  // Third 3 means 3 cubes per edge.
  // But everything has to be connected up, so you get a closed tesseract.
  // Nice math description, not so great for imperative code.
  // Also only good for nice symmetric stuff, so the four-cylinder is no go.
  void buildQuad(float size, Vec4f offset, Vec4f step); // {4}
  void buildCube(float size, Vec4f offset, Vec4f numberedSkewStep = Vec4f()); // {4,3}
  void buildSphere(float size, Vec4f offset);
  void buildSpherinder(float sphereSize, float cylSize, Vec4f offset = Vec4f());
  
  void buildTesseract(float size, Vec4f offset = Vec4f(), Vec4f step = Vec4f()); // {4,3,3}
  void buildTesseract(const Vec4f& min, const Vec4f& max);
  void buildReferenceTesseract(float size, Vec4f offset, Vec4f step); // {4,3,3}
  void buildFourTetrad(float size, Vec4f offset); // 5-cell {3,3,3}
  void buildCircle(float radius, Vec4f center, Vec4f right, Vec4f up, int faceCount);
  void buildCylinder(float radius, float length, int faceCount);
  void buildFourCylinder(float radius, float length, float inside, int faceCount);
  void build16cell(float radius, Vec4f offset); // {3,3,4}
  void buildGeneralized16cell(float radius, Vec4f offset);
  void build120cell(float radius, Vec4f offset); // {5,3,3}
  void buildGeneralizedTesseract(float size, const Vec4f& start);
  
  void buildCaylayTesseract(float radius, Vec4f offset);
  void buildCaylay16Cell(float radius, Vec4f offset);
  void buildCaylay120Cell(float radius, Vec4f offset);
  void buildCaylay24Cell(float radius, Vec4f offset);
  void buildCaylay600Cell(float radius, Vec4f offset);

  // uses internal state and essentially iterates through building stuff it knows about
  void buildCaylayEnumerated(float radius, Vec4f offset, int enumDir = 1);

  void buildQuaxolTesseract(float size); // assumptions about tri positions

  void fillSolidColors(Vec4f& color);
  //void carveSolids(

  virtual void clearCurrent();
  void projectIntoFour(float insideDist, Vec4f numberedSkewStep = Vec4f());
  void merge(const Mesh& other);
  void tesselateByThree();
  void tesselateBySix();
  
private:
  void populateVerts(float size, int dim, const Vec4f& offset, const Vec4f& numberedSkewStep);
  void populateVerts(const Vec4f& min, const Vec4f& max, int dim);
  void buildNormals(const VecList& verts, const IndexList& indices, VecList& normals);
  void addTri(int a, int b, int c);
  void addQuad(int a, int b, int c, int d);
  void addTri(int a, int b, int c, IndexList& indices);
  void addQuad(int a, int b, int c, int d, IndexList& indices);
  void addCube(int a, int b, int c, int d, int e, int f, int g, int h);
  int addUniqueVert(const Vec4f& vert); // wow this is slow and inaccurate
  void addPolyVerts(float radius, Vec4f center, Vec4f right, Vec4f up, int faceCount);

  typedef std::pair<int, int> Edge;
  typedef std::vector<Edge> Edges;
  typedef std::map<int64, int> TriHash;

  // 2d thing, with
  class Polygon {
  public:
    Mesh& _mesh;
    Vec4f _normal; // in 3d bi-unique, planar in 4d, but pick one
    Vec4f _center;

    IndexList _indVerts;
    Edges _edges; // redudant with _vert[i],_vert[i+1%l] ?

    int64 _hash;
    bool _hashIsDirty;
  public:
    Polygon(Mesh& mesh) : _mesh(mesh), _hashIsDirty(true) {}
    void MarkDirty() { _hashIsDirty = true; }
    int64 GetHash();
    void AddUniqueTriangles();
  };
  typedef std::vector<Polygon*> Polygons;
  typedef std::map<int64, Polygon*> PolyHash;

  // 3d thing, for polytopes
  class Cell {
  public:
    Mesh& _mesh;

    Vec4f _normal; // the one 4d dir this doesn't go
    Vec4f _center;
    Polygons _polys;

    int64 _hash;
    bool _hashIsDirty;

  public:
    Cell(Mesh& mesh) : _mesh(mesh), _hashIsDirty(true) {}
    void MarkDirty() { _hashIsDirty = true; }
    int64 GetHash();
  };
  typedef std::vector<Cell*> Cells;
  typedef std::map<int64, Cell*> CellHash;

  PolyHash _polys; // temp storage
  CellHash _cells; // temp storage
  TriHash _uniqueTris;

  Polygon* addPolygon(float baseLen, const Vec4f& baseVert,
      const Vec4f& planeX, const Vec4f& planeY, const Vec4f& normal,
      int vertsPerPoly);
  Cell* addCell(float baseLen, Polygon* poly, const Vec4f& cellNormal,
      int vertsPerPoly, int polysPerCellVert, float windingSignFlip);
  void buildPolytope(float baseLen, Vec4f start,
      int vertsPerPoly, int polysPerCellVert, int cellsPerEdge);
  void cleanupPolysAndCells();
  void cleanupUniqueTriangles();

  class Shape;
  typedef std::vector<Shape*> Shapes;

  // 3d thing, arbitrary tri collections supported
  class Shape {
   private:
    // vertex storage elsewhere
    VecList& _globalVerts;
    // index storage elsewhere
    IndexList& _globalIndices;

    IndexList _triangles;

    typedef std::map<int64, int> EdgeMap;
    EdgeMap _allEdges; // edge codes to count of edges
    Edges _interiors;
    Edges _exteriors;

    TriHash _uniqueTris;

   public:
    Shape(VecList& verts, IndexList& indices) : _globalVerts(verts), _globalIndices(indices) {}
    const Edges& getExteriors() const { return _exteriors; }
    const IndexList& getTriangles() const { return _triangles; }

    void analyzeEdges();
    void printIt();

    void addTriangleWithEdges(int a, int b, int c); // doesn't update unique tri list, adds edges
    void addEdge(int a, int b); // Handles dups.
    void addUniqueTriangle(int a, int b, int c); // handles dups, no edges added

    static void cleanupShapes(Shapes& shapes);
  };


  class TriConnectivity {
    // vertex storage elsewhere
    VecList& _verts;
    // index storage elsewhere
    IndexList& _indices;

   public:
    TriConnectivity(VecList& verts, IndexList& indices) : _verts(verts), _indices(indices) {}
    ~TriConnectivity();

    Shape* buildEmptyShape() const { return new Shape(_verts, _indices); }
    void buildGraph();

    class Triangle {
    public:
      typedef std::list<Triangle*> TriangleList;
      typedef TriangleList::iterator iterator;
      typedef std::set<int64> EdgeSet;

     private:
      IndexList _indices;
      const VecList& _globalVerts;
      TriangleList _connections;
      EdgeSet _edges;
      int64 _triCode;
      int _triIndex;

     public:
      Triangle(int startIndex, const VecList& verts, const IndexList& indices);

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

  Shape* collectCoPlanar(TriConnectivity& connectivity, int triangle, TriHash& unique);
  void buildShapes(VecList& verts, IndexList& indices, Shapes& outShapes);
};

}  // namespace fd
