#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include "fdMesh.h"
#include "fdMath.h"

using namespace ::fd;

void Mesh::getTriangle(int index, Vec4f& a, Vec4f& b, Vec4f& c) {
  assert(index < getNumberTriangles() && index >= 0);

  int aInd = _indices[index * 3];
  int bInd = _indices[index * 3 + 1];
  int cInd = _indices[index * 3 + 2];

  assert(aInd < (int)_verts.size() && aInd >= 0);
  assert(bInd < (int)_verts.size() && bInd >= 0);
  assert(cInd < (int)_verts.size() && cInd >= 0);

  a = _verts[aInd];
  b = _verts[bInd];
  c = _verts[cInd];
}

void Mesh::printIt() {
  printf("Had %d verts, %d indices, %d tris\n", (int)_verts.size(), (int)_indices.size(), getNumberTriangles());
  for (unsigned int iV = 0; iV < _verts.size(); iV++) {
    Vec4f& v = _verts[iV];
    printf("v(%d):\t%f\t%f\t%f\t%f\n", iV, v.x, v.y, v.z, v.w);
  }
  for (unsigned int iI = 0; iI < _indices.size(); iI++) {
    int& i = _indices[iI];
    printf("i(%d):%d\n", iI, i);
  }
}

void Mesh::populateVerts(float size, int dim, const Vec4f& offset, const Vec4f& step) {
  assert(dim <= 4 && dim >= 2);
  int numVerts = 1 << dim;
  _verts.resize(0);
  _verts.reserve(numVerts);
  for (int i = 0; i < numVerts; i++) {
    int possibleDim = dim - 1;
    const int& mask = i;
    Vec4f vert;
    while (possibleDim >= 0) {
      if (mask & (1 << possibleDim)) {
        vert.set(possibleDim, size);
      }
      possibleDim--;
    }

    _verts.push_back(vert + offset + (step * i));
  }
}

void Mesh::addTri(int a, int b, int c) {
  addTri(a, b, c, _indices);
}

void Mesh::addQuad(int a, int b, int c, int d) {
  addQuad(a, b, c, d, _indices);
}

void Mesh::addTri(int a, int b, int c, IndexList& indices) {
  indices.push_back(a);
  indices.push_back(b);
  indices.push_back(c);
}

void Mesh::addQuad(int a, int b, int c, int d, IndexList& indices) {
  addTri(a, b, c, indices);
  addTri(c, b, d, indices);
}

void Mesh::buildQuad(float size, Vec4f offset, Vec4f step) {
  populateVerts(size, 2, offset, step);

  _indices.resize(0);
  _indices.reserve(6);

  addQuad(0, 1, 2, 3);
}

void Mesh::addCube(int a, int b, int c, int d, int e, int f, int g, int h) {
  addQuad(a, b, c, d);
  addQuad(a, b, e, f);
  addQuad(a, c, e, g);
  addQuad(b, d, f, h);
  addQuad(c, d, g, h);
  addQuad(e, f, g, h);
}

void Mesh::buildCube(float size, Vec4f offset, Vec4f step) {
  int dim = 3;
  populateVerts(size, dim, offset, step);

  _indices.resize(0);
  _indices.reserve(6 * 6);

  addCube(0, 1, 2, 3, 4, 5, 6, 7);
}

// The manual one using explicitly defined indices:
// 00  01
// 02  03
//   04  05
//   06  07
//  08  09
//  10  11
//    12  13
//    14  15
// Note this shape doesn't do duplicate face removal so there are doubles
// where two cubes are next to each other and share a face (everywhere).
void Mesh::buildReferenceTesseract(float size, Vec4f offset, Vec4f step) {
  int dim = 4;
  populateVerts(size, dim, offset, step);

  _indices.resize(0);
  _indices.reserve(6 * 6 * 8);

  addCube(0, 1, 2, 3, 4, 5, 6, 7);
  addCube(8, 9, 10, 11, 12, 13, 14, 15);
  addCube(0, 2, 4, 6, 8, 10, 12, 14);
  addCube(0, 1, 4, 5, 8, 9, 12, 13);
  addCube(1, 3, 5, 7, 9, 11, 13, 15);
  addCube(2, 3, 6, 7, 10, 11, 14, 15);
  addCube(4, 5, 6, 7, 12, 13, 14, 15);
  addCube(0, 1, 2, 3, 8, 9, 10, 11);
}

// TODO: did I mention finding and using a good unit test framework?

void Mesh::buildTesseract(float size, Vec4f offset, Vec4f step) {
  buildCube(size, offset, step);
  projectIntoFour(size);
}

void Mesh::buildFourTetrad(float size, Vec4f offset) {
  int numVerts = 5;
  _verts.resize(0);
  _verts.reserve(numVerts);

  Vec4f vert;
  vert.set(0,0,0,0);
  _verts.push_back(vert);
  vert.set(size, 0, 0, 0);
  _verts.push_back(vert);
  vert.set(0, size, 0, 0);
  _verts.push_back(vert);
  vert.set(0, 0, size, 0);
  _verts.push_back(vert);
  vert.set(0, 0, 0, size);
  _verts.push_back(vert);

  for (VertList::iterator iV = _verts.begin();
      iV != _verts.end();
      ++iV) {
    Vec4f& v = *iV;
    v += offset;
  }

  assert(numVerts == (int)_verts.size());

  _indices.resize(0);
  for (int i = 0; i < numVerts; i++) {
    for (int j = i + 1; j < numVerts; j++) {
      for (int k = j + 1; k < numVerts; k++) {
        // The windings will be haphazard...
        addTri(i, j, k);
      }
    }
  }
}

// This doesn't work. You need a vector per dimension you want to constrain but
// with triangles you are only really constrained in two reduced dimensions, leaving an
// entire plane (in 4d, or n-2 in n) with triangles. I started writing this thinking that
// I could use the 3rd edge but then realized I am dumb and that is defined as 0 (pretty sure
// there's an elegant tie in with curvature for that). Anyways, fcuk me?
void Mesh::buildNormals(const VertList& verts, const IndexList& indices, VertList& normals) {
  int numTris = (int)(indices.size() / 3);
  normals.resize(0);
  normals.reserve(numTris);

  for (int iT = 0; iT < numTris; ++iT) {
    int iI = iT * 3;
    const Vec4f& v1 = verts[indices[iI]];
    const Vec4f& v2 = verts[indices[iI + 1]];
    const Vec4f& v3 = verts[indices[iI + 2]];

    Vec4f edge1 = v2 - v1;
    Vec4f edge2 = v3 - v1;
    Vec4f fcukMeThisIsTooUnconstrained = Vec4f(0.1f, 0.2f, 0.3f, 0.4f);
    Vec4f edge3 = fcukMeThisIsTooUnconstrained;
    Vec4f normal = edge1.cross(edge2, edge3);
    normal.storeNormalized();

    normals.push_back(normal);
  }
}

int64 Mesh::makeUniqueEdgeCode(int a, int b) {
  if (a > b) {
    return int64((int64)b & c_edgeIndexMask)
        | (((int64)a & c_edgeIndexMask) << c_edgeShiftAmount);
  } else {
    return int64((int64)a & c_edgeIndexMask)
        | (((int64)b & c_edgeIndexMask) << c_edgeShiftAmount);
  }
}

Mesh::Edge Mesh::decodeEdgeCode(int64 edgeCode) {
  int lower = edgeCode & c_edgeIndexMask;
  int upper = (edgeCode >> c_edgeShiftAmount) & c_edgeIndexMask;
  return Edge(lower, upper);
}

int64 Mesh::makeUniqueTriCode(int a, int b, int c) {
  typedef std::vector<int> IntList;
  IntList indices;
  indices.push_back(a);
  indices.push_back(b);
  indices.push_back(c);
  std::sort(indices.begin(), indices.end());
  const int shiftAmount = 16;
  const int indexMask = (1 << shiftAmount) - 1; // hidden here for your dismay, a limit on indices of 32k
  return int64(((int64)indices[0] & indexMask)
      | (((int64)indices[1] & indexMask) << shiftAmount)
      | (((int64)indices[2] & indexMask) << (shiftAmount * 2)));
}

Mesh::Connectivity::Triangle::Triangle(int startIndex, const VertList& verts, const IndexList& indices)
    : _globalVerts(verts) {
  _triIndex = startIndex / 3;
  _indices.resize(3);
  _indices[0] = indices[startIndex];
  _indices[1] = indices[startIndex + 1];
  _indices[2] = indices[startIndex + 2];
  _triCode = Mesh::makeUniqueTriCode(_indices[0], _indices[1], _indices[2]);

  int numIndices = (int)_indices.size();
  for (int i = 0; i < numIndices; i++) {
    int nextIndex = (i + 1) % _indices.size();
    _edges.insert(makeUniqueEdgeCode(_indices[i], _indices[nextIndex]));
  }
}

Mesh::Connectivity::~Connectivity() {
  for (TriangleHash::iterator iTri = _triHash.begin();
      iTri != _triHash.end();
      ++iTri) {
    Triangle* triangle = iTri->second;
    delete triangle;
  }
}

void Mesh::Connectivity::buildGraph() {
  int numIndices = (int)(_indices.size());
  // first just build all the triangles
  for (int iT = 0; iT < numIndices; iT = iT + 3) {
    Triangle* triangle = new Triangle(iT, _verts, _indices);
    int triIndex = iT / 3;
    _triHash.insert(std::make_pair(triIndex, triangle));
    _triCodeHash.insert(std::make_pair(triangle->getTriCode(), triangle));
    _triIndexHash.insert(std::make_pair(triangle->getIndex(0), triangle));
    _triIndexHash.insert(std::make_pair(triangle->getIndex(1), triangle));
    _triIndexHash.insert(std::make_pair(triangle->getIndex(2), triangle));
  }

  for (auto iTri = _triCodeHash.begin();
      iTri != _triCodeHash.end();
      ++iTri) {
    Triangle* triangle = iTri->second;
    for (int which = 0; which < 3; which++) {
      int searchIndex = triangle->getIndex(which);
      IndexHashRange iConnects = _triIndexHash.equal_range(searchIndex);
      for (;
          iConnects.first != iConnects.second && iConnects.first != _triIndexHash.end();
          ++iConnects.first) {
        IndexHashPair connectedPair = *(iConnects.first);
        Triangle* connectedTri = connectedPair.second;
        if (connectedTri != triangle && triangle->isNextTo(connectedTri)) {
          triangle->addConnection(connectedTri);
        }
      }
    }
  }
}

Mesh::Connectivity::Triangle* Mesh::Connectivity::getTriangle(int triIndex) {
  TriangleHash::iterator iTri = _triHash.find(triIndex);
  if (iTri != _triHash.end()) {
    return iTri->second;
  }
  return NULL;
}

bool Mesh::Connectivity::Triangle::isNextTo(const Triangle* other) const {
  EdgeSet intersectionSet;
  std::set_intersection(_edges.begin(), _edges.end(),
      other->_edges.begin(), other->_edges.end(),
      std::inserter(intersectionSet, intersectionSet.begin()));
  return !intersectionSet.empty();
}

bool Mesh::Connectivity::Triangle::isCoplanarWith(const Triangle* other) const {
  EdgeSet disjointSet;
  std::set_symmetric_difference(_edges.begin(), _edges.end(),
      other->_edges.begin(), other->_edges.end(),
      std::inserter(disjointSet, disjointSet.begin()));

  if (disjointSet.size() < 3) {
    printf("Degenerate triangles in coplanar test!\n");
    return true;
  }

  VertList edgeVectors;
  for (EdgeSet::iterator iEdge = disjointSet.begin();
      iEdge != disjointSet.end();
      ++iEdge) {
    Edge edge = decodeEdgeCode(*iEdge);
    Vec4f line = _globalVerts[edge.first] - _globalVerts[edge.second];
    line.storeNormalized();
    edgeVectors.push_back(line);
  }

  Vec4f polyCross = edgeVectors[0].cross(edgeVectors[1], edgeVectors[2]);
  const float coplanarThreshold = 0.1f; // in dot product space
  return (polyCross.length() < coplanarThreshold);
}

// TODO setup eclipse pretty printing of STL structures

Mesh::Shape* Mesh::collectCoPlanar(
    Connectivity& connectivity, int triIndex, TriHash& unique) {
  Shape* shape = connectivity.buildEmptyShape();

  Connectivity::Triangle* triangle = connectivity.getTriangle(triIndex);
  shape->addTriangle(triangle->getIndex(0), triangle->getIndex(1), triangle->getIndex(2));
  unique.insert(std::make_pair(triangle->getTriCode(), triangle->getTriIndex()));

  for (Connectivity::Triangle::iterator iC = triangle->begin();
      iC != triangle->end();
      ++iC) {
    Connectivity::Triangle* query = *iC;
    if (unique.find(query->getTriCode()) != unique.end())
      continue;

    if (triangle->isCoplanarWith(query)) {
      shape->addTriangle(query->getIndex(0), query->getIndex(1), query->getIndex(2));
      unique.insert(std::make_pair(query->getTriCode(), query->getTriIndex()));
    }
    //shape->printIt();
  }

  shape->analyzeEdges();
  return shape;
}

void Mesh::Shape::analyzeEdges() {
  for (EdgeMap::iterator iEdge = _allEdges.begin();
      iEdge != _allEdges.end();
      ++iEdge) {
    Edge edge = decodeEdgeCode(iEdge->first);
    if (iEdge->second > 1) {
      _interiors.push_back(edge);
    } else {
      _exteriors.push_back(edge);
    }
  }
}

void Mesh::Shape::printIt() {
  printf("Shape num edges:%d interior:%d exterior:%d tris:%d\n",
      (int)_allEdges.size(), (int)_interiors.size(), (int)_exteriors.size(), (int)_triangles.size());
  for (int iI = 0; iI < (int)_triangles.size(); iI += 3) {
    printf("Tri: a:%d b:%d c:%d\n", _triangles[iI], _triangles[iI + 1], _triangles[iI + 2]);
  }
  for (EdgeMap::iterator iEdge = _allEdges.begin();
      iEdge != _allEdges.end();
      ++iEdge) {
    Edge edge = decodeEdgeCode(iEdge->first);
    printf("edge: a:%d b:%d count:%d\n",
        edge.first, edge.second, iEdge->second);
  }
  for (Edges::iterator iInterior = _interiors.begin();
      iInterior != _interiors.end();
      ++iInterior) {
    Edge& edge = *iInterior;
    printf("interior: a:%d b:%d\n", edge.first, edge.second);
  }
  for (Edges::iterator iExterior = _exteriors.begin();
      iExterior != _exteriors.end();
      ++iExterior) {
    Edge& edge = *iExterior;
    printf("exterior: a:%d b:%d\n", edge.first, edge.second);
  }
}

void Mesh::buildShapes(const VertList& verts, const IndexList& indices, Shapes& outShapes) {
  Connectivity connectivity(verts, indices);
  connectivity.buildGraph();

  TriHash triHash;
  int numTris = (int)(indices.size() / 3);
  for (int iT = 0; iT < numTris; ++iT) {
    int baseIndex = iT * 3;
    int64 triCode = makeUniqueTriCode(indices[baseIndex], indices[baseIndex + 1], indices[baseIndex + 2]);
    if (triHash.find(triCode) != triHash.end()) {
      continue; // it's already been incorporated in another shape
    }

    Shape* shape = collectCoPlanar(connectivity, iT, triHash);
    if (shape == NULL)
    {
      printf("null shape? TODO: do a goddamn logger");
      continue;
    }

    const IndexList& connectedTriangles = shape->getTriangles();
    int numConnectTris = (int)(connectedTriangles.size() / 3);
    for (int iConnectTri = 0; iConnectTri < numConnectTris; iConnectTri++) {
      int baseConnectIndex = iConnectTri * 3;
      int64 newTriCode = makeUniqueTriCode(
          indices[baseConnectIndex], indices[baseConnectIndex + 1], indices[baseConnectIndex + 2]);
      triHash.insert(std::make_pair(newTriCode, iT));
    }
    outShapes.push_back(shape);
  }

}

// The premise of this is to collect all co-planar triangles that share an edge into groups
// of shapes. Then, make a copy of the 3d shape separated in the fourth dim.
// Then, link together each clump co-planar triangles along their non-shared edges.
// This should be able to take a cube and turn it into a tesseract.
// The analogous operation from 2d to 3d would take a square into a cube, a
// circle into a cylinder, and an aribtrary 2d shape into a cutout 2d shape with volume.
// More interesting 4d projections will require more interesting shape definitions.
// More intelligent projections that don't make tons of cubes will require
// a 4d approach to rendering which would do the equivalent of interior surface removal.
void Mesh::projectIntoFour(float insideDist) {
  Vec4f shift(0, 0, 0, insideDist);
  VertList fourVerts;
  fourVerts.resize(_verts.size());
  std::copy(_verts.begin(), _verts.end(), fourVerts.begin());
  for (VertList::iterator iV = fourVerts.begin();
      iV != fourVerts.end();
      ++iV) {
    Vec4f& v = *iV;
    v += shift;
  }
  int numOldVerts = (int)_verts.size();
  _verts.resize(_verts.size() + fourVerts.size());
  std::copy(fourVerts.begin(), fourVerts.end(), _verts.begin() + numOldVerts);

  int num3dVerts = numOldVerts;
  IndexList newIndices;
  newIndices.resize(_indices.size());
  std::copy(_indices.begin(), _indices.end(), newIndices.begin());
  for (IndexList::iterator iI = newIndices.begin();
      iI != newIndices.end();
      ++iI) {
    int& index = *iI;
    index += num3dVerts;
  }

  Shapes shapes;
  buildShapes(_verts, _indices, shapes);

  //printf("Had %d shapes\n", (int)shapes.size());
  //int shapeCounter = 0;
  for (Shapes::iterator iS = shapes.begin();
      iS != shapes.end();
      ++iS) {
    Shape* shape = *iS;
    //printf("For shape numero %d\n", shapeCounter);
    //shapeCounter++;

    //shape->printIt();
    const Edges& exteriors = shape->getExteriors();
    for (Edges::const_iterator iE = exteriors.begin();
        iE != exteriors.end();
        ++iE) {
      const Edge& edge = *iE;
      addQuad(edge.first, edge.second, edge.first + num3dVerts, edge.second + num3dVerts, newIndices);
    }
  }
  Shape::cleanupShapes(shapes);

  TriHash triHash;
  int numNewIndices = (int)newIndices.size();
  _indices.reserve(_indices.size() + newIndices.size());
  for (int iI = 0; iI < numNewIndices; iI += 3) {
    int64 triCode = makeUniqueTriCode(newIndices[iI], newIndices[iI + 1], newIndices[iI + 2]);
    TriHash::iterator iTri = triHash.find(triCode);
    if (iTri == triHash.end()) {
      int triIndex = iI / 3;
      triHash.insert(std::make_pair(triCode, triIndex));
      _indices.push_back(newIndices[iI]);
      _indices.push_back(newIndices[iI + 1]);
      _indices.push_back(newIndices[iI + 2]);
    } else {
      //printf("About to drop a dup! Code:x%llx a:%d b:%d c:%d, index:%d tridex:%d theirdex:%d theircode:x%llx\n",
      //    triCode, newIndices[iI], newIndices[iI + 1], newIndices[iI + 2],
      //    iI, iI / 3, iTri->second, iTri->first);
    }
  }
}

void Mesh::merge(const Mesh& other) {
  int prevIndexCount = (int)_indices.size();
  int prevVertCount = (int)_verts.size();
  std::copy(other._indices.begin(), other._indices.end(), std::back_inserter(_indices));
  std::copy(other._verts.begin(), other._verts.end(), std::back_inserter(_verts));
  int numIndices = (int)_indices.size();
  for (int i = prevIndexCount; i < numIndices; ++i) {
    _indices[i] += prevVertCount;
  }
}

// Essentially build a centered circle, in the plane made by right and up, as a fan.
void Mesh::buildCircle(float radius, Vec4f center, Vec4f right, Vec4f up, int faceCount) {
  assert(faceCount >= 3);
  right.storeNormalized();
  up.storeNormalized();

  _verts.resize(0);
  _verts.reserve(faceCount + 1);
  _verts.push_back(center);

  // I am sorry Stephanie that I could not present more of myself to make a connection.
  // I suspect I could have learned much from you but would have liked to lend more of
  // a hand than a burrito.
  for (int face = 0; face < faceCount; face++) {
    float rotation = (float)face / (float)faceCount * 2 * PI;
    float rightAmount = cos(rotation) * radius;
    float upAmount = sin(rotation) * radius;
    Vec4f next(center);
    next += (right * rightAmount) + (up * upAmount);
    _verts.push_back(next);
  }

  _indices.resize(0);
  _indices.reserve(faceCount * 3);
  for (int face = 0; face < faceCount; face++) {
    _indices.push_back(0); // center
    _indices.push_back(face + 1);
    _indices.push_back(((face + 1) % faceCount) + 1);
  }
}

void Mesh::buildCylinder(float radius, float length, int faceCount) {
  Vec4f right(radius, 0, 0, 0);
  Vec4f up(0, radius, 0, 0);
  Vec4f base(0, 0, 0, 0);
  Vec4f cap(0, 0, length, 0);
  buildCircle(radius, base, right, up, faceCount);

  int vertCount = (int)_verts.size();
  // You know, if I were smart, the extrusion into 4d code could be used
  // to extrude from the circle into 3d.
  Mesh other;
  other.buildCircle(radius, cap, right, up, faceCount);
  merge(other);

  int baseIndexOffset = 1; // +1 for base center
  int capIndexOffset = vertCount + 1; // +vertCount for base, +1 for cap center
  for (int i = 0; i < faceCount; ++i) {
    int nextFaceIndex = (i + 1) % faceCount;
    addQuad(baseIndexOffset + i, baseIndexOffset + nextFaceIndex, capIndexOffset + i, capIndexOffset + nextFaceIndex);
  }
}

void Mesh::buildFourCylinder(float radius, float length, float inside, int faceCount) {
  buildCylinder(radius, length, faceCount);
  projectIntoFour(inside);
}

void Mesh::Shape::addTriangle(int a, int b, int c) {
  _triangles.push_back(a);
  _triangles.push_back(b);
  _triangles.push_back(c);

  addEdge(a, b);
  addEdge(b, c);
  addEdge(c, a);
}

void Mesh::Shape::addEdge(int a, int b) {
  int64 edgeCode = makeUniqueEdgeCode(a, b);
  EdgeMap::iterator iEdge = _allEdges.find(edgeCode);
  if (iEdge == _allEdges.end()) {
    _allEdges.insert(std::make_pair(edgeCode, 1 /* initial count */));
  } else {
    iEdge->second++;
  }
}

void Mesh::Shape::cleanupShapes(Shapes& shapes) {
  for (Shapes::iterator iS = shapes.begin();
      iS != shapes.end();
      ++iS) {
    Shape* shape = *iS;
    delete shape;
  }
  shapes.resize(0);
}
