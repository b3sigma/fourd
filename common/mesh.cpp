#include <algorithm>
#include <assert.h>
#include <functional>
#include <map>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "mesh.h"
#include "fourmath.h"
#include "thirdparty/jenn3d/polytopes.h"

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

void Mesh::getColors(int index, Vec4f& a, Vec4f& b, Vec4f& c) {
  if(_colors.empty() || _colors.size() < _verts.size()) return;

  assert(index < getNumberTriangles() && index >= 0);

  int aInd = _indices[index * 3];
  int bInd = _indices[index * 3 + 1];
  int cInd = _indices[index * 3 + 2];

  a = _colors[aInd];
  b = _colors[bInd];
  c = _colors[cInd];
}

// seriously wasteful for our currently only usecase
void Mesh::fillSolidColors(Vec4f& color) {
  _colors.resize(0);
  _colors.reserve(_verts.size());
  _colors.insert(_colors.begin(), _verts.size(), color);
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

void Mesh::populateVerts(const Vec4f& min, const Vec4f& max, int dim) {
  assert(dim <= 4 && dim >= 2);
  int numVerts = 1 << dim;
  _verts.resize(0);
  _verts.reserve(numVerts);
  for (int i = 0; i < numVerts; i++) {
    int possibleDim = dim - 1;
    const int& mask = i;
    Vec4f vert(min);
    while (possibleDim >= 0) {
      if (mask & (1 << possibleDim)) {
        vert.set(possibleDim, max[possibleDim]);
      }
      possibleDim--;
    }

    _verts.push_back(vert);
  }
}

void Mesh::populateVerts(float size, int dim, const Vec4f& offset, const Vec4f& numberedSkewStep) {
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

    _verts.push_back(vert + offset + (numberedSkewStep * static_cast<float>(i)));
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

void Mesh::buildCube(float size, Vec4f offset, Vec4f numberedSkewStep) {
  int dim = 3;
  populateVerts(size, dim, offset, numberedSkewStep);

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

// 00  01
// 02  03
//   04  05
//   06  07
//  08  09
//  10  11
//    12  13
//    14  15
// The purpose of this is for copying into the canonical cubes for quaxol
// connectivity.
// assumptions about tris:
// x+, x-, y+, y-, z+, z-, w+, w-
// each taking 6 * 6 indices
void Mesh::buildQuaxolTesseract(float size) {
  int dim = 4;
  populateVerts(size, dim, Vec4f(0,0,0,0), Vec4f(0,0,0,0));

  _indices.resize(0);
  _indices.reserve(6 * 6 * 8);

  addCube(1, 3, 5, 7, 9, 11, 13, 15); // x plus
  addCube(0, 2, 4, 6, 8, 10, 12, 14); // x minus
  addCube(2, 3, 6, 7, 10, 11, 14, 15); // y plus
  addCube(0, 1, 4, 5, 8, 9, 12, 13); // y minus
  addCube(4, 5, 6, 7, 12, 13, 14, 15); // z plus
  addCube(0, 1, 2, 3, 8, 9, 10, 11); // z minus
  addCube(8, 9, 10, 11, 12, 13, 14, 15); // w plus
  addCube(0, 1, 2, 3, 4, 5, 6, 7); // w minus
}

void Mesh::buildTesseract(const Vec4f& min, const Vec4f& max) {
  populateVerts(min, max, 4 /*dim*/);

  _indices.resize(0);
  _indices.reserve(6 * 6 * 8);

  addCube(1, 3, 5, 7, 9, 11, 13, 15); // x plus
  addCube(0, 2, 4, 6, 8, 10, 12, 14); // x minus
  addCube(2, 3, 6, 7, 10, 11, 14, 15); // y plus
  addCube(0, 1, 4, 5, 8, 9, 12, 13); // y minus
  addCube(4, 5, 6, 7, 12, 13, 14, 15); // z plus
  addCube(0, 1, 2, 3, 8, 9, 10, 11); // z minus
  addCube(8, 9, 10, 11, 12, 13, 14, 15); // w plus
  addCube(0, 1, 2, 3, 4, 5, 6, 7); // w minus
}

void Mesh::buildTesseract(float size, Vec4f offset, Vec4f step) {
  buildCube(size, offset, step);
  projectIntoFour(size, step);
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

  for (VecList::iterator iV = _verts.begin();
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

void Mesh::buildSphere(float size, Vec4f offset) {
  Vec4f localOffset(size, size, size, 0.0f);
  localOffset *= -0.5f; // this makes the first cube centered on the origin
  buildCube(size, offset + localOffset);
  tesselateBySix();
  //tesselateBySix();
  for(auto& vertex : _verts) {
    Vec4f localCoords(vertex - offset);
    localCoords.storeNormalized();
    localCoords *= size;
    vertex = localCoords + offset;
  }
}

void Mesh::tesselateByThree() {
  size_t numOldTris = _indices.size() / 3; // index list for triangles
  assert(numOldTris * 3 == _indices.size()); // if it's not an index list, like a strip, will have 67% of getting caught here :)

  // each tri gets a new vert
  size_t numNewVerts = numOldTris;
  size_t numOldVerts = _verts.size();
  _verts.reserve(numOldVerts + numNewVerts);

  size_t numNewTris = numOldTris * 2; // going to reuse the slot for the split tri 
  size_t numNewIndices = numNewTris * 3;
  size_t numOldIndices = _indices.size(); 
  _indices.reserve(numOldIndices + numNewIndices);
  
  for(size_t t = 0; t < numOldTris; t++) {
    size_t startInd = t * 3;
    const int& aInd = _indices[startInd + 0]; 
    int& bInd = _indices[startInd + 1]; // this one is going to get overwritten
    const int& cInd = _indices[startInd + 2]; 
    const Vec4f& a = _verts[aInd];
    const Vec4f& b = _verts[bInd];
    const Vec4f& c = _verts[cInd];
    _verts.emplace_back((a + b + c) * (1.0f/3.0f));
    int dInd = (int)_verts.size() - 1; // I am so sorry 64-bit persona, we are going to need to make these 32 bits explicitly ugh
    // new tri the first
    _indices.emplace_back(dInd); // try and preserve windings
    _indices.emplace_back(aInd);
    _indices.emplace_back(bInd);

    // new tri the second
    _indices.emplace_back(dInd);
    _indices.emplace_back(bInd);
    _indices.emplace_back(cInd);

    // for the 3rd tri, overwrite the original
    // aInd = aInd; // including for symmetry
    bInd = dInd; // this keeps the winding
    // cInd = cInd; // including for symmetry
  }
}

void Mesh::tesselateBySix() {
  size_t numOldTris = _indices.size() / 3; // index list for triangles
  assert(numOldTris * 3 == _indices.size()); // if it's not an index list, like a strip, will have 67% of getting caught here :)

  // each tri gets 4 new verts (ugh, we are duplicating verts * 1.5, but whatever)
  size_t numNewVerts = numOldTris * 4;
  size_t numOldVerts = _verts.size();
  _verts.reserve(numOldVerts + numNewVerts);

  size_t numNewTris = numOldTris * 5; // 1 goes in, 6 leave
  size_t numNewIndices = numNewTris * 3;
  size_t numOldIndices = _indices.size(); 
  _indices.reserve(numOldIndices + numNewIndices);
  
  for(size_t t = 0; t < numOldTris; t++) {
    size_t startInd = t * 3;
    int& aInd = _indices[startInd + 0]; 
    int& bInd = _indices[startInd + 1];
    int& cInd = _indices[startInd + 2]; 
    const Vec4f& a = _verts[aInd];
    const Vec4f& b = _verts[bInd];
    const Vec4f& c = _verts[cInd];
    _verts.emplace_back((a + b) * 0.5f);
    int abInd = (int)_verts.size() - 1;
    _verts.emplace_back((b + c) * 0.5f);
    int bcInd = (int)_verts.size() - 1;
    _verts.emplace_back((c + a) * 0.5f);
    int caInd = (int)_verts.size() - 1;
    _verts.emplace_back((a + b + c) * (1.0f/3.0f));
    int dInd = (int)_verts.size() - 1;

    // try and preserve abc winding
    _indices.emplace_back(aInd);
    _indices.emplace_back(abInd);
    _indices.emplace_back(dInd);

    _indices.emplace_back(aInd);
    _indices.emplace_back(dInd);
    _indices.emplace_back(caInd);

    _indices.emplace_back(caInd);
    _indices.emplace_back(dInd);
    _indices.emplace_back(cInd);

    _indices.emplace_back(cInd);
    _indices.emplace_back(dInd);
    _indices.emplace_back(bcInd);

    _indices.emplace_back(bcInd);
    _indices.emplace_back(dInd);
    _indices.emplace_back(bInd);

    // for the last tri, overwrite the original
    aInd = bInd;
    bInd = dInd;
    cInd = abInd;
  }
}

void Mesh::buildSpherinder(float sphereSize, float cylSize, Vec4f offset) {
  Mesh sphere;
  int numSlices = 3;
  float sliceOffset = cylSize / numSlices;
  for(int i = 0; i < numSlices; i++) {
    Vec4f offset(0.0f, 0.0f, 0.0f, sliceOffset * i);
    sphere.buildSphere(sphereSize, offset);
    sphere.projectIntoFour(sliceOffset);
    this->merge(sphere);
  }
}


// This doesn't work. You need a vector per dimension you want to constrain but
// with triangles you are only really constrained in two reduced dimensions, leaving an
// entire plane (in 4d, or n-2 in n) with triangles. I started writing this thinking that
// I could use the 3rd edge but then realized I am dumb and that is defined as 0 (pretty sure
// there's an elegant tie in with curvature for that). Anyways, fcuk me?
void Mesh::buildNormals(const VecList& verts, const IndexList& indices, VecList& normals) {
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

Mesh::TriConnectivity::Triangle::Triangle(int startIndex, const VecList& verts, const IndexList& indices)
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

Mesh::TriConnectivity::~TriConnectivity() {
  for (TriangleHash::iterator iTri = _triHash.begin();
      iTri != _triHash.end();
      ++iTri) {
    Triangle* triangle = iTri->second;
    delete triangle;
  }
}

void Mesh::TriConnectivity::buildGraph() {
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

Mesh::TriConnectivity::Triangle* Mesh::TriConnectivity::getTriangle(int triIndex) {
  TriangleHash::iterator iTri = _triHash.find(triIndex);
  if (iTri != _triHash.end()) {
    return iTri->second;
  }
  return NULL;
}

bool Mesh::TriConnectivity::Triangle::isNextTo(const Triangle* other) const {
  EdgeSet intersectionSet;
  std::set_intersection(_edges.begin(), _edges.end(),
      other->_edges.begin(), other->_edges.end(),
      std::inserter(intersectionSet, intersectionSet.begin()));
  return !intersectionSet.empty();
}

bool Mesh::TriConnectivity::Triangle::isCoplanarWith(const Triangle* other) const {
  EdgeSet disjointSet;
  std::set_symmetric_difference(_edges.begin(), _edges.end(),
      other->_edges.begin(), other->_edges.end(),
      std::inserter(disjointSet, disjointSet.begin()));

  if (disjointSet.size() < 3) {
    printf("Degenerate triangles in coplanar test!\n");
    return true;
  }

  VecList edgeVectors;
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
    TriConnectivity& connectivity, int triIndex, TriHash& unique) {
  Shape* shape = connectivity.buildEmptyShape();

  TriConnectivity::Triangle* triangle = connectivity.getTriangle(triIndex);
  shape->addTriangleWithEdges(triangle->getIndex(0), triangle->getIndex(1), triangle->getIndex(2));
  unique.insert(std::make_pair(triangle->getTriCode(), triangle->getTriIndex()));

  for (TriConnectivity::Triangle::iterator iC = triangle->begin();
      iC != triangle->end();
      ++iC) {
    TriConnectivity::Triangle* query = *iC;
    if (unique.find(query->getTriCode()) != unique.end())
      continue;

    if (triangle->isCoplanarWith(query)) {
      shape->addTriangleWithEdges(query->getIndex(0), query->getIndex(1), query->getIndex(2));
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

void Mesh::buildShapes(VecList& verts, IndexList& indices, Shapes& outShapes) {
  TriConnectivity connectivity(verts, indices);
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
void Mesh::projectIntoFour(float insideDist, Vec4f numberedSkewStep) {
  Vec4f shift(0, 0, 0, insideDist);
  VecList fourVerts;
  fourVerts.resize(_verts.size());
  std::copy(_verts.begin(), _verts.end(), fourVerts.begin());
  for (VecList::iterator iV = fourVerts.begin();
      iV != fourVerts.end();
      ++iV) {
    Vec4f& v = *iV;
    v += shift;
    v += numberedSkewStep;
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
  // Retrospective note: Wow this read weird until I remembered Stephanie was a homeless
  // lady who had asked for some change and instead bought her a burrito. Then went back
  // to coding without actually helping her in a long term meaningful way... So it goes.
  // Seriously who writes these kinds of notes in a public codebase anyway?
  for (int face = 0; face < faceCount; face++) {
    float rotation = (float)face / (float)faceCount * 2.0f * (float)PI;
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
  projectIntoFour(inside, fd::Vec4f());
}

// Basically a tetrad that is extruded into an inverted tetrad. All the faces
// should line up so there are 16 tetrads (cells).
// Schlafli {3,3,4}.
// So the first 3 means 3 edges in 2d (triangle) to make a face.
// Second 3 means 3 faces per vertex (tetrad) to make a cell.
// Third 4 means 4 tetrads per edge.
void Mesh::build16cell(float radius, Vec4f offset) {
  _verts.resize(0);
  _verts.reserve(8);
  // An alternate construction would keep all edge lengths equal. Too lazy.
  _verts.emplace_back(radius, 0.0f, 0.0f, 0.0f);
  _verts.emplace_back(0.0f, radius, 0.0f, 0.0f);
  _verts.emplace_back(0.0f, 0.0f, radius, 0.0f);
  _verts.emplace_back(0.0f, 0.0f, 0.0f, radius);
  _verts.emplace_back(-radius, 0.0f, 0.0f, 0.0f);
  _verts.emplace_back(0.0f, -radius, 0.0f, 0.0f);
  _verts.emplace_back(0.0f, 0.0f, -radius, 0.0f);
  _verts.emplace_back(0.0f, 0.0f, 0.0f, -radius);

  Shape shape(_verts, _indices);
  // Every vert is linked to every other vert, except the one directly across.
  // We can brute force this by just making triangles for any set of verts that
  // connect to each other.
  // Maybe after this one we look into arbitrary generation from schlafli codes?
  for (int a = 0; a < 8; ++a) {
    for (int b = 0; b < 8; ++b) {
      if((a + 4) % 8 == b || a == b)
        continue;
      // each a should have 6 connections to b
      for (int c = 0; c < 8; ++c) {
        if(c == a || c == b) continue;
        if((a + 4) % 8 == c) continue;
        if((b + 4) % 8 == c) continue;
        // each a ends up with 4 triangles per b

        shape.addUniqueTriangle(a, b, c);
      }
    }
  }

  _indices.assign(shape.getTriangles().begin(), shape.getTriangles().end());

  //printf("Had %d verts and %d indices for %d tris\n",
  //    _verts.size(), _indices.size(), _indices.size() / 3);
}

void Mesh::addPolyVerts(float radius, Vec4f center, Vec4f normalRight, Vec4f normalUp, int faceCount) {
  assert(faceCount >= 3);

  _verts.reserve(_verts.size() + faceCount);
  for (int face = 0; face < faceCount; face++) {
    float rotation = (float)face / (float)faceCount * 2.0f * (float)PI;
    float rightAmount = cos(rotation) * radius;
    float upAmount = sin(rotation) * radius;
    Vec4f next(center + (normalRight * rightAmount) + (normalUp * upAmount));
    _verts.push_back(next);
  }
}

int64 Mesh::Polygon::GetHash() {
  if(!_hashIsDirty)
    return _hash;

  // Intentionally ignoring the normal and other stuff as they wouldn't
  // produce a different set of rendered triangles (gave up on windings).
  IndexList sortedIndices(_indVerts);
  std::sort(sortedIndices.begin(), sortedIndices.end());
  _hash = 0xdeaff001;
  for(auto index : sortedIndices) {
    // horrible
    static std::hash<int64> hasher;
    _hash = (int64)hasher(((int64)index << 33) + _hash);
  }
  _hashIsDirty = false;
  return _hash;
}

int64 Mesh::Cell::GetHash() {
  if(!_hashIsDirty)
    return _hash;

  struct {
    bool operator()(Polygon* l, Polygon* r) {
      return l->GetHash() < r->GetHash();
    }
  } customPolyComp;

  // Intentionally ignoring the normal and other stuff as they wouldn't
  // produce a different set of rendered triangles (gave up on windings).
  Polygons sortedPolys(_polys);
  std::sort(sortedPolys.begin(), sortedPolys.end(), customPolyComp);
  _hash = 0xaddb00b5;
  for(auto poly : sortedPolys) {
    // horrible
    static std::hash<int64> hasher;
    _hash = (int64)hasher(poly->GetHash() + _hash);
  }
  _hashIsDirty = false;
  return _hash;
}

//{V,F,C}
//
//make a poly, (startVert, planeX, planeY, #verts)
//  for each vert, make a node
//  for each node pair, make edge
//  all edges to poly
//  make normal from edges
Mesh::Polygon* Mesh::addPolygon(float baseLen, const Vec4f& baseVert,
    const Vec4f& planeX, const Vec4f& planeY, const Vec4f& normal,
    int vertsPerPoly) {
  Polygon* poly = new Polygon(*this);
  float interiorAngle = 2.0f * (float)PI / vertsPerPoly;

  float radius = baseLen * 0.5f / sinf(interiorAngle * 0.5f);
  float dropLen = radius * cosf(interiorAngle * 0.5f);
  poly->_center = baseVert + (planeY * (baseLen * 0.5f)) + (planeX * dropLen);

  // gross because this obviously already exists.
  poly->_indVerts.push_back(addUniqueVert(baseVert));

  for (int v = 1; v < vertsPerPoly; ++v) {
    // negative cos because been envisioning clockwise and only shot at getting
    // windings right on the first try (hahaha) is to stick with it.
    float rotation = ((float)v / (float)vertsPerPoly * 2.0f * (float)PI) - (interiorAngle * 0.5f);
    float xAmount = -cos(rotation) * radius;
    float yAmount = sin(rotation) * radius;
    Vec4f newVert(poly->_center + (planeY * yAmount) + (planeX * xAmount));
    poly->_indVerts.push_back(addUniqueVert(newVert));
  }
  poly->_normal = normal;

#ifdef _DEBUG
  static bool dupCheck = false;
  if(dupCheck) { //asserty code
    for(int v = 0; v < vertsPerPoly; ++v) {
      for(int u = v + 1; u < vertsPerPoly; ++u) {
        assert(poly->_indVerts[v] != poly->_indVerts[u]);
      }
    }
  }
#endif

  if(_polys.find(poly->GetHash()) == _polys.end()) {
    _polys.insert(std::make_pair(poly->GetHash(), poly));
    return poly;
  } else {
    delete poly;
    return NULL;
  }
}

//make cell, (poly, planeZ, #facesPerVert)
//  add poly to unFinishedPoly
//  for each vert in unFinishedPoly
//    for F-1 faces
//      make new nVert from angles
//      //make a poly, (vert, nVert-v, p-v, #verts)
//      make a poly, (vert, n-v, nVert-v, #verts)
//      check if poly has already been created
//        sort vert indices, create hash, check hash
//      if new, add poly to unFinishedPoly
Mesh::Cell* Mesh::addCell(float baseLen, Polygon* startPoly,
    const Vec4f& cellNormal, int vertsPerPoly, int polysPerCellVert,
    float windingSignFlip) {
  Cell* cell = new Cell(*this);
  cell->_polys.push_back(startPoly);

#ifdef _DEBUG
  int startupVerts = (int)_verts.size();
  printf("Adding new cell winding:%f\nnormal:", windingSignFlip);
  cellNormal.printIt();
  printf("\npoly0:");
  _verts[startPoly->_indVerts[0]].printIt();
  printf("\npoly1:");
  _verts[startPoly->_indVerts[1]].printIt();
  printf("\npoly2:");
  _verts[startPoly->_indVerts[2]].printIt();
  printf("\npoly3:");
  _verts[startPoly->_indVerts[3]].printIt();
  printf("\n");
#endif // _DEBUG

  // wrong wrong wrong, but ok for a moment
  cell->_center = startPoly->_center + (startPoly->_normal * (baseLen * 0.5f));

  Polygons unfinishedPolys;
  unfinishedPolys.push_back(startPoly);

  // so gross
  // since we are essentially assuming polysPerCellVert==3
  // this shouldn't happen, but still
  const int safetyMax = 100;
  int safeyCounter = 0;

  while(!unfinishedPolys.empty()) {
    Polygon* poly = unfinishedPolys.back();
    unfinishedPolys.pop_back();

    int polyVerts = (int)poly->_indVerts.size();
    for(int v = 0; v < polyVerts; ++v) {
      // do something wrong for the moment.
      int nextIndex = (v + 1) % polyVerts;
      int prevIndex = (v - 1 + polyVerts) % polyVerts;

      const Vec4f& pos = _verts[poly->_indVerts[v]];
      const Vec4f& prev = _verts[poly->_indVerts[prevIndex]];
      const Vec4f& next = _verts[poly->_indVerts[nextIndex]];

      Vec4f oldPrev = (prev - pos).normalized();
      Vec4f oldNext = (next - pos).normalized();
      Vec4f oldPolyNormal = cellNormal.cross(oldPrev, oldNext).normalized();
      if(oldPolyNormal.dot(pos + oldPolyNormal - cell->_center) > 0.001f) {
        oldPolyNormal = -oldPolyNormal;
        oldPolyNormal *= windingSignFlip;
      }

      Vec4f newVert(pos + (oldPolyNormal * baseLen));
      addUniqueVert(newVert);
      Vec4f newNext = (newVert - pos).normalized();
      Vec4f newPrev = oldNext;
      Vec4f newPolyNormal = oldPrev;

      Polygon* newPoly = addPolygon(baseLen, pos,
          newNext, newPrev, newPolyNormal, polyVerts);
      if(newPoly == NULL) continue;

      // hmm.. the last cell will be empty? hash problem?
      cell->_polys.push_back(newPoly);

      unfinishedPolys.push_back(newPoly);
    }

    if(safeyCounter++ > safetyMax) break;
  }

#ifdef _DEBUG
  int endupVerts = (int)_verts.size();
  printf("\nAdded %d verts\n", endupVerts - startupVerts);
  for(int v = startupVerts; v < endupVerts; ++v) {
    printf("New vert(%d)\t", v);
    _verts[v].printIt();
    printf("\n");
  }
#endif // _DEBUG


  if(_cells.find(cell->GetHash()) == _cells.end()) {
    _cells.insert(std::make_pair(cell->GetHash(), cell));
    return cell;
  } else {
    // So *in theory* this souldn't leak polys because if the cell hashed the
    // same as something already around, all the polys should already be
    // around also. Anyway, any actually leaked would get cleaned up eventually
    // anyway, anyway. Any other way... whatever anyway.
    delete cell;
    return NULL;
  }
}

//make tope (cell, #cellsPerEdge)
//  add all polys to unFinishedPolys
//  for poly in unFinishedPolys
//    make cell, (poly, poly.normal, #facesPerVert)
//    check if cell has already been created
//      per poly in cell, sort poly vert indices
//      sort poly vert lists
//      hash these, check hash
//    if new, add all cell polys to unFinishedPolys
//      hash these also
//  for all polys in all cells
//    add indexed tris
void Mesh::buildPolytope(float baseLen, Vec4f start,
    int vertsPerPoly, int polysPerCellVert, int cellsPerEdge) {
  clearCurrent();
  cleanupPolysAndCells(); // safety!

  Vec4f arbitraryPlaneX(1.0f, 0.0f, 0.0f, 0.0f);
  Vec4f arbitraryPlaneY(0.0f, 1.0f, 0.0f, 0.0f);
  Vec4f arbitraryPlaneZ(0.0f, 0.0f, 1.0f, 0.0f);
  Vec4f arbitraryPlaneW(0.0f, 0.0f, 0.0f, 1.0f);

  Polygon* basePoly = addPolygon(baseLen, start, arbitraryPlaneX, arbitraryPlaneY, arbitraryPlaneZ, vertsPerPoly);
  Cell* baseCell = addCell(baseLen, basePoly, arbitraryPlaneW, vertsPerPoly,
      polysPerCellVert, 1.0f);
  Cells cells;
  cells.push_back(baseCell);

  typedef std::map<Polygon*, Cell*> CellTargets;
  CellTargets unfinishedPolys;
  for(auto poly : baseCell->_polys) {
    unfinishedPolys.insert(std::make_pair(poly, baseCell));
  }

  // so gross...
  // I think the massive downside of this approach is I'm not even sure if
  // a shape is closed and thus will complete for some sets of params...
  // (even theoretically).
  // Gonna have to do the graph theory approach?
  const int safetyMax = 1000;
  int safeyCounter = 0;

  while(!unfinishedPolys.empty()) {
    auto cellTargetIt = unfinishedPolys.begin();
    Polygon* poly = cellTargetIt->first;
    Cell* creatorCell = cellTargetIt->second;
    unfinishedPolys.erase(cellTargetIt);

    Vec4f& vStart = _verts[poly->_indVerts[0]];
    Vec4f& vNext = _verts[poly->_indVerts[1]];
    Vec4f& vPrev = _verts[poly->_indVerts[poly->_indVerts.size() - 1]];
    Vec4f nextDir = (vNext - vStart).normalized();
    Vec4f prevDir = (vPrev - vStart).normalized();
    Vec4f prevNormal = (creatorCell->_center - poly->_center).normalized();
    Vec4f prevWindingNormal = nextDir.cross(prevNormal, prevDir).normalized();
    Vec4f newCellNormal = prevNormal;
    float windingSignFlip = 1.0f;
    if(prevWindingNormal.dot(creatorCell->_normal) > 0.001f) {
      windingSignFlip = -1.0f;
    }

    Cell* newCell = addCell(baseLen, poly, newCellNormal,
        vertsPerPoly, polysPerCellVert, windingSignFlip);
    if(!newCell) // dupe
      continue;
    cells.push_back(newCell);
    for(auto poly : newCell->_polys) {
      if(_polys.find(poly->GetHash()) == _polys.end()) {
        unfinishedPolys.insert(std::make_pair(poly, newCell));
      }
    }

    if(safeyCounter++ > safetyMax) break;
  }

  cleanupUniqueTriangles(); // safety
  //for(auto cell : _cells) {
    for(auto polyPair : _polys) {
      polyPair.second->AddUniqueTriangles();
    }
  //}

  cleanupPolysAndCells();
  cleanupUniqueTriangles();
}

void Mesh::Polygon::AddUniqueTriangles() {
  assert(_indVerts.size() >= 3);

  // to make this extra idempotent, first find the smallest index
  // then, decide the overall poly winding by finding the next smallest index
  // we will make that the root of the fan and set the iteration direction
  // thus, even with a different ordering of starting indices, we always get
  // the same triangles, which means the trihash works, which means we should
  // never get doubled interior faces.
  // It would probably be more efficient to just make sure this function
  // is only called for polys from their hash, which handles this stuff
  // but what the hell. Once you get to a point of ugly, you figure, fuck it
  // just make it work and then we will see how fast it is.

  int polyVerts = (int)_indVerts.size();
  // note that we are actually dealing with an array of indices into the vertex
  // array, and we need an index into that array of indices. Thus:
  int minIndexsValue = INT_MAX; // a case for apostrophes in var names?
  int minIndexsIndex;
  for (int v = 0; v < polyVerts; ++v) {
    if(_indVerts[v] < minIndexsValue) {
      minIndexsIndex = v;
      minIndexsValue = _indVerts[v];
    }
  }

  int root = _indVerts[minIndexsIndex];
  int nextRoot = _indVerts[(minIndexsIndex + 1) % polyVerts];
  int prevRoot = _indVerts[(minIndexsIndex - 1 + polyVerts) % polyVerts];
  int stepDir = (nextRoot < prevRoot) ? 1 : -1;

  int end = minIndexsIndex;
  int prev = (minIndexsIndex + stepDir + polyVerts) % polyVerts;
  int current = (prev + stepDir + polyVerts) % polyVerts;

  do {
    int64 triHash = makeUniqueTriCode(root, _indVerts[prev], _indVerts[current]);
    if(_mesh._uniqueTris.find(triHash) == _mesh._uniqueTris.end()) {
      _mesh._uniqueTris.insert(std::make_pair(triHash, 1)); // why is this a map?
      _mesh.addTri(root, _indVerts[prev], _indVerts[current]);
    }
    prev = current;
    current = (current + stepDir + polyVerts) % polyVerts;
  } while(current != end);

  // The number of comments this function ended up with is an indication that
  // the author sucks.
}

void Mesh::cleanupPolysAndCells() {
  for(auto polyPair : _polys) {
    delete polyPair.second;
  }
  _polys.clear();

  for(auto cellPair : _cells) {
    delete cellPair.second;
  }
  _cells.clear();
}

void Mesh::cleanupUniqueTriangles() {
  _uniqueTris.clear();
}

void Mesh::buildGeneralizedTesseract(float size, const Vec4f& start) {
  buildPolytope(size, start, 4 /*vertsPerPoly*/, 3 /*polysPerCellVert*/,
      3 /*cellsPerEdge*/);
}

void Mesh::clearCurrent() {
  _verts.resize(0);
  _indices.resize(0);
}

// This was going to be the same 16 cell but in a more generalized way.
void Mesh::buildGeneralized16cell(float radius, Vec4f offset) {

  const int polyVerts = 3;
  float polyAngle = (float)PI * (float)(polyVerts - 2) / (float)polyVerts;
  float betweenVertAngle = (float)PI - polyAngle;
  float sideLength = 2.0f * radius * sinf(betweenVertAngle * 0.5f);
  Vec4f right(1.0f, 0.0f, 0.0f, 0.0f);
  Vec4f forward(0.0f, 1.0f, 0.0f, 0.0f);
  Vec4f up(0.0f, 0.0f, 1.0f, 0.0f);
  Vec4f inward(0.0f, 0.0f, 0.0f, 1.0f);
   // first build the face
  clearCurrent();
  addPolyVerts(radius, offset, right, forward, polyVerts);

  // for a cell only
  //const int facesPerVert = 3;

  for(int vert = 0; vert < polyVerts; vert++) {
    int nextIndex = (vert + 1) % polyVerts;
    int prevIndex = (vert - 1 + polyVerts) % polyVerts;


    const Vec4f& pos = _verts[vert];
    const Vec4f& prev = _verts[prevIndex];
    const Vec4f& next = _verts[nextIndex];

    Vec4f prevRay = prev - pos;
    Vec4f nextRay = next - pos;
    //Vec4f centerRay = (prevRay + nextRay) * 0.5f;

    Vec4f perpRay = prevRay.cross(nextRay, inward);
    perpRay.storeNormalized();
    float perpHeight = sqrt((sideLength * sideLength) - (radius * radius));
    Vec4f perp = offset + (perpRay * (perpHeight * sinf(polyAngle)));
    addUniqueVert(perp);
  }

  printIt();
}

void Mesh::build120cell(float radius, Vec4f offset) {

  // Don't think this worked
  //buildPolytope(radius, offset, 5 /*vertsPerPoly*/, 3 /*polysPerCellVert*/,
  //    3 /*cellsPerEdge*/);
}

int Mesh::addUniqueVert(const Vec4f& vert) { // wow this is slow and inaccurate
  const float threshold = 0.001f; // so horrible
  for (int v = 0; v < (int)_verts.size(); ++v) {
    const Vec4f& exist = _verts[v];
    if(vert.approxEqual(exist, threshold)) {
      return v;
    }
  }
  _verts.push_back(vert);
  return (int)_verts.size() - 1;
}

void Mesh::Shape::addUniqueTriangle(int a, int b, int c) {
  int64 triCode = makeUniqueTriCode(a, b, c);
  TriHash::iterator iTri = _uniqueTris.find(triCode);
  if (iTri == _uniqueTris.end()) {
    int triIndex = (int)_uniqueTris.size() + 1;
    _uniqueTris.insert(std::make_pair(triCode, triIndex));
    // totally gave up on windings
    _triangles.push_back(a);
    _triangles.push_back(b);
    _triangles.push_back(c);
  }
}

void Mesh::Shape::addTriangleWithEdges(int a, int b, int c) {
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

class JennGraphConverter : public MeshConverter {

public:
  JennGraphConverter() {}

  static bool Convert(const jenn::ToddCoxeter::Graph* graph, Mesh* mesh, float radius, Vec4f offset) {
    if(!graph || !mesh || graph->faces.empty())
      return false;

    Mesh::VecList& _verts = mesh->_verts;
    Mesh::IndexList& _indices = mesh->_indices;

    int numVerts = (int)graph->points.size();
    _verts.resize(numVerts);
    for(int v = 0; v < numVerts; v++) {
      const jenn::Vect& p = graph->points[v];
      _verts[v].set(p[0], p[1], p[2], p[3]);
      _verts[v] *= radius;
      _verts[v] += offset;
    }

    int numFaces = (int)graph->faces.size();
    // not sure if the assumption that all faces have the same length is always valid
    int numIndicesGuess = numFaces * ((int)(graph->faces[0].size()) - 2) * 3; // tri-list a polygon
    _indices.resize(0);
    if(numIndicesGuess > 0)
      _indices.reserve(numIndicesGuess);
    for(int f = 0; f < numFaces; f++) {
      const jenn::ToddCoxeter::Ring& poly = graph->faces[f];
      int polySize = (int)poly.size();
      int numTris = (polySize - 2);
      for(int tri = 0; tri < numTris; tri++) {
        _indices.push_back(poly[0]);
        _indices.push_back(poly[1 + tri]);
        _indices.push_back(poly[2 + tri]);
      }
    }

    printf("Graph had %lu faces, %lu verts, made  %lu indices %lu tris\n",
        graph->faces.size(), graph->points.size(), _indices.size(), _indices.size() / 3);

    return true;
  }
};

void Mesh::buildCaylayTesseract(float radius, Vec4f offset) {
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(jenn::Polytope::select(jenn::Polytope::the_8_cell));
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

void Mesh::buildCaylay16Cell(float radius, Vec4f offset) {
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(jenn::Polytope::select(jenn::Polytope::the_16_cell));
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

void Mesh::buildCaylay120Cell(float radius, Vec4f offset) {
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(jenn::Polytope::select(jenn::Polytope::the_120_cell));
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

void Mesh::buildCaylay24Cell(float radius, Vec4f offset) {
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(jenn::Polytope::select(jenn::Polytope::the_24_cell));
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

void Mesh::buildCaylay600Cell(float radius, Vec4f offset) {
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(jenn::Polytope::select(jenn::Polytope::the_600_cell));
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

void Mesh::buildCaylayEnumerated(float radius, Vec4f offset, int enumDir) {
  // uses internal state and essentially iterates through building stuff it knows about
  std::unique_ptr<jenn::ToddCoxeter::Graph> graph(
      (enumDir >= 0) ? jenn::Polytope::selectNext() : jenn::Polytope::selectPrev());
  JennGraphConverter::Convert(graph.get(), this, radius, offset);
}

#if 0 // currently abandoned approach to generic polytope creation
  {3,3,4} {4,3,3}, {5,3,3}
  {P,F,C}
  make a poly
  for each vert, make a node
  for each node pair, make an edge
  for each edge pair, connect to a polygon

  now for each node that isnt full
    fill vert by creating to (F+1) verts
    create new vert according to angle math
    for new vert
      start new node, hook to prev via edge
      for each prevs edge,
        start new polygon or connect to unfinished coplanar
        maybe finish polygon

  difficulty with all of this is windings seem like they can go in different \
    directions easily and then connections will be disconnected

      class Polygon;
  typedef std::vector<Polygon*> Polygons;

  class Node;
  typedef std::vector<Node*> Nodes;
  typedef std::pair<Node*, bool> DirectedNode;
  typedef std::vector<DirectedNode> DNodes;
  // 1d thing, information about poly connectivity
  class Node {
  public:
    Mesh& _mesh;

    // polys this node is part of
    Polygons _shapes; // not owned
    // connected nodes
    DNodes _nodes; // not owned

    int _vert;

  public:
    Node(Mesh& mesh) : _mesh(mesh) {}

    // connect us to other with direction
    // connect other to us with !direction
    void ConnectNode(Node* other, bool direction);
    bool IsFull(int expectedConnections);
  };

  // 2d thing, with
  class Polygon {
  public:
    Mesh& _mesh;
    DNodes _nodes;
    Vec4f _normal;
    int _targetVerts;

  public:
    Polygon(Mesh& mesh, int targetVerts) : _mesh(mesh), _targetVerts(targetVerts) {}
    void AddEdge(Node* prev, Node* next);

  };

    const int polyVerts = 3;
  float polyAngle = (float)PI * (float)(polyVerts - 2) / (float)polyVerts;
  float betweenVertAngle = (float)PI - polyAngle;
  float sideLength = 2.0f * radius * sinf(betweenVertAngle * 0.5f);
  Vec4f right(1.0f, 0.0f, 0.0f, 0.0f);
  Vec4f forward(0.0f, 1.0f, 0.0f, 0.0f);
  Vec4f up(0.0f, 0.0f, 1.0f, 0.0f);
  Vec4f inward(0.0f, 0.0f, 0.0f, 1.0f);
   // first build the face
  clearCurrent();
  addPolyVerts(radius, offset, right, forward, polyVerts);

  // for a cell only
  const int facesPerVert = 3;

  Nodes nodeStorage;
  //nodeStorage.reserve(polyVerts); // yeah I dunno
  Polygons polyStorage;
  Polygon* polygon = new Polygon(*this, polyVerts);

  for(int vert = 0; vert < polyVerts; vert++) {
    int nextIndex = (vert + 1) % polyVerts;
    int prevIndex = (vert - 1 + polyVerts) % polyVerts;

     Node* node = new Node(*this);
     nodeStorage.push_back(node);
     Node* prevNode = nodeStorage[prevIndex];
     prevNode->ConnectNode(node, true);

     polygon->AddEdge(prevNode, node);
  }

  while(true) {
    Node* node = NULL;
    for(auto n : nodeStorage) {
      if(n->IsFull()) continue;
      node = n;
      break;
    }



    const Vec4f& pos = _verts[vert];
    const Vec4f& prev = _verts[prevIndex];
    const Vec4f& next = _verts[nextIndex];

    Vec4f prevRay = prev - pos;
    Vec4f nextRay = next - pos;
    Vec4f centerRay = (prevRay + nextRay) * 0.5f;

    Vec4f perpRay = prevRay.cross(nextRay, inward);
    perpRay.storeNormalized();
    float perpHeight = sqrt((sideLength * sideLength) - (radius * radius));
    Vec4f perp = offset + (perpRay * (perpHeight * sinf(polyAngle)));
    addUniqueVert(perp);
  }

void Mesh::Node::ConnectNode(Node* other, bool direction) {
  _nodes.push_back(std::make_pair(other, direction));
  other->_nodes.push_back(std::make_pair(this, !direction));
}


#endif // 0
